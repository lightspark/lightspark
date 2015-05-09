/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

tiny_string::tiny_string(const char* s,bool copy):_buf_static(),buf(_buf_static),type(READONLY)
{
	if(copy)
		makePrivateCopy(s);
	else
	{
		stringSize=strlen(s)+1;
		buf=(char*)s; //This is an unsafe conversion, we have to take care of the RO data
	}
}

tiny_string::tiny_string(const tiny_string& r):_buf_static(),buf(_buf_static),stringSize(r.stringSize),type(STATIC)
{
	//Fast path for static read-only strings
	if(r.type==READONLY)
	{
		type=READONLY;
		buf=r.buf;
		return;
	}
	if(stringSize > STATIC_SIZE)
		createBuffer(stringSize);
	memcpy(buf,r.buf,stringSize);
}

tiny_string::tiny_string(const std::string& r):_buf_static(),buf(_buf_static),stringSize(r.size()+1),type(STATIC)
{
	if(stringSize > STATIC_SIZE)
		createBuffer(stringSize);
	memcpy(buf,r.c_str(),stringSize);
}

tiny_string::~tiny_string()
{
	resetToStatic();
}

tiny_string& tiny_string::operator=(const tiny_string& s)
{
	resetToStatic();
	stringSize=s.stringSize;
	//Fast path for static read-only strings
	if(s.type==READONLY)
	{
		type=READONLY;
		buf=s.buf;
	}
	else
	{
		if(stringSize > STATIC_SIZE)
			createBuffer(stringSize);
		memcpy(buf,s.buf,stringSize);
	}
	return *this;
}

tiny_string& tiny_string::operator=(const std::string& s)
{
	resetToStatic();
	stringSize=s.size()+1;
	if(stringSize > STATIC_SIZE)
		createBuffer(stringSize);
	memcpy(buf,s.c_str(),stringSize);
	return *this;
}

tiny_string& tiny_string::operator=(const char* s)
{
	makePrivateCopy(s);
	return *this;
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

tiny_string& tiny_string::operator+=(const std::string& s)
{
	//TODO: optimize
	return *this += tiny_string(s);
}

tiny_string& tiny_string::operator+=(uint32_t c)
{
	return (*this += tiny_string::fromChar(c));
}

const tiny_string tiny_string::operator+(const tiny_string& r) const
{
	tiny_string ret(*this);
	ret+=r;
	return ret;
}

const tiny_string tiny_string::operator+(const char* s) const
{
	return *this + tiny_string(s);
}

const tiny_string tiny_string::operator+(const std::string& r) const
{
	return *this + tiny_string(r);
}

bool tiny_string::operator<(const tiny_string& r) const
{
	//don't check trailing \0
	return memcmp(buf,r.buf,std::min(stringSize,r.stringSize))<0;
}

bool tiny_string::operator>(const tiny_string& r) const
{
	//don't check trailing \0
	return memcmp(buf,r.buf,std::min(stringSize,r.stringSize))>0;
}

bool tiny_string::operator==(const tiny_string& r) const
{
	//The length is checked as an optimization before checking the contents
	if(stringSize != r.stringSize)
		return false;
	//don't check trailing \0
	return memcmp(buf,r.buf,stringSize-1)==0;
}

bool tiny_string::operator==(const std::string& r) const
{
	//The length is checked as an optimization before checking the contents
	if(stringSize != r.size()+1)
		return false;
	//don't check trailing \0
	return memcmp(buf,r.c_str(),stringSize-1)==0;
}

bool tiny_string::operator!=(const std::string& r) const
{
	if(stringSize != r.size()+1)
		return true;
	//don't check trailing \0
	return memcmp(buf,r.c_str(),stringSize-1)!=0;
}

bool tiny_string::operator!=(const tiny_string& r) const
{
	return !(*this==r);
}

bool tiny_string::operator==(const char* r) const
{
	return strcmp(buf,r)==0;
}

bool tiny_string::operator==(const xmlChar* r) const
{
	return strcmp(buf,reinterpret_cast<const char*>(r))==0;
}

bool tiny_string::operator!=(const char* r) const
{
	return !(*this==r);
}

const char* tiny_string::raw_buf() const
{
	return buf;
}

bool tiny_string::empty() const
{
	return stringSize == 1;
}

/* returns the length in bytes, not counting the trailing \0 */
uint32_t tiny_string::numBytes() const
{
	return stringSize-1;
}

/* returns the length in utf-8 characters, not counting the trailing \0 */
uint32_t tiny_string::numChars() const
{
	//we cannot use g_utf8_strlen, as we may have '\0' inside our string
	uint32_t len = 0;
	char* end = buf+numBytes();
	char* p = buf;
	while(p < end)
	{
		p = g_utf8_next_char(p);
		++len;
	}
	return len;
}

char* tiny_string::strchr(char c) const
{
	//TODO: does this handle '\0' in middle of buf gracefully?
	return g_utf8_strchr(buf, numBytes(), c);
}

char* tiny_string::strchrr(char c) const
{
	//TODO: does this handle '\0' in middle of buf gracefully?
	return g_utf8_strrchr(buf, numBytes(), c);
}

tiny_string::operator std::string() const
{
	return std::string(buf,stringSize-1);
}

bool tiny_string::startsWith(const char* o) const
{
	return strncmp(buf,o,strlen(o)) == 0;
}
bool tiny_string::endsWith(const char* o) const
{
	size_t olen = strlen(o);
	return (numBytes() >= olen) && 
		(strncmp(buf+numBytes()-olen,o,olen) == 0);
}

/* idx is an index of utf-8 characters */
uint32_t tiny_string::charAt(uint32_t idx) const
{
	return g_utf8_get_char(g_utf8_offset_to_pointer(buf,idx));
}

/* start is an index of characters.
 * returns index of character */
uint32_t tiny_string::find(const tiny_string& needle, uint32_t start) const
{
	//TODO: omit copy into std::string
	size_t bytestart = g_utf8_offset_to_pointer(buf,start) - buf;
	size_t bytepos = std::string(*this).find(needle.raw_buf(),bytestart,needle.numBytes());
	if(bytepos == std::string::npos)
		return npos;
	else
		return g_utf8_pointer_to_offset(buf,buf+bytepos);
}

uint32_t tiny_string::rfind(const tiny_string& needle, uint32_t start) const
{
	//TODO: omit copy into std::string
	size_t bytestart;
	if(start == npos)
		bytestart = std::string::npos;
	else
		bytestart = g_utf8_offset_to_pointer(buf,start) - buf;

	size_t bytepos = std::string(*this).rfind(needle.raw_buf(),bytestart,needle.numBytes());
	if(bytepos == std::string::npos)
		return npos;
	else
		return g_utf8_pointer_to_offset(buf,buf+bytepos);
}

void tiny_string::makePrivateCopy(const char* s)
{
	resetToStatic();
	stringSize=strlen(s)+1;
	if(stringSize > STATIC_SIZE)
		createBuffer(stringSize);
	strcpy(buf,s);
}

void tiny_string::createBuffer(uint32_t s)
{
	type=DYNAMIC;
	reportMemoryChange(s);
	buf=new char[s];
}

void tiny_string::resizeBuffer(uint32_t s)
{
	assert(type==DYNAMIC);
	char* oldBuf=buf;
	reportMemoryChange(s-stringSize);
	buf=new char[s];
	assert(s >= stringSize);
	memcpy(buf,oldBuf,stringSize);
	delete[] oldBuf;
}

void tiny_string::resetToStatic()
{
	if(type==DYNAMIC)
	{
		reportMemoryChange(-stringSize);
		delete[] buf;
	}
	stringSize=1;
	_buf_static[0] = '\0';
	buf=_buf_static;
	type=STATIC;
}

tiny_string tiny_string::fromChar(uint32_t c)
{
	tiny_string ret;
	ret.buf = ret._buf_static;
	ret.type = STATIC;
	ret.stringSize = g_unichar_to_utf8(c,ret.buf) + 1;
	ret.buf[ret.stringSize-1] = '\0';
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

tiny_string tiny_string::lowercase() const
{
	// have to loop manually, because g_utf8_strdown doesn't
	// handle nul-chars
	tiny_string ret;
	uint32_t allocated = 2*numBytes()+7;
	ret.createBuffer(allocated);
	char *p = ret.buf;
	uint32_t len = 0;
	for (CharIterator it=begin(); it!=end(); it++)
	{
		assert(pend-p >= 6);
		gunichar c = g_unichar_tolower(*it);
		gint n = g_unichar_to_utf8(c, p);
		p += n;
		len += n;
	}
	*p = '\0';
	ret.stringSize = len+1;
	return ret;
}

tiny_string tiny_string::uppercase() const
{
	// have to loop manually, because g_utf8_strup doesn't
	// handle nul-chars
	tiny_string ret;
	uint32_t allocated = 2*numBytes()+7;
	ret.createBuffer(allocated);
	char *p = ret.buf;
	uint32_t len = 0;
	for (CharIterator it=begin(); it!=end(); it++)
	{
		assert(pend-p >= 6);
		gunichar c = g_unichar_toupper(*it);
		gint n = g_unichar_to_utf8(c, p);
		p += n;
		len += n;
	}
	*p = '\0';
	ret.stringSize = len+1;
	return ret;
}

/* like strcasecmp(s1.raw_buf(),s2.raw_buf()) but for unicode
 * TODO: slow! */
int tiny_string::strcasecmp(tiny_string& s2) const
{
	char* str1 = g_utf8_casefold(this->raw_buf(),this->numBytes());
	char* str2 = g_utf8_casefold(s2.raw_buf(),s2.numBytes());
	int ret = g_utf8_collate(str1,str2);
	g_free(str1);
	g_free(str2);
	return ret;
}

uint32_t tiny_string::bytePosToIndex(uint32_t bytepos) const
{
	if (bytepos >= numBytes())
		return numChars();

	return g_utf8_pointer_to_offset(raw_buf(), raw_buf() + bytepos);
}

CharIterator tiny_string::begin()
{
	return CharIterator(buf);
}

CharIterator tiny_string::begin() const
{
	return CharIterator(buf);
}

CharIterator tiny_string::end()
{
	//points to the trailing '\0' byte
	return CharIterator(buf+numBytes());
}

CharIterator tiny_string::end() const
{
	//points to the trailing '\0' byte
	return CharIterator(buf+numBytes());
}

int tiny_string::compare(const tiny_string& r) const
{
	int l = std::min(stringSize,r.stringSize);
	int res = 0;
	for(int i=0;i < l-1;i++)
	{
		res = (int)buf[i] - (int)r.buf[i];
		if (res != 0)
			return res;
	}
	if (stringSize > r.stringSize)
		res = 1;
	else if (stringSize < r.stringSize)
		res = -1;
	return res;
}

#ifdef MEMORY_USAGE_PROFILING
void tiny_string::reportMemoryChange(int32_t change) const
{
	getSys()->stringMemory->addBytes(change);
}
#endif
