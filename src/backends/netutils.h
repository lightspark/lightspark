/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)
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

#include "compat.h"
#include <streambuf>
#include <fstream>
#include <list>
#include <map>
#include "swftypes.h"
#include "thread_pool.h"
#include "backends/urlutils.h"
#include "smartrefs.h"

namespace lightspark
{

class Downloader;

class ILoadable
{
protected:
	~ILoadable(){}
public:
	virtual void setBytesTotal(uint32_t b) = 0;
	virtual void setBytesLoaded(uint32_t b) = 0;
};

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
	virtual Downloader* download(const URLInfo& url, bool cached, ILoadable* owner)=0;
	virtual Downloader* downloadWithData(const URLInfo& url, const std::vector<uint8_t>& data,
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
	Downloader* download(const URLInfo& url, bool cached, ILoadable* owner);
	Downloader* downloadWithData(const URLInfo& url, const std::vector<uint8_t>& data,
			const std::list<tiny_string>& headers, ILoadable* owner);
	void destroy(Downloader* downloader);
};

class DLL_PUBLIC Downloader: public std::streambuf
{
private:
	//Handles streambuf out-of-data events
	virtual int_type underflow();
	//Seeks to absolute position
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
	//Seeks to relative position
	virtual pos_type seekpos(pos_type, std::ios_base::openmode);
	//Helper to get the current offset
	pos_type getOffset() const;
protected:
	//Abstract base class, can't be constructed
	Downloader(const tiny_string& _url, bool _cached, ILoadable* o);
	Downloader(const tiny_string& _url, const std::vector<uint8_t>& data,
		   const std::list<tiny_string>& headers, ILoadable* o);
	//-- LOCKING
	//Provides internal mutual exclusing
	Mutex mutex;
	//Signals the cache opening
	Semaphore cacheOpened;
	//Signals new bytes available for reading
	Semaphore dataAvailable;
	//Signals termination of the download
	Semaphore terminated;
	//True if the download is terminated
	bool hasTerminated;
	//True if cache has opened
	bool cacheHasOpened;

	//-- STATUS
	//True if the downloader is waiting for the cache to be opened
	bool waitingForCache;
	//True if the downloader is waiting for data
	bool waitingForData;
	void waitForData_locked();
	//True if the downloader is waiting for termination
	bool waitingForTermination;

	//-- FLAGS
	//This flag forces a stop in internal code
	bool forceStop;
	//These flags specify what type of termination happened
	bool failed;
	bool finished;
	//Mark the download as failed
	void setFailed();
	//Mark the download as finished
	void setFinished();

	//-- PROPERTIES
	tiny_string url;
	tiny_string originalURL;

	//-- BUFFERING
	//This will hold the whole download (non-cached) or a window into the download (cached)
	uint8_t* buffer;
	//We can't change the used buffer (for example when resizing) asynchronously. We can only do that on underflows
	uint8_t* stableBuffer;
	//Minimum growth of the buffer
	static const size_t bufferMinGrowth = 4096;
	//(Re)allocate the buffer
	void allocateBuffer(size_t size);
	//Synchronize stableBuffer and buffer
	void syncBuffers();

	//-- PROGRESS MONITORING
	ILoadable* owner;
	void notifyOwnerAboutBytesTotal() const;
	void notifyOwnerAboutBytesLoaded() const;

	//-- CACHING
	//Cache filename
	tiny_string cacheFilename;
	//Cache fstream
	std::fstream cache;
	//Position of the cache buffer into the file
	uint32_t cachePos;
	//Size of data in the buffer
	uint32_t cacheSize;
	//Maximum size of the cache buffer
	static const size_t cacheMaxSize = 8192;
	//True if the cache file doesn't need to be deleted on destruction
	bool keepCache:1;
	//True if the file is cached to disk (default = false)
	bool cached:1;
	//Creates & opens a temporary cache file
	void openCache();
	//Opens an existing cache file
	void openExistingCache(tiny_string filename);

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
	//Amount of data already received
	uint32_t receivedLength;
	//Append data to the internal buffer
	void append(uint8_t* buffer, uint32_t length);
	//Set the length of the downloaded file, can be called multiple times to accomodate a growing file
	void setLength(uint32_t _length);
public:
	//This class can only get destroyed by DownloadManager derivate classes
	virtual ~Downloader();
	//Stop the download
	void stop();
	//Wait for cache to be opened
	void waitForCache();
	//Wait for data to become available
	void waitForData() { Mutex::Lock l(mutex); waitForData_locked(); }
	//Wait for the download to terminate
	void waitForTermination();

	//True if the download has failed
	bool hasFailed() { return failed; }
	//True if the download has finished
	//Can be used in conjunction with failed to find out if it finished successfully
	bool hasFinished() { return finished; }

	//True if the download is cached
	bool isCached() { return cached; }

	const tiny_string& getURL() { return url; }

	//Gets the total length of the downloaded file (may change)
	uint32_t getLength() { return length; }
	//Gets the length of downloaded data
	uint32_t getReceivedLength() { return receivedLength; }

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
	ThreadedDownloader(const tiny_string& url, bool cached, ILoadable* o);
	ThreadedDownloader(const tiny_string& url, const std::vector<uint8_t>& data,
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
	CurlDownloader(const tiny_string& _url, bool _cached, ILoadable* o);
	CurlDownloader(const tiny_string& _url, const std::vector<uint8_t>& data,
		       const std::list<tiny_string>& headers, ILoadable* o);
};

//LocalDownloader can be used as a thread job, standalone or as a streambuf
class LocalDownloader: public ThreadedDownloader
{
private:
	static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t write_header(void *buffer, size_t size, size_t nmemb, void *userp);
	static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	void execute();
	void threadAbort();
	
	//Size of the reading buffer
	static const size_t bufSize = 8192;
public:
	LocalDownloader(const tiny_string& _url, bool _cached, ILoadable* o);
};

class IDownloaderThreadListener
{
protected:
	virtual ~IDownloaderThreadListener() {}
public:
	virtual void threadFinished(IThreadJob*)=0;
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
	Spinlock downloaderLock;
	Downloader* downloader;
	bool createDownloader(bool cached,
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
