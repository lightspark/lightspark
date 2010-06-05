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

sync_stream::sync_stream():head(0),tail(0),wait_notfull(false),wait_notempty(false),buf_size(1024*1024),failed(false)
{
	buffer=new char[buf_size];
	sem_init(&mutex,0,1);
	sem_init(&notfull,0,0);
	sem_init(&notempty,0,0);
}

sync_stream::~sync_stream()
{
	sem_destroy(&mutex);
	sem_destroy(&notfull);
	sem_destroy(&notempty);
}

void sync_stream::destroy()
{
	failed=true;
	sem_post(&notfull);
	sem_post(&notempty);
}

int sync_stream::provideBuffer(int limit)
{
	sem_wait(&mutex);
	if(tail==head)
	{
		wait_notempty=true;
		sem_post(&mutex);
		sem_wait(&notempty);
		if(failed)
			return 0;
	}

	int available=(tail-head+buf_size)%buf_size;
	available=imin(available,limit);
	if(head+available>buf_size)
	{
		int i=buf_size-head;
		memcpy(in_buf,buffer+head,i);
		memcpy(in_buf+i,buffer,available-i);
	}
	else
		memcpy(in_buf,buffer+head,available);

	head+=available;
	head%=buf_size;
	if(wait_notfull)
	{
		wait_notfull=true;
		sem_post(&notfull);
	}
	else
		sem_post(&mutex);
	return available;
}

uint32_t sync_stream::write(char* buf, int len)
{
	sem_wait(&mutex);
	if(((tail-head+buf_size)%buf_size)==buf_size-1)
	{
		wait_notfull=true;
		sem_post(&mutex);
		sem_wait(&notfull);
		if(failed)
			return 0;
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

zlib_filter::zlib_filter():compressed(false),consumed(0),available(0)
{
}

zlib_filter::~zlib_filter()
{
	if(compressed)
		inflateEnd(&strm);
}

bool zlib_filter::initialize()
{
	//Should check that this is called only once
	available=provideBuffer(8);
	if(available!=8)
		return false;
	//We read only the first 8 bytes, as those are always uncompressed

	//Now check the signature
	if(in_buf[1]!='W' || in_buf[2]!='S')
		return false;
	if(in_buf[0]=='F')
		compressed=false;
	else if(in_buf[0]=='C')
	{
		compressed=true;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		int ret = inflateInit(&strm);
		if (ret != Z_OK)
			throw lightspark::RunTimeException("Failed to initialize ZLib");
	}
	else
		return false;

	//Ok, it seems to be a valid SWF, from now, if the file is compressed, data has to be inflated

	//Copy the in_buf to the out buffer
	memcpy(buffer,in_buf,8);
	setg(buffer,buffer,buffer+available);
	return true;
}

zlib_filter::int_type zlib_filter::underflow()
{
	assert(gptr()==egptr());

	//First of all we add the lenght of the buffer to the consumed variable
	consumed+=(gptr()-eback());

	//The first time
	if(consumed==0)
	{
		bool ret=initialize();
		if(ret)
			return (unsigned char)buffer[0];
		else
			throw lightspark::ParseException("Not an SWF file");
	}

	if(!compressed)
	{
		int real_count=provideBuffer(4096);
		assert(real_count>0);
		memcpy(buffer,in_buf,real_count);
		setg(buffer,buffer,buffer+real_count);
	}
	else
	{
		// run inflate() on input until output buffer is full
		strm.avail_out = 4096;
		strm.next_out = (unsigned char*)buffer;
		do
		{
			if(strm.avail_in==0)
			{
				int real_count=provideBuffer(4096);
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
	}

	//Cast to signed, otherwise 0xff would become eof
	return (unsigned char)buffer[0];
}

zlib_filter::pos_type zlib_filter::seekoff(off_type off, ios_base::seekdir dir,ios_base::openmode mode)
{
	assert(off==0);
	//The current offset is the amount of byte completely consumed plus the amount used in the buffer
	int ret=consumed+(gptr()-eback());
	return ret;
}

zlib_file_filter::zlib_file_filter(const char* file_name)
{
	f=fopen(file_name,"rb");
	if(f==NULL)
		throw lightspark::RunTimeException("File does not exists");
}

int zlib_file_filter::provideBuffer(int limit)
{
	int ret=fread(in_buf,1,limit,f);
	if(feof(f))
		fclose(f);
	return ret;
}

zlib_bytes_filter::zlib_bytes_filter(const uint8_t* b, int l):buf(b),offset(0),len(l)
{
}

int zlib_bytes_filter::provideBuffer(int limit)
{
	int ret=imin(limit,len-offset);
	memcpy(in_buf,buf+offset,ret);
	offset+=ret;
	return ret;
}

