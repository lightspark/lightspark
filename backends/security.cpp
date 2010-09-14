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

#include <libxml++/libxml++.h>
#include <libxml++/parsers/textreader.h>

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
	for(std::vector<PolicyFile*>::iterator i = policyFiles.begin(); 
			i != policyFiles.end(); ++i)
		delete (*i);
}

//Currently only HTTP(S) is supported
SecurityManager::EVALUATIONRESULT SecurityManager::evaluateURL(const URLInfo& url, bool loadPendingFiles)
{
	LOG(LOG_NO_INFO, "Evaluating URL for cross domain policies: " << url.getParsedURL());
	//The URL has exactly the same domain name as the origin, always allowed
	if(url.getHostname() == sys->getOrigin().getHostname())
	{
		LOG(LOG_NO_INFO, "Same hostname as origin, allowing");
		return ALLOWED;
	}

	LOG(LOG_NO_INFO, "Checking master x-domain policy file, loading pending: " << loadPendingFiles);
	URLInfo masterURL = url.goToURL("/crossdomain.xml");
	//The domain doesn't exactly match the domain of the origin, so lets check the master policy file first
	//Get or create the master policy file object
	PolicyFile* master = getPolicyFileByURL(masterURL);
	if(master == NULL)
		master = addPolicyFile(masterURL);
	if(loadPendingFiles)
		master->load();
	if(master->isLoaded())
	{
		//Master defines no policy files are allowed at all
		if(master->getSiteControl()->getPermittedCrossDomainPolicies() == PolicySiteControl::NONE)
		{
			LOG(LOG_NO_INFO, "Master policy file disallows policy files, disallowing");
			return DISALLOWED_CROSSDOMAIN_POLICY;
		}
		//Master is invalid AND only master policy files are allowed (by default)
		if(!master->isValid() && master->getSiteControl()->getPermittedCrossDomainPolicies() == PolicySiteControl::MASTER_ONLY)
		{
			LOG(LOG_NO_INFO, "Master policy file is invalid and allows master policy files only, disallowing");
			return DISALLOWED_CROSSDOMAIN_POLICY;
		}
		
		LOG(LOG_NO_INFO, "Master policy file is valid and allows master policy files only, checking against origin:\n" << 
				sys->getOrigin().getParsedURL());
		if(master->allowsAccessFrom(sys->getOrigin(), url))
		{
			LOG(LOG_NO_INFO, "Master policy file allowed access, allowing");
			return ALLOWED;
		}
		//Non-master policy files are allowed (/subdir/crossdomain.xml, loadPolicyFile())
		//These can further by restricted by content-type and ftp filename.
		//Lets check the other policy files.
		if(master->getSiteControl()->getPermittedCrossDomainPolicies() != PolicySiteControl::MASTER_ONLY)
		{
			for(std::vector<PolicyFile*>::iterator i = policyFiles.begin(); i != policyFiles.end(); ++i)
			{
				if(loadPendingFiles)
					(*i)->load();
				if((*i)->isLoaded() && (*i)->allowsAccessFrom(sys->getOrigin(), url))
				{
					LOG(LOG_NO_INFO, "Non-master policy file allowed access, allowing");
					return ALLOWED;
				}
			}
		}
	}

	return DISALLOWED_CROSSDOMAIN_POLICY;
}

PolicyFile* SecurityManager::addPolicyFile(const tiny_string& url)
{
	PolicyFile* file = new PolicyFile(url);
	if(file->isValid())
		policyFiles.push_back(file);
	return file;
}

PolicyFile* SecurityManager::getPolicyFileByURL(const URLInfo& url) const
{
	PolicyFile* file;
	for(std::vector<PolicyFile*>::const_iterator i = policyFiles.begin(); 
			i != policyFiles.end(); ++i)
	{
		file = (*i);
		if(file->getURL() == url)
			return file;
	}
	return NULL;
}

PolicyFile::PolicyFile(const tiny_string& _url):
	url(_url),type(GENERIC),valid(url.isValid()),ignore(false),
	master(false),loaded(false),siteControl(NULL)
{
	if(url.getProtocol() == "http")
		type = HTTP;
	else if(url.getProtocol() == "https")
		type = HTTPS;
	else if(url.getProtocol() == "ftp")
		type = FTP;
	
	if((type == HTTP || type == HTTPS || type == FTP) || url.getPath() == "/crossdomain.xml")
		master = true;

	//TODO: support other types of policy files
	//Socket policy files don't use an URL, but send a <policy-file-request/> to xmlsocket://domain:843
}
PolicyFile::~PolicyFile()
{
	for(std::vector<PolicyAllowAccessFrom*>::iterator i = allowAccessFrom.begin(); 
			i != allowAccessFrom.end(); ++i)
		delete (*i);
	for(std::vector<PolicyAllowHTTPRequestHeadersFrom*>::iterator i = allowHTTPRequestHeadersFrom.begin();
			i != allowHTTPRequestHeadersFrom.end(); ++i)
		delete (*i);
}

PolicyFile* PolicyFile::getMasterPolicyFile()
{
	//TODO: handle FTP, SOCKET policy files
	if(isMaster() && isValid()) return this;
	PolicyFile* file = sys->securityManager->getPolicyFileByURL(url.goToURL("/crossdomain.xml"));
	if(file == NULL)
	{
		file = sys->securityManager->addPolicyFile(url.goToURL("/crossdomain.xml"));
	}
	return file;
}

//TODO: support download timeout handling
void PolicyFile::load()
{
	//Invalid PolicyFile, ignore this call
	if(!isValid()) return;
	//Already loaded, no action needed
	if(isLoaded()) return;
	//We only try loading once, if something goes wrong, valid will be set to 'invalid'
	loaded = true;

	LOG(LOG_NO_INFO, "Loading cross domain policy file: " << url.getParsedURL());

	//Check if this file is allowed/ignored by the master policy file
	if(!isMaster())
	{
		PolicyFile* master = getMasterPolicyFile();
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
			else if(type == FTP && permittedPolicies == PolicySiteControl::BY_FTP_FILENAME && url.getPathFile() != "crossdomain.xml")
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
		
		xmlpp::TextReader xml(buf, downloader->getLength());
		//Ease-of-use variables
		std::string tagName;
		std::string attrValue;
		int attrCount;
		std::string domain;
		std::string toPorts;
		std::string headers;
		bool secure;
		bool secureSpecified;
		std::string fingerprint;
		std::string fingerprintAlgorithm;
		while(xml.read())
		{
			tagName = xml.get_name();
			//We only handle elements
			if(xml.get_node_type() != xmlpp::TextReader::Element)
				continue;

			//Some reasons for marking this file as invalid (extraneous content)
			if(xml.get_depth() > 1 || xml.has_value() || (xml.get_depth() == 0 && tagName != "cross-domain-policy"))
			{
				valid = false;
				break;
			}

			if(xml.get_depth() == 0 && tagName == "cross-domain-policy")
				continue;

			//We are inside the cross-domain-policy tag so lets handle elements.
			attrCount = xml.get_attribute_count();
			//Handle the first site-control element, if this is a master file and if the element has attributes
			if(tagName == "site-control" && siteControl == NULL && isMaster() && attrCount == 1)
			{
				attrValue = xml.get_attribute("permitted-cross-domain-policies");
				//Find the required attribute and set the siteControl variable
				if(attrValue != "")
				{
					siteControl = new PolicySiteControl(this, attrValue);
					//No more parsing is needed if this site-control entry specifies that no policy files are allowed
					if(isMaster() && siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::NONE)
						break;
				}
			}
			//Handle the allow-access-from element if the element has attributes
			else if(tagName == "allow-access-from" && xml.get_attribute_count() >= 1 && attrCount <= 3)
			{
				domain = xml.get_attribute("domain");
				toPorts = xml.get_attribute("to-ports");
				secure = false;
				secureSpecified = false;
				if(xml.get_attribute("secure") == "0")
				{
					secure = false;
					secureSpecified = true;
				}
				else if(xml.get_attribute("secure") == "1")
				{
					secure = true;
					secureSpecified = true;
				}
				//We found the required attribute, now lets add the object
				if(domain != "" && (type == HTTP || type == HTTPS))
					allowAccessFrom.push_back(new PolicyAllowAccessFrom(this, domain, toPorts, secure, secureSpecified));
				else if(domain != "" && toPorts != "" && type == SOCKET)
					allowAccessFrom.push_back(new PolicyAllowAccessFrom(this, domain, toPorts, secure, secureSpecified));
			}
			//Handle the allow-http-request-headers-from element if the element has attributes and if the policy file type is HTTP(S)
			else if(tagName == "allow-http-request-headers-from" && attrCount >= 2 && attrCount <= 3 && (type == HTTP || type == HTTPS))
			{
				domain = xml.get_attribute("domain");
				headers = xml.get_attribute("headers");
				secure = false;
				secureSpecified = false;
				if(xml.get_attribute("secure") == "0")
				{
					secure = false;
					secureSpecified = true;
				}
				else if(xml.get_attribute("secure") == "1")
				{
					secure = true;
					secureSpecified = true;
				}
				//We found the required attributes, now lets add the object
				if(domain != "" && headers != "")
					allowHTTPRequestHeadersFrom.push_back(new PolicyAllowHTTPRequestHeadersFrom(this, domain, headers, secure, secureSpecified));
			}
		}
		if(siteControl == NULL) //Set siteControl to the default value
			siteControl = new PolicySiteControl(this, "");

		if(isMaster() && siteControl->getPermittedCrossDomainPolicies() == PolicySiteControl::NONE)
			ignore = true;
	}
	else
	{
		//Failed to download the file, marking this file as invalid
		valid = false;
	}
	sys->downloadManager->destroy(downloader);
}

//TODO: support SOCKET specific method
bool PolicyFile::allowsAccessFrom(const URLInfo& u, const URLInfo& to)
{
	//This policy file doesn't apply to the given URL
	if(!isMaster() && !to.isSubOf(url))
		return false;

	load();
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

bool PolicyFile::allowsHTTPRequestHeaderFrom(const URLInfo& u, const URLInfo& to, const tiny_string& header)
{
	//Only used for HTTP(S)
	if(type != HTTP && type != HTTPS) return true;

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
	else if(file->getType() == PolicyFile::SOCKET) //Default for SOCKET
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
		if(file->getType() == PolicyFile::HTTPS)
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
}

bool PolicyAllowAccessFrom::allowsAccessFrom(const URLInfo& url) const
{
	//Check if domains match
	//TODO: resolve domain names using DNS before checking for a match?
	//See section 1.5.9 in specification
	if(!URLInfo::matchesDomain(domain, url.getHostname()))
		return false;

	//Check if the requesting URL is secure, if needed
	if(file->getType() == PolicyFile::HTTPS && secure && url.getProtocol() != "https")
		return false;
	
	//TODO: add check for to-ports and secure for SOCKET type policy files

	return true;
}

PolicyAllowHTTPRequestHeadersFrom::PolicyAllowHTTPRequestHeadersFrom(PolicyFile* _file, const std::string _domain, 
		const std::string _headers, bool _secure, bool secureSpecified):
	file(_file),domain(_domain),secure(_secure)
{
	if(!secureSpecified)
	{
		if(file->getType() == PolicyFile::HTTPS)
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
}

bool PolicyAllowHTTPRequestHeadersFrom::allowsHTTPRequestHeaderFrom(const URLInfo& url, const tiny_string& header) const
{
	if(file->getType() != PolicyFile::HTTP && file->getType() != PolicyFile::HTTPS)
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
	if(file->getType() == PolicyFile::HTTPS && secure && url.getProtocol() != "https")
		return false;
	
	return true;
}
