/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "parsing/crossdomainpolicy.h"

#include "swf.h"
#include "compat.h"
#include <sstream>
#include <string>
#include <algorithm>
#include <ctype.h>

#include "security.h"

using namespace lightspark;
using namespace std;

/**
 * \brief SecurityManager constructor
 */
SecurityManager::SecurityManager():
	sandboxType(REMOTE),exactSettings(true),exactSettingsLocked(false)
{
	sandboxNames = {"remote","localWithFile","localWithNetwork","localTrusted"};
	sandboxTitles = {"remote","local-with-filesystem","local-with-networking","local-trusted"};

	sem_init(&mutex,0,1);
	//== Lock initialized
}

/**
 * \brief SecurityManager destructor
 *
 * SecurityManager destructor. Acquires mutex before proceeding.
 */
SecurityManager::~SecurityManager()
{
	sem_wait(&mutex);
	//-- Lock acquired
	
	URLPFileMapIt i = pendingURLPFiles.begin();
	for(; i != pendingURLPFiles.end(); ++i)
		delete (*i).second;

	i = loadedURLPFiles.begin();
	for(;i != loadedURLPFiles.end(); ++i)
		delete (*i).second;
	
	//== Destroy lock
	sem_destroy(&mutex);
}

/**
 * \brief Add policy file at the given URL
 *
 * Adds a policy file at the given URL to list of managed policy files.
 * Classifies the URL first and then decides whether to call \c addURLPolicyFile() 
 * (the currently only supported type)
 * \param url The URL where the policy file resides
 * \return A pointer to the newly created PolicyFile object or NULL if the URL is of an unsupported type
 */
PolicyFile* SecurityManager::addPolicyFile(const URLInfo& url)
{
	if(url.getProtocol() == "http" || url.getProtocol() == "https" || url.getProtocol() == "ftp")
		return addURLPolicyFile(url);
	else if(url.getProtocol() == "xmlsocket")
		LOG(LOG_NOT_IMPLEMENTED, _("SECURITY: SocketPolicFile not implemented yet!"));
	return NULL;
}

/**
 * \brief Add an URL policy file at the given URL
 *
 * Adds an URL policy file at the given URL to the list of managed URL policy files.
 * Policy files aren't loaded when adding, this is delayed until the policy file is actually needed.
 * Waits for mutex at start and releases mutex when finished
 * \param url The URL where the URL policy file resides
 * \return A pointer to the newly created URLPolicyFile object
 */
URLPolicyFile* SecurityManager::addURLPolicyFile(const URLInfo& url)
{
	sem_wait(&mutex);
	//-- Lock acquired
	
	URLPolicyFile* file = new URLPolicyFile(url);
	if(file->isValid())
	{
		LOG(LOG_NO_INFO, 
				_("SECURITY: Added URL policy file is valid, adding to URL policy file list (") << url << ")");
		pendingURLPFiles.insert(URLPFilePair(url.getHostname(), file));
	}
	
	//++ Release lock
	sem_post(&mutex);
	return file;
}

/**
 * \brief Search for an URL policy file object in lists
 *
 * Searches for an URL policy file object in the lists of managed URL policy files by URL.
 * Returns NULL when such an object could not be found.
 * Waits for mutex at start and releases mutex when finished
 * \param url The URL where the URL policy file object would reside.
 * \return The found URL policy file object or NULL
 */
URLPolicyFile* SecurityManager::getURLPolicyFileByURL(const URLInfo& url)
{
	sem_wait(&mutex);
	//-- Lock acquired

	URLPFileMapConstItPair range = loadedURLPFiles.equal_range(url.getHostname());
	URLPFileMapConstIt i = range.first;

	//Check the loaded URL policy files first
	for(;i != range.second; ++i)
	{
		if((*i).second->getOriginalURL() == url)
		{
			LOG(LOG_NO_INFO, _("SECURITY: URL policy file found in loaded list (") << url << ")");

			//++ Release lock
			sem_post(&mutex);
			return (*i).second;
		}
	}

	//Check the pending URL policy files next
	range = pendingURLPFiles.equal_range(url.getHostname());
	i = range.first;
	for(;i != range.second; ++i)
	{
		if((*i).second->getOriginalURL() == url)
		{
			LOG(LOG_NO_INFO, _("SECURITY: URL policy file found in pending list (") << url << ")");

			//++ Release lock
			sem_post(&mutex);
			return (*i).second;
		}
	}

	//++ Release lock
	sem_post(&mutex);
	return NULL;
}

/**
 * \brief Load a given URL policy file object
 *
 * Loads a given URL policy file object (if it isn't loaded yet).
 * This moves the object from the pending URL policy files list to the loaded URL policy files list.
 * Waits for mutex at start and releases mutex when finished
 * \param file A pointer to the URL policy file object to load.
 */
void SecurityManager::loadPolicyFile(URLPolicyFile* file)
{
	sem_wait(&mutex);
	//-- Lock acquired

	if(pendingURLPFiles.count(file->getURL().getHostname()) > 0)
	{
		LOG(LOG_NO_INFO, _("SECURITY: Loading policy file (") << file->getURL() << ")");
		file->load();

		URLPFileMapItPair range = pendingURLPFiles.equal_range(file->getURL().getHostname());
		URLPFileMapIt i = range.first;
		for(;i != range.second; ++i)
		{
			if((*i).second == file)
			{
				loadedURLPFiles.insert(URLPFilePair(file->getURL().getHostname(), (*i).second));
				pendingURLPFiles.erase(i);
				break;
			}
		}
	}

	//++ Release lock
	sem_post(&mutex);
}

/**
 * \brief Search for URL policy files relevant to a given URL
 *
 * Searches the loaded URL policy file list for URL policy files that are relevant to a given URL.
 * If \c loadPendingPolicies is true, it search the pending URL policy files list next, 
 * loading every relative policy file.
 * Waits for mutex at start and releases mutex when finished
 * \param url The URL that will be evaluated using the relevant policy files.
 * \param loadPendingPolicies Whether or not to load (and thus check) pending URL policy files.
 * \return An pointer to a newly created URLPFileList containing the relevant policy files.
 *         This list needs to be deleted after use.
 */
URLPFileList* SecurityManager::searchURLPolicyFiles(const URLInfo& url, bool loadPendingPolicies)
{
	URLPFileList* result = new URLPFileList;

	//Get or create the master policy file object
	URLInfo masterURL = url.goToURL("/crossdomain.xml");
	URLPolicyFile* master = getURLPolicyFileByURL(masterURL);

	if(master == NULL)
		master = addURLPolicyFile(masterURL);

	if(loadPendingPolicies)
		sys->securityManager->loadPolicyFile(master);

	sem_wait(&mutex);
	//-- Lock acquired

	//Check if the master policy file is loaded.
	//If another user-added relevant policy file is already loaded, 
	//it's master will have already been loaded too (to check if it is allowed).
	//So IF any relevant policy file is loaded already, then the master will be too.
	if(master->isLoaded() && master->isValid())
	{
		LOG(LOG_NO_INFO, _("SECURITY: Master policy file is loaded and valid (") << url << ")");

		PolicySiteControl::METAPOLICY siteControl = master->getSiteControl()->getPermittedPolicies();
		//Master defines no policy files are allowed at all
		if(siteControl == PolicySiteControl::NONE)
		{
			LOG(LOG_NO_INFO, _("SECURITY: DISALLOWED: Master policy file disallows policy files"));

			//++ Release lock
			sem_post(&mutex);
			return NULL;
		}

		result->push_back(master);

		//Non-master policy files are allowed
		if(siteControl != PolicySiteControl::MASTER_ONLY)
		{
			LOG(LOG_NO_INFO, _("SECURITY: Searching for loaded non-master policy files (") <<
					loadedURLPFiles.count(url.getHostname()) << ")");

			URLPFileMapConstItPair range = loadedURLPFiles.equal_range(url.getHostname());
			URLPFileMapConstIt i = range.first;
			for(;i != range.second; ++i)
			{
				if((*i).second == master)
					continue;
				result->push_back((*i).second);
			}

			//And check the pending policy files next (if we are allowed to)
			if(loadPendingPolicies)
			{
				LOG(LOG_NO_INFO, _("SECURITY: Searching for and loading pending non-master policy files (") <<
						pendingURLPFiles.count(url.getHostname()) << ")");

				range = pendingURLPFiles.equal_range(url.getHostname());
				i = range.first;
				for(; i != range.second; ++i)
				{
					//++ Release lock
					sem_post(&mutex);
					//NOTE: loadPolicyFile() will change pendingURLPFiles, erasing & moving to loadURLPFiles
					sys->securityManager->loadPolicyFile((*i).second);
					sem_wait(&mutex);
					//-- Lock acquired

					result->push_back((*i).second);
				}
			}
		}
	}

	//++ Release lock
	sem_post(&mutex);
	return result;
}

/**
 * \brief Checks if a given sandbox is in the given allowed sandboxes
 *
 * \param sandbox The sandbox to check
 * \param allowedSandboxes A bitwise expression of allowed sandboxes.
 * \return \c true if the sandbox is allowed, \c false if not
 */
bool SecurityManager::evaluateSandbox(SANDBOXTYPE sandbox, int allowedSandboxes)
{
	return (allowedSandboxes & sandbox) != 0;
}

/**
 * \brief Evaluates an URL to see if it is allowed by the various security features.
 *
 * This first checks if the current sandbox is allowed access to the URL.
 * Next the port is checked to see if isn't restricted.
 * Then, if requested, the URL is checked to see if it doesn't point to a resource 
 * above the current directory in the directory hierarchy (only applicable to local URLs).
 * And finally the URL policy files are checked to see if they allow the player 
 * to access the given URL.
 * \param url The URL to evaluate
 * \param loadPendingPolicies Whether or not to load pending policy files while checking
 * \param allowedSandboxesRemote The sandboxes that are allowed to access remote URLs in this case.
 *                               Can be a bitwise expression of sandboxes.
 * \param allowedSandboxesLocal The sandboxes that are allowed to access local URLs in this case.
 *                              Can be a bitwise expression of sandboxes.
 * \return \c ALLOWED if the URL is allowed or one of \c NA_*, where * is the reason for not allowing.
 * \see SecurityManager::evaluateSandboxURL()
 * \see SecurityManager::evaluatePortURL()
 * \see SecurityManager::evaluateLocalDirectoryURL()
 * \see SecurityManager::evaluatePoliciesURL()
 */
SecurityManager::EVALUATIONRESULT SecurityManager::evaluateURL(const URLInfo& url,
		bool loadPendingPolicies,	int allowedSandboxesRemote, int allowedSandboxesLocal,
		bool restrictLocalDirectory)
{
	//Check the sandbox first
	EVALUATIONRESULT sandboxResult =
		evaluateSandboxURL(url, allowedSandboxesRemote, allowedSandboxesLocal);
	if(sandboxResult != ALLOWED)
		return sandboxResult;

	//Check if the URL points to a restricted port
	EVALUATIONRESULT portResult = evaluatePortURL(url);
	if(portResult != ALLOWED)
		return portResult;

	//If requested, make sure local URLs are restricted to the local directory
	if(restrictLocalDirectory)
	{
		EVALUATIONRESULT restrictLocalDirResult = evaluateLocalDirectoryURL(url);
		if(restrictLocalDirResult != ALLOWED)
			return restrictLocalDirResult;
	}

	//Finally, check policy files
	EVALUATIONRESULT policiesResult = evaluatePoliciesURL(url, loadPendingPolicies);
	if(policiesResult != ALLOWED)
		return policiesResult;

	//All checks passed, so we allow the URL connection
	return ALLOWED;
}

/**
 * \brief Checks if the current sandbox is allowed access to the given URL.
 * 
 * \param url The URL to evaluate the sandbox against
 * \param allowedSandboxesRemote The sandboxes that are allowed to access remote URLs in this case.
 *                               Can be a bitwise expression of sandboxes.
 * \param allowedSandboxesLocal The sandboxes that are allowed to access local URLs in this case.
 *                              Can be a bitwise expression of sandboxes.
 * \return \c ALLOWED if allowed or otherwise \c NA_REMOTE_SANDBOX/NA_LOCAL_SANDBOX depending on the reason.
 */
SecurityManager::EVALUATIONRESULT SecurityManager::evaluateSandboxURL(const URLInfo& url, 
		int allowedSandboxesRemote, int allowedSandboxesLocal)
{
	//The URL is remote and the current sandbox doesn't match the allowed sandboxes for remote URLs
	if(url.getProtocol() != "file" && (~allowedSandboxesRemote) & sandboxType)
		return NA_REMOTE_SANDBOX;
	//The URL is local and the current sandbox doesn't match the allowed sandboxes for local URLs
	if(url.getProtocol() == "file" && (~allowedSandboxesLocal) & sandboxType)
		return NA_LOCAL_SANDBOX;

	return ALLOWED;
}

/**
 * \brief Checks if the URL doesn't point to a resource higher up the directory hierarchy than 
 * the current directory
 *
 * \param url The URL to evaluate
 * \return \c ALLOWED if allowed or otherwise \c NA_RESTRICT_LOCAL_DIRECTORY
 */
SecurityManager::EVALUATIONRESULT SecurityManager::evaluateLocalDirectoryURL(const URLInfo& url)
{
	//The URL is local and points to a directory above the origin
	if(url.getProtocol() == "file" && !url.isSubOf(sys->getOrigin()))
		return NA_RESTRICT_LOCAL_DIRECTORY;

	return ALLOWED;
}

/**
 * \brief Checks if the port in the given URL isn't restricted, depending on the protocol of the URL
 *
 * \param url The URL to evaluate
 * \return \c ALLOWED if allowed or otherwise \c NA_PORT
 */
SecurityManager::EVALUATIONRESULT SecurityManager::evaluatePortURL(const URLInfo& url)
{
	if(url.getProtocol() == "http" || url.getProtocol() == "https")
		if(url.getPort() == 20 || url.getPort() == 21)
			return NA_PORT;

	if(url.getProtocol() == "http" || url.getProtocol() == "https" || url.getProtocol() == "ftp")
	{
		switch(url.getPort())
		{
		case 1:		case 7:		case 9:		case 11: case 13: case 15: case 17: case 19: case 22: case 23:
		case 25: case 37: case 42: case 43: case 53: case 77: case 79: case 87: case 95: case 101:
		case 102: case 103: case 104: case 109: case 110: case 111: case 113: case 115: case 117:
		case 119: case 123: case 135: case 139: case 143: case 179: case 389: case 465: case 512:
		case 513: case 514: case 515: case 526: case 530: case 531: case 532: case 540: case 556:
		case 563: case 587: case 601: case 636: case 993: case 995: case 2049: case 4045: case 6000:
			return NA_PORT;
		}
	}
	return ALLOWED;
}

/**
 * \brief Checks URL policy files to see if the player is allowed access to the given UR
 *
 * Waits for mutex at start and releases mutex when finished
 * \param url The URL to evaluate
 * \param loadPendingPolicies Whether to load (and thus check) pending policies before evaluating
 * \return \c ALLOWED if allowed or otherwise \c NA_CROSSDOMAIN_POLICY
 */
SecurityManager::EVALUATIONRESULT SecurityManager::evaluatePoliciesURL(const URLInfo& url,
		bool loadPendingPolicies)
{
	//This check doesn't apply to local files
	if(url.getProtocol() == "file" && sys->getOrigin().getProtocol() == "file")
		return ALLOWED;

	LOG(LOG_NO_INFO, _("SECURITY: Evaluating URL for cross domain policies:"));
	LOG(LOG_NO_INFO, _("SECURITY: --> URL:    ") << url);
	LOG(LOG_NO_INFO, _("SECURITY: --> Origin: ") << sys->getOrigin());

	//The URL has exactly the same domain name as the origin, always allowed
	if(url.getProtocol() == sys->getOrigin().getProtocol() &&
			url.getHostname() == sys->getOrigin().getHostname())
	{
		LOG(LOG_NO_INFO, _("SECURITY: Same hostname as origin, allowing"));
		return ALLOWED;
	}

	//Search for the policy files to check
	URLPFileList* files = searchURLPolicyFiles(url, loadPendingPolicies);
	
	sem_wait(&mutex);
	//-- Lock acquired

	//Check the policy files
	if(files != NULL)
	{
		URLPFileListConstIt it = files->begin();
		for(; it != files->end(); ++it)
		{
			if((*it)->allowsAccessFrom(sys->getOrigin(), url))
			{
				LOG(LOG_NO_INFO, _("SECURITY: ALLOWED: A policy file explicitly allowed access"));
				delete files;

				//++ Release lock
				sem_post(&mutex);
				return ALLOWED;
			}
		}
	}

	LOG(LOG_NO_INFO, _("SECURITY: DISALLOWED: No policy file explicitly allowed access"));
	delete files;

	//++ Release lock
	sem_post(&mutex);
	return NA_CROSSDOMAIN_POLICY;
}

/**
 * \brief Checks URL policy files to see if the player is allowed to send a given request header 
 * as part of a request for the given URL
 *
 * Waits for mutex at start and releases mutex when finished
 * \param url The URL of the request to which the request header belongs
 * \param header The request header to evaluate
 * \param loadPendingPolicies Whether or not to load (and thus check) pending policy files
 * \return \c ALLOWED if allowed or otherwise \c NA_HEADER
 */
SecurityManager::EVALUATIONRESULT SecurityManager::evaluateHeader(const URLInfo& url,
		const tiny_string& header, bool loadPendingPolicies)
{
	//This check doesn't apply to local files
	if(url.getProtocol() == "file" && sys->getOrigin().getProtocol() == "file")
		return ALLOWED;

	LOG(LOG_NO_INFO, _("SECURITY: Evaluating header for cross domain policies ('") << header << "'):");
	LOG(LOG_NO_INFO, _("SECURITY: --> URL: ") << url);
	LOG(LOG_NO_INFO, _("SECURITY: --> Origin: ") << sys->getOrigin());

	string headerStrLower(header.raw_buf());
	transform(headerStrLower.begin(), headerStrLower.end(), headerStrLower.begin(), ::tolower);
	string headerStr = headerStrLower;
	if(headerStr.find("_") != string::npos)
		headerStr.replace(headerStr.find("_"), 1, "-");

	//Disallowed headers, in any case
	if(headerStr == "accept-charset" &&	headerStr == "accept-encoding" &&	headerStr == "accept-ranges" &&
			headerStr == "age" &&	headerStr == "allow" && headerStr == "allowed" && headerStr == "authorization" &&
			headerStr == "charge-to" && headerStr == "connect" && headerStr == "connection" &&
			headerStr == "content-length" && headerStr == "content-location" && headerStr == "content-range" &&
			headerStr == "cookie" && headerStr == "date" && headerStr == "delete" && headerStr == "etag" &&
			headerStr == "expect" && headerStr == "get" && headerStr == "head" && headerStr == "host" &&
			headerStr == "if-modified-since" && headerStr == "keep-alive" && headerStr == "last-modified" &&
			headerStr == "location" && headerStr == "max-forwards" && headerStr == "options" &&
			headerStr == "origin" && headerStr == "post" && headerStr == "proxy-authenticate" &&
			headerStr == "proxy-authorization" && headerStr == "proxy-connection" && headerStr == "public" &&
			headerStr == "put" && headerStr == "range" && headerStr == "referer" && headerStr == "request-range" &&
			headerStr == "retry-after" && headerStr == "server" && headerStr == "te" && headerStr == "trace" &&
			headerStr == "trailer" && headerStr == "transfer-encoding" && headerStr == "upgrade" &&
			headerStr == "uri" && headerStr == "user-agent" && headerStr == "vary" && headerStr == "via" &&
			headerStr == "warning" && headerStr == "www-authenticate" && headerStr == "x-flash-version")
	{
		LOG(LOG_NO_INFO, _("SECURITY: DISALLOWED: Header is restricted"));
		return NA_HEADER;
	}

	//The URL has exactly the same domain name as the origin, always allowed
	if(url.getProtocol() == sys->getOrigin().getProtocol() &&
			url.getHostname() == sys->getOrigin().getHostname())
	{
		LOG(LOG_NO_INFO, _("SECURITY: ALLOWED: Same hostname as origin"));
		return ALLOWED;
	}

	//Search for the policy files to check
	URLPFileList* files = searchURLPolicyFiles(url, loadPendingPolicies);
	
	sem_wait(&mutex);
	//-- Lock acquired

	//Check the policy files
	if(files != NULL)
	{
		URLPFileListConstIt it = files->begin();
		for(; it != files->end(); ++it)
		{
			if((*it)->allowsHTTPRequestHeaderFrom(sys->getOrigin(), url, headerStrLower))
			{
				LOG(LOG_NO_INFO, _("SECURITY: ALLOWED: A policy file explicitly allowed the header"));
				delete files;

				//++ Release lock
				sem_post(&mutex);
				return ALLOWED;
			}
		}
	}

	LOG(LOG_NO_INFO, _("SECURITY: DISALLOWED: No policy file explicitly allowed the header"));
	delete files;

	//++ Release lock
	sem_post(&mutex);
	return NA_CROSSDOMAIN_POLICY;
}

/**
 * \brief Constructor for PolicyFile
 *
 * PolicyFile is an abstract base class so this method can only be called from a derived class.
 * \param _url The URL where this policy file resides
 * \param _type The type of policy file (URL or SOCKET)
 */
PolicyFile::PolicyFile(URLInfo _url, TYPE _type):
	url(_url),type(_type),valid(false),ignore(false),
	loaded(false),siteControl(NULL)
{
	sem_init(&mutex,0,1);
	//== Lock initialized
}

/**
 * \brief Destructor for PolicyFile.
 *
 * Can only be called from SecurityManager.
 * Waits for mutex before proceeding.
 * \see SecurityManager
 */
PolicyFile::~PolicyFile()
{
	sem_wait(&mutex);
	//-- Lock acquired
	
	for(list<PolicyAllowAccessFrom*>::iterator i = allowAccessFrom.begin(); 
			i != allowAccessFrom.end(); ++i)
		delete (*i);

	//== Destroy lock
	sem_destroy(&mutex);
}

/**
 * \brief Constructor for URLPolicyFile
 *
 * Can only be called from SecurityManager
 * \param _url The URL where this URL policy file resides
 * \see SecurityManager::addURLPolicyFile()
 */
URLPolicyFile::URLPolicyFile(const URLInfo& _url):
	PolicyFile(_url, URL),originalURL(_url)
{
	if(url.isValid())
		valid = true;

	if(url.getProtocol() == "http")
		subtype = HTTP;
	else if(url.getProtocol() == "https")
		subtype = HTTPS;
	else if(url.getProtocol() == "ftp")
		subtype = FTP;
}

/**
 * \brief Destructor for URLPolicyFile
 *
 * Can only be called from SecurityManager.
 * Waits for mutex at start and releases mutex when finished
 * \see SecurityManager
 */
URLPolicyFile::~URLPolicyFile()
{
	sem_wait(&mutex);
	//-- Lock acquired

	for(list<PolicyAllowHTTPRequestHeadersFrom*>::iterator i = allowHTTPRequestHeadersFrom.begin();
			i != allowHTTPRequestHeadersFrom.end(); ++i)
		delete (*i);

	//++ Release lock
	sem_post(&mutex);
}

/**
 * \brief Gets the master URL policy file controlling this URL policy file
 *
 * Searches for the master policy file in the list of managed policy files.
 * If it isn't found, the master policy file gets added.
 * Waits for mutex at start and releases mutex when finished
 * \return The master policy file controlling this policy file.
 */
URLPolicyFile* URLPolicyFile::getMasterPolicyFile()
{
	sem_wait(&mutex);
	//-- Lock acquired

	if(isMaster())
	{
		//++ Release lock
		sem_post(&mutex);
		return this;
	}

	URLPolicyFile* file = sys->securityManager->getURLPolicyFileByURL(url.goToURL("/crossdomain.xml"));
	if(file == NULL)
		file = sys->securityManager->addURLPolicyFile(url.goToURL("/crossdomain.xml"));

	//++ Release lock
	sem_post(&mutex);
	return file;
}

/**
 * \brief Checks whether this URL policy file is a master policy file
 *
 * \return \c true if this policy file is a master policy file, otherwise \c false
 */
bool URLPolicyFile::isMaster()
{
	return url.getPath() == "/crossdomain.xml";
}

/**
 * \brief Loads and parses an URLPolicy file
 *
 * Can only be called from within SecurityManager
 * Waits for mutex at start and releases mutex when finished
 * \see SecurityManager::loadURLPolicyFile()
 */
void URLPolicyFile::load()
{
	//TODO: support download timeout handling
	
	//Invalid URLPolicyFile or already loaded, ignore this call
	if(!isValid() || isLoaded())
		return;
	//We only try loading once, if something goes wrong, valid will be set to 'invalid'
	loaded = true;

	URLPolicyFile* master = getMasterPolicyFile();

	sem_wait(&mutex);
	//-- Lock acquired

	//Check if this file is allowed/ignored by the master policy file
	if(!isMaster())
	{
		//Load master policy file if not loaded yet
		sys->securityManager->loadPolicyFile(master);
		//Master policy file found and valid and has a site-control entry
		if(master->isValid() && master->getSiteControl() != NULL)
		{
			PolicySiteControl::METAPOLICY permittedPolicies = master->getSiteControl()->getPermittedPolicies();
			//For all types: master-only, none
			if(permittedPolicies == PolicySiteControl::MASTER_ONLY ||	permittedPolicies == PolicySiteControl::NONE)
				ignore = true;
			//Only for FTP: by-ftp-filename
			else if(subtype == FTP && permittedPolicies == PolicySiteControl::BY_FTP_FILENAME && 
					url.getPathFile() != "crossdomain.xml")
				ignore = true;
		}
	}

	//No caching needed for this download, we don't expect very big files
	Downloader* downloader=sys->downloadManager->download(url, false);

	//Wait until the file is fetched
	downloader->waitForTermination();
	if(downloader->hasFailed())
		valid = false;

	//If files are redirected, we use the new URL as the file's URL
	if(isValid() && downloader->isRedirected())
	{
		URLInfo newURL(downloader->getURL());
		if(url.getHostname() != newURL.getHostname())
		{
			LOG(LOG_NO_INFO, _("SECURITY: Policy file was redirected to other domain, marking invalid"));
			valid = false;
		}
		url = newURL;
		LOG(LOG_NO_INFO, _("SECURITY: Policy file was redirected"));
	}

	//Policy files must have on of the following content-types to be valid:
	//text/*, application/xml or application/xhtml+xml
	tiny_string contentType = downloader->getHeader("content-type");
	if(isValid() && (subtype == HTTP || subtype == HTTPS) && 
			contentType.substr(0, 5) != "text/" &&
			contentType != "application/xml" &&
			contentType != "application/xhtml+xml")
	{
		LOG(LOG_NO_INFO, _("SECURITY: Policy file has an invalid content-type, marking invalid"));
		valid = false;
	}

	//One more check from the master file: see if the content-type is OK 
	//if site-control specifies by-content-type
	if(isValid() && !isMaster())
	{
		//If the site-control policy of the master policy file is by-content-type, only policy files with
		//content-type = text/x-cross-domain-policy are allowed.
		if(master->isValid() && master->getSiteControl() != NULL &&
				(subtype == HTTP || subtype == HTTPS) &&
				master->getSiteControl()->getPermittedPolicies() == PolicySiteControl::BY_CONTENT_TYPE &&
				contentType != "text/x-cross-domain-policy")
		{
			LOG(LOG_NO_INFO, _("SECURITY: Policy file content-type isn't strict, marking invalid"));
			ignore = true;
		}
	}

	//We've checked the master file to see of we need to ignore this file. (not the case)
	//Now let's parse this file. A HTTP 404 results in a failed download.
	if(isValid() && !isIgnored())
	{
		istream s(downloader);
		size_t bufLength = downloader->getLength();
		uint8_t* buf=new uint8_t[bufLength];
		//TODO: avoid this useless copy
		s.read((char*)buf,bufLength);

		//We're done with the downloader, lets destroy ASAP
		sys->downloadManager->destroy(downloader);

		CrossDomainPolicy::POLICYFILESUBTYPE parserSubtype = CrossDomainPolicy::NONE;
		if(subtype == HTTP)
			parserSubtype = CrossDomainPolicy::HTTP;
		else if(subtype == HTTPS)
			parserSubtype = CrossDomainPolicy::HTTPS;
		else if(subtype == FTP)
			parserSubtype = CrossDomainPolicy::FTP;
		CrossDomainPolicy parser(buf, bufLength, CrossDomainPolicy::URL, parserSubtype, isMaster());
		CrossDomainPolicy::ELEMENT elementType = parser.getNextElement();

		while(elementType != CrossDomainPolicy::END && elementType != CrossDomainPolicy::INVALID)
		{
			if(elementType == CrossDomainPolicy::SITE_CONTROL)
			{
				siteControl = new PolicySiteControl(this, parser.getPermittedPolicies());
				//No more parsing is needed if this site-control entry specifies that
				//no policy files are allowed
				if(isMaster() &&
						siteControl->getPermittedPolicies() == PolicySiteControl::NONE)
					break;
			}
			else if(elementType == CrossDomainPolicy::ALLOW_ACCESS_FROM)
				//URL policy files don't use to-ports
				allowAccessFrom.push_back(new PolicyAllowAccessFrom(this, 
							parser.getDomain(), "", parser.getSecure(), parser.getSecureSpecified()));
			else if(elementType == CrossDomainPolicy::ALLOW_HTTP_REQUEST_HEADERS_FROM)
				allowHTTPRequestHeadersFrom.push_back(new PolicyAllowHTTPRequestHeadersFrom(this, 
							parser.getDomain(), parser.getHeaders(),
							parser.getSecure(), parser.getSecureSpecified()));

			elementType = parser.getNextElement();
		}

		//The last element was INVALID
		if(elementType == CrossDomainPolicy::INVALID)
			valid = false;

		if(isMaster())
		{
			//Set siteControl to the default value if it isn't set before and we are a master file
			if(siteControl == NULL)
				siteControl = new PolicySiteControl(this, "");

			//Ignore this file if the site control policy is "none"
			if(siteControl->getPermittedPolicies() == PolicySiteControl::NONE)
				ignore = true;
			//by-ftp-filename only applies to FTP
			if((subtype == HTTP || subtype == HTTPS) && 
					siteControl->getPermittedPolicies() == PolicySiteControl::BY_FTP_FILENAME)
				valid = false;
			//by-content-type only applies to HTTP(S)
			else if(subtype == FTP && 
					siteControl->getPermittedPolicies() == PolicySiteControl::BY_CONTENT_TYPE)
				valid = false;
		}
	}
	else
	{
		//Failed to download the file, marking this file as invalid
		valid = false;
		sys->downloadManager->destroy(downloader);
	}

	//++ Release lock
	sem_post(&mutex);
}

/**
 * \brief Checks whether this policy file allows the given URL access
 *
 * \param url The URL to check if it is allowed by the policy file
 * \param to The URL that is being requested by a resource at \c url
 * \return \c true if allowed, otherwise \c false
 */
bool URLPolicyFile::allowsAccessFrom(const URLInfo& url, const URLInfo& to)
{
	//File must be loaded
	if(!isLoaded())
		return false;

	//This policy file doesn't apply to the given URL
	if(!isMaster() && !to.isSubOf(url))
		return false;

	//Check if the file is invalid or ignored
	if(!isValid() || isIgnored())
		return false;

	list<PolicyAllowAccessFrom*>::const_iterator i = allowAccessFrom.begin();
	for(; i != allowAccessFrom.end(); ++i)
	{
		//This allow-access-from entry applies to our domain AND it allows our domain
		if((*i)->allowsAccessFrom(url))
			return true;
	}
	return false;
}

/**
 * \brief Checks whether this policy file allowed the given URL to send a given request header
 *
 * \param url The URL to check if it is allowed by the policy file to send the given header
 * \param to The URL of the request to which the request header belongs
 * \param header The request header which needs to be checked if it is allowed
 * \return \c true if allowed, otherwise \c false
 */
bool URLPolicyFile::allowsHTTPRequestHeaderFrom(const URLInfo& url, const URLInfo& to, const string& header)
{
	//File must be loaded
	if(!isLoaded())
		return false;

	//Only used for HTTP(S)
	if(subtype != HTTP && subtype != HTTPS)
		return false;

	//This policy file doesn't apply to the given URL
	if(!isMaster() && !to.isSubOf(url))
		return false;

	//Check if the file is invalid or ignored
	if(!isValid() || isIgnored())
		return false;

	list<PolicyAllowHTTPRequestHeadersFrom*>::const_iterator i = allowHTTPRequestHeadersFrom.begin();
	for(; i != allowHTTPRequestHeadersFrom.end(); ++i)
	{
		if((*i)->allowsHTTPRequestHeaderFrom(url, header))
			return true;
	}
	return false;
}

/**
 * \brief Constructor for the PolicySiteControl class
 *
 * Can only be called from within PolicyFile.
 * \param _file The policy file this entry belongs to
 * \param _permittedPolicies The value of the permitted-cross-domain-policies attribute.
 * \see URLPolicyFile::load()
 */
PolicySiteControl::PolicySiteControl(PolicyFile* _file, const string _permittedPolicies):
	file(_file)
{
	if(_permittedPolicies == "all")
		permittedPolicies = ALL;
	else if(_permittedPolicies == "by-content-type")
		permittedPolicies = BY_CONTENT_TYPE;
	else if(_permittedPolicies == "by-ftp-filename")
		permittedPolicies = BY_FTP_FILENAME;
	else if(_permittedPolicies == "master-only")
		permittedPolicies = MASTER_ONLY;
	else if(_permittedPolicies == "none")
		permittedPolicies = NONE;
	else if(file->getType() == PolicyFile::SOCKET) //Default for SOCKET
		permittedPolicies = ALL;
	else //Default for everything except SOCKET
		permittedPolicies = MASTER_ONLY;
}

/**
 * \brief Constructor for the PolicyAllowAccessFrom class
 *
 * Can only be called from within PolicyFile.
 * \param _file The policy file this entry belongs to
 * \param _domain The value of the domain attribute
 * \param _toPorts The value of the to-ports attribute
 * \param _secure The value of the secure attribute
 * \param secureSpecified Whether or not the secure attribute was present
 * \see URLPolicyFile::load()
 */
PolicyAllowAccessFrom::PolicyAllowAccessFrom(PolicyFile* _file, const string _domain,
		const string _toPorts, bool _secure, bool secureSpecified):
	file(_file),domain(_domain),secure(_secure)
{
	if(!secureSpecified)
	{
		if(file->getType() == PolicyFile::URL &&
				dynamic_cast<URLPolicyFile*>(file)->getSubtype() == URLPolicyFile::HTTPS)
			secure = true;
		if(file->getType() == PolicyFile::SOCKET)
			secure = false;
	}

	//Set the default value
	if(_toPorts.length() == 0 || _toPorts == "*")
		toPorts.push_back(new PortRange(0, 0, false, true));
	else
	{
		string ports = _toPorts;
		size_t cursor = 0;
		size_t commaPos;
		size_t dashPos;
		string startPortStr;
		string endPortStr;
		istringstream is;
		uint16_t startPort;
		uint16_t endPort;
		do
		{
			commaPos = ports.find(",", cursor);
			dashPos = ports.find("-", commaPos);

			if(dashPos != string::npos)
			{
				startPortStr = ports.substr(cursor, dashPos-cursor);
				if(startPortStr == "*")
				{
					toPorts.push_back(new PortRange(0, 0, false, true));
					break;
				}
				startPort = atoi(startPortStr.c_str());
				if(ports.length() < dashPos+1)
				{
					endPortStr = ports.substr(dashPos+1, commaPos-(dashPos+1));
					endPort = atoi(endPortStr.c_str());
					toPorts.push_back(new PortRange(startPort, endPort, true));
				}
				else
					toPorts.push_back(new PortRange(startPort));
			}
			else
			{
				startPortStr = ports.substr(cursor, commaPos-cursor);
				if(startPortStr == "*")
				{
					toPorts.push_back(new PortRange(0, 0, false, true));
					break;
				}
				startPort = atoi(startPortStr.c_str());
				toPorts.push_back(new PortRange(startPort));
			}
			cursor = commaPos+1;
		}
		while(commaPos != string::npos);
	}
}

/**
 * \brief Destructor for the PolicyAllowsAccessFrom class
 *
 * Can only be called from within PolicyFile.
 * \see PolicyFile
 */
PolicyAllowAccessFrom::~PolicyAllowAccessFrom()
{
	for(list<PortRange*>::iterator i = toPorts.begin(); i != toPorts.end(); ++i)
		delete (*i);
	toPorts.clear();
}

/**
 * \brief Checks if the entry allows access from the given URL
 *
 * \param url The URL to check this entry against
 * \return \c true if this entry allows the given URL access, otherwise \c false
 */
bool PolicyAllowAccessFrom::allowsAccessFrom(const URLInfo& url) const
{
	//TODO: resolve domain names using DNS before checking for a match?
	//See section 1.5.9 in specification

	//TODO: add check for to-ports and secure for SOCKET type policy files
	
	//Check if domains match
	if(!URLInfo::matchesDomain(domain, url.getHostname()))
		return false;

	//Check if the requesting URL is secure, if needed
	if(file->getType() == PolicyFile::URL && 
			dynamic_cast<URLPolicyFile*>(file)->getSubtype() == URLPolicyFile::HTTPS && 
			secure && url.getProtocol() != "https")
		return false;

	return true;
}

/**
 * \brief Constructor for the PolicyAllowHTTPRequestHeadersFrom class
 *
 * Can only be called from within PolicyFile.
 * \param _file The policy file this entry belongs to
 * \param _domain The value of the domain attribute
 * \param _headers The value of the header attribute
 * \param _secure The value of the secure attribute
 * \param secureSpecified Whether or not the secure attribute was present
 * \see URLPolicyFile::load()
 */
PolicyAllowHTTPRequestHeadersFrom::PolicyAllowHTTPRequestHeadersFrom(URLPolicyFile* _file,
		const string _domain, const string _headers, bool _secure, bool secureSpecified):
	file(_file),domain(_domain),secure(_secure)
{
	if(!secureSpecified && file->getSubtype() == URLPolicyFile::HTTPS)
			secure = true;

	if(_headers.length() == 0 || _headers == "*")
		headers.push_back(new string("*"));
	else {
		string headersStr = _headers;
		size_t cursor = 0;
		size_t commaPos;
		do
		{
			commaPos = headersStr.find(",", cursor);
			headers.push_back(new string(headersStr.substr(cursor, commaPos-cursor)));
			cursor = commaPos+1;
		}
		while(commaPos != string::npos);
	}
}

/**
 * \brief Destructor for the PolicyAllowHTTPRequestHeadersFrom class
 *
 * Can only be called from within PolicyFile.
 * \see PolicyFile
 */
PolicyAllowHTTPRequestHeadersFrom::~PolicyAllowHTTPRequestHeadersFrom()
{
	for(list<string*>::iterator i = headers.begin(); i != headers.end(); ++i)
		delete (*i);
	headers.clear();
}

/**
 * \brief Checks if the entry allows a given request header from a given URL
 *
 * \param url The URL to check this entry against
 * \param header The header to check this entry against
 * \return \c true if this entry allows the given URL to send the given request header, otherwise \c false
 */
bool PolicyAllowHTTPRequestHeadersFrom::allowsHTTPRequestHeaderFrom(const URLInfo& url,
		const string& header) const
{
	if(file->getSubtype() != URLPolicyFile::HTTP && file->getSubtype() != URLPolicyFile::HTTPS)
		return false;

	//Check if domains match
	if(!URLInfo::matchesDomain(domain, url.getHostname()))
		return false;

	//Check if the requesting URL is secure, if needed
	if(file->getSubtype() == URLPolicyFile::HTTPS && secure && url.getProtocol() != "https")
		return false;


	//Check if the header is explicitly allowed
	bool headerFound = false;
	string expression;
	for(list<string*>::const_iterator i = headers.begin();
			i != headers.end(); ++i)
	{
		expression = (**i);
		transform(expression.begin(), expression.end(), expression.begin(), ::tolower);
		if(expression == header || expression == "*")
		{
			headerFound = true;
			break;
		}
		//Match suffix wildcards
		else if(expression[expression.length()-1] == '*' &&
				header.substr(0, expression.length()-1) == expression.substr(0, expression.length()-1))
		{
			headerFound = true;
			break;
		}
	}
	if(!headerFound)
		return false;

	return true;
}
