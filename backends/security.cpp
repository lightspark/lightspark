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

#include "security.h"

using namespace lightspark;
using namespace std;
extern TLSDATA SystemState* sys;

SecurityManager::SecurityManager():
	sandboxType(REMOTE),exactSettings(true),exactSettingsLocked(false)
{
	sandboxNames = {"remote","localWithFile","localWithNetwork","localTrusted"};
	sandboxTitles = {"remote","local-with-filesystem","local-with-networking","local-trusted"};
	sem_init(&mutex,0,1);
}

SecurityManager::~SecurityManager()
{
	multimap<tiny_string, URLPolicyFile*>::iterator i = pendingURLPolicyFiles.begin();
	for(; i != pendingURLPolicyFiles.end(); ++i)
	{
		delete (*i).second;
	}
	pendingURLPolicyFiles.clear();
	i = loadedURLPolicyFiles.begin();

	for(;i != loadedURLPolicyFiles.end(); ++i)
	{
		delete (*i).second;
	}
	loadedURLPolicyFiles.clear();
	sem_destroy(&mutex);
}

bool SecurityManager::evaluateSandbox(SANDBOXTYPE sandbox, int allowedSandboxes)
{
	return (allowedSandboxes & sandbox) != 0;
}

SecurityManager::EVALUATIONRESULT SecurityManager::evaluateURL(const URLInfo& url, bool loadPendingPolicies,
		int allowedSandboxesRemote, int allowedSandboxesLocal, bool restrictLocalDirectory)
{
	EVALUATIONRESULT sandboxResult = evaluateSandboxURL(url, allowedSandboxesRemote, allowedSandboxesLocal);
	if(sandboxResult != ALLOWED)
		return sandboxResult;

	if(restrictLocalDirectory)
	{
		EVALUATIONRESULT restrictLocalDirResult = evaluateLocalDirectoryURL(url);
		if(restrictLocalDirResult != ALLOWED)
			return restrictLocalDirResult;
	}

	EVALUATIONRESULT policiesResult = evaluatePoliciesURL(url, loadPendingPolicies);
	if(policiesResult != ALLOWED)
		return policiesResult;

	return ALLOWED;
}
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
SecurityManager::EVALUATIONRESULT SecurityManager::evaluateLocalDirectoryURL(const URLInfo& url)
{
	if(url.getProtocol() == "file" && !url.isSubOf(sys->getOrigin()))
		return NA_RESTRICT_LOCAL_DIRECTORY;
	return ALLOWED;
}

//Check URL policy files
SecurityManager::EVALUATIONRESULT SecurityManager::evaluatePoliciesURL(const URLInfo& url,
		bool loadPendingPolicies)
{
	sem_wait(&mutex);
	LOG(LOG_NO_INFO, "SECURITY: Evaluating URL for cross domain policies: \nURL: " << url
			<< "\nOrigin: " << sys->getOrigin());
	//The URL has exactly the same domain name as the origin, always allowed
	if(url.getProtocol() == sys->getOrigin().getProtocol() &&
			url.getHostname() == sys->getOrigin().getHostname())
	{
		LOG(LOG_NO_INFO, "SECURITY: Same hostname as origin, allowing");
		return ALLOWED;
	}

	LOG(LOG_NO_INFO, "SECURITY: Checking master x-domain policy file, loading "
			"pending policies allowed = " << loadPendingPolicies);
	URLInfo masterURL = url.goToURL("/crossdomain.xml");
	//The domain doesn't exactly match the domain of the origin, 
	//so lets check the master policy file first
	
	//Get or create the master policy file object
	sem_post(&mutex);
	URLPolicyFile* master = getURLPolicyFileByURL(masterURL);
	sem_wait(&mutex);
	if(master == NULL)
	{
		LOG(LOG_NO_INFO, "SECURITY: No master policy file found, trying to add one");
		sem_post(&mutex);
		master = addURLPolicyFile(masterURL);
		sem_wait(&mutex);
	}
	if(loadPendingPolicies)
	{
		LOG(LOG_NO_INFO, "SECURITY: Loading of pending files is allowed, loading master policy file");
		sem_post(&mutex);
		sys->securityManager->loadPolicyFile(master);
		sem_wait(&mutex);
	}
	else
		LOG(LOG_NO_INFO, "SECURITY: Loading of pending files is NOT allowed, not loading "
				"master policy file");

	//Check if the master policy file is loaded.
	//If another user-added relevant policy file is already loaded, 
	//it's master will have already been loaded too (to check if it is allowed).
	//So IF any relevant policy file is loaded already, then the master will be too.
	if(master->isLoaded() && master->isValid())
	{
		LOG(LOG_NO_INFO, "SECURITY: Master policy file is loaded, is master");
		//Master defines no policy files are allowed at all
		if(master->getSiteControl()->getPermittedCrossDomainPolicies() == PolicySiteControl::NONE)
		{
			LOG(LOG_NO_INFO, "SECURITY: DISALLOWED: Master policy file disallows policy files");
			return NA_CROSSDOMAIN_POLICY;
		}
		
		if(master->allowsAccessFrom(sys->getOrigin(), url))
		{
			LOG(LOG_NO_INFO, "SECURITY: ALLOWED: Master policy file allowed access");
			return ALLOWED;
		}
		//Non-master policy files are allowed (/subdir/crossdomain.xml, loaded URLPolicyFile())
		//These can further by restricted by content-type and ftp filename.
		//Lets check the loaded policy files first
		if(master->getSiteControl()->getPermittedCrossDomainPolicies() != PolicySiteControl::MASTER_ONLY)
		{
			LOG(LOG_NO_INFO, "SECURITY: Master policy file allows non-master policy files, checking already loaded ones (" <<
					loadedURLPolicyFiles.count(url.getHostname()) << ")");

			pair<multimap<tiny_string, URLPolicyFile*>::const_iterator, 
				multimap<tiny_string, URLPolicyFile*>::const_iterator> range = 
				loadedURLPolicyFiles.equal_range(url.getHostname());
			multimap<tiny_string, URLPolicyFile*>::const_iterator i = range.first;
			for(;i != range.second; ++i)
			{
				if((*i).second == master)
					continue;
				if((*i).second->allowsAccessFrom(sys->getOrigin(), url))
				{
					LOG(LOG_NO_INFO, "SECURITY: ALLOWED: Already loaded non-master policy file allowed access");
					return ALLOWED;
				}
			}

			//And check the pending policy files next (if we are allowed to)
			if(loadPendingPolicies)
			{
				LOG(LOG_NO_INFO, "SECURITY: Loading of pending files is allowed, checking and loading pending non-master policy files (" <<
						pendingURLPolicyFiles.count(url.getHostname()) << ")");
				range = pendingURLPolicyFiles.equal_range(url.getHostname());
				i = range.first;
				for(; i != range.second; ++i)
				{
					//NOTE: load() will change pendingURLPolicyFiles (it will move the policy file to the loaded list)
					sem_post(&mutex);
					sys->securityManager->loadPolicyFile((*i).second);
					sem_wait(&mutex);
					if((*i).second->allowsAccessFrom(sys->getOrigin(), url))
					{
						LOG(LOG_NO_INFO, "SECURITY: ALLOWED: Pending non-master policy file allowed access");
						return ALLOWED;
					}
				}
			}
			else
				LOG(LOG_NO_INFO, "SECURITY: Loading of pending files is NOT allowed or there are no pending files");
		}
	}
	else
		LOG(LOG_NO_INFO, "SECURITY: Master policy file isn't loaded or isn't valid");

	LOG(LOG_NO_INFO, "SECURITY: DISALLOWED: No policy file explicitly allowed access");
	sem_post(&mutex);
	return NA_CROSSDOMAIN_POLICY;
}

PolicyFile* SecurityManager::addPolicyFile(const URLInfo& url)
{
	LOG(LOG_NO_INFO, "SECURITY: Trying to add policy file, classifying URL: " << url);
	if(url.getProtocol() == "http" || url.getProtocol() == "https" || url.getProtocol() == "ftp")
	{
		LOG(LOG_NO_INFO, "SECURITY: Classified policy file URL, policy file type = URL");
		return addURLPolicyFile(url);
	}
	else if(url.getProtocol() == "xmlsocket")
	{
		LOG(LOG_NO_INFO, "SECURITY: Classified policy file URL, policy file type = SOCKET");
		LOG(LOG_NOT_IMPLEMENTED, "SECURITY: SocketPolicFile not implemented yet!");
		//return addSocketPolicyFile(url);
		return NULL;
	}

	LOG(LOG_NO_INFO, "SECURITY: Couldn't classify policy file URL, returning NULL");
	return NULL;
}

URLPolicyFile* SecurityManager::addURLPolicyFile(const URLInfo& url)
{
	sem_wait(&mutex);
	LOG(LOG_NO_INFO, "SECURITY: Trying to add URL policy file: " << url);
	URLPolicyFile* file = new URLPolicyFile(url);
	if(file->isValid())
	{
		LOG(LOG_NO_INFO, "SECURITY: URL policy file is valid, adding to URL policy file list");
		//Policy files get added to pending files list
		pendingURLPolicyFiles.insert(pair<tiny_string, URLPolicyFile*>(url.getHostname(), file));
	}
	sem_post(&mutex);
	return file;
}

URLPolicyFile* SecurityManager::getURLPolicyFileByURL(const URLInfo& url)
{
	sem_wait(&mutex);
	LOG(LOG_NO_INFO, "SECURITY: Searching for URL policy file in lists, at url:\n" << url);

	pair<multimap<tiny_string, URLPolicyFile*>::const_iterator, 
		multimap<tiny_string, URLPolicyFile*>::const_iterator> range = 
		loadedURLPolicyFiles.equal_range(url.getHostname());
	multimap<tiny_string, URLPolicyFile*>::const_iterator i = range.first;

	LOG(LOG_NO_INFO, "SECURITY: Searching for URL policy file in loaded list");
	//Check the loaded URL policy files first
	for(;i != range.second; ++i)
	{
		if((*i).second->getOriginalURL() == url)
		{
			LOG(LOG_NO_INFO, "SECURITY: URL policy file found in loaded list");
			sem_post(&mutex);
			return (*i).second;
		}
	}

	LOG(LOG_NO_INFO, "SECURITY: Searching for URL policy file in pending list");
	//Check the pending URL policy files next
	range = pendingURLPolicyFiles.equal_range(url.getHostname());
	i = range.first;
	for(;i != range.second; ++i)
	{
		if((*i).second->getOriginalURL() == url)
		{
			LOG(LOG_NO_INFO, "SECURITY: URL policy file found in pending list");
			sem_post(&mutex);
			return (*i).second;
		}
	}
	LOG(LOG_NO_INFO, "SECURITY: No such URL policy file found in lists");
	sem_post(&mutex);
	return NULL;
}

void SecurityManager::loadPolicyFile(URLPolicyFile* file)
{
	sem_wait(&mutex);
	LOG(LOG_NO_INFO, "SECURITY: Loading policy file");
	if(pendingURLPolicyFiles.count(file->getURL().getHostname()) > 0)
	{
		file->load();
		pair<multimap<tiny_string, URLPolicyFile*>::iterator, multimap<tiny_string, URLPolicyFile*>::iterator> range = 
			pendingURLPolicyFiles.equal_range(file->getURL().getHostname());
		multimap<tiny_string, URLPolicyFile*>::iterator i = range.first;
		for(;i != range.second; ++i)
		{
			if((*i).second == file)
			{
				loadedURLPolicyFiles.insert(pair<tiny_string, URLPolicyFile*>(file->getURL().getHostname(), (*i).second));
				pendingURLPolicyFiles.erase(i);
				break;
			}
		}
	}
	sem_post(&mutex);
}

PolicyFile::PolicyFile(URLInfo _url, TYPE _type):
	url(_url),type(_type),valid(false),ignore(false),
	loaded(false),siteControl(NULL)
{
	sem_init(&mutex,0,1);
}
PolicyFile::~PolicyFile()
{
	for(vector<PolicyAllowAccessFrom*>::iterator i = allowAccessFrom.begin(); 
			i != allowAccessFrom.end(); ++i)
		delete (*i);
	allowAccessFrom.clear();
	sem_destroy(&mutex);
}

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
URLPolicyFile::~URLPolicyFile()
{
	for(vector<PolicyAllowHTTPRequestHeadersFrom*>::iterator i = allowHTTPRequestHeadersFrom.begin();
			i != allowHTTPRequestHeadersFrom.end(); ++i)
		delete (*i);
	allowHTTPRequestHeadersFrom.clear();
}

URLPolicyFile* URLPolicyFile::getMasterPolicyFile()
{
	if(isMaster())
		return this;

	URLPolicyFile* file = sys->securityManager->getURLPolicyFileByURL(url.goToURL("/crossdomain.xml"));
	if(file == NULL)
	{
		file = sys->securityManager->addURLPolicyFile(url.goToURL("/crossdomain.xml"));
	}
	return file;
}

bool URLPolicyFile::isMaster()
{
	return url.getPath() == "/crossdomain.xml";
}

//TODO: support download timeout handling
void URLPolicyFile::load()
{
	//Invalid URLPolicyFile or already loaded, ignore this call
	if(!isValid() || isLoaded()) return;
	sem_wait(&mutex);

	//We only try loading once, if something goes wrong, valid will be set to 'invalid'
	loaded = true;

	LOG(LOG_NO_INFO, "SECURITY: Loading cross domain policy file: " << url.getParsedURL());

	sem_post(&mutex);
	URLPolicyFile* master = getMasterPolicyFile();
	sem_wait(&mutex);
	//Check if this file is allowed/ignored by the master policy file
	if(!isMaster())
	{
		//Load master policy file if not loaded yet
		sys->securityManager->loadPolicyFile(master);
		//Master policy file found and valid and has a site-control entry
		if(master->isValid() && master->getSiteControl() != NULL)
		{
			PolicySiteControl::METAPOLICYTYPE permittedPolicies = 
				master->getSiteControl()->getPermittedCrossDomainPolicies();
			//For all types: master-only, none
			if(permittedPolicies == PolicySiteControl::MASTER_ONLY ||
					permittedPolicies == PolicySiteControl::NONE)
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
	if(!sys->isShuttingDown())
		downloader->wait();
	else
		valid = false;

	//If files are redirected, we use the new URL as the policy's URL
	if(downloader->isRedirected())
	{
		URLInfo newURL(downloader->getURL());
		if(url.getHostname() != newURL.getHostname())
		{
			LOG(LOG_NO_INFO, "SECURITY: Policy file was redirected to other domain, marking invalid");
			valid = false;
		}
		url = newURL;
		LOG(LOG_NO_INFO, "SECURITY: Policy file was redirected, has failed: " << downloader->hasFailed());
	}

	//Policy files must have on of the following content-types to be valid:
	//text/*, application/xml or application/xhtml+xml
	tiny_string contentType = downloader->getHeader("content-type");
	if((subtype == HTTP || subtype == HTTPS) && 
			contentType.substr(0, 5) != "text/" &&
			contentType != "application/xml" &&
			contentType != "application/xhtml+xml")
	{
		LOG(LOG_NO_INFO, "SECURITY: Policy file has an invalid content-type, marking invalid");
		valid = false;
	}

	//One more check from the master file: see if the content-type is OK 
	//if site-control specifies by-content-type
	if(!isMaster())
	{
		//Master policy file found and valid and has a site-control entry
		if(master->isValid() && master->getSiteControl() != NULL)
		{
			//If the site-control policy of the master policy file is by-content-type, only policy files with
			//content-type = text/x-cross-domain-policy are allowed.
			PolicySiteControl::METAPOLICYTYPE permittedPolicies = master->getSiteControl()->getPermittedCrossDomainPolicies();
			if((subtype == HTTP || subtype == HTTPS) && permittedPolicies == PolicySiteControl::BY_CONTENT_TYPE && 
					contentType != "text/x-cross-domain-policy")
			{
				LOG(LOG_NO_INFO, "SECURITY: Policy file content-type isn't strict, marking invalid");
				ignore = true;
			}
		}
	}

	//We've checked the master file to see of we need to ignore this file. (not the case)
	//Now let's parse this file.
	
	if(isValid() && !isIgnored() && !downloader->hasFailed())
	{
		istream s(downloader);
		size_t bufLength = downloader->getLength();
		uint8_t* buf=new uint8_t[bufLength];
		//TODO: avoid this useless copy
		s.read((char*)buf,bufLength);

		//DEBUGGING
		//char strbuf[downloader->getLength()+1];
		//strncpy(strbuf, (const char*) buf, downloader->getLength());
		//strbuf[downloader->getLength()] = '\0';
		//LOG(LOG_NO_INFO, "Loaded policy file: length: " <<
		//		downloader->getLength() << "B\n" << strbuf);
		//DEBUGGING: end
		
		//We're done with the downloader, lets destroy ASAP
		sys->downloadManager->destroy(downloader);
		
		CrossDomainPolicy::POLICYFILESUBTYPE parserSubtype;
		if(subtype == HTTP)
			parserSubtype = CrossDomainPolicy::HTTP;
		else if(subtype == HTTPS)
			parserSubtype = CrossDomainPolicy::HTTPS;
		else if(subtype == FTP)
			parserSubtype = CrossDomainPolicy::FTP;
		CrossDomainPolicy parser(buf, bufLength, CrossDomainPolicy::URL, parserSubtype, isMaster());
		CrossDomainPolicy::ELEMENTTYPE elementType = parser.getNextElement();

		while(elementType != CrossDomainPolicy::END_OF_FILE &&
				elementType != CrossDomainPolicy::INVALID_FILE)
		{
			if(elementType == CrossDomainPolicy::SITE_CONTROL)
			{
				siteControl = new PolicySiteControl(this, parser.getPermittedCrossDomainPolicies());
				//No more parsing is needed if this site-control entry specifies that
				//no policy files are allowed
				if(isMaster() &&
						siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::NONE)
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

		if(elementType == CrossDomainPolicy::INVALID_FILE)
			valid = false;

		if(isMaster())
		{
			LOG(LOG_NO_INFO, "Loaded master file");
			//Set siteControl to the default value if it isn't set before and we are a master file
			if(siteControl == NULL)
			{
				siteControl = new PolicySiteControl(this, "");
				LOG(LOG_NO_INFO, "PolicySiteControl == null");
			}

			//Ignore this file if the site control policy is "none"
			if(siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::NONE)
				ignore = true;
			if((subtype == HTTP || subtype == HTTPS) && 
					siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::BY_FTP_FILENAME)
				valid = false;
			else if(subtype == FTP && 
					siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::BY_CONTENT_TYPE)
				valid = false;
		}
	}
	else
	{
		//Failed to download the file, marking this file as invalid
		valid = false;
		sys->downloadManager->destroy(downloader);
	}
	sem_post(&mutex);
}

bool URLPolicyFile::allowsAccessFrom(const URLInfo& u, const URLInfo& to)
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

	PolicyAllowAccessFrom* access;
	for(vector<PolicyAllowAccessFrom*>::const_iterator i = allowAccessFrom.begin(); 
			i != allowAccessFrom.end(); ++i)
	{
		access = (*i);
		//This allow-access-from entry applies to our domain AND it allows our domain
		if(access->allowsAccessFrom(u))
			return true;
	}
	return false;
}

bool URLPolicyFile::allowsHTTPRequestHeaderFrom(const URLInfo& u, const URLInfo& to, const tiny_string& header)
{
	//File must be loaded
	if(!isLoaded())
		return false;

	//Only used for HTTP(S)
	if(subtype != HTTP && subtype != HTTPS) return true;

	//This policy file doesn't apply to the given URL
	if(!isMaster() && !to.isSubOf(url))
		return false;

	sys->securityManager->loadPolicyFile(this);
	if(!isValid() || isIgnored())
		return false;

	//TODO: handle default allowed headers
	PolicyAllowHTTPRequestHeadersFrom* headers;
	for(vector<PolicyAllowHTTPRequestHeadersFrom*>::const_iterator i = 
			allowHTTPRequestHeadersFrom.begin();
			i != allowHTTPRequestHeadersFrom.end(); ++i)
	{
		headers = (*i);
		if(headers->allowsHTTPRequestHeaderFrom(u, header))
			return true;
	}
	return false;
}

PolicySiteControl::PolicySiteControl(PolicyFile* _file, const string _permittedCrossDomainPolicies):
	file(_file)
{
	if(_permittedCrossDomainPolicies == "all")
		permittedCrossDomainPolicies = ALL;
	else if(_permittedCrossDomainPolicies == "by-content-type")
		permittedCrossDomainPolicies = BY_CONTENT_TYPE;
	else if(_permittedCrossDomainPolicies == "by-ftp-filename")
		permittedCrossDomainPolicies = BY_FTP_FILENAME;
	else if(_permittedCrossDomainPolicies == "master-only")
		permittedCrossDomainPolicies = MASTER_ONLY;
	else if(_permittedCrossDomainPolicies == "none")
		permittedCrossDomainPolicies = NONE;
	else if(file->getType() == PolicyFile::SOCKET) //Default for SOCKET
		permittedCrossDomainPolicies = ALL;
	else //Default for everything except SOCKET
		permittedCrossDomainPolicies = MASTER_ONLY;
}

PolicyAllowAccessFrom::PolicyAllowAccessFrom(PolicyFile* _file, const string _domain, const string _toPorts, 
		bool _secure, bool secureSpecified):
	file(_file),domain(_domain),secure(_secure)
{
	LOG(LOG_NO_INFO, "Const domain: " << _domain << ", secure: " << secure << ", secureSpecified: " << secureSpecified);
	if(!secureSpecified)
	{
		if(file->getType() == PolicyFile::URL && dynamic_cast<URLPolicyFile*>(file)->getSubtype() == URLPolicyFile::HTTPS)
			secure = true;
		if(file->getType() == PolicyFile::SOCKET)
			secure = false;
	}
	//Set the default value
	if(_toPorts.length() == 0 || _toPorts == "*")
	{
		toPorts.push_back(new PortRange(0, 0, false, true));
	}
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

PolicyAllowAccessFrom::~PolicyAllowAccessFrom()
{
	for(vector<PortRange*>::iterator i = toPorts.begin(); i != toPorts.end(); ++i)
		delete (*i);
	toPorts.clear();
}

bool PolicyAllowAccessFrom::allowsAccessFrom(const URLInfo& url) const
{
	//Check if domains match
	//TODO: resolve domain names using DNS before checking for a match?
	//See section 1.5.9 in specification
	if(!URLInfo::matchesDomain(domain, url.getHostname()))
		return false;

	LOG(LOG_NO_INFO, "Domain: " << domain << ", Secure: " << secure);
	//Check if the requesting URL is secure, if needed
	if(file->getType() == PolicyFile::URL && 
			dynamic_cast<URLPolicyFile*>(file)->getSubtype() == URLPolicyFile::HTTPS && 
			secure && url.getProtocol() != "https")
		return false;
	
	//TODO: add check for to-ports and secure for SOCKET type policy files

	return true;
}

PolicyAllowHTTPRequestHeadersFrom::PolicyAllowHTTPRequestHeadersFrom(URLPolicyFile* _file, const string _domain, 
		const string _headers, bool _secure, bool secureSpecified):
	file(_file),domain(_domain),secure(_secure)
{
	if(!secureSpecified)
	{
		if(file->getSubtype() == URLPolicyFile::HTTPS)
			secure = true;
	}
	if(_headers.length() == 0 || _headers == "*")
	{
		headers.push_back(new string("*"));
	}
	else {
		string headersStr = _headers;
		size_t cursor = 0;
		size_t commaPos;
		do
		{
			commaPos = headersStr.find(",");
			headers.push_back(new string(headersStr.substr(cursor, commaPos-cursor)));
		}
		while(commaPos != string::npos);
	}
}

PolicyAllowHTTPRequestHeadersFrom::~PolicyAllowHTTPRequestHeadersFrom()
{
	for(vector<string*>::iterator i = headers.begin();
			i != headers.end(); ++i)
		delete (*i);
	headers.clear();
}

bool PolicyAllowHTTPRequestHeadersFrom::allowsHTTPRequestHeaderFrom(const URLInfo& url, const tiny_string& header) const
{
	if(file->getSubtype() != URLPolicyFile::HTTP && file->getSubtype() != URLPolicyFile::HTTPS)
		return false;

	//TODO: handle default allowed request headers
	
	//Check if domains match
	if(!URLInfo::matchesDomain(domain, url.getHostname()))
		return false;

	//Check if the header is explicitly allowed
	bool headerFound = false;
	for(vector<string*>::const_iterator i = headers.begin();
			i != headers.end(); ++i)
	{
		if((**i) == header.raw_buf())
			headerFound = true;
	}
	if(!headerFound)
		return false;

	//Check if the requesting URL is secure, if needed
	if(file->getSubtype() == URLPolicyFile::HTTPS && secure && url.getProtocol() != "https")
		return false;
	
	return true;
}
