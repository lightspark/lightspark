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
extern TLSDATA SystemState* sys;

SecurityManager::SecurityManager():
	sandboxType(REMOTE),exactSettings(true),exactSettingsLocked(false)
{
}

SecurityManager::~SecurityManager()
{
	std::map<tiny_string,std::vector<URLPolicyFile*>>::iterator i = pendingURLPolicyFiles.begin();
	std::vector<URLPolicyFile*>::iterator j;
	for(; i != pendingURLPolicyFiles.end(); ++i)
	{
		for(j = (*i).second.begin(); j != (*i).second.end(); ++j)
			delete (*j);
		(*i).second.clear();
	}

	pendingURLPolicyFiles.clear();
	i = loadedURLPolicyFiles.begin();
	for(;i != loadedURLPolicyFiles.end(); ++i)
	{
		for(j = (*i).second.begin(); j != (*i).second.end(); ++j)
			delete (*j);
		(*i).second.clear();
	}
	loadedURLPolicyFiles.clear();
}

//Check URL policy files
SecurityManager::EVALUATIONRESULT SecurityManager::evaluateURL(const URLInfo& url, bool loadPendingFiles)
{
	LOG(LOG_NO_INFO, "SECURITY: Evaluating URL for cross domain policies: \nURL: " << url
			<< "\nOrigin: " << sys->getOrigin());
	//The URL has exactly the same domain name as the origin, always allowed
	if(url.getHostname() == sys->getOrigin().getHostname())
	{
		LOG(LOG_NO_INFO, "SECURITY: Same hostname as origin, allowing");
		return ALLOWED;
	}

	LOG(LOG_NO_INFO, "SECURITY: Checking master x-domain policy file, loading pending: " << loadPendingFiles);
	URLInfo masterURL = url.goToURL("/crossdomain.xml");
	//The domain doesn't exactly match the domain of the origin, so lets check the master policy file first
	//Get or create the master policy file object
	URLPolicyFile* master = getURLPolicyFileByURL(masterURL);
	if(master == NULL)
	{
		LOG(LOG_NO_INFO, "SECURITY: No master policy file found, trying to add one");
		master = addURLPolicyFile(masterURL);
	}
	if(loadPendingFiles)
	{
		LOG(LOG_NO_INFO, "SECURITY: Loading of pending files is allowed, loading master policy file");
		master->load();
	}
	else
	{
		LOG(LOG_NO_INFO, "SECURITY: Loading of pending files is NOT allowed, not loading master policy file");
	}
	//Check if the master policy file is loaded. If another user-added relevant policy file is already loaded, 
	//it's master will have already been loaded too (to check if it is allowed).
	//So IF any relevant policy file is loaded already, then the master will be too.
	if(master->isLoaded())
	{
		LOG(LOG_NO_INFO, "SECURITY: Master policy file is loaded");
		//Master defines no policy files are allowed at all
		if(master->getSiteControl()->getPermittedCrossDomainPolicies() == PolicySiteControl::NONE)
		{
			LOG(LOG_NO_INFO, "SECURITY: DISALLOWED: Master policy file disallows policy files");
			return DISALLOWED_CROSSDOMAIN_POLICY;
		}
		//Master is invalid AND only master policy files are allowed (by default)
		if(!master->isValid() && master->getSiteControl()->getPermittedCrossDomainPolicies() == PolicySiteControl::MASTER_ONLY)
		{
			LOG(LOG_NO_INFO, "SECURITY: DISALLOWED: Master policy file is invalid and allows master policy files only");
			return DISALLOWED_CROSSDOMAIN_POLICY;
		}
		
		LOG(LOG_NO_INFO, "SECURITY: Master policy file is valid and allows master policy files only, checking if origin has access:\n" << 
				sys->getOrigin());
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
			if(loadedURLPolicyFiles.count(url.getHostname()) == 1)
			{
				LOG(LOG_NO_INFO, "SECURITY: Master policy file allows non-master policy files, checking already loaded ones (" <<
						loadedURLPolicyFiles[url.getHostname()].size() << ")");
				std::vector<URLPolicyFile*>::const_iterator i = loadedURLPolicyFiles[url.getHostname()].begin();
				for(;i != loadedURLPolicyFiles[url.getHostname()].end(); ++i)
				{
					if((*i)->allowsAccessFrom(sys->getOrigin(), url))
					{
						LOG(LOG_NO_INFO, "SECURITY: ALLOWED: Already loaded non-master policy file allowed access");
						return ALLOWED;
					}
				}
			}
			//And check the pending policy files next (if we are allowed to)
			if(pendingURLPolicyFiles.count(url.getHostname()) == 1 && loadPendingFiles)
			{
				LOG(LOG_NO_INFO, "SECURITY: Loading of pending files is allowed, checking and loading pending non-master policy files (" <<
						pendingURLPolicyFiles[url.getHostname()].size() << ")");
				std::vector<URLPolicyFile*>::iterator i = pendingURLPolicyFiles[url.getHostname()].begin();
				while(i != pendingURLPolicyFiles[url.getHostname()].end())
				{
					//NOTE: load() will change pendingURLPolicyFiles (it will move the policy file to the loaded list)
					(*i)->load();
					if((*i)->allowsAccessFrom(sys->getOrigin(), url))
					{
						LOG(LOG_NO_INFO, "SECURITY: ALLOWED: Pending non-master policy file allowed access");
						return ALLOWED;
					}
				}
			}
			else
			{
				LOG(LOG_NO_INFO, "SECURITY: Loading of pending files is NOT allowed or there are no pending files");
			}
		}
	}
	else
	{
		LOG(LOG_NO_INFO, "SECURITY: Master policy file isn't loaded");
	}

	LOG(LOG_NO_INFO, "SECURITY: DISALLOWED: No policy file explicitly allowed access");
	return DISALLOWED_CROSSDOMAIN_POLICY;
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
	else
	{
		LOG(LOG_NO_INFO, "SECURITY: Couldn't classify policy file URL, return NULL");
		return NULL;
	}
}

URLPolicyFile* SecurityManager::addURLPolicyFile(const URLInfo& url)
{
	LOG(LOG_NO_INFO, "SECURITY: Trying to add URL policy file: " << url);
	URLPolicyFile* file = new URLPolicyFile(url);
	if(file->isValid())
	{
		LOG(LOG_NO_INFO, "SECURITY: URL policy file is valid, adding to URL policy file list");
		//Policy files get added to pending files list
		pendingURLPolicyFiles[url.getHostname()].push_back(file);
	}
	return file;
}

URLPolicyFile* SecurityManager::getURLPolicyFileByURL(const URLInfo& url)
{
	URLPolicyFile* file;
	LOG(LOG_NO_INFO, "SECURITY: Searching for URL policy file in lists, at url:\n" << url);

	LOG(LOG_NO_INFO, "SECURITY: Searching for URL policy file in loaded list");
	std::vector<URLPolicyFile*> vec;
	//Check the loaded URL policy files first
	if(loadedURLPolicyFiles.count(url.getHostname()) == 1)
	{
		LOG(LOG_NO_INFO, "SECURITY: URL policy file domain has entry in loaded list");
		vec = loadedURLPolicyFiles[url.getHostname()];
		std::vector<URLPolicyFile*>::const_iterator i = vec.begin();
		for(;i != vec.end(); ++i)
		{
			file = (*i);
			if(file->getURL() == url)
			{
				LOG(LOG_NO_INFO, "SECURITY: URL policy file found in loaded list");
				return file;
			}
		}
	}
	LOG(LOG_NO_INFO, "SECURITY: Searching for URL policy file in pending list");
	//Check the pending URL policy files next
	if(pendingURLPolicyFiles.count(url.getHostname()) == 1)
	{
		LOG(LOG_NO_INFO, "SECURITY: URL policy file domain has entry in pending list");
		vec = pendingURLPolicyFiles[url.getHostname()];
		std::vector<URLPolicyFile*>::const_iterator i = vec.begin();
		for(;i != vec.end(); ++i)
		{
			file = (*i);
			if(file->getURL() == url)
			{
				LOG(LOG_NO_INFO, "SECURITY: URL policy file found in pending list");
				return file;
			}
		}
	}
	LOG(LOG_NO_INFO, "SECURITY: No such URL policy file found in lists");
	return NULL;
}

void SecurityManager::markPolicyFileLoaded(const URLPolicyFile* file)
{
	if(pendingURLPolicyFiles.count(file->getURL().getHostname()) == 1)
	{
		std::vector<URLPolicyFile*>::iterator i = pendingURLPolicyFiles[file->getURL().getHostname()].begin();
		for(;i != pendingURLPolicyFiles[file->getURL().getHostname()].end(); ++i)
		{
			if((*i) == file)
			{
				loadedURLPolicyFiles[file->getURL().getHostname()].push_back((*i));
				pendingURLPolicyFiles[file->getURL().getHostname()].erase(i);
				break;
			}
		}
	}
}

PolicyFile::PolicyFile(URLInfo _url, TYPE _type):
	url(_url),type(_type),valid(false),ignore(false),
	master(false),loaded(false),siteControl(NULL)
{
}
PolicyFile::~PolicyFile()
{
	for(std::vector<PolicyAllowAccessFrom*>::iterator i = allowAccessFrom.begin(); 
			i != allowAccessFrom.end(); ++i)
		delete (*i);
	allowAccessFrom.clear();
}

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
	
	if((subtype == HTTP || subtype == HTTPS || subtype == FTP) && url.getPath() == "/crossdomain.xml")
	{
		master = true;
	}
}
URLPolicyFile::~URLPolicyFile()
{
	for(std::vector<PolicyAllowHTTPRequestHeadersFrom*>::iterator i = allowHTTPRequestHeadersFrom.begin();
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

//TODO: support download timeout handling
void URLPolicyFile::load()
{
	//Invalid URLPolicyFile, ignore this call
	if(!isValid()) return;
	//Already loaded, no action needed
	if(isLoaded()) return;
	//We only try loading once, if something goes wrong, valid will be set to 'invalid'
	loaded = true;
	//Move this policy file to the loaded list in securityManager
	sys->securityManager->markPolicyFileLoaded(this);

	LOG(LOG_NO_INFO, "Loading cross domain policy file: " << url.getParsedURL());

	//Check if this file is allowed/ignored by the master policy file
	if(!isMaster())
	{
		URLPolicyFile* master = getMasterPolicyFile();
		//Load master policy file if not loaded yet
		master->load();
		//Master policy file found and valid and has a site-control entry
		if(master->isValid() && master->getSiteControl() != NULL)
		{
			PolicySiteControl::METAPOLICYTYPE permittedPolicies = master->getSiteControl()->getPermittedCrossDomainPolicies();
			//For all types: master-only, none
			if(permittedPolicies == PolicySiteControl::MASTER_ONLY || permittedPolicies == PolicySiteControl::NONE)
			{
				ignore = true;
				return;
			}
			//Only for FTP: by-ftp-filename
			else if(subtype == FTP && permittedPolicies == PolicySiteControl::BY_FTP_FILENAME && url.getPathFile() != "crossdomain.xml")
			{
				ignore = true;
				return;
			}
			//TODO: add support for HTTP: by-content-type
		}
	}

	//We've checked the master file to see of we need to ignore this file. (not the case)
	//Now let's parse this file.
	
	//TODO: disallow cross-domain redirect for policy files
	//No caching needed for this download, we don't expect very big files
	Downloader* downloader=sys->downloadManager->download(url, false);
	//Wait until the file is fetched
	downloader->wait();
	if(!downloader->hasFailed())
	{
		std::istream s(downloader);
		uint8_t* buf=new uint8_t[downloader->getLength()];
		//TODO: avoid this useless copy
		s.read((char*)buf,downloader->getLength());	

		//DEBUGGING
		char strbuf[downloader->getLength()+1];
		strncpy(strbuf, (const char*) buf, downloader->getLength());
		strbuf[downloader->getLength()] = '\0';
		LOG(LOG_NO_INFO, "Loaded policy file: length: " << downloader->getLength() << "B\n" << strbuf);
		//DEBUGGING: end
		
		CrossDomainPolicy::POLICYFILESUBTYPE parserSubtype;
		if(subtype == HTTP)
			parserSubtype = CrossDomainPolicy::HTTP;
		else if(subtype == HTTPS)
			parserSubtype = CrossDomainPolicy::HTTPS;
		else if(subtype == FTP)
			parserSubtype = CrossDomainPolicy::FTP;
		CrossDomainPolicy parser(buf, downloader->getLength(), CrossDomainPolicy::URL, parserSubtype, isMaster());
		CrossDomainPolicy::ELEMENTTYPE elementType = parser.getNextElement();
		while(elementType != CrossDomainPolicy::END_OF_FILE && elementType != CrossDomainPolicy::INVALID_FILE)
		{
			if(elementType == CrossDomainPolicy::SITE_CONTROL)
			{
				siteControl = new PolicySiteControl(this, parser.getPermittedCrossDomainPolicies());
				//No more parsing is needed if this site-control entry specifies that no policy files are allowed
				if(isMaster() && siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::NONE)
					break;
			}
			else if(elementType == CrossDomainPolicy::ALLOW_ACCESS_FROM)
			{
				//URL policy files don't use to-ports
				allowAccessFrom.push_back(new PolicyAllowAccessFrom(this, parser.getDomain(), "", parser.getSecure(), parser.getSecureSpecified()));
			}
			else if(elementType == CrossDomainPolicy::ALLOW_HTTP_REQUEST_HEADERS_FROM)
			{
				allowHTTPRequestHeadersFrom.push_back(new PolicyAllowHTTPRequestHeadersFrom(this, 
							parser.getDomain(), parser.getHeaders(), parser.getSecure(), parser.getSecureSpecified()));
			}

			elementType = parser.getNextElement();
		}

		if(elementType == CrossDomainPolicy::INVALID_FILE)
			valid = false;

		if(isMaster())
		{
			//Set siteControl to the default value if it isn't set before and we are a master file
			if(siteControl == NULL)
				siteControl = new PolicySiteControl(this, "");

			//Ignore this file if the site control policy is "none"
			if(siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::NONE)
				ignore = true;
			if((subtype == HTTP || subtype == HTTPS) && siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::BY_FTP_FILENAME)
				valid = false;
			else if(subtype == FTP && siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::BY_CONTENT_TYPE)
				valid = false;
		}
	}
	else
	{
		//Failed to download the file, marking this file as invalid
		valid = false;
	}
	sys->downloadManager->destroy(downloader);
}

bool URLPolicyFile::allowsAccessFrom(const URLInfo& u, const URLInfo& to)
{
	//This policy file doesn't apply to the given URL
	if(!isMaster() && !to.isSubOf(url))
		return false;

	//Check if the file is invalid or ignored even before loading
	if(!isValid() || isIgnored())
		return false;

	load();

	//Check if the file is invalid or ignored after loading
	if(!isValid() || isIgnored())
		return false;

	PolicyAllowAccessFrom* access;
	for(std::vector<PolicyAllowAccessFrom*>::iterator i = allowAccessFrom.begin(); 
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
	//Only used for HTTP(S)
	if(subtype != HTTP && subtype != HTTPS) return true;

	//This policy file doesn't apply to the given URL
	if(!isMaster() && !to.isSubOf(url))
		return false;

	load();
	if(!isValid() || isIgnored())
		return false;

	//TODO: handle default allowed headers
	PolicyAllowHTTPRequestHeadersFrom* headers;
	for(std::vector<PolicyAllowHTTPRequestHeadersFrom*>::iterator i = allowHTTPRequestHeadersFrom.begin();
			i != allowHTTPRequestHeadersFrom.end(); ++i)
	{
		headers = (*i);
		if(headers->allowsHTTPRequestHeaderFrom(u, header))
			return true;
	}
	return false;
}

PolicySiteControl::PolicySiteControl(PolicyFile* _file, const std::string _permittedCrossDomainPolicies):
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
	else if(file->getType() == URLPolicyFile::SOCKET) //Default for SOCKET
		permittedCrossDomainPolicies = ALL;
	else //Default for everything except SOCKET
		permittedCrossDomainPolicies = MASTER_ONLY;
}

PolicyAllowAccessFrom::PolicyAllowAccessFrom(PolicyFile* _file, const std::string _domain, const std::string _toPorts, 
		bool _secure, bool secureSpecified):
	file(_file),domain(_domain),secure(_secure)
{
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
		std::string ports = _toPorts;
		size_t cursor = 0;
		size_t commaPos;
		size_t dashPos;
		std::string startPortStr;
		std::string endPortStr;
		std::istringstream is;
		uint16_t startPort;
		uint16_t endPort;
		do
		{
			commaPos = ports.find(",", cursor);
			dashPos = ports.find("-", commaPos);

			if(dashPos != std::string::npos)
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
		while(commaPos != std::string::npos);
	}
}

PolicyAllowAccessFrom::~PolicyAllowAccessFrom()
{
	for(std::vector<PortRange*>::iterator i = toPorts.begin(); i != toPorts.end(); ++i)
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

	//Check if the requesting URL is secure, if needed
	if(file->getType() == PolicyFile::URL && 
			dynamic_cast<URLPolicyFile*>(file)->getSubtype() == URLPolicyFile::HTTPS && 
			secure && url.getProtocol() != "https")
		return false;
	
	//TODO: add check for to-ports and secure for SOCKET type policy files

	return true;
}

PolicyAllowHTTPRequestHeadersFrom::PolicyAllowHTTPRequestHeadersFrom(URLPolicyFile* _file, const std::string _domain, 
		const std::string _headers, bool _secure, bool secureSpecified):
	file(_file),domain(_domain),secure(_secure)
{
	if(!secureSpecified)
	{
		if(file->getSubtype() == URLPolicyFile::HTTPS)
			secure = true;
	}
	if(_headers.length() == 0 || _headers == "*")
	{
		headers.push_back(new std::string("*"));
	}
	else {
		std::string headersStr = _headers;
		size_t cursor = 0;
		size_t commaPos;
		do
		{
			commaPos = headersStr.find(",");
			headers.push_back(new std::string(headersStr.substr(cursor, commaPos-cursor)));
		}
		while(commaPos != std::string::npos);
	}
}

PolicyAllowHTTPRequestHeadersFrom::~PolicyAllowHTTPRequestHeadersFrom()
{
	for(std::vector<std::string*>::iterator i = headers.begin();
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
	for(std::vector<std::string*>::const_iterator i = headers.begin();
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
