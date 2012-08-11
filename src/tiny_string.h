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

#ifndef TINY_STRING_H
#define TINY_STRING_H 1

#include <cstring>
#include <cstdint>
#include <ostream>
#include <list>
/* for utf8 handling */
#include <glib.h>
#include "compat.h"

/* forward declare for tiny_string conversion */
namespace Glib { class ustring; }
typedef unsigned char xmlChar;

namespace lightspark
{

/* Iterates over utf8 characters */
class CharIterator /*: public forward_iterator<uint32_t>*/
{
friend class tiny_string;
private:
	char* buf_ptr;
public:
	CharIterator(char* buf) : buf_ptr(buf) {}
	/* Return the utf8-character value */
	uint32_t operator*() const
	{
		return g_utf8_get_char(buf_ptr);
	}
	CharIterator& operator++() //prefix
	{
		buf_ptr = g_utf8_next_char(buf_ptr);
		return *this;
	}
	CharIterator operator++(int) // postfix
	{
		CharIterator result = *this;
	    ++(*this);
	    return result;
	}
	bool operator==(const CharIterator& o) const
	{
		return buf_ptr == o.buf_ptr;
	}
	bool operator!=(const CharIterator& o) const { return !(*this == o); }
	/* returns true if the current character is a digit */
	bool isdigit() const
	{
		return g_unichar_isdigit(g_utf8_get_char(buf_ptr));
	}
	/* return the value of the current character interpreted as decimal digit,
	 * i.e. '7' -> 7
	 */
	int32_t digit_value() const
	{
		return g_unichar_digit_value(g_utf8_get_char(buf_ptr));
	}
};

/*
 * String class.
 * The string can contain '\0's, so don't use raw_buf().
 * Use len() to determine actual size.
 */
class tiny_string
{
friend std::ostream& operator<<(std::ostream& s, const tiny_string& r);
private:
	enum TYPE { READONLY=0, STATIC, DYNAMIC };
	/*must be at least 6 bytes for tiny_string(uint32_t c) constructor */
	#define STATIC_SIZE 64
	char _buf_static[STATIC_SIZE];
	char* buf;
	/*
	   stringSize includes the trailing \0
	*/
	uint32_t stringSize;
	TYPE type;
#ifdef MEMORY_USAGE_PROFILING
	//Implemented in memory_support.cpp
	DLL_PUBLIC void reportMemoryChange(int32_t change) const;
#else
	//NOP
	void reportMemoryChange(int32_t change) const {}
#endif
	//TODO: use static buffer again if reassigning to short string
	void makePrivateCopy(const char* s)
	{
		resetToStatic();
		stringSize=strlen(s)+1;
		if(stringSize > STATIC_SIZE)
			createBuffer(stringSize);
		strcpy(buf,s);
	}
	void createBuffer(uint32_t s)
	{
		type=DYNAMIC;
		reportMemoryChange(s);
		buf=new char[s];
	}
	void resizeBuffer(uint32_t s)
	{
		assert(type==DYNAMIC);
		char* oldBuf=buf;
		reportMemoryChange(s-stringSize);
		buf=new char[s];
		assert(s >= stringSize);
		memcpy(buf,oldBuf,stringSize);
		delete[] oldBuf;
	}
	void resetToStatic()
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
public:
	static const uint32_t npos = (uint32_t)(-1);

	tiny_string():_buf_static(),buf(_buf_static),stringSize(1),type(STATIC){buf[0]=0;}
	/* construct from utf character */
	static tiny_string fromChar(uint32_t c)
	{
		tiny_string ret;
		ret.buf = ret._buf_static;
		ret.type = STATIC;
		ret.stringSize = g_unichar_to_utf8(c,ret.buf) + 1;
		ret.buf[ret.stringSize-1] = '\0';
		return ret;
	}
	tiny_string(const char* s,bool copy=false):_buf_static(),buf(_buf_static),type(READONLY)
	{
		if(copy)
			makePrivateCopy(s);
		else
		{
			stringSize=strlen(s)+1;
			buf=(char*)s; //This is an unsafe conversion, we have to take care of the RO data
		}
	}
	tiny_string(const tiny_string& r):_buf_static(),buf(_buf_static),stringSize(r.stringSize),type(STATIC)
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
	tiny_string(const std::string& r):_buf_static(),buf(_buf_static),stringSize(r.size()+1),type(STATIC)
	{
		if(stringSize > STATIC_SIZE)
			createBuffer(stringSize);
		memcpy(buf,r.c_str(),stringSize);
	}
	tiny_string(const Glib::ustring& r);
	tiny_string(std::istream& in, int len);
	~tiny_string()
	{
		resetToStatic();
	}
	tiny_string& operator=(const tiny_string& s)
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
	tiny_string& operator=(const std::string& s)
	{
		resetToStatic();
		stringSize=s.size()+1;
		if(stringSize > STATIC_SIZE)
			createBuffer(stringSize);
		memcpy(buf,s.c_str(),stringSize);
		return *this;
	}
	tiny_string& operator=(const char* s)
	{
		makePrivateCopy(s);
		return *this;
	}
	tiny_string& operator=(const Glib::ustring& s);
	tiny_string& operator+=(const char* s);
	tiny_string& operator+=(const tiny_string& r);
	tiny_string& operator+=(const std::string& s)
	{
		//TODO: optimize
		return *this += tiny_string(s);
	}
	tiny_string& operator+=(const Glib::ustring& s);
	tiny_string& operator+=(uint32_t c)
	{
		return (*this += tiny_string::fromChar(c));
	}
	const tiny_string operator+(const tiny_string& r) const;
	const tiny_string operator+(const char* s) const
	{
		return *this + tiny_string(s);
	}
	const tiny_string operator+(const std::string& r) const
	{
		return *this + tiny_string(r);
	}
	const tiny_string operator+(const Glib::ustring& r) const;
	bool operator<(const tiny_string& r) const
	{
		//don't check trailing \0
		return memcmp(buf,r.buf,std::min(stringSize,r.stringSize))<0;
	}
	bool operator>(const tiny_string& r) const
	{
		//don't check trailing \0
		return memcmp(buf,r.buf,std::min(stringSize,r.stringSize))>0;
	}
	bool operator==(const tiny_string& r) const
	{
		//The length is checked as an optimization before checking the contents
		if(stringSize != r.stringSize)
			return false;
		//don't check trailing \0
		return memcmp(buf,r.buf,stringSize-1)==0;
	}
	bool operator==(const std::string& r) const
	{
		//The length is checked as an optimization before checking the contents
		if(stringSize != r.size()+1)
			return false;
		//don't check trailing \0
		return memcmp(buf,r.c_str(),stringSize-1)==0;
	}
	bool operator!=(const std::string& r) const
	{
		if(stringSize != r.size()+1)
			return true;
		//don't check trailing \0
		return memcmp(buf,r.c_str(),stringSize-1)!=0;
	}
	bool operator!=(const tiny_string& r) const
	{
		return !(*this==r);
	}
	bool operator==(const char* r) const
	{
		return strcmp(buf,r)==0;
	}
	bool operator==(const xmlChar* r) const
	{
		return strcmp(buf,reinterpret_cast<const char*>(r))==0;
	}
	bool operator!=(const char* r) const
	{
		return !(*this==r);
	}
	bool operator==(const Glib::ustring&) const;
	bool operator!=(const Glib::ustring&) const;
	const char* raw_buf() const
	{
		return buf;
	}
	bool empty() const
	{
		return stringSize == 1;
	}
	/* returns the length in bytes, not counting the trailing \0 */
	uint32_t numBytes() const
	{
		return stringSize-1;
	}
	/* returns the length in utf-8 characters, not counting the trailing \0 */
	uint32_t numChars() const
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
	/* start and len are indices of utf8-characters */
	tiny_string substr(uint32_t start, uint32_t len) const;
	tiny_string substr(uint32_t start, const CharIterator& end) const;
	/* start and len are indices of bytes */
	tiny_string substr_bytes(uint32_t start, uint32_t len) const;
	/* finds the first occurence of char in the utf-8 string
	 * Return NULL if not found, else ptr to beginning of first occurence of c */
	char* strchr(char c) const
	{
		//TODO: does this handle '\0' in middle of buf gracefully?
		return g_utf8_strchr(buf, numBytes(), c);
	}
	char* strchrr(char c) const
	{
		//TODO: does this handle '\0' in middle of buf gracefully?
		return g_utf8_strrchr(buf, numBytes(), c);
	}
	/*explicit*/ operator std::string() const
	{
		return std::string(buf,stringSize-1);
	}
	operator Glib::ustring() const;
	bool startsWith(const char* o) const
	{
		return strncmp(buf,o,strlen(o)) == 0;
	}
	/* idx is an index of utf-8 characters */
	uint32_t charAt(uint32_t idx) const
	{
		return g_utf8_get_char(g_utf8_offset_to_pointer(buf,idx));
	}
	/* start is an index of characters.
	 * returns index of character */
	uint32_t find(const tiny_string& needle, uint32_t start = 0) const
	{
		//TODO: omit copy into std::string
		size_t bytestart = g_utf8_offset_to_pointer(buf,start) - buf;
		size_t bytepos = std::string(*this).find(needle.raw_buf(),bytestart,needle.numBytes());
		if(bytepos == std::string::npos)
			return npos;
		else
			return g_utf8_pointer_to_offset(buf,buf+bytepos);
	}
	uint32_t rfind(const tiny_string& needle, uint32_t start = npos) const
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
	tiny_string& replace(uint32_t pos1, uint32_t n1, const tiny_string& o);
	tiny_string& replace_bytes(uint32_t bytestart, uint32_t bytenum, const tiny_string& o);
	tiny_string lowercase() const
	{
		//TODO: omit copy, handle \0 in string
		char *strdown = g_utf8_strdown(buf,numBytes());
		tiny_string ret(strdown,/*copy:*/true);
		g_free(strdown);
		return ret;
	}
	tiny_string uppercase() const
	{
		//TODO: omit copy, handle \0 in string
		char *strup = g_utf8_strup(buf,numBytes());
		tiny_string ret(strup,/*copy:*/true);
		g_free(strup);
		return ret;
	}
	/* like strcasecmp(s1.raw_buf(),s2.raw_buf()) but for unicode
	 * TODO: slow! */
	int strcasecmp(tiny_string& s2) const
	{
		char* str1 = g_utf8_casefold(this->raw_buf(),this->numBytes());
		char* str2 = g_utf8_casefold(s2.raw_buf(),s2.numBytes());
		int ret = g_utf8_collate(str1,str2);
		g_free(str1);
		g_free(str2);
		return ret;
	}
	/* split string at each occurrence of delimiter character */
	std::list<tiny_string> split(uint32_t delimiter) const;
	/* iterate over utf8 characters */
	CharIterator begin()
	{
		return CharIterator(buf);
	}
	CharIterator begin() const
	{
		return CharIterator(buf);
	}
	CharIterator end()
	{
		//points to the trailing '\0' byte
		return CharIterator(buf+numBytes());
	}
	CharIterator end() const
	{
		//points to the trailing '\0' byte
		return CharIterator(buf+numBytes());
	}
};

};
#endif /* TINY_STRING_H */
