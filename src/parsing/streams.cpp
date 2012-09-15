/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "parsing/streams.h"
#include "logger.h"
#include "exceptions.h"
#include "compat.h"
#include "class.h"
#include "toplevel/Error.h"
#include <cstdlib>
#include <cstring>
#include <assert.h>

#define LZMA_PROP_LENGTH 5

using namespace std;


uncompressing_filter::uncompressing_filter(streambuf* b):backend(b),consumed(0),eof(false)
{
}

int uncompressing_filter::underflow()
{
	assert(gptr()==egptr());
	if(eof)
		return -1;

	//First of all we add the length of the buffer to the consumed variable
	consumed+=(gptr()-eback());

	int available=fillBuffer();
	setg(buffer,buffer,buffer+available);
	
	//Cast to unsigned, otherwise 0xff would become eof
	return (unsigned char)buffer[0];
}

streampos uncompressing_filter::seekoff(off_type off, ios_base::seekdir dir,ios_base::openmode mode)
{
	assert(off==0);
	assert(dir==ios_base::cur);
	//The current offset is the amount of byte completely consumed plus the amount used in the buffer
	int ret=consumed+(gptr()-eback());
	return ret;
}

zlib_filter::zlib_filter(streambuf* b):uncompressing_filter(b)
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

int zlib_filter::fillBuffer()
{
	strm.avail_out = sizeof(buffer);
	strm.next_out = (unsigned char*)buffer;
	do
	{
		if(strm.avail_in==0)
		{
			int real_count=backend->sgetn(compressed_buffer,sizeof(compressed_buffer));
			if(real_count==0)
			{
				//File is not big enough
				throw lightspark::ParseException("Unexpected end of file");
			}
			strm.next_in=(unsigned char*)compressed_buffer;
			strm.avail_in=real_count;
		}
		int ret=inflate(&strm, Z_NO_FLUSH);
		if(ret==Z_STREAM_END)
		{
			//The stream ended, close the buffer here
			eof=true;
			break;
		}
		else if (ret!=Z_OK)
			throw lightspark::ParseException("Unexpected Zlib error");
	}
	while(strm.avail_out!=0);

	return sizeof(buffer) - strm.avail_out;
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

liblzma_filter::liblzma_filter(streambuf* b):uncompressing_filter(b)
{
	strm = LZMA_STREAM_INIT;
	lzma_ret ret = lzma_alone_decoder(&strm, UINT64_MAX);
	if (ret != LZMA_OK)
		throw lightspark::RunTimeException("Failed to initialize lzma decoder");
	setg((char*)buffer, (char*)buffer, (char*)buffer);
	consumed+=pubseekoff(0, ios_base::cur, ios_base::in);

	// First 32 bit uint is the compressed file length, which we
	// ignore
	streamsize nbytes=backend->sgetn((char *)compressed_buffer,sizeof(uint32_t));
	if (nbytes != sizeof(uint32_t))
		throw lightspark::ParseException("Unexpected end of file");

	/* SWF uses a deprecated (pre-5.0?) LZMA SDK stream format,
	 * which differs slightly from the newer liblzma's lzma_alone
	 * format. The both begin with 5 byte property data. The
	 * lzma_alone continues with a 64 bit int uncompressed file
	 * length variable, which is missing from the older format.
	 * And then both contain the compressed LZMA data, which ends
	 * with a LZMA end-of-payload marker.
	 *
	 * To process the stream with liblzma, we append the length
	 * variable here manually. A special value 0xFFFF FFFF FFFF
	 * FFFF means that the uncompressed length is unknown.
	 */
	assert(LZMA_PROP_LENGTH + sizeof(int64_t) < sizeof(compressed_buffer));
	nbytes=backend->sgetn((char *)compressed_buffer,LZMA_PROP_LENGTH);
	if (nbytes != LZMA_PROP_LENGTH)
		throw lightspark::ParseException("Unexpected end of file");
	for (unsigned int i=0; i<sizeof(int64_t); i++)
		compressed_buffer[LZMA_PROP_LENGTH + i] = 0xFF;

	strm.next_in = compressed_buffer;
	strm.avail_in = LZMA_PROP_LENGTH + sizeof(int64_t);
}

liblzma_filter::~liblzma_filter()
{
	lzma_end(&strm);
}

int liblzma_filter::fillBuffer()
{
	strm.avail_out = sizeof(buffer);
	strm.next_out = (uint8_t *)buffer;
	do
	{
		if(strm.avail_in==0)
		{
			int real_count=backend->sgetn((char *)compressed_buffer,sizeof(compressed_buffer));
			if(real_count==0)
			{
				//File is not big enough
				throw lightspark::ParseException("Unexpected end of file");
			}
			strm.next_in=compressed_buffer;
			strm.avail_in=real_count;
		}

		lzma_ret ret=lzma_code(&strm, LZMA_RUN);

		if(ret==LZMA_STREAM_END)
		{
			//The stream ended, close the buffer here
			eof=true;
			break;
		}
		else if(ret!=LZMA_OK)
		{
			char errmsg[64];
			snprintf(errmsg, 64, "lzma decoder error %d", (int)ret);
			throw lightspark::ParseException(errmsg);
		}
	}
	while(strm.avail_out!=0);

	return sizeof(buffer) - strm.avail_out;
}

memorystream::memorystream(const char* const b, unsigned int l)
  : code(b), len(l), pos(0), read_past_end(false)
{
}

unsigned int memorystream::tellg() const
{
	return pos;
}

void memorystream::seekg(unsigned int offset)
{
	if (offset > len)
		pos = len;
	else
		pos = offset;
}

unsigned int memorystream::size() const
{
	return len;
}

void memorystream::read(char *out, unsigned int nbytes)
{
	if ((nbytes == 1) && (pos+1 > len))
	{
		// fastpath for one bytes reads
		*out = code[pos];
		pos++;
	}
	else if (pos+nbytes > len)
	{
		memcpy(out, code+pos, len-pos);
		pos = len;
		read_past_end = true;
	}
	else
	{
		memcpy(out, code+pos, nbytes);
		pos += nbytes;
	}
}

bool memorystream::eof() const
{
	return read_past_end;
}

memorystream& lightspark::operator>>(memorystream& in, lightspark::u8& v)
{
	uint8_t t;
	in.read((char*)&t,1);
	v.val= t;
	return in;
}

memorystream& lightspark::operator>>(memorystream& in, lightspark::s24& v)
{
	uint32_t ret=0;
	in.read((char*)&ret,3);
	v.val=LittleEndianToSignedHost24(ret);
	return in;
}

memorystream& lightspark::operator>>(memorystream& in, lightspark::u30& v)
{
	lightspark::u32 vv;
	in >> vv;
	uint32_t val = vv;
	if(val&0xc0000000)
		throw lightspark::Class<lightspark::VerifyError>::getInstanceS("Invalid u30");
	v.val = val;
	return in;
}

memorystream& lightspark::operator>>(memorystream& in, lightspark::u32& v)
{
	int i=0;
	uint32_t val=0;
	uint8_t t;
	do
	{
		in.read((char*)&t,1);
		//No more than 5 bytes should be read
		if(i==28)
		{
			//Only the first 4 bits should be used to reach 32 bits
			if((t&0xf0))
				LOG(LOG_ERROR,"Error in u32");
			uint8_t t2=(t&0xf);
			val|=(t2<<i);
			break;
		}
		else
		{
			uint8_t t2=(t&0x7f);
			val|=(t2<<i);
			i+=7;
		}
	}
	while(t&0x80);
	v.val=val;
	return in;
}
