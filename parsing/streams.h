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

#ifndef _STREAMS_H
#define _STREAMS_H

#include "compat.h"
#include <streambuf>
#include <fstream>
#include <semaphore.h>
#include <inttypes.h>
#include "zlib.h"

class zlib_filter: public std::streambuf
{
private:
	std::streambuf* backend;
	int consumed;
	z_stream strm;
	char buffer[4096];
protected:
	char in_buf[4096];
	virtual int_type underflow();
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
public:
	zlib_filter(std::streambuf* b);
	~zlib_filter();
};

class DLL_PUBLIC sync_stream: public std::streambuf
{
public:
	sync_stream();
	void destroy();
	~sync_stream();
	uint32_t write(char* buf, int len);
	uint32_t getFree();
	virtual int_type underflow();
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
private:
	char* buffer;
	int head;
	int tail;
	sem_t mutex;
	sem_t notfull;
	sem_t notempty;
	bool wait_notfull;
	bool wait_notempty;
	const int buf_size;
	bool failed;
	int consumed;
};

class bytes_buf:public std::streambuf
{
private:
	const uint8_t* buf;
	int offset;
	int len;
public:
	bytes_buf(const uint8_t* b, int l);
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
};

#endif
