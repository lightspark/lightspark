/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef PARSING_STREAMS_H
#define PARSING_STREAMS_H 1

#include "compat.h"
#include "abctypes.h"
#include "swftypes.h"
#include <streambuf>
#include <fstream>
#include <cinttypes>
#include <zlib.h>
#include <lzma.h>

class uncompressing_filter: public std::streambuf
{
protected:
	static const unsigned int BUFFER_LENGTH = 4096;

	// The compressed input data stream
	std::streambuf* backend;
	// Current uncompressed bytes (accessible input sequence in
	// std::streambuf terminology)
	char buffer[BUFFER_LENGTH];
	// Total number of uncompressed read bytes not including the
	// bytes read from the current buffer.
	int consumed;
	bool eof;
	virtual int underflow();
	virtual std::streampos seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
	// Abstract function that fills buffer by uncompressing bytes
	// from backend. buffer is empty when this is called. Returns
	// number of bytes written to buffer.
	virtual int fillBuffer()=0;
public:
	uncompressing_filter(std::streambuf* b);
};


class zlib_filter: public uncompressing_filter
{
private:
	z_stream strm;
	// Temporary buffer for data before it is uncompressed
	char compressed_buffer[BUFFER_LENGTH];
protected:
	virtual int fillBuffer();
public:
	zlib_filter(std::streambuf* b);
	~zlib_filter();
};

class liblzma_filter: public uncompressing_filter
{
private:
	lzma_stream strm;
	// Temporary buffer for data before it is uncompressed
	uint8_t compressed_buffer[BUFFER_LENGTH];
protected:
	virtual int fillBuffer();
public:
	liblzma_filter(std::streambuf* b);
	~liblzma_filter();
};

class bytes_buf:public std::streambuf
{
private:
	const uint8_t* buf;
	int len;
public:
	bytes_buf(const uint8_t* b, int l);
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
};

// A lightweight, istream-like interface for reading from a memory
// buffer.
// 
// This is used in the interpreter for reading bytecode because this
// is faster than istringstream.
class memorystream
{
private:
	const char* const code;
	const char* lastcodepos;
	const char* codepos;
public:
	// Create a stream from a buffer b.
	//
	// The buffer is not copied, so b must continue to exists for
	// the life-time of this memorystream instance.
	memorystream(const char* const b, unsigned int l): code(b), lastcodepos(code+l), codepos(code) {}
	static void handleError(const char *msg);
	inline unsigned int size() const
	{
		return lastcodepos-code;
	}
	
	inline unsigned int tellg() const
	{
		return codepos-code;
	}
	
	inline bool atend() const
	{
		return codepos == lastcodepos;
	}

	inline void seekg(unsigned int offset)
	{
		if (offset >= (unsigned int)(lastcodepos-code))
			codepos = lastcodepos;
		else
			codepos = code+offset;
	}

	inline void read(char *out, unsigned int nbytes)
	{
		if (codepos+nbytes >= lastcodepos)
		{
			memcpy(out, codepos, lastcodepos-codepos);
			codepos = lastcodepos;
		}
		else
		{
			memcpy(out, codepos, nbytes);
			codepos += nbytes;
		}
	}
	
	inline uint8_t readbyte()
	{
		if (codepos < lastcodepos)
		{
			return *codepos++;
		}
		else
		{
			return 0;
		}
	}
	inline uint8_t peekbyte()
	{
		if (codepos < lastcodepos)
		{
			return *(codepos);
		}
		else
		{
			return 0;
		}
	}
	inline uint8_t peekbyteFromPosition(uint32_t pos)
	{
		const char* codepos_peek = code+pos;
		if (codepos_peek < lastcodepos)
		{
			return *(codepos_peek);
		}
		else
		{
			return 0;
		}
	}
	inline uint32_t skipu30FromPosition(uint32_t pos)
	{
		const char* codepos_prev = codepos;
		codepos = code+pos;
		readu32();
		uint32_t codepos_new = codepos-code;
		codepos = codepos_prev;
		return codepos_new;
	}
	inline uint32_t peeku30FromPosition(uint32_t pos)
	{
		const char* codepos_prev = codepos;
		codepos = code+pos;
		uint32_t val = readu32();
		if(val&0xc0000000)
			memorystream::handleError("Invalid u30");
		codepos = codepos_prev;
		return val;
	}
	inline uint32_t readu30()
	{
		uint32_t val = readu32();
		if(val&0xc0000000)
			memorystream::handleError("Invalid u30");
		return val;
	}
	inline uint32_t readu32()
	{
		int i=0;
		uint32_t val=0;
		uint8_t t;
		do
		{
			t = readbyte();
			//No more than 5 bytes should be read
			if(i==28)
			{
				//Only the first 4 bits should be used to reach 32 bits
				if((t&0xf0))
					LOG(LOG_ERROR,"Error in u32");
				val|=((t&0xf)<<i);
				break;
			}
			else
			{
				val|=((t&0x7f)<<i);
				i+=7;
			}
		}
		while(t&0x80);
		return val;
	}
	inline int32_t peeks24FromPosition(uint32_t pos)
	{
		const char* codepos_prev = codepos;
		codepos = code+pos;
		int32_t val = reads24();
		codepos = codepos_prev;
		return val;
	}
	inline int32_t reads24()
	{
		uint32_t val=0;
		read((char*)&val,3);
		int32_t ret = LittleEndianToSignedHost24(val);
		return ret;
	}
};
#endif /* PARSING_STREAMS_H */
