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

#include "streams.h"
#include "logger.h"
#include "exceptions.h"
#include "compat.h"
#include <cstdlib>
#include <cstring>
#include <assert.h>

using namespace std;

sync_stream::sync_stream():head(0),tail(0),wait_notfull(false),wait_notempty(false),buf_size(1024*1024),failed(false),ended(false),consumed(0)
{
	buffer=new char[buf_size];
	sem_init(&mutex,0,1);
	sem_init(&notfull,0,0);
	sem_init(&notempty,0,0);
	setg(buffer,buffer,buffer);
}

sync_stream::~sync_stream()
{
	delete[] buffer;
	sem_destroy(&mutex);
	sem_destroy(&notfull);
	sem_destroy(&notempty);
}

void sync_stream::stop()
{
	sem_wait(&mutex);
	failed=true;
	if(wait_notfull)
	{
		wait_notfull=false;
		sem_post(&notfull);
		sem_wait(&mutex);
	}
	if(wait_notempty)
	{
		wait_notempty=false;
		sem_post(&notempty);
		sem_wait(&mutex);
	}
	sem_post(&mutex);
}

void sync_stream::eof()
{
	sem_wait(&mutex);
	ended=true;
	if(wait_notempty)
	{
		wait_notempty=false;
		sem_post(&notempty);
	}
	else
		sem_post(&mutex);
}

sync_stream::pos_type sync_stream::seekoff(off_type off, ios_base::seekdir dir,ios_base::openmode mode)
{
	assert(off==0);
	//The current offset is the amount of byte completely consumed plus the amount used in the buffer
	int ret=consumed+(gptr()-eback());
	return ret;
}

sync_stream::int_type sync_stream::underflow()
{
	assert(gptr()==egptr());

	sem_wait(&mutex);
	//First of all we add the length of the buffer to the consumed variable
	int consumedNow=(gptr()-eback());
	consumed+=consumedNow;
	head+=consumedNow;
	head%=buf_size;
	if(failed)
	{
		sem_post(&mutex);
		//Return EOF
		return -1;
	}
	if(tail==head)
	{
		if(ended) //There is no way more data will arrive
		{
			sem_post(&mutex);
			//Return EOF
			return -1;
		}
		wait_notempty=true;
		sem_post(&mutex);
		sem_wait(&notempty);
		if(failed || ended)
		{
			sem_post(&mutex);
			//Return EOF
			return -1;
		}
	}

	if(head<tail)
		setg(buffer+head,buffer+head,buffer+tail);
	else
		setg(buffer+head,buffer+head,buffer+buf_size);

	//Verify that there is room
	if(wait_notfull && ((tail-head+buf_size)%buf_size)<buf_size-1)
	{
		wait_notfull=true;
		sem_post(&notfull);
	}
	else
		sem_post(&mutex);

	//Cast to unsigned, otherwise 0xff would become eof
	return (unsigned char)buffer[head];
}

uint32_t sync_stream::write(char* buf, int len)
{
	sem_wait(&mutex);
	if(failed)
	{
		sem_post(&mutex);
		return 0;
	}
	if(((tail-head+buf_size)%buf_size)==buf_size-1)
	{
		wait_notfull=true;
		sem_post(&mutex);
		sem_wait(&notfull);
		if(failed)
		{
			sem_post(&mutex);
			return 0;
		}
	}

	if((head-tail+buf_size-1)%buf_size<len)
		len=(head-tail+buf_size-1)%buf_size;

	if(tail+len>buf_size)
	{
		int i=buf_size-tail;
		memcpy(buffer+tail,buf,i);
		memcpy(buffer,buf+i,len-i);
	}
	else
		memcpy(buffer+tail,buf,len);
	tail+=len;
	tail%=buf_size;
	assert(head!=tail);
	if(wait_notempty)
	{
		wait_notempty=false;
		sem_post(&notempty);
	}
	else
		sem_post(&mutex);
	return len;
}

uint32_t sync_stream::getFree()
{
	sem_wait(&mutex);
	uint32_t freeBytes=(head-tail+buf_size-1)%buf_size;
	sem_post(&mutex);
	return freeBytes;
}

zlib_filter::zlib_filter(streambuf* b):backend(b),consumed(0)
{
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	int ret = inflateInit(&strm);
	if (ret != Z_OK)
		throw lightspark::RunTimeException("Failed to initialize ZLib");
	setg(buffer,buffer,buffer);
	consumed+=pubseekoff(0, ios_base::cur, ios_base::in);
}

zlib_filter::~zlib_filter()
{
	inflateEnd(&strm);
}

zlib_filter::int_type zlib_filter::underflow()
{
	assert(gptr()==egptr());

	//First of all we add the length of the buffer to the consumed variable
	consumed+=(gptr()-eback());

	// run inflate() on input until output buffer is full
	strm.avail_out = 4096;
	strm.next_out = (unsigned char*)buffer;
	do
	{
		if(strm.avail_in==0)
		{
			int real_count=backend->sgetn(in_buf,4096);
			if(real_count==0)
			{
				//File is not big enough
				throw lightspark::ParseException("Unexpected end of file");
			}
			strm.next_in=(unsigned char*)in_buf;
			strm.avail_in=real_count;
		}
		int ret=inflate(&strm, Z_NO_FLUSH);
		if(ret==Z_OK);
		else if(ret==Z_STREAM_END)
		{
			//The stream ended, close the buffer here
			break;
		}
		else
			throw lightspark::ParseException("Unexpected Zlib error");
	}
	while(strm.avail_out!=0);
	int available=4096-strm.avail_out;
	setg(buffer,buffer,buffer+available);

	//Cast to unsigned, otherwise 0xff would become eof
	return (unsigned char)buffer[0];
}

zlib_filter::pos_type zlib_filter::seekoff(off_type off, ios_base::seekdir dir,ios_base::openmode mode)
{
	assert(off==0);
	//The current offset is the amount of byte completely consumed plus the amount used in the buffer
	int ret=consumed+(gptr()-eback());
	return ret;
}

bytes_buf::bytes_buf(const uint8_t* b, int l):buf(b),offset(0),len(l)
{
	setg((char*)buf,(char*)buf,(char*)buf+len);
}

bytes_buf::pos_type bytes_buf::seekoff(off_type off, ios_base::seekdir dir,ios_base::openmode mode)
{
	assert(off==0);
	//The current offset is the amount used in the buffer
	int ret=(gptr()-eback());
	return ret;
}

