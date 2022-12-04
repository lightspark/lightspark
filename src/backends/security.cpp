/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010-2011  Timon Van Overveldt (timonvo@gmail.com)

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
#include "scripting/class.h"
#include "scripting/toplevel/Error.h"
#include "scripting/flash/net/XMLSocket.h"
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>

#include "backends/security.h"

using namespace lightspark;
using namespace std;

const unsigned int SocketPolicyFile::MASTER_PORT = 843;
const char *SocketPolicyFile::MASTER_PORT_URL = ":843";

/**
 * \brief SecurityManager constructor
 */
SecurityManager::SecurityManager():
	sandboxType(REMOTE),exactSettings(true),exactSettingsLocked(false)
{
	sandboxNames[0] = "remote";
	sandboxNames[1] = "localWithFile";
	sandboxNames[2] = "localWithNetwork";
	sandboxNames[3] = "localTrusted";

	sandboxTitles[0] = "remote";
	sandboxTitles[1] = "local-with-filesystem";
	sandboxTitles[2] = "local-with-networking";
	sandboxTitles[3] = "local-trusted";

	//== Lock initialized
}

/**
 * \brief SecurityManager destructor
 *
 * SecurityManager destructor. Acquires mutex before proceeding.
 */
SecurityManager::~SecurityManager()
{
	Locker l(mutex);

	URLPFileMapIt i = pendingURLPFiles.begin();
	for(; i != pendingURLPFiles.end(); ++i)
		delete (*i).second;

	i = loadedURLPFiles.begin();
	for(;i != loadedURLPFiles.end(); ++i)
		delete (*i).second;
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
		return addSocketPolicyFile(url);
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
	Locker l(mutex);

	URLPolicyFile* file = new URLPolicyFile(url);
	if(file->isValid())
	{
		LOG(LOG_INFO, 
				"SECURITY: Added URL policy file is valid, adding to URL policy file list (" << url << ")");
		pendingURLPFiles.insert(URLPFilePair(url.getHostname(), file));
	}

	return file;
}

/**
 * \brief Add a socket policy file at the given URL
 *
 * Adds an socket policy file at the given URL to the list of managed socket policy files.
 * Policy files aren't loaded when adding, this is delayed until the policy file is actually needed.
 * Waits for mutex at start and releases mutex when finished
 * \param url The URL where the socket policy file resides
 * \return A pointer to the newly created SocketPolicyFile object
 */
SocketPolicyFile* SecurityManager::addSocketPolicyFile(const URLInfo& url)
{
	Locker l(mutex);

	SocketPolicyFile* file = new SocketPolicyFile(url);
	if(file->isValid())
	{
		LOG(LOG_INFO, "SECURITY: Added socket policy file is valid, adding to socket policy file list (" << url << ")");
		pendingSocketPFiles.insert(SocketPFilePair(url.getHostname(), file));
	}

	return file;
}

template <class T>
T* SecurityManager::getPolicyFileByURL(std::multimap<tiny_string, T*>& pendingFiles, std::multimap<tiny_string, T*>& loadedFiles, const URLInfo& url)
{
	Locker l(mutex);
	std::pair< typename std::multimap<tiny_string, T*>::iterator, typename std::multimap<tiny_string, T*>::iterator > range;
	typename std::multimap<tiny_string, T*>::iterator i;

	//Check the loaded URL policy files first
	range = loadedFiles.equal_range(url.getHostname());
	for(i = range.first; i != range.second; ++i)
	{
		if((*i).second->getOriginalURL() == url)
		{
			LOG(LOG_INFO, "SECURITY: URL policy file found in loaded list (" << url << ")");

			return (*i).second;
		}
	}

	//Check the pending URL policy files next
	range = pendingFiles.equal_range(url.getHostname());
	for(i = range.first; i != range.second; ++i)
	{
		if((*i).second->getOriginalURL() == url)
		{
			LOG(LOG_INFO, "SECURITY: URL policy file found in pending list (" << url << ")");

			return (*i).second;
		}
	}

	return nullptr;
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
	return getPolicyFileByURL<URLPolicyFile>(pendingURLPFiles, loadedURLPFiles, url);
}

/**
 * \brief Search for an socket policy file object in lists
 *
 * Searches for a socket policy file object in the lists of managed socket policy files by URL.
 * Returns NULL when such an object could not be found.
 * Waits for mutex at start and releases mutex when finished
 * \param url The URL where the socket policy file object would reside.
 * \return The found socket policy file object or NULL
 */
SocketPolicyFile* SecurityManager::getSocketPolicyFileByURL(const URLInfo& url)
{
	return getPolicyFileByURL<SocketPolicyFile>(pendingSocketPFiles, loadedSocketPFiles, url);
}

template <class T>
void SecurityManager::loadPolicyFile(std::multimap<tiny_string, T*>& pendingFiles, std::multimap<tiny_string, T*>& loadedFiles, PolicyFile *file)
{
	Locker l(mutex);

	if(pendingFiles.count(file->getURL().getHostname()) > 0)
	{
		LOG(LOG_INFO, "SECURITY: Loading policy file (" << file->getURL() << ")");

		// Policy files are downloaded in blocking manner,
		// release the lock during loading
		l.release();

		file->load();

		l.acquire();

		std::pair< typename std::multimap<tiny_string, T*>::iterator, typename std::multimap<tiny_string, T*>::iterator > range;
		range = pendingFiles.equal_range(file->getURL().getHostname());
		typename std::multimap<tiny_string, T*>::iterator i;
		for(i = range.first; i != range.second; ++i)
		{
			if((*i).second == file)
			{
				loadedFiles.insert(make_pair(file->getURL().getHostname(), (*i).second));
				pendingFiles.erase(i);
				break;
			}
		}
	}
}

/**
 * \brief Load a given URL policy file object
 *
 * Loads a given URL policy file object (if it isn't loaded yet).
 * This moves the object from the pending URL policy files list to the loaded URL policy files list.
 * Waits for mutex at start and releases mutex when finished
 * \param file A pointer to the URL policy file object to load.
 */
void SecurityManager::loadURLPolicyFile(URLPolicyFile* file)
{
	loadPolicyFile<URLPolicyFile>(pendingURLPFiles, loadedURLPFiles, file);
}

/**
 * \brief Load a given socket policy file object
 *
 * Loads a given socket policy file object (if it isn't loaded yet).
 * This moves the object from the pending socekt policy files list to the loaded socket policy files list.
 * Waits for mutex at start and releases mutex when finished
 * \param file A pointer to the socket policy file object to load.
 */
void SecurityManager::loadSocketPolicyFile(SocketPolicyFile* file)
{
	loadPolicyFile<SocketPolicyFile>(pendingSocketPFiles, loadedSocketPFiles, file);
}

template<class T>
std::list<T*> *SecurityManager::searchPolicyFiles(const URLInfo& url,
						  T *master,
						  bool loadPendingPolicies,
						  std::multimap<tiny_string, T*>& pendingFiles,
						  std::multimap<tiny_string, T*>& loadedFiles)
{
	std::list<T*> *result = new std::list<T*>;

	Locker l(mutex);
	//Check if the master policy file is loaded.
	//If another user-added relevant policy file is already loaded, 
	//it's master will have already been loaded too (to check if it is allowed).
	//So IF any relevant policy file is loaded already, then the master will be too.
	if(master->isLoaded() && master->isValid())
	{
		LOG(LOG_INFO, "SECURITY: Master policy file is loaded and valid (" << url << ")");

		PolicyFile::METAPOLICY siteControl = master->getMetaPolicy();
		//Master defines no policy files are allowed at all
		if(siteControl == PolicyFile::NONE)
		{
			LOG(LOG_INFO, "SECURITY: DISALLOWED: Master policy file disallows policy files");
			delete result;
			return nullptr;
		}

		result->push_back(master);

		//Non-master policy files are allowed
		if(siteControl != PolicyFile::MASTER_ONLY)
		{
			LOG(LOG_INFO, "SECURITY: Searching for loaded non-master policy files (" <<
					loadedFiles.count(url.getHostname()) << ")");

			std::pair< typename std::multimap<tiny_string, T*>::iterator, typename std::multimap<tiny_string, T*>::iterator > range;
			typename std::multimap<tiny_string, T*>::const_iterator i;
			range = loadedFiles.equal_range(url.getHostname());
			i = range.first;
			for(;i != range.second; ++i)
			{
				if((*i).second == master)
					continue;
				result->push_back((*i).second);
			}

			//And check the pending policy files next (if we are allowed to)
			if(loadPendingPolicies)
			{
				LOG(LOG_INFO, "SECURITY: Searching for and loading pending non-master policy files (" <<
						pendingFiles.count(url.getHostname()) << ")");

				while(true)
				{
					i=pendingFiles.find(url.getHostname());
					if(i==pendingFiles.end())
						break;

					result->push_back((*i).second);

					l.release();
					getSys()->securityManager->loadPolicyFile<T>(pendingFiles, loadedFiles, (*i).second);
					//NOTE: loadURLPolicyFile() will change pendingURLPFiles,
					//erasing & moving to loadURLPFiles. Therefore, the
					//iterator i is now invalid.
					l.acquire();
				}
			}
		}
	}

	return result;
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
	//Get or create the master policy file object
	URLInfo masterURL = url.goToURL("/crossdomain.xml");
	URLPolicyFile* master = getURLPolicyFileByURL(masterURL);

	if(master == nullptr)
		master = addURLPolicyFile(masterURL);

	if(loadPendingPolicies)
		getSys()->securityManager->loadURLPolicyFile(master);

	//Get applicable policy files
	return searchPolicyFiles<URLPolicyFile>(url, master, loadPendingPolicies,
						pendingURLPFiles, loadedURLPFiles);
}

/**
 * \brief Search for socket policy files relevant to a given URL
 *
 * Searches the loaded socket policy file list for policy files that are relevant to a given URL.
 * If \c loadPendingPolicies is true, it search the pending socket policy files list next, 
 * loading every relative policy file.
 * Waits for mutex at start and releases mutex when finished
 * \param url The URL that will be evaluated using the relevant policy files.
 * \param loadPendingPolicies Whether or not to load (and thus check) pending socket policy files.
 * \return An pointer to a newly created SocketPFileList containing the relevant policy files.
 *         This list needs to be deleted after use.
 */
SocketPFileList* SecurityManager::searchSocketPolicyFiles(const URLInfo& url, bool loadPendingPolicies)
{
	//Get or create the master policy file object
	URLInfo masterURL = url.goToURL(SocketPolicyFile::MASTER_PORT_URL);
	SocketPolicyFile* master = getSocketPolicyFileByURL(masterURL);

	if(master == nullptr)
		master = addSocketPolicyFile(masterURL);

	if(loadPendingPolicies)
		getSys()->securityManager->loadSocketPolicyFile(master);

	//Get applicable policy files
	SocketPFileList *result;
	result = searchPolicyFiles<SocketPolicyFile>(url, master, loadPendingPolicies,
						     pendingSocketPFiles, loadedSocketPFiles);

	//The target port is checked last if allowed
	if(master->isLoaded() && master->isValid() && (result != nullptr))
	{
		if (master->getMetaPolicy() == PolicyFile::ALL)
		{
			SocketPolicyFile* destination = getSocketPolicyFileByURL(url);
			if (destination == nullptr)
			{
				//Create and add policy file if it
				//didn't exist. If it exists
				//searchPolicyFiles() already added it
				//to result.
				destination = addSocketPolicyFile(url);
				if(loadPendingPolicies)
					getSys()->securityManager->loadSocketPolicyFile(destination);
				result->push_back(destination);
			}
		}
	}

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
 * \brief Evaluates an URL static properties to see if it is allowed by the various security features.
 *
 * This first checks if the current sandbox is allowed access to the URL.
 * Next the port is checked to see if isn't restricted.
 * Then, if requested, the URL is checked to see if it doesn't point to a resource 
 * above the current directory in the directory hierarchy (only applicable to local URLs).
 * \param url The URL to evaluate
 * \param allowedSandboxesRemote The sandboxes that are allowed to access remote URLs in this case.
 *                               Can be a bitwise expression of sandboxes.
 * \param allowedSandboxesLocal The sandboxes that are allowed to access local URLs in this case.
 *                              Can be a bitwise expression of sandboxes.
 * \return \c ALLOWED if the URL is allowed or one of \c NA_*, where * is the reason for not allowing.
 * \see SecurityManager::evaluateSandboxURL()
 * \see SecurityManager::evaluatePortURL()
 * \see SecurityManager::evaluateLocalDirectoryURL()
 */
SecurityManager::EVALUATIONRESULT SecurityManager::evaluateURLStatic(const URLInfo& url,
		int allowedSandboxesRemote, int allowedSandboxesLocal, bool restrictLocalDirectory)
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

	//All checks passed, so we allow the URL connection
	return ALLOWED;
}

/**
 * \brief Throw SecurityError if URL doesn't satisfy security constrains.
 */
void SecurityManager::checkURLStaticAndThrow(const URLInfo& url,
					     int allowedSandboxesRemote, 
					     int allowedSandboxesLocal,
					     bool restrictLocalDirectory)
{
	SecurityManager::EVALUATIONRESULT evaluationResult = 
		getSys()->securityManager->evaluateURLStatic(url, allowedSandboxesRemote,
			allowedSandboxesLocal, restrictLocalDirectory);
	//Network sandboxes can't access local files (this should be a SecurityErrorEvent)
	if(evaluationResult == SecurityManager::NA_REMOTE_SANDBOX)
		createError<SecurityError>(getSys()->worker,0,"SecurityError: "
				"connect to network");
	//Local-with-filesystem sandbox can't access network
	else if(evaluationResult == SecurityManager::NA_LOCAL_SANDBOX)
		createError<SecurityError>(getSys()->worker,0,"SecurityError: "
				"connect to local file");
	else if(evaluationResult == SecurityManager::NA_PORT)
		createError<SecurityError>(getSys()->worker,0,"SecurityError: "
				"connect to restricted port");
	else if(evaluationResult == SecurityManager::NA_RESTRICT_LOCAL_DIRECTORY)
		createError<SecurityError>(getSys()->worker,0,"SecurityError: "
				"not allowed to navigate up for local files");
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
	if(url.getProtocol() == "file" && !url.isSubOf(getSys()->mainClip->getOrigin()))
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
	//This check doesn't apply to data generation mode
	if(url.isEmpty())
		return ALLOWED;

	//This check doesn't apply to local files
	if(url.getProtocol() == "file" && getSys()->mainClip->getOrigin().getProtocol() == "file")
		return ALLOWED;

	// no need to check if we trust the source
	if (getSys()->mainClip->getOrigin().getProtocol() == "file" && sandboxType == LOCAL_TRUSTED)
		return ALLOWED;

	//Streaming from RTMP is always allowed (see
	//http://forums.adobe.com/thread/422391)
	if(url.isRTMP())
		return ALLOWED;

	LOG(LOG_INFO, "SECURITY: Evaluating URL for cross domain policies:");
	LOG(LOG_INFO, "SECURITY: --> URL:    " << url);
	LOG(LOG_INFO, "SECURITY: --> Origin: " << getSys()->mainClip->getOrigin());

	//The URL has exactly the same domain name as the origin, always allowed
	if(url.getProtocol() == getSys()->mainClip->getOrigin().getProtocol() &&
			url.getHostname() == getSys()->mainClip->getOrigin().getHostname())
	{
		LOG(LOG_INFO, "SECURITY: Same hostname as origin, allowing");
		return ALLOWED;
	}

	//Search for the policy files to check
	URLPFileList* files = searchURLPolicyFiles(url, loadPendingPolicies);

	Locker l(mutex);

	//Check the policy files
	if(files != nullptr)
	{
		URLPFileListConstIt it = files->begin();
		for(; it != files->end(); ++it)
		{
			if((*it)->allowsAccessFrom(getSys()->mainClip->getOrigin(), url))
			{
				LOG(LOG_INFO, "SECURITY: ALLOWED: A policy file explicitly allowed access");
				delete files;
				return ALLOWED;
			}
		}
	}

	LOG(LOG_INFO, "SECURITY: DISALLOWED: No policy file explicitly allowed access:"<<url);
	delete files;

	return NA_CROSSDOMAIN_POLICY;
}

SecurityManager::EVALUATIONRESULT SecurityManager::evaluateSocketConnection(const URLInfo& url,
									    bool loadPendingPolicies)
{
	if(url.getProtocol() != "xmlsocket")
		return NA_CROSSDOMAIN_POLICY;
	// allow local socket connections if we trust the source
	if(sandboxType == LOCAL_TRUSTED && (url.getHostname() == "127.0.0.1" || url.getHostname() == "localhost"))
		return ALLOWED;

	LOG(LOG_INFO, "SECURITY: Evaluating socket policy:");
	LOG(LOG_INFO, "SECURITY: --> URL:    " << url);
	LOG(LOG_INFO, "SECURITY: --> Origin: " << getSys()->mainClip->getOrigin());

	//Search for the policy files to check
	SocketPFileList* files = searchSocketPolicyFiles(url, loadPendingPolicies);

	Locker l(mutex);

	//Check the policy files
	if(files != nullptr)
	{
		SocketPFileListConstIt it = files->begin();
		for(; it != files->end(); ++it)
		{
			if((*it)->allowsAccessFrom(getSys()->mainClip->getOrigin(), url))
			{
				LOG(LOG_INFO, "SECURITY: ALLOWED: A policy file explicitly allowed access");
				delete files;
				return ALLOWED;
			}
		}
	}

	LOG(LOG_INFO, "SECURITY: DISALLOWED: No policy file explicitly allowed access");
	delete files;

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
	//This check doesn't apply to data generation mode
	if (url.isEmpty())
		return ALLOWED;
	//This check doesn't apply to local files
	if(url.getProtocol() == "file" && getSys()->mainClip->getOrigin().getProtocol() == "file")
		return ALLOWED;

	LOG(LOG_INFO, "SECURITY: Evaluating header for cross domain policies ('" << header << "'):");
	LOG(LOG_INFO, "SECURITY: --> URL: " << url);
	LOG(LOG_INFO, "SECURITY: --> Origin: " << getSys()->mainClip->getOrigin());

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
		LOG(LOG_INFO, "SECURITY: DISALLOWED: Header is restricted");
		return NA_HEADER;
	}

	//The URL has exactly the same domain name as the origin, always allowed
	if(url.getProtocol() == getSys()->mainClip->getOrigin().getProtocol() &&
			url.getHostname() == getSys()->mainClip->getOrigin().getHostname())
	{
		LOG(LOG_INFO, "SECURITY: ALLOWED: Same hostname as origin");
		return ALLOWED;
	}

	//Search for the policy files to check
	URLPFileList* files = searchURLPolicyFiles(url, loadPendingPolicies);

	Locker l(mutex);

	//Check the policy files
	if(files != nullptr)
	{
		URLPFileListConstIt it = files->begin();
		for(; it != files->end(); ++it)
		{
			if((*it)->allowsHTTPRequestHeaderFrom(getSys()->mainClip->getOrigin(), url, headerStrLower))
			{
				LOG(LOG_INFO, "SECURITY: ALLOWED: A policy file explicitly allowed the header");
				delete files;
				return ALLOWED;
			}
		}
	}

	LOG(LOG_INFO, "SECURITY: DISALLOWED: No policy file explicitly allowed the header");
	delete files;

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
	originalURL(_url),url(_url),type(_type),valid(false),ignore(false),
	loaded(false),siteControl(nullptr)
{
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
	Locker l(mutex);
	for(list<PolicyAllowAccessFrom*>::iterator i = allowAccessFrom.begin();
			i != allowAccessFrom.end(); ++i)
		delete (*i);

	if (siteControl)
		delete siteControl;
}

/**
 * \brief Loads and parses a policy file
 *
 * Can only be called from within SecurityManager
 * Waits for mutex at start and releases mutex when finished
 * \see SecurityManager::loadURLPolicyFile()
 */
void PolicyFile::load()
{
	Locker l(mutex);
	//TODO: support download timeout handling
	
	//Invalid URLPolicyFile or already loaded, ignore this call
	if(!isValid() || isLoaded())
		return;
	
	ignore = isIgnoredByMaster();

	//Download the policy file
	vector<unsigned char> policy;
	if (!isIgnored())
		valid = retrievePolicyFile(policy);


	if (isLoaded())
	{
		// Another thread already processed this PolicyFile
		// while we were downloading
		return;
	}

	//We only try loading once, if something goes wrong, valid will be set to 'invalid'
	loaded = true;

	//We've checked the master file to see of we need to ignore this file. (not the case)
	//Now let's parse this file.
	if(isValid() && !isIgnored())
	{
		CrossDomainPolicy::POLICYFILETYPE parserType;
		CrossDomainPolicy::POLICYFILESUBTYPE parserSubtype;
		getParserType(parserType, parserSubtype);
		CrossDomainPolicy parser(&policy[0], policy.size(),
					 parserType, parserSubtype, isMaster());
		CrossDomainPolicy::ELEMENT elementType = parser.getNextElement();

		while(elementType != CrossDomainPolicy::END && elementType != CrossDomainPolicy::INVALID)
		{
			handlePolicyElement(elementType, parser);

			//No more parsing is needed if this site-control entry specifies that
			//no policy files are allowed
			if (elementType == CrossDomainPolicy::SITE_CONTROL && isMaster() && 
			    getMetaPolicy() == PolicyFile::NONE)
			{
				break;
			}

			elementType = parser.getNextElement();
		}

		//The last element was INVALID
		if(elementType == CrossDomainPolicy::INVALID)
			valid = false;

		if(isMaster())
		{
			//Ignore this file if the site control policy is "none"
			if(getMetaPolicy() == PolicyFile::NONE)
				ignore = true;

			valid = checkSiteControlValidity();
		}
	}
	else
	{
		valid = false;
	}
}

bool PolicyFile::checkSiteControlValidity()
{
	return true;
}

void PolicyFile::handlePolicyElement(CrossDomainPolicy::ELEMENT& elementType, 
				     const CrossDomainPolicy& parser)
{
	if(elementType == CrossDomainPolicy::SITE_CONTROL)
	{
		if (siteControl)
		{
			// From spec: If site-control is defined
			// multiple times, "none" takes preference.
			if (getMetaPolicy() == NONE)
				return;

			delete siteControl;
			siteControl = NULL;
		}

		siteControl = new PolicySiteControl(parser.getPermittedPolicies());
	}
	else if(elementType == CrossDomainPolicy::ALLOW_ACCESS_FROM)
	{
		allowAccessFrom.push_back(new PolicyAllowAccessFrom(this, 
			parser.getDomain(), parser.getToPorts(), parser.getSecure(), parser.getSecureSpecified()));
	}
}

PolicyFile::METAPOLICY PolicyFile::getMetaPolicy()
{
	if (!isMaster())
	{
		// Only the master can have site-control
		return PolicyFile::NONE;
	}
	else if (siteControl)
	{
		return siteControl->getPermittedPolicies();
	}
	else if (isValid() && isLoaded())
	{
		// Policy file is loaded but does not contain
		// site-control tag
		return PolicySiteControl::defaultSitePolicy(getType());
	}
	else
	{
		// Policy file is invalid, deny everything
		return PolicyFile::NONE;
	}
}

/**
 * \brief Constructor for URLPolicyFile
 *
 * Can only be called from SecurityManager
 * \param _url The URL where this URL policy file resides
 * \see SecurityManager::addURLPolicyFile()
 */
URLPolicyFile::URLPolicyFile(const URLInfo& _url):
	PolicyFile(_url, URL)
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
	Locker l(mutex);
	for(list<PolicyAllowHTTPRequestHeadersFrom*>::iterator i = allowHTTPRequestHeadersFrom.begin();
			i != allowHTTPRequestHeadersFrom.end(); ++i)
		delete (*i);
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
	Locker l(mutex);

	if(isMaster())
		return this;

	URLPolicyFile* file = getSys()->securityManager->getURLPolicyFileByURL(url.goToURL("/crossdomain.xml"));
	if(file == nullptr)
		file = getSys()->securityManager->addURLPolicyFile(url.goToURL("/crossdomain.xml"));

	return file;
}

/**
 * \brief Checks whether this URL policy file is a master policy file
 *
 * \return \c true if this policy file is a master policy file, otherwise \c false
 */
bool URLPolicyFile::isMaster() const
{
	return url.getPath() == "/crossdomain.xml";
}

bool URLPolicyFile::isIgnoredByMaster()
{
	if(!isMaster())
	{
		//Load master policy file if not loaded yet
		URLPolicyFile* master = getMasterPolicyFile();
		getSys()->securityManager->loadURLPolicyFile(master);
		//Master policy file found and valid and has a site-control entry
		if(master->isValid())
		{
			PolicyFile::METAPOLICY permittedPolicies = master->getMetaPolicy();
			//For all types: master-only, none
			if(permittedPolicies == PolicyFile::MASTER_ONLY || permittedPolicies == PolicyFile::NONE)
				return true;
			//Only for FTP: by-ftp-filename
			else if(subtype == FTP && permittedPolicies == PolicyFile::BY_FTP_FILENAME && 
					url.getPathFile() != "crossdomain.xml")
				return true;
		}
	}

	return false;
}

bool URLPolicyFile::retrievePolicyFile(vector<unsigned char>& outData)
{
	bool ok = true;

	//No caching needed for this download, we don't expect very big files
	Downloader* downloader=getSys()->downloadManager->download(url, _MR(new MemoryStreamCache(getSys())), nullptr);

	//Wait until the file is fetched
	downloader->waitForTermination();
	if(downloader->hasFailed())
		ok = false;

	//If files are redirected, we use the new URL as the file's URL
	if(ok && downloader->isRedirected())
	{
		URLInfo newURL(downloader->getURL());
		if(url.getHostname() != newURL.getHostname())
		{
			LOG(LOG_INFO, "SECURITY: Policy file was redirected to other domain, marking invalid");
			ok = false;
		}
		url = newURL;
		LOG(LOG_INFO, "SECURITY: Policy file was redirected");
	}

	//Policy files must have on of the following content-types to be valid:
	//text/*, application/xml or application/xhtml+xml
	std::list<tiny_string> contenttypelist = downloader->getHeader("content-type").split(';');
	tiny_string contentType = contenttypelist.size() == 0 ? "" : contenttypelist.front();
	if(ok && (subtype == HTTP || subtype == HTTPS) && 
	   contentType.substr(0, 5) != "text/" &&
	   contentType != "application/xml" &&
	   contentType != "application/xhtml+xml")
	{
		LOG(LOG_INFO, "SECURITY: Policy file has an invalid content-type, marking invalid");
		ok = false;
	}

	//One more check from the master file: see if the content-type is OK 
	//if site-control specifies by-content-type
	if(ok && !isMaster())
	{
		//If the site-control policy of the master policy file is by-content-type, only policy files with
		//content-type = text/x-cross-domain-policy are allowed.
		URLPolicyFile* master = getMasterPolicyFile();
		if(master->isValid() &&
				(subtype == HTTP || subtype == HTTPS) &&
				master->getMetaPolicy() == PolicyFile::BY_CONTENT_TYPE &&
				contentType != "text/x-cross-domain-policy")
		{
			LOG(LOG_INFO, "SECURITY: Policy file content-type isn't strict, marking invalid");
			ignore = true;
		}
	}

	if (ok)
	{
		std::streambuf *sbuf = downloader->getCache()->createReader();
		istream s(sbuf);
		size_t bufLength = downloader->getLength();
		size_t offset = outData.size();
		outData.resize(offset+bufLength);
		s.read((char*)&outData[offset], bufLength);
		delete sbuf;
	}

	getSys()->downloadManager->destroy(downloader);

	return ok;
}

void URLPolicyFile::getParserType(CrossDomainPolicy::POLICYFILETYPE& parserType,
				  CrossDomainPolicy::POLICYFILESUBTYPE& parserSubtype)
{
	parserType = CrossDomainPolicy::URL;
	parserSubtype = CrossDomainPolicy::NONE;
	if(subtype == HTTP)
		parserSubtype = CrossDomainPolicy::HTTP;
	else if(subtype == HTTPS)
		parserSubtype = CrossDomainPolicy::HTTPS;
	else if(subtype == FTP)
		parserSubtype = CrossDomainPolicy::FTP;
}

bool URLPolicyFile::checkSiteControlValidity()
{
	//by-ftp-filename only applies to FTP
	if((subtype == HTTP || subtype == HTTPS) && 
	   getMetaPolicy() == PolicyFile::BY_FTP_FILENAME)
		return false;
	//by-content-type only applies to HTTP(S)
	else if(subtype == FTP && 
		getMetaPolicy() == PolicyFile::BY_CONTENT_TYPE)
		return false;

	return true;
}

/**
 * \brief Checks whether this policy file allows the given URL access
 *
 * \param requestingUrl The URL (of the SWF file) that is requesting the data
 * \param to The URL that is being requested by a resource at \c requestingUrl
 * \return \c true if allowed, otherwise \c false
 */
bool URLPolicyFile::allowsAccessFrom(const URLInfo& requestingUrl, const URLInfo& to)
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
		// we allow access to https urls even if the main url is not https
		if((*i)->allowsAccessFrom(requestingUrl,0,requestingUrl.getProtocol()=="https"))
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

void URLPolicyFile::handlePolicyElement(CrossDomainPolicy::ELEMENT& elementType, 
					const CrossDomainPolicy& parser)
{
	PolicyFile::handlePolicyElement(elementType, parser);

	if (elementType == CrossDomainPolicy::ALLOW_HTTP_REQUEST_HEADERS_FROM)
	{
		allowHTTPRequestHeadersFrom.push_back(
			new PolicyAllowHTTPRequestHeadersFrom(this, 
				parser.getDomain(), parser.getHeaders(),
				parser.getSecure(), parser.getSecureSpecified()));
	}
}

/**
 * \brief Constructor for SocketPolicyFile
 *
 * Can only be called from SecurityManager
 * \param _url The URL where this socket policy file resides
 */
SocketPolicyFile::SocketPolicyFile(const URLInfo& _url):
	PolicyFile(_url, SOCKET)
{
	if(url.isValid())
		valid = true;
}

/**
 * \brief Checks whether this policy file allows the given URL access
 *
 * \param requestingUrl The URL (of the SWF file) that is requesting the data
 * \param to The URL that is being requested by a resource at \c requestingUrl
 * \return \c true if allowed, otherwise \c false
 */
bool SocketPolicyFile::allowsAccessFrom(const URLInfo& requestingUrl, const URLInfo& to)
{
	//File must be loaded
	if(!isLoaded())
		return false;

	//Check if the file is invalid or ignored
	if(!isValid() || isIgnored())
		return false;

	list<PolicyAllowAccessFrom*>::const_iterator i = allowAccessFrom.begin();
	for(; i != allowAccessFrom.end(); ++i)
	{
		//This allow-access-from entry applies to our domain AND it allows our domain
		if((*i)->allowsAccessFrom(requestingUrl, to.getPort()))
			return true;
	}
	return false;
}

/**
 * \brief Checks whether this socket policy file is a master policy file
 *
 * \return \c true if this policy file is a master policy file, otherwise \c false
 */
bool SocketPolicyFile::isMaster() const
{
	return url.getPort() == MASTER_PORT;
}

SocketPolicyFile* SocketPolicyFile::getMasterPolicyFile()
{
	if(isMaster())
		return this;

	URLInfo masterurl = url.goToURL(MASTER_PORT_URL);
	SocketPolicyFile* file = getSys()->securityManager->getSocketPolicyFileByURL(masterurl);
	if(file == NULL)
		file = getSys()->securityManager->addSocketPolicyFile(masterurl);

	return file;
}

bool SocketPolicyFile::isIgnoredByMaster()
{
	//Check if this file is allowed/ignored by the master policy file
	if(!isMaster())
	{
		//Load master policy file if not loaded yet
		SocketPolicyFile* master = getMasterPolicyFile();
		getSys()->securityManager->loadSocketPolicyFile(master);
		//Master policy file found and valid and has a site-control entry
		if(master->isValid())
		{
			PolicyFile::METAPOLICY permittedPolicies = master->getMetaPolicy();
			if(permittedPolicies == PolicyFile::MASTER_ONLY || 
			   permittedPolicies == PolicyFile::NONE)
				return true;
		}
	}

	return false;
}

/**
 * \brief Download the content of the policy file
 *
 * \param url Location of the policy file to load
 * \param outData Policy file content will be inserted in here
 * \return \c true if policy file was downloaded without errors, otherwise \c false (content of outData will be undefined)
 */
bool SocketPolicyFile::retrievePolicyFile(vector<unsigned char>& outData)
{
	tiny_string hostname = url.getHostname();
	uint16_t port = url.getPort();
	SocketIO sock;
	// according to http://www.lightsphere.com/dev/articles/flash_socket_policy.html adobe has a 3 second timeout when retrieving the socket policy file
	if (!sock.connect(hostname, port,3))
	{
		if (isMaster())
		{
			//It's legal not to have a master socket policy.
			//We still check the other policy files.
			LOG(LOG_INFO, "SECURITY: Master socket policy file not available, using default policy");
			const char *default_policy = "<cross-domain-policy/>";
			unsigned int len = strlen(default_policy);
			outData.insert(outData.end(), default_policy, default_policy+len);
			return true;
		}
		else
			return false;
	}

	const char *socket_policy_cmd = "<policy-file-request/>\0";
	unsigned int socket_policy_cmd_len = strlen(socket_policy_cmd)+1;
	ssize_t nbytes = sock.sendAll(socket_policy_cmd, socket_policy_cmd_len);
	if (nbytes != (int)socket_policy_cmd_len)
	{
		return false;
	}

	do
	{
		char buf[4096];
		nbytes = sock.receive(buf, sizeof buf);
		if (nbytes > 0)
			outData.insert(outData.end(), buf, buf + nbytes);
	}
	while (nbytes == 4096);

	if (nbytes < 0 && outData.size() == 0)
	{
		// error reading from socket
		return false;
	}

	// The policy file is considered invalid if the last character
	// is not '\0' or '>' or '\n'
	if (outData.size() == 0 || (outData[outData.size()-1] != '\0' && outData[outData.size()-1] != '>' && outData[outData.size()-1] != '\n'))
	{
		return false;
	}

	// Ignore the null terminator
	outData.resize(outData.size()-1);

	return true;
}

void SocketPolicyFile::getParserType(CrossDomainPolicy::POLICYFILETYPE& parserType,
				     CrossDomainPolicy::POLICYFILESUBTYPE& parserSubtype)
{
	parserType = CrossDomainPolicy::SOCKET;
	parserSubtype = CrossDomainPolicy::NONE;
}

/**
 * \brief Constructor for the PolicySiteControl class
 *
 * Can only be called from within PolicyFile.
 * \param _file The policy file this entry belongs to
 * \param _permittedPolicies The value of the permitted-cross-domain-policies attribute.
 * \see URLPolicyFile::load()
 */
PolicySiteControl::PolicySiteControl(const string _permittedPolicies)
{
	if(_permittedPolicies == "all")
		permittedPolicies = PolicyFile::ALL;
	else if(_permittedPolicies == "by-content-type")
		permittedPolicies = PolicyFile::BY_CONTENT_TYPE;
	else if(_permittedPolicies == "by-ftp-filename")
		permittedPolicies = PolicyFile::BY_FTP_FILENAME;
	else if(_permittedPolicies == "master-only")
		permittedPolicies = PolicyFile::MASTER_ONLY;
	else if(_permittedPolicies == "none")
		permittedPolicies = PolicyFile::NONE;
	else
	{
		LOG(LOG_ERROR, "SECURITY: Unknown site-policy value: " << _permittedPolicies);
		permittedPolicies = PolicyFile::NONE;
	}
}

PolicyFile::METAPOLICY PolicySiteControl::defaultSitePolicy(PolicyFile::TYPE policyFileType)
{
	if (policyFileType == PolicyFile::SOCKET)
		return PolicyFile::ALL;
	else
		return PolicyFile::MASTER_ONLY;
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
				static_cast<URLPolicyFile*>(file)->getSubtype() == URLPolicyFile::HTTPS)
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
bool PolicyAllowAccessFrom::allowsAccessFrom(const URLInfo& url, uint16_t toPort, bool bCheckHttps) const
{
	//TODO: resolve domain names using DNS before checking for a match?
	//See section 1.5.9 in specification
	
	//Check if domains match
	if(!URLInfo::matchesDomain(domain, url.getHostname()))
		return false;

	//Check if the requesting URL is secure, if needed
	if (bCheckHttps)
	{
		if(file->getType() == PolicyFile::URL && 
				static_cast<URLPolicyFile*>(file)->getSubtype() == URLPolicyFile::HTTPS && 
				secure && url.getProtocol() != "https")
			return false;
		if(file->getType() == PolicyFile::SOCKET && 
				secure && url.getProtocol() != "https")
			return false;
	}
	//Check for to-ports (only applies to SOCKET connections)
	if(file->getType() == PolicyFile::SOCKET)
	{
		// toPort should always be specified for sockets
		if (toPort == 0)
			return false;

		bool match = false;
		std::list<PortRange*>::const_iterator it;
		for (it=toPorts.begin(); it!=toPorts.end(); ++it)
		{
			if ((*it)->matches(toPort))
			{
				match = true;
				break;
			}
		}

		if (!match)
			return false;
	}

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

	//Socket policy files don't use http-request-headers-from
	if((_file->getType()==PolicyFile::SOCKET) || _headers.length() == 0 || _headers == "*")
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
