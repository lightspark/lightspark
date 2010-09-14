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

#ifndef _SECURITY_H
#define _SECURITY_H

#include "compat.h"
#include <string>
#include <vector>
#include <inttypes.h>
#include "swftypes.h"

namespace lightspark
{

class PolicyFile;

class SecurityManager
{
public:
	enum SANDBOXTYPE { REMOTE, LOCAL_WITH_FILE, LOCAL_WITH_NETWORK, LOCAL_TRUSTED };
private:
	//List of added policy files (note: not necessarily loaded)
	std::vector<PolicyFile*> policyFiles;

	//Security sandbox type
	SANDBOXTYPE sandboxType;
	//Use exact domains for player settings
	bool exactSettings;
	//True if exactSettings was already set once
	bool exactSettingsLocked;
public:
	SecurityManager();
	~SecurityManager();
	//Add a policy file located at url (only for HTTP)
	PolicyFile* addPolicyFile(const URLInfo& url) { return addPolicyFile(url.getParsedURL()); }
	PolicyFile* addPolicyFile(const tiny_string& url);
	//Get the policy file object (if any) for the policy file at url (only for HTTP)
	PolicyFile* getPolicyFileByURL(const tiny_string& url) const { return getPolicyFileByURL(URLInfo(url)); }
	PolicyFile* getPolicyFileByURL(const URLInfo& url) const;
	//TODO: handle FTP, socket policy files
	
	//Set the sandbox type
	void setSandboxType(SANDBOXTYPE type) { sandboxType = type; }
	SANDBOXTYPE getSandboxType() const { return sandboxType; }
	//Set exactSettings
	void setExactSettings(bool settings, bool locked=true)
	{ 
		if(!exactSettingsLocked) { 
			exactSettings = settings;
			exactSettingsLocked = locked;
		}
	}
	bool getExactSettings() const { return exactSettings; }
	bool getExactSettingsLocked() const { return exactSettingsLocked; }
	
	enum EVALUATIONRESULT { ALLOWED, DISALLOWED_SANDOX, DISALLOWED_CROSSDOMAIN_POLICY };
	EVALUATIONRESULT evaluateURL(const tiny_string& url, bool loadPendingFiles=true) 
	{ return evaluateURL(URLInfo(url), loadPendingFiles); }
	EVALUATIONRESULT evaluateURL(const URLInfo& url, bool loadPendingFiles=true);
};

class PolicySiteControl;
class PolicyAllowAccessFrom;
class PolicyAllowHTTPRequestHeadersFrom;
//TODO: add support for FTP, SOCKET policy files
class PolicyFile
{
public:
	enum POLICYFILETYPE { GENERIC, HTTP, HTTPS, FTP, SOCKET };
private:
	URLInfo url;
	POLICYFILETYPE type;

	//Is this PolicyFile object valid?
	//Reason for invalidness can be: incorrect URL, download failed
	bool valid;
	//Ignore this object?
	//Reason for ignoring can be: master policy file doesn't allow other/any policy files
	bool ignore;
	//Is this a master file
	bool master;
	//Is this file loaded and parsed yet?
	bool loaded;

	PolicySiteControl* siteControl;
	std::vector<PolicyAllowAccessFrom*> allowAccessFrom;
	std::vector<PolicyAllowHTTPRequestHeadersFrom*> allowHTTPRequestHeadersFrom;
public:
	//URL constructor, only for HTTP currently
	PolicyFile(const tiny_string& _url);
	//TODO: handle FTP, socket policy files
	~PolicyFile();
	const URLInfo& getURL() const { return url; }
	POLICYFILETYPE getType() const { return type; }
	bool isValid() const { return valid; }
	bool isIgnored() const { return ignore; }
	bool isMaster() const { return master; }
	bool isLoaded() const { return loaded; }
	//Get the master policy file controlling this one
	PolicyFile* getMasterPolicyFile();
	//Load and parse the policy file
	void load();

	const PolicySiteControl* getSiteControl() const { return siteControl ;}

	//Is access to the policy file URL allowed by this policy file?
	bool allowsAccessFrom(const URLInfo& url, const URLInfo& to);
	//Is this request header allowed by this policy file for the given URL?
	bool allowsHTTPRequestHeaderFrom(const URLInfo& u, const URLInfo& to, const tiny_string& header);
};

//Site-wide declarations for master policy file
//Only valid inside master policy files
class PolicySiteControl
{
public:
	enum METAPOLICYTYPE { 
		ALL, //All types of policy files are allowed (default for SOCKET)
		BY_CONTENT_TYPE, //Only policy files served with 'Content-Type: text/x-cross-domain-policy' are allowed (only for HTTP)
		BY_FTP_FILENAME, //Only policy files with 'crossdomain.xml' as filename are allowed (only for FTP)
		MASTER_ONLY, //Only this master policy file is allowed (default for HTTP/HTTPS/FTP)
		NONE, //No policy files are allowed, including this master policy file
		NONE_THIS_RESPONSE //Don't use this policy file, provided as a HTTP header only (TODO: support this type)
	};
private:
	PolicyFile* file;
	METAPOLICYTYPE permittedCrossDomainPolicies; //Required
public:
	PolicySiteControl(PolicyFile* _file, const std::string _permittedCrossDomainPolicies="");
	METAPOLICYTYPE getPermittedCrossDomainPolicies() const { return permittedCrossDomainPolicies; }
};

class PortRange
{
private:
	uint16_t startPort;
	uint16_t endPort;
	bool isRange;
	bool matchAll;
public:
	PortRange(uint16_t _startPort, uint16_t _endPort=0, bool _isRange=false, bool _matchAll=false):
		startPort(_startPort),endPort(_endPort),isRange(_isRange),matchAll(_matchAll){};
	bool matches(uint16_t port)
	{
		if(matchAll)
			return true;
		if(isRange)
		{
			return startPort >= port && endPort <= port;
		}
		else
			return startPort == port;
	}
};

//Permit access by documents from specified domains
class PolicyAllowAccessFrom
{
private:
	PolicyFile* file;
	std::string domain; //Required
	std::vector<PortRange*> toPorts; //Only used for SOCKET policy files, required
	bool secure; //Only used for SOCKET & HTTPS, optional, default: SOCKET=false, HTTPS=true
public:
	PolicyAllowAccessFrom(PolicyFile* _file, const std::string _domain, const std::string _toPorts, bool _secure, bool secureSpecified);
	~PolicyAllowAccessFrom();
	const std::string& getDomain() const { return domain; }
	size_t getToPortsLength() const { return toPorts.size(); }
	PortRange const* getToPort(size_t index) const { return toPorts[index]; }
	bool getSecure() const { return secure; }

	//Does this entry allow a given URL?
	bool allowsAccessFrom(const URLInfo& url) const;
};

//Permit HTTP request header sending (only for HTTP)
class PolicyAllowHTTPRequestHeadersFrom
{
private:
	PolicyFile* file;
	std::string domain; //Required
	std::vector<std::string*> headers; //Required
	bool secure; //Only used for HTTPS, optional, default=true
public:
	PolicyAllowHTTPRequestHeadersFrom(PolicyFile* _file, const std::string _domain, const std::string _headers, bool _secure, bool secureSpecified);
	~PolicyAllowHTTPRequestHeadersFrom();
	const std::string getDomain() const { return domain; }
	size_t getHeadersLength() const { return headers.size(); }
	std::string const* getHeader(size_t index) const { return headers[index]; }
	bool getSecure() const { return secure; }

	//Does this entry allow a given request header for a given URL?
	bool allowsHTTPRequestHeaderFrom(const URLInfo& url, const tiny_string& header) const;
};

}

#endif
