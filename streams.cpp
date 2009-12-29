/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "streams.h"
#include "logger.h"
#include <cstdlib>
#include <cstring>

using namespace std;

sync_stream::sync_stream():head(0),tail(0),wait(-1),buf_size(1024*1024),compressed(0),offset(0)
{
	printf("syn stream\n");
	buffer= new char[buf_size];
	sem_init(&mutex,0,1);
	sem_init(&empty,0,0);
	sem_init(&full,0,0);
	sem_init(&ready,0,0);
}

void sync_stream::setCompressed()
{
	sem_wait(&mutex);
	compressed=1;
	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	int ret = inflateInit(&strm);
	if (ret != Z_OK)
		LOG(LOG_ERROR,"Failed to initialize ZLib");
	sem_post(&mutex);
}

streamsize sync_stream::xsgetn ( char * s, streamsize n )
{
	sem_wait(&mutex);
	if(compressed)
	{
		/* decompress until deflate stream ends or end of file */
		if(head<tail)
		{
			strm.avail_in=tail-head;
			strm.next_in=(unsigned char*)buffer+head;
		}
		else
			LOG(LOG_ERROR,"Sync stream WIP 1");

		/* run inflate() on input until output buffer not full */
		strm.avail_out = n;
		strm.next_out = (unsigned char*)s;
		inflate(&strm, Z_NO_FLUSH);

		head+=(tail-head)-strm.avail_in;
		head%=buf_size;

		//check if output full and wrap around
		while(strm.avail_out!=0)
		{
			LOG(LOG_NO_INFO,"Try code");
			wait=(head+1)%buf_size;
			sem_post(&mutex);
			sem_wait(&ready);
			wait=-1;
			if(head<tail)
			{
				strm.avail_in=tail-head;
				strm.next_in=(unsigned char*)buffer+head;
				inflate(&strm, Z_NO_FLUSH);
			}
			else
				LOG(LOG_ERROR,"Sync stream WIP 3");
			head+=(tail-head)-strm.avail_in;
			head%=buf_size;
		}
	}
	else
	{
		if((tail-head+buf_size)%buf_size<n)
		{
			wait=(head+n)%buf_size;
			sem_post(&mutex);
			sem_wait(&ready);
			wait=-1;
		}
		if(head+n>buf_size)
		{
			int i=buf_size-head;
			memcpy(s,buffer+head,i);
			memcpy(s+i,buffer,n-i);
		}
		else
			memcpy(s,buffer+head,n);
		head+=n;
		head%=buf_size;
	}
	offset+=n;
	sem_post(&mutex);
	return n;
}


streamsize sync_stream::xsputn ( const char * s, streamsize n )
{
	sem_wait(&mutex);
	if((head-tail+buf_size-1)%buf_size<n)
	{
		n=(head-tail+buf_size-1)%buf_size;
	}
	if(tail+n>buf_size)
	{
		int i=buf_size-tail;
		memcpy(buffer+tail,s,i);
		memcpy(buffer,s+i,n-i);
	}
	else
		memcpy(buffer+tail,s,n);
	tail+=n;
	tail%=buf_size;
	if(wait>0 && tail>=wait)
		sem_post(&ready);
	else
		sem_post(&mutex);
	return n;
}

std::streampos sync_stream::seekpos ( std::streampos sp, std::ios_base::openmode which )
{
	printf("puppa1\n");
	abort();
	return 0;
}

std::streampos sync_stream::seekoff ( std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which )
{
	int ret;
	sem_wait(&mutex);
	ret=offset;
	if(off!=0)
	{
		printf("puppa2\n");
		abort();
	}
	sem_post(&mutex);
	return ret;
}

std::streamsize sync_stream::showmanyc( )
{
	printf("puppa3\n");
	abort();
	return 0;
}

zlib_file_filter::zlib_file_filter():compressed(0),offset(0)
{
}

std::streamsize zlib_file_filter::xsgetn ( char * s, std::streamsize n )
{
	if(!compressed)
		return filebuf::xsgetn(s,n);
	else
	{
		/* run inflate() on input until output buffer not full */
		strm.avail_out = n;
		strm.next_out = (unsigned char*)s;
		inflate(&strm, Z_NO_FLUSH);
		//check if output full and wrap around
		while(strm.avail_out!=0)
		{
			int real_count=filebuf::xsgetn((char*)buffer,4096);
			strm.next_in=buffer;
			strm.avail_in=real_count;
			inflate(&strm, Z_NO_FLUSH);
		}
		offset+=n;
	}
	return n;
}

std::streamsize zlib_file_filter::xsputn ( const char * s, std::streamsize n )
{
	return filebuf::xsputn(s,n);
}

std::streampos zlib_file_filter::seekpos ( std::streampos sp, std::ios_base::openmode which)
{
	printf("puppa1\n");
	abort();
	return 0;
}

std::streampos zlib_file_filter::seekoff ( std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which)
{
	if(!compressed)
		return filebuf::seekoff(off,way,which);
	else
	{
		if(off!=0)
		{
			printf("puppa2\n");
			abort();
		}
		return offset;
	}
}

std::streamsize zlib_file_filter::showmanyc( )
{
	printf("puppa3\n");
	abort();
	return 0;
}

void zlib_file_filter::setCompressed()
{
	compressed=true;
	offset=filebuf::seekoff(0,ios_base::cur,ios_base::in);
	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	int ret = inflateInit(&strm);
	if (ret != Z_OK)
		LOG(LOG_ERROR,"Failed to initialize ZLib");
}
