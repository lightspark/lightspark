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

#ifndef BACKENDS_SECURITY_H
#define BACKENDS_SECURITY_H 1

#include "compat.h"
#include <string>
#include <list>
#include <map>
#include <cinttypes>
#include "swftypes.h"
#include "threading.h"
#include "urlutils.h"
#include "parsing/crossdomainpolicy.h"

namespace lightspark
{

class PolicyFile;
class URLPolicyFile;
class SocketPolicyFile;
typedef std::list<URLPolicyFile*> URLPFileList;
typedef std::list<URLPolicyFile*>::iterator URLPFileListIt;
typedef std::list<URLPolicyFile*>::const_iterator URLPFileListConstIt;
typedef std::pair<tiny_string, URLPolicyFile*> URLPFilePair;
typedef std::multimap<tiny_string, URLPolicyFile*> URLPFileMap;
typedef std::multimap<tiny_string, URLPolicyFile*>::iterator URLPFileMapIt;
typedef std::multimap<tiny_string, URLPolicyFile*>::const_iterator URLPFileMapConstIt;
typedef std::pair<URLPFileMapIt, URLPFileMapIt> URLPFileMapItPair;
typedef std::pair<URLPFileMapConstIt, URLPFileMapConstIt> URLPFileMapConstItPair;

typedef std::list<SocketPolicyFile*> SocketPFileList;
typedef std::list<SocketPolicyFile*>::const_iterator SocketPFileListConstIt;
typedef std::pair<tiny_string, SocketPolicyFile*> SocketPFilePair;
typedef std::multimap<tiny_string, SocketPolicyFile*> SocketPFileMap;

class SecurityManager
{
public:
	enum SANDBOXTYPE { REMOTE=1, LOCAL_WITH_FILE=2, LOCAL_WITH_NETWORK=4, LOCAL_TRUSTED=8 };
private:
	Mutex mutex;

	const char* sandboxNames[4];
	const char* sandboxTitles[4];

	//Multimap (by domain) of pending policy files
	URLPFileMap pendingURLPFiles;
	//Multimap (by domain) of loaded policy files
	URLPFileMap loadedURLPFiles;

	//Multimap (by domain) of pending socket policy files
	SocketPFileMap pendingSocketPFiles;
	//Multimap (by domain) of loaded socket policy files
	SocketPFileMap loadedSocketPFiles;

	//Security sandbox type
	SANDBOXTYPE sandboxType;
	//Use exact domains for player settings
	bool exactSettings;
	//True if exactSettings was already set once
	bool exactSettingsLocked;

	template <class T>
	void loadPolicyFile(std::multimap<tiny_string, T*>& pendingFiles, std::multimap<tiny_string, T*>& loadedFiles, PolicyFile *file);
	template <class T>
	T* getPolicyFileByURL(std::multimap<tiny_string, T*>& pendingFiles, std::multimap<tiny_string, T*>& loadedFiles, const URLInfo& url);
	template<class T>
	std::list<T*> *searchPolicyFiles(const URLInfo& url,
					 T *master,
					 bool loadPendingPolicies,
					 std::multimap<tiny_string, T*>& pendingFiles,
					 std::multimap<tiny_string, T*>& loadedFiles);

	//Search for and loads (if allowed) policy files which should be checked
	//(used by evaluatePoliciesURL & evaluateHeader)
	//Master policy file is first-in-line and should be checked first
	URLPFileList* searchURLPolicyFiles(const URLInfo& url,
			bool loadPendingPolicies);
	SocketPFileList* searchSocketPolicyFiles(const URLInfo& url,
			bool loadPendingPolicies);
public:
	SecurityManager();
	~SecurityManager();

	//Add a policy file located at url, will decide what type of policy file it is.
	PolicyFile* addPolicyFile(const tiny_string& url) { return addPolicyFile(URLInfo(url)); }
	PolicyFile* addPolicyFile(const URLInfo& url);
	//Add an URL policy file located at url
	URLPolicyFile* addURLPolicyFile(const URLInfo& url);
	//Add a Socket policy file located at url
	SocketPolicyFile* addSocketPolicyFile(const URLInfo& url);
	//Get the URL policy file object (if any) for the URL policy file at url
	URLPolicyFile* getURLPolicyFileByURL(const URLInfo& url);
	SocketPolicyFile* getSocketPolicyFileByURL(const URLInfo& url);

	void loadURLPolicyFile(URLPolicyFile* file);
	void loadSocketPolicyFile(SocketPolicyFile* file);
	
	//Set the sandbox type
	void setSandboxType(SANDBOXTYPE type) { sandboxType = type; }
	SANDBOXTYPE getSandboxType() const { return sandboxType; }
	const char* getSandboxName() const { return getSandboxName(sandboxType); }
	const char* getSandboxName(SANDBOXTYPE type) const 
	{
		if(type == REMOTE) return sandboxNames[0];
		else if(type == LOCAL_WITH_FILE) return sandboxNames[1];
		else if(type == LOCAL_WITH_NETWORK) return sandboxNames[2];
		else if(type == LOCAL_TRUSTED) return sandboxNames[3];
		return NULL;
	}
	const char* getSandboxTitle() const { return getSandboxTitle(sandboxType); }
	const char* getSandboxTitle(SANDBOXTYPE type) const
	{
		if(type == REMOTE) return sandboxTitles[0];
		else if(type == LOCAL_WITH_FILE) return sandboxTitles[1];
		else if(type == LOCAL_WITH_NETWORK) return sandboxTitles[2];
		else if(type == LOCAL_TRUSTED) return sandboxTitles[3];
		return NULL;
	}
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
	
	//Evaluates whether the current sandbox is in the allowed sandboxes
	bool evaluateSandbox(int allowedSandboxes)
	{ return evaluateSandbox(sandboxType, allowedSandboxes); }
	bool evaluateSandbox(SANDBOXTYPE sandbox, int allowedSandboxes);

	//The possible results for the URL evaluation methods below
	enum EVALUATIONRESULT { ALLOWED, NA_RESTRICT_LOCAL_DIRECTORY,
		NA_REMOTE_SANDBOX, 	NA_LOCAL_SANDBOX,
		NA_CROSSDOMAIN_POLICY, NA_PORT, NA_HEADER };
	
	//Evaluates an URL by checking allowed sandboxes (URL policy files are not checked)
	EVALUATIONRESULT evaluateURLStatic(const tiny_string& url, int allowedSandboxesRemote,
			int allowedSandboxesLocal, bool restrictLocalDirectory)
	{
		return evaluateURLStatic(URLInfo(url), allowedSandboxesRemote, allowedSandboxesLocal);
	}
	EVALUATIONRESULT evaluateURLStatic(const URLInfo& url, int allowedSandboxesRemote, 
			int allowedSandboxesLocal, bool restrictLocalDirectory=true);
        static void checkURLStaticAndThrow(const URLInfo& url, int allowedSandboxesRemote, 
                        int allowedSandboxesLocal, bool restrictLocalDirectory=true);
	
	//Evaluates an URL by checking if the type of URL (local/remote) matches the allowed sandboxes
	EVALUATIONRESULT evaluateSandboxURL(const URLInfo& url,
			int allowedSandboxesRemote, int allowedSandboxesLocal);
	//Checks for restricted ports
	EVALUATIONRESULT evaluatePortURL(const URLInfo& url);
	//Evaluates a (local) URL by checking if it points to a subdirectory of the origin
	EVALUATIONRESULT evaluateLocalDirectoryURL(const URLInfo& url);
	//Checks URL policy files
	EVALUATIONRESULT evaluatePoliciesURL(const URLInfo& url, bool loadPendingPolicies);

	//Check socket policy files
	EVALUATIONRESULT evaluateSocketConnection(const URLInfo& url, bool loadPendingPolicies);

	//Check for restricted headers and policy files explicitly allowing certain headers
	EVALUATIONRESULT evaluateHeader(const tiny_string& url, const tiny_string& header,
			bool loadPendingPolicies) { return evaluateHeader(URLInfo(url), header, loadPendingPolicies); }
	EVALUATIONRESULT evaluateHeader(const URLInfo& url, const tiny_string& header,
			bool loadPendingPolicies);
};

class PolicySiteControl;
class PolicyAllowAccessFrom;
class PolicyAllowHTTPRequestHeadersFrom;
class PolicyFile
{
	friend class SecurityManager;
public:
	enum TYPE { URL, SOCKET };

	enum METAPOLICY { 
		ALL, //All types of policy files are allowed (default for SOCKET)
		BY_CONTENT_TYPE, //Only policy files served with 'Content-Type: text/x-cross-domain-policy' are allowed (only for HTTP)
		BY_FTP_FILENAME, //Only policy files with 'crossdomain.xml' as filename are allowed (only for FTP)
		MASTER_ONLY, //Only this master policy file is allowed (default for HTTP/HTTPS/FTP)
		NONE, //No policy files are allowed, including this master policy file
		NONE_THIS_RESPONSE //Don't use this policy file, provided as a HTTP header only (TODO: support this type)
	};
private:
	URLInfo originalURL;
protected:
	PolicyFile(URLInfo _url, TYPE _type);
	virtual ~PolicyFile();
	Mutex mutex;

	URLInfo url;
	TYPE type;

	//Is this PolicyFile object valid?
	//Reason for invalidness can be: incorrect URL, download failed (in case of URLPolicyFiles)
	bool valid;
	//Ignore this object?
	//Reason for ignoring can be: master policy file doesn't allow other/any policy files
	bool ignore;
	//Is this file loaded and parsed yet?
	bool loaded;
	//Load and parse the policy file
	void load();
	//Return true, if master policy file tells to ignore this file
	virtual bool isIgnoredByMaster()=0;
	//Download polic file content from url into policy.
	//Return true, if file was downloaded successfully and is valid.
	virtual bool retrievePolicyFile(std::vector<unsigned char>& policy)=0;
	//Return parser parameters required to handle this policy file
	virtual void getParserType(CrossDomainPolicy::POLICYFILETYPE&, CrossDomainPolicy::POLICYFILESUBTYPE&)=0;
	//Return false if siteControl is invalid
	virtual bool checkSiteControlValidity();
	//Process one element from the policy file
        virtual void handlePolicyElement(CrossDomainPolicy::ELEMENT& elementType, const CrossDomainPolicy& parser);

	PolicySiteControl* siteControl;
	std::list<PolicyAllowAccessFrom*> allowAccessFrom;
public:

	const URLInfo& getURL() const { return url; }
	const URLInfo& getOriginalURL() const { return originalURL; }
	TYPE getType() const { return type; }

	bool isValid() const { return valid; }
	bool isIgnored() const { return ignore; }
	virtual bool isMaster() const = 0;
	bool isLoaded() const { return loaded; }

	//Get the master policy file controlling this one
	//virtual PolicyFile* getMasterPolicyFile()=0;

	METAPOLICY getMetaPolicy();

	//Is access to the policy file URL allowed by this policy file?
	virtual bool allowsAccessFrom(const URLInfo& url, const URLInfo& to) = 0;
};

class URLPolicyFile : public PolicyFile
{
	friend class SecurityManager;
public:
	enum SUBTYPE { HTTP, HTTPS, FTP };
private:
	SUBTYPE subtype;

	std::list<PolicyAllowHTTPRequestHeadersFrom*> allowHTTPRequestHeadersFrom;
protected:
	URLPolicyFile(const URLInfo& _url);
	~URLPolicyFile();
	//Load and parse the policy file
	bool isIgnoredByMaster();
	bool retrievePolicyFile(std::vector<unsigned char>& policy);
	void getParserType(CrossDomainPolicy::POLICYFILETYPE&, CrossDomainPolicy::POLICYFILESUBTYPE&);
	bool checkSiteControlValidity();
        void handlePolicyElement(CrossDomainPolicy::ELEMENT& elementType, const CrossDomainPolicy& parser);
public:
	SUBTYPE getSubtype() const { return subtype; }

	//If strict is true, the policy will be loaded first to see if it isn't redirected
	bool isMaster() const;
	//Get the master policy file controlling this one
	URLPolicyFile* getMasterPolicyFile();

	//Is access to the policy file URL allowed by this policy file?
	bool allowsAccessFrom(const URLInfo& url, const URLInfo& to);
	//Is this request header allowed by this policy file for the given URL?
	bool allowsHTTPRequestHeaderFrom(const URLInfo& u, const URLInfo& to, const std::string& header);
};

class SocketPolicyFile : public PolicyFile
{
	friend class SecurityManager;
protected:
	SocketPolicyFile(const URLInfo& _url);
	bool isIgnoredByMaster();
	bool retrievePolicyFile(std::vector<unsigned char>& outData);
	void getParserType(CrossDomainPolicy::POLICYFILETYPE&, CrossDomainPolicy::POLICYFILESUBTYPE&);
public:
	static const unsigned int MASTER_PORT;
	static const char *MASTER_PORT_URL;

	//If strict is true, the policy will be loaded first to see if it isn't redirected
	bool isMaster() const;

	//Get the master policy file controlling this one
	SocketPolicyFile* getMasterPolicyFile();

	//Is access to the policy file URL allowed by this policy file?
	bool allowsAccessFrom(const URLInfo& url, const URLInfo& to);
};

//Site-wide declarations for master policy file
//Only valid inside master policy files
class PolicySiteControl
{
private:
	PolicyFile::METAPOLICY permittedPolicies; //Required
public:
	PolicySiteControl(const std::string _permittedPolicies);
	PolicyFile::METAPOLICY getPermittedPolicies() const { return permittedPolicies; }
	static PolicyFile::METAPOLICY defaultSitePolicy(PolicyFile::TYPE policyFileType);
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
	friend class PolicyFile;
	friend class URLPolicyFile;
	friend class SocketPolicyFile;
private:
	PolicyFile* file;
	std::string domain; //Required
	std::list<PortRange*> toPorts; //Only used for SOCKET policy files, required
	bool secure; //Only used for SOCKET & HTTPS, optional, default: SOCKET=false, HTTPS=true
protected:
	PolicyAllowAccessFrom(PolicyFile* _file, const std::string _domain, const std::string _toPorts, bool _secure, bool secureSpecified);
	~PolicyAllowAccessFrom();
public:
	const std::string& getDomain() const { return domain; }
	size_t getToPortsLength() const { return toPorts.size(); }
	std::list<PortRange*>::const_iterator getToPortsBegin() const { return toPorts.begin(); }
	std::list<PortRange*>::const_iterator getToPortsEnd() const { return toPorts.end(); }
	bool getSecure() const { return secure; }

	//Does this entry allow a given URL?
	bool allowsAccessFrom(const URLInfo& url, uint16_t toPort=0, bool bCheckHttps=true) const;
};

//Permit HTTP request header sending (only for HTTP)
class PolicyAllowHTTPRequestHeadersFrom
{
	friend class PolicyFile;
	friend class URLPolicyFile;
private:
	URLPolicyFile* file;
	std::string domain; //Required
	std::list<std::string*> headers; //Required
	bool secure; //Only used for HTTPS, optional, default=true
protected:
	PolicyAllowHTTPRequestHeadersFrom(URLPolicyFile* _file, const std::string _domain, const std::string _headers, bool _secure, bool secureSpecified);
	~PolicyAllowHTTPRequestHeadersFrom();
public:
	const std::string getDomain() const { return domain; }
	size_t getHeadersLength() const { return headers.size(); }
	std::list<std::string*>::const_iterator getHeadersBegin() const { return headers.begin(); }
	std::list<std::string*>::const_iterator getHeadersEnd() const { return headers.end(); }
	bool getSecure() const { return secure; }

	//Does this entry allow a given request header for a given URL?
	bool allowsHTTPRequestHeaderFrom(const URLInfo& url, const std::string& header) const;
};

}

#endif /* BACKENDS_SECURITY_H */
