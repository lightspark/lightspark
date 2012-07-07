/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <glibmm/ustring.h>
#include "tiny_string.h"
#include "exceptions.h"
#include "swf.h"

using namespace lightspark;

/* Implementation of Glib::ustring conversion for libxml++.
 * We implement them in the source file to not pollute the header with glib.h
 */
tiny_string::tiny_string(const Glib::ustring& r):buf(_buf_static),stringSize(r.bytes()+1),type(STATIC)
{
	if(stringSize > STATIC_SIZE)
		createBuffer(stringSize);
	memcpy(buf,r.c_str(),stringSize);
}

tiny_string::tiny_string(std::istream& in, int len):buf(_buf_static),stringSize(len+1),type(STATIC)
{
	if(stringSize > STATIC_SIZE)
		createBuffer(stringSize);
	in.read(buf,len);
	buf[len]='\0';
}

tiny_string& tiny_string::operator=(const Glib::ustring& r)
{
	resetToStatic();
	stringSize = r.bytes()+1;
	if(stringSize > STATIC_SIZE)
		createBuffer(stringSize);
	memcpy(buf,r.c_str(),stringSize);
	return *this;
}

tiny_string::operator Glib::ustring() const
{
	return Glib::ustring(buf,numChars());
}

bool tiny_string::operator==(const Glib::ustring& u) const
{
	return *this == u.raw();
}

bool tiny_string::operator!=(const Glib::ustring& u) const
{
	return *this != u.raw();
}

const tiny_string tiny_string::operator+(const Glib::ustring& r) const
{
	return *this + tiny_string(r);
}

tiny_string& tiny_string::operator+=(const Glib::ustring& s)
{
	//TODO: optimize
	return *this += tiny_string(s);
}

tiny_string& tiny_string::operator+=(const char* s)
{	//deprecated, cannot handle '\0' inside string
	if(type==READONLY)
	{
		char* tmp=buf;
		makePrivateCopy(tmp);
	}
	uint32_t addedLen=strlen(s);
	uint32_t newStringSize=stringSize + addedLen;
	if(type==STATIC && newStringSize > STATIC_SIZE)
	{
		createBuffer(newStringSize);
		//don't copy trailing \0
		memcpy(buf,_buf_static,stringSize-1);
	}
	else if(type==DYNAMIC && addedLen!=0)
		resizeBuffer(newStringSize);
	//also copy \0 at the end
	memcpy(buf+stringSize-1,s,addedLen+1);
	stringSize=newStringSize;
	return *this;
}

tiny_string& tiny_string::operator+=(const tiny_string& r)
{
	if(type==READONLY)
	{
		char* tmp=buf;
		makePrivateCopy(tmp);
	}
	uint32_t newStringSize=stringSize + r.stringSize-1;
	if(type==STATIC && newStringSize > STATIC_SIZE)
	{
		createBuffer(newStringSize);
		//don't copy trailing \0
		memcpy(buf,_buf_static,stringSize-1);
	}
	else if(type==DYNAMIC && r.stringSize>1)
		resizeBuffer(newStringSize);
	//start position is where the \0 was
	memcpy(buf+stringSize-1,r.buf,r.stringSize);
	stringSize=newStringSize;
	return *this;
}

const tiny_string tiny_string::operator+(const tiny_string& r) const
{
	tiny_string ret(*this);
	ret+=r;
	return ret;
}

tiny_string& tiny_string::replace(uint32_t pos1, uint32_t n1, const tiny_string& o )
{
	assert(pos1 <= numChars());
	uint32_t bytestart = g_utf8_offset_to_pointer(buf,pos1)-buf;
	if(pos1 + n1 > numChars())
		n1 = numChars()-pos1;
	uint32_t byteend = g_utf8_offset_to_pointer(buf,pos1+n1)-buf;
	return replace_bytes(bytestart, byteend-bytestart, o);
}

tiny_string& tiny_string::replace_bytes(uint32_t bytestart, uint32_t bytenum, const tiny_string& o)
{
	//TODO avoid copy into std::string
	*this = std::string(*this).replace(bytestart,bytenum,std::string(o));
	return *this;
}

tiny_string tiny_string::substr_bytes(uint32_t start, uint32_t len) const
{
	tiny_string ret;
	assert(start+len < stringSize);
	if(len+1 > STATIC_SIZE)
		ret.createBuffer(len+1);
	memcpy(ret.buf,buf+start,len);
	ret.buf[len]=0;
	ret.stringSize = len+1;
	return ret;
}

tiny_string tiny_string::substr(uint32_t start, uint32_t len) const
{
	assert_and_throw(start <= numChars());
	if(start+len > numChars())
		len = numChars()-start;
	uint32_t bytestart = g_utf8_offset_to_pointer(buf,start) - buf;
	uint32_t byteend = g_utf8_offset_to_pointer(buf,start+len) - buf;
	return substr_bytes(bytestart, byteend-bytestart);
}

tiny_string tiny_string::substr(uint32_t start, const CharIterator& end) const
{
	assert_and_throw(start < numChars());
	uint32_t bytestart = g_utf8_offset_to_pointer(buf,start) - buf;
	uint32_t byteend = end.buf_ptr - buf;
	return substr_bytes(bytestart, byteend-bytestart);
}

std::list<tiny_string> tiny_string::split(uint32_t delimiter) const
{
	std::list<tiny_string> res;
	uint32_t pos, end;
	tiny_string delimiterstring = tiny_string::fromChar(delimiter);

	pos = 0;
	while (pos < numChars())
	{
		end = find(delimiterstring, pos);
		if (end == tiny_string::npos)
		{
			res.push_back(substr(pos, numChars()-pos));
			break;
		}
		else
		{
			res.push_back(substr(pos, end-pos));
			pos = end+1;
		}
	}

	return res;
}

#ifdef MEMORY_USAGE_PROFILING
void tiny_string::reportMemoryChange(int32_t change) const
{
	getSys()->stringMemory->addBytes(change);
}
#endif
