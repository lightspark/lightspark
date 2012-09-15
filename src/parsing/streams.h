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
	int offset;
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
	unsigned int len;
	unsigned int pos;
	bool read_past_end;
public:
	// Create a stream from a buffer b.
	//
	// The buffer is not copied, so b must continue to exists for
	// the life-time of this memorystream instance.
	memorystream(const char* const b, unsigned int l);
	unsigned int size() const;
	unsigned int tellg() const;
	void seekg(unsigned int offset);
	void read(char *out, unsigned int nbytes);
	bool eof() const;
};

memorystream& lightspark::operator>>(memorystream& in, lightspark::u8& v);
memorystream& lightspark::operator>>(memorystream& in, lightspark::s24& v);
memorystream& lightspark::operator>>(memorystream& in, lightspark::u30& v);
memorystream& lightspark::operator>>(memorystream& in, lightspark::u32& v);

#endif /* PARSING_STREAMS_H */
