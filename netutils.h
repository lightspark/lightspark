/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef _NET_UTILS_H
#define _NET_UTILS_H

#include <streambuf>
#include <inttypes.h>
#include "swftypes.h"

#include "thread_pool.h"

namespace lightspark
{

//CurlDownloader can be used as a thread job, standalone or as a streambuf
class CurlDownloader: public IThreadJob, public std::streambuf
{
private:
	uint8_t* buffer;
	uint32_t len;
	uint32_t tail;
	sem_t available;
	sem_t mutex;
	tiny_string url;
	bool failed;
	bool waiting;
	static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t write_header(void *buffer, size_t size, size_t nmemb, void *userp);
	static int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	void execute();
	void abort();
	void setFailed();
public:
	CurlDownloader(const tiny_string& u);
	virtual int_type underflow();
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
	virtual pos_type seekpos(pos_type, std::ios_base::openmode);
	bool download();
	uint8_t* getBuffer()
	{
		return buffer;
	}
	uint32_t getLen()
	{
		return len;
	}
};

};
#endif
