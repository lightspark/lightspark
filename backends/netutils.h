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
#include <inttypes.h>
#include "swftypes.h"
#include "thread_pool.h"

namespace lightspark
{

class Downloader;

class DownloadManager
{
public:
	virtual Downloader* download(const tiny_string& u)=0;
	virtual void destroy(Downloader* d)=0;
	virtual ~DownloadManager(){}
};

class DLL_PUBLIC CurlDownloadManager:public DownloadManager
{
public:
	Downloader* download(const tiny_string& u);
	void destroy(Downloader* d);
};

class DLL_PUBLIC Downloader: public std::streambuf
{
private:
	sem_t mutex;
	uint8_t* buffer;
	uint32_t len;
	uint32_t tail;
	bool waiting;
	virtual int_type underflow();
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
	virtual pos_type seekpos(pos_type, std::ios_base::openmode);
	sem_t available;
protected:
	sem_t terminated;
	void setFailed();
	bool failed;
public:
	Downloader();
	virtual ~Downloader();
	void setLen(uint32_t l);
	void append(uint8_t* buffer, uint32_t len);
	void stop();
	void wait();
	bool hasFailed() { return failed; }
	uint8_t* getBuffer()
	{
		return buffer;
	}
	uint32_t getLen()
	{
		return len;
	}
};

//CurlDownloader can be used as a thread job, standalone or as a streambuf
class CurlDownloader: public Downloader, public IThreadJob
{
private:
	tiny_string url;
	static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t write_header(void *buffer, size_t size, size_t nmemb, void *userp);
	static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	void execute();
	void threadAbort();
public:
	CurlDownloader(const tiny_string& u);
};

};
#endif
