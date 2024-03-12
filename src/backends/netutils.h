/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010-2012  Timon Van Overveldt (timonvo@gmail.com)

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

#ifndef BACKENDS_NETUTILS_H
#define BACKENDS_NETUTILS_H 1

#include "forwards/threading.h"
#include "forwards/thread_pool.h"
#include "forwards/backends/netutils.h"
#include "interfaces/threading.h"
#include "compat.h"
#include <streambuf>
#include <fstream>
#include <list>
#include <map>
#include "swftypes.h"
#include "backends/urlutils.h"
#include "backends/streamcache.h"
#include "smartrefs.h"

namespace lightspark
{

class DLL_PUBLIC DownloadManager
{
private:
	Mutex mutex;
	std::list<Downloader*> downloaders;
protected:
	DownloadManager();
	void addDownloader(Downloader* downloader);
	bool removeDownloader(Downloader* downloader);
	void cleanUp();
public:
	virtual ~DownloadManager();
	virtual Downloader* download(const URLInfo& url, _R<StreamCache> cache, ILoadable* owner)=0;
	virtual Downloader* downloadWithData(const URLInfo& url, _R<StreamCache> cache, 
			const std::vector<uint8_t>& data,
			const std::list<tiny_string>& headers, ILoadable* owner)=0;
	virtual void destroy(Downloader* downloader)=0;
	void stopAll();

	enum MANAGERTYPE { NPAPI, STANDALONE };
	MANAGERTYPE type;
};

class DLL_PUBLIC StandaloneDownloadManager:public DownloadManager
{
public:
	StandaloneDownloadManager();
	~StandaloneDownloadManager();
	Downloader* download(const URLInfo& url, _R<StreamCache> cache, ILoadable* owner);
	Downloader* downloadWithData(const URLInfo& url, _R<StreamCache> cache,
			const std::vector<uint8_t>& data,
			const std::list<tiny_string>& headers, ILoadable* owner);
	void destroy(Downloader* downloader);
};

class DLL_PUBLIC Downloader
{
protected:
	//Abstract base class, can't be constructed
	Downloader(const tiny_string& _url, _R<StreamCache> _cache, ILoadable* o);
	Downloader(const tiny_string& _url, _R<StreamCache> _cache, const std::vector<uint8_t>& data,
		   const std::list<tiny_string>& headers, ILoadable* o);

	//-- FLAGS
	//Mark the download as failed
	void setFailed();
	//Mark the download as finished
	void setFinished();

	//-- PROPERTIES
	tiny_string url;
	tiny_string originalURL;

	//-- CACHING
        _R<StreamCache> cache;

	//-- PROGRESS MONITORING
	ILoadable* owner;
	void notifyOwnerAboutBytesTotal() const;
	void notifyOwnerAboutBytesLoaded() const;

	//-- HTTP REDIRECTION, STATUS & HEADERS
	bool redirected:1;
	void setRedirected(const tiny_string& newURL)
	{
		redirected = true;
		url = newURL;
	}
	uint16_t requestStatus;
	std::map<tiny_string, tiny_string> headers;
	void parseHeaders(const char* headers, bool _setLength);
	void parseHeader(std::string header, bool _setLength);
	//Data to send to the host
	const std::list<tiny_string> requestHeaders;
	const std::vector<uint8_t> data;

	//-- DOWNLOADED DATA
	//File length (can change in certain cases, resulting in reallocation of the buffer (non-cached))
	uint32_t length;
	//Set the length of the downloaded file, can be called multiple times to accomodate a growing file
	void setLength(uint32_t _length);
	
	bool emptyanswer;
public:
	//This class can only get destroyed by DownloadManager derivate classes
	virtual ~Downloader();
	//Stop the download
	void stop();

	//True if the download has failed
	bool hasFailed() { return cache->hasFailed(); }
	//True if the download has finished
	//Can be used in conjunction with failed to find out if it finished successfully
	bool hasFinished() { return cache->hasTerminated(); }
	
	// true if status header is 204 or content-length header is 0
	bool hasEmptyAnswer() { return emptyanswer; }

	const tiny_string& getURL() { return url; }

	//Wait until the downloader completes
	void waitForTermination() { return cache->waitForTermination(); }

	_R<StreamCache> getCache() { return cache; }
	//Gets the total length of the downloaded file (may change)
	uint32_t getLength() { return length; }
	//Gets the length of downloaded data
	uint32_t getReceivedLength() { return cache->getReceivedLength(); }

	size_t getHeaderCount() { return headers.size(); }
	tiny_string getHeader(const char* header) { return getHeader(tiny_string(header)); }
	tiny_string getHeader(tiny_string header) { return headers[header]; }
	std::map<tiny_string, tiny_string>::iterator getHeadersBegin()
	{ return headers.begin(); }
	std::map<tiny_string, tiny_string>::iterator getHeadersEnd()
	{ return headers.end(); }
	
	bool isRedirected() { return redirected; }
	const tiny_string& getOriginalURL() { return originalURL; }
	uint16_t getRequestStatus() { return requestStatus; }
	//Append data to the internal buffer
	void append(uint8_t* buffer, uint32_t length);
};

class ThreadedDownloader : public Downloader, public IThreadJob
{
private:
	ACQUIRE_RELEASE_FLAG(fenceState);
public:
	void enableFencingWaiting();
	void jobFence();
	void waitFencing();
protected:
	//Abstract base class, can not be constructed
	ThreadedDownloader(const tiny_string& url, _R<StreamCache> cache, ILoadable* o);
	ThreadedDownloader(const tiny_string& url, _R<StreamCache> cache,
			   const std::vector<uint8_t>& data,
			   const std::list<tiny_string>& headers, ILoadable* o);
//	//This class can only get destroyed by DownloadManager
//	virtual ~ThreadedDownloader();
};

//CurlDownloader can be used as a thread job, standalone or as a streambuf
class CurlDownloader: public ThreadedDownloader
{
private:
	static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t write_header(void *buffer, size_t size, size_t nmemb, void *userp);
	static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	void execute();
	void threadAbort();
public:
	CurlDownloader(const tiny_string& _url, _R<StreamCache> cache, ILoadable* o);
	CurlDownloader(const tiny_string& _url, _R<StreamCache> cache, const std::vector<uint8_t>& data,
		       const std::list<tiny_string>& headers, ILoadable* o);
};

//LocalDownloader can be used as a thread job, standalone or as a streambuf
class LocalDownloader: public ThreadedDownloader
{
private:
	bool dataGenerationMode;
	static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t write_header(void *buffer, size_t size, size_t nmemb, void *userp);
	static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	void execute();
	void threadAbort();
	
	//Size of the reading buffer
	static const size_t bufSize = 8192;
public:
	LocalDownloader(const tiny_string& _url, _R<StreamCache> _cache, ILoadable* o, bool dataGeneration = false);
};

class URLRequest;
class EventDispatcher;

// Common functionality for the resource loading classes (Loader,
// URLStream, etc)
class DownloaderThreadBase : public IThreadJob
{
private:
	IDownloaderThreadListener* listener;
protected:
	URLInfo url;
	std::vector<uint8_t> postData;
	std::list<tiny_string> requestHeaders;
	Mutex downloaderLock;
	Downloader* downloader;
	bool createDownloader(_R<StreamCache> cache,
			      _NR<EventDispatcher> dispatcher=NullRef,
			      ILoadable* owner=NULL,
			      bool checkPolicyFile=true);
	void jobFence();
public:
	DownloaderThreadBase(_NR<URLRequest> request, IDownloaderThreadListener* listener);
	void execute()=0;
	void threadAbort();
};

};
#endif /* BACKENDS_NETUTILS_H */
