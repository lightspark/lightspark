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

#ifndef _NET_UTILS_H
#define _NET_UTILS_H

#include "compat.h"
#include <streambuf>
#include <fstream>
#include <inttypes.h>
#include "swftypes.h"
#include "thread_pool.h"
#include "backends/urlutils.h"

namespace lightspark
{

class Downloader;

class DownloadManager
{
public:
	virtual ~DownloadManager(){};
	virtual Downloader* download(const tiny_string& u, bool cached=false)=0;
	virtual Downloader* download(const URLInfo& u, bool cached=false)=0;
	virtual void destroy(Downloader* d)=0;

	enum MANAGERTYPE { NPAPI, STANDALONE };
	MANAGERTYPE type;
};

class DLL_PUBLIC StandaloneDownloadManager:public DownloadManager
{
private:
public:
	StandaloneDownloadManager();
	~StandaloneDownloadManager(){}
	Downloader* download(const tiny_string& u, bool cached=false);
	Downloader* download(const URLInfo& u, bool cached=false);
	void destroy(Downloader* d);
};

class DLL_PUBLIC Downloader: public std::streambuf
{
private:
	//Signals new bytes available for reading
	sem_t available;

	//Whether to allow dynamically growing the buffer
	bool allowBufferRealloc;
	//True if the file is cached to disk (default = false)
	bool cached;	

	bool waiting;
	//Handles streambuf out-of-data events
	virtual int_type underflow();
	//Seeks to absolute position
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
	//Seeks to relative position
	virtual pos_type seekpos(pos_type, std::ios_base::openmode);
	//Helper to get the current offset
	pos_type getOffset() const;
protected:
	sem_t mutex;
	sem_t cacheOpen;
	//Signals termination of the download
	sem_t terminated;
	//True if the download is terminated
	bool hasTerminated;
	//True if the download has failed at some point
	bool failed;
	//True if the download has finished downloading all data
	bool finished;

	//This will hold the whole download (non-cached) or a window into the download (cached)
	uint8_t* buffer;

	//File length (can change in certain cases, resulting in reallocation of the buffer (non-cached))
	uint32_t len;
	//Amount of data already received
	uint32_t tail;

	//Cache filename
	tiny_string cacheFileName;
	//Cache fstream
	std::fstream cache;
	//Position of the cache buffer into the file
	uint32_t cachePos;
	//Size of data in the buffer
	uint32_t cacheSize;
	//Maximum size of the cache buffer
	static const size_t cacheMaxSize = 8192;
	//True if the cache file doesn't need to be deleted on destruction
	bool keepCache;
	//Wait for cache to be opened
	void waitForCache();
public:
	Downloader(bool cached);
	virtual ~Downloader();

	bool isCached() { return cached; }

	//Set to true to allow growing the buffer dynamically
	void setAllowBufferRealloc(bool allow) { allowBufferRealloc = allow; }
	bool getAllowBufferRealloc() { return allowBufferRealloc; }

	//Append data to the internal buffer
	void append(uint8_t* buffer, uint32_t len);

	//Stop the download
	void stop();
	//Wait for the download to finish
	void wait();

	//Mark the download as failed
	void setFailed();
	bool hasFailed() { return failed; }
	//Mark the download as finished
	void setFinished();
	bool hasFinished() { return finished; }

	//Set the length of the downloaded file, can be called multiple times to accomodate a growing file
	void setLen(uint32_t l);
	//Gets the total length of the downloaded file (may change)
	uint32_t getLength() { return len; }
	//Gets the length of downloaded data
	uint32_t getReceivedLength() { return tail; }
};

class ThreadedDownloader : public Downloader, public IThreadJob
{
public:
	ThreadedDownloader(bool cached):Downloader(cached){}
};

//CurlDownloader can be used as a thread job, standalone or as a streambuf
class CurlDownloader: public ThreadedDownloader
{
private:
	tiny_string url;
	//Used to detect redirects, we'll need this later anyway (e.g.: HTTPStatusEvent)
	uint32_t requestStatus;
	static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t write_header(void *buffer, size_t size, size_t nmemb, void *userp);
	static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	void execute();
	void threadAbort();
public:
	uint32_t getRequestStatus() { return requestStatus; }
	void setRequestStatus(uint32_t status) { requestStatus = status; }
	CurlDownloader(bool cached, const tiny_string& u);
};

//LocalDownloader can be used as a thread job, standalone or as a streambuf
class LocalDownloader: public ThreadedDownloader
{
private:
	tiny_string url;
	static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t write_header(void *buffer, size_t size, size_t nmemb, void *userp);
	static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	void execute();
	void threadAbort();
	
	//Size of the reading buffer
	static const size_t bufSize = 8192;
public:
	LocalDownloader(bool cached, const tiny_string& u);
};

};
#endif
