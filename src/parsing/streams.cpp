/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

