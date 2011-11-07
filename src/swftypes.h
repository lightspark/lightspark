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

#ifndef SWFTYPES_H
#define SWFTYPES_H

#include "compat.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <list>

#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "exceptions.h"
#ifndef WIN32
 // TODO: Proper CMake check
 #include <arpa/inet.h>
#endif
#include <endian.h>

#ifdef BIG_ENDIAN
#include <algorithm>
#endif

/* for utf8 handling */
#include <glib.h>

namespace lightspark
{

enum SWFOBJECT_TYPE { T_OBJECT=0, T_INTEGER=1, T_NUMBER=2, T_FUNCTION=3, T_UNDEFINED=4, T_NULL=5, T_STRING=6, 
	T_DEFINABLE=7, T_BOOLEAN=8, T_ARRAY=9, T_CLASS=10, T_QNAME=11, T_NAMESPACE=12, T_UINTEGER=13, T_PROXY=14, T_TEMPLATE=15};

enum STACK_TYPE{STACK_NONE=0,STACK_OBJECT,STACK_INT,STACK_UINT,STACK_NUMBER,STACK_BOOLEAN};
inline std::ostream& operator<<(std::ostream& s, const STACK_TYPE& st)
{
	switch(st)
	{
	case STACK_NONE:
		s << "none";
		break;
	case STACK_OBJECT:
		s << "object";
		break;
	case STACK_INT:
		s << "int";
		break;
	case STACK_UINT:
		s << "uint";
		break;
	case STACK_NUMBER:
		s << "number";
		break;
	case STACK_BOOLEAN:
		s << "boolean";
		break;
	default:
		assert(false);
	}
	return s;
}

enum TRISTATE { TFALSE=0, TTRUE, TUNDEFINED };

enum FILE_TYPE { FT_UNKNOWN=0, FT_SWF, FT_COMPRESSED_SWF, FT_PNG, FT_JPEG, FT_GIF };

typedef double number_t;

class ASObject;
class Bitmap;

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
		buf=new char[s];
	}
	void resizeBuffer(uint32_t s)
	{
		assert(type==DYNAMIC);
		char* oldBuf=buf;
		buf=new char[s];
		assert(s >= stringSize);
		memcpy(buf,oldBuf,stringSize);
		delete[] oldBuf;
	}
	void resetToStatic()
	{
		if(type==DYNAMIC)
			delete[] buf;
		stringSize=1;
		buf=_buf_static;
		buf[0] = '\0';
		type=STATIC;
	}
public:
	static const uint32_t npos = (uint32_t)(-1);

	tiny_string():buf(_buf_static),stringSize(1),type(STATIC){buf[0]=0;}
	/* construct from utf character */
	tiny_string(uint32_t c):buf(_buf_static),type(STATIC)
	{
		stringSize = g_unichar_to_utf8(c,buf) + 1;
		buf[stringSize-1] = '\0';
	}
	tiny_string(const char* s,bool copy=false):buf(_buf_static),type(READONLY)
	{
		if(copy)
			makePrivateCopy(s);
		else
		{
			stringSize=strlen(s)+1;
			buf=(char*)s; //This is an unsafe conversion, we have to take care of the RO data
		}
	}
	tiny_string(const tiny_string& r):buf(_buf_static),stringSize(r.stringSize),type(STATIC)
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
	tiny_string(const std::string& r):buf(_buf_static),stringSize(r.size()+1),type(STATIC)
	{
		if(stringSize > STATIC_SIZE)
			createBuffer(stringSize);
		memcpy(buf,r.c_str(),stringSize);
	}
	~tiny_string()
	{
		resetToStatic();
	}
	static tiny_string fromInt(int i)
	{
		tiny_string t;
		t.stringSize=snprintf(t.buf,STATIC_SIZE,"%i",i)+1;
		return t;
	}
	static tiny_string fromNumber(number_t d)
	{
		tiny_string t;
		t.stringSize=snprintf(t.buf,STATIC_SIZE,"%g",d)+1;
		return t;
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
	tiny_string& operator+=(const char* s);
	tiny_string& operator+=(const tiny_string& r);
	tiny_string& operator+=(uint32_t c)
	{
		return (*this += tiny_string(c));
	}
	const tiny_string operator+(const tiny_string& r) const;
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
	bool operator!=(const tiny_string& r) const
	{
		return !(*this==r);
	}
	bool operator==(const char* r) const
	{
		return strcmp(buf,r)==0;
	}
	bool operator!=(const char* r) const
	{
		return !(*this==r);
	}
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
	tiny_string lowercase() const
	{
		//TODO: omit copy, handle \0 in string
		char *strdown = g_utf8_strdown(buf,numBytes());
		tiny_string ret(strdown,/*copy:*/true);
		free(strdown);
		return ret;
	}
	tiny_string uppercase() const
	{
		//TODO: omit copy, handle \0 in string
		char *strup = g_utf8_strup(buf,numBytes());
		tiny_string ret(strup,/*copy:*/true);
		free(strup);
		return ret;
	}
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

class QName
{
public:
	tiny_string ns;
	tiny_string name;
	QName(const tiny_string& _name, const tiny_string& _ns):ns(_ns),name(_name){}
	bool operator<(const QName& r) const
	{
		if(ns==r.ns)
			return name<r.name;
		else
			return ns<r.ns;
	}
	tiny_string getQualifiedName() const;
};

class UI8 
{
friend std::istream& operator>>(std::istream& s, UI8& v);
private:
	uint8_t val;
public:
	UI8():val(0){}
	UI8(uint8_t v):val(v){}
	operator uint8_t() const { return val; }
};

class UI16_SWF
{
friend std::istream& operator>>(std::istream& s, UI16_SWF& v);
protected:
	uint16_t val;
public:
	UI16_SWF():val(0){}
	UI16_SWF(uint16_t v):val(v){}
	operator uint16_t() const { return val; }
};

class UI16_FLV
{
friend std::istream& operator>>(std::istream& s, UI16_FLV& v);
protected:
	uint16_t val;
public:
	UI16_FLV():val(0){}
	UI16_FLV(uint16_t v):val(v){}
	operator uint16_t() const { return val; }
};

class SI16_SWF
{
friend std::istream& operator>>(std::istream& s, SI16_SWF& v);
protected:
	int16_t val;
public:
	SI16_SWF():val(0){}
	SI16_SWF(int16_t v):val(v){}
	operator int16_t() const { return val; }
};

class SI16_FLV
{
friend std::istream& operator>>(std::istream& s, SI16_FLV& v);
protected:
	int16_t val;
public:
	SI16_FLV():val(0){}
	SI16_FLV(int16_t v):val(v){}
	operator int16_t(){ return val; }
};

class UI24_SWF
{
friend std::istream& operator>>(std::istream& s, UI24_SWF& v);
protected:
	uint32_t val;
public:
	UI24_SWF():val(0){}
	operator uint32_t() const { return val; }
};

class UI24_FLV
{
friend std::istream& operator>>(std::istream& s, UI24_FLV& v);
protected:
	uint32_t val;
public:
	UI24_FLV():val(0){}
	operator uint32_t() const { return val; }
};

class SI24_SWF
{
friend std::istream& operator>>(std::istream& s, SI24_SWF& v);
protected:
	int32_t val;
public:
	SI24_SWF():val(0){}
	operator int32_t() const { return val; }
};

class SI24_FLV
{
friend std::istream& operator>>(std::istream& s, SI24_FLV& v);
protected:
	int32_t val;
public:
	SI24_FLV():val(0){}
	operator int32_t() const { return val; }
};

class UI32_SWF
{
friend std::istream& operator>>(std::istream& s, UI32_SWF& v);
protected:
	uint32_t val;
public:
	UI32_SWF():val(0){}
	UI32_SWF(uint32_t v):val(v){}
	operator uint32_t() const{ return val; }
};

class UI32_FLV
{
friend std::istream& operator>>(std::istream& s, UI32_FLV& v);
protected:
	uint32_t val;
public:
	UI32_FLV():val(0){}
	UI32_FLV(uint32_t v):val(v){}
	operator uint32_t() const{ return val; }
};

class u32
{
friend std::istream& operator>>(std::istream& in, u32& v);
private:
	uint32_t val;
public:
	operator uint32_t() const{return val;}
};

class STRING
{
friend std::ostream& operator<<(std::ostream& s, const STRING& r);
friend std::istream& operator>>(std::istream& stream, STRING& v);
friend class ASString;
private:
	std::string String;
public:
	STRING():String(){};
	STRING(const char* s):String(s)
	{
	}
	bool operator==(const STRING& s)
	{
		if(String.size()!=s.String.size())
			return false;
		for(uint32_t i=0;i<String.size();i++)
		{
			if(String[i]!=s.String[i])
				return false;
		}
		return true;
	}
	/*STRING operator+(const STRING& s)
	{
		STRING ret(*this);
		for(unsigned int i=0;i<s.String.size();i++)
			ret.String.push_back(s.String[i]);
		return ret;
	}*/
	bool isNull() const
	{
		return !String.size();
	}
	operator const std::string&() const
	{
		return String;
	}
	operator const tiny_string() const
	{
		return tiny_string(String);
	}
	operator const char*() const
	{
		return String.c_str();
	}
	int size()
	{
		return String.size();
	}
};

//Numbers taken from AVM2 specs
enum NS_KIND { NAMESPACE=0x08, PACKAGE_NAMESPACE=0x16, PACKAGE_INTERNAL_NAMESPACE=0x17, PROTECTED_NAMESPACE=0x18, 
			EXPLICIT_NAMESPACE=0x19, STATIC_PROTECTED_NAMESPACE=0x1A, PRIVATE_NAMESPACE=0x05 };

struct nsNameAndKind
{
	tiny_string name;
	NS_KIND kind;
	nsNameAndKind(const tiny_string& _name, NS_KIND _kind):name(_name),kind(_kind){}
	nsNameAndKind(const char* _name, NS_KIND _kind):name(_name),kind(_kind){}
	bool operator<(const nsNameAndKind& r) const
	{
		return name < r.name;
	}
	bool operator>(const nsNameAndKind& r) const
	{
		return name > r.name;
	}
	bool operator==(const nsNameAndKind& r) const
	{
		return /*kind==r.kind &&*/ name==r.name;
	}
};

struct multiname
{
	enum NAME_TYPE {NAME_STRING,NAME_INT,NAME_NUMBER,NAME_OBJECT};
	NAME_TYPE name_type;
	tiny_string name_s;
	union
	{
		int32_t name_i;
		number_t name_d;
		ASObject* name_o;
	};
	std::vector<nsNameAndKind> ns;
	bool isAttribute;
	/*
		Returns a string name whatever is the name type
	*/
	tiny_string normalizedName() const;
	tiny_string qualifiedString() const;
	/* sets name_type, name_s/name_d based on the object n */
	void setName(ASObject* n);
	bool isQName() const { return ns.size() == 1; }
};

class FLOAT 
{
friend std::istream& operator>>(std::istream& s, FLOAT& v);
private:
	float val;
public:
	FLOAT():val(0){}
	FLOAT(float v):val(v){}
	operator float(){ return val; }
};

class DOUBLE 
{
friend std::istream& operator>>(std::istream& s, DOUBLE& v);
private:
	double val;
public:
	DOUBLE():val(0){}
	DOUBLE(double v):val(v){}
	operator double(){ return val; }
};

//TODO: Really implement or suppress
typedef UI32_SWF FIXED;

//TODO: Really implement or suppress
typedef UI16_SWF FIXED8;

class RECORDHEADER
{
friend std::istream& operator>>(std::istream& s, RECORDHEADER& v);
private:
	UI16_SWF CodeAndLen;
	UI32_SWF Length;
public:
	unsigned int getLength() const
	{
		if((CodeAndLen&0x3f)==0x3f)
			return Length;
		else
			return CodeAndLen&0x3f;
	}
	unsigned int getTagType() const
	{
		return CodeAndLen>>6;
	}
};

class RGB
{
public:
	RGB(){};
	RGB(int r,int g, int b):Red(r),Green(g),Blue(b){};
	RGB(uint color):Red((color>>16)&0xFF),Green((color>>8)&0xFF),Blue(color&0xFF){}
	UI8 Red;
	UI8 Green;
	UI8 Blue;
	uint32_t toUInt() const { return Blue + (Green<<8) + (Red<<16); }
};

class RGBA
{
public:
	RGBA():Red(0),Green(0),Blue(0),Alpha(255){}
	RGBA(int r, int g, int b, int a):Red(r),Green(g),Blue(b),Alpha(a){}
	RGBA(uint color, int a):Red((color>>16)&0xFF),Green((color>>8)&0xFF),Blue(color&0xFF),Alpha(a){}
	UI8 Red;
	UI8 Green;
	UI8 Blue;
	UI8 Alpha;
	RGBA& operator=(const RGB& r)
	{
		Red=r.Red;
		Green=r.Green;
		Blue=r.Blue;
		Alpha=255;
		return *this;
	}
	float rf() const
	{
		float ret=Red;
		ret/=255;
		return ret;
	}
	float gf() const
	{
		float ret=Green;
		ret/=255;
		return ret;
	}
	float bf() const
	{
		float ret=Blue;
		ret/=255;
		return ret;
	}
	float af() const
	{
		float ret=Alpha;
		ret/=255;
		return ret;
	}
};

typedef UI8 LANGCODE;

std::istream& operator>>(std::istream& s, RGB& v);

inline std::istream& operator>>(std::istream& s, UI8& v)
{
	s.read((char*)&v.val,1);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI16_SWF& v)
{
	s.read((char*)&v.val,2);
	v.val=LittleEndianToHost16(v.val);
	return s;
}

inline std::istream & operator>>(std::istream &s, SI16_FLV& v)
{
	s.read((char*)&v.val,2);
	v.val=BigEndianToHost16(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16_SWF& v)
{
	s.read((char*)&v.val,2);
	v.val=LittleEndianToHost16(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16_FLV& v)
{
	s.read((char*)&v.val,2);
	v.val=BigEndianToHost16(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI24_SWF& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=LittleEndianToUnsignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI24_FLV& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=BigEndianToUnsignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI24_SWF& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=LittleEndianToSignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI24_FLV& v)
{
	assert(v.val==0);
	s.read((char*)&v.val,3);
	v.val=BigEndianToSignedHost24(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32_SWF& v)
{
	s.read((char*)&v.val,4);
	v.val=LittleEndianToHost32(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32_FLV& v)
{
	s.read((char*)&v.val,4);
	v.val=BigEndianToHost32(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& in, u32& v)
{
	int i=0;
	v.val=0;
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
			v.val|=(t2<<i);
			break;
		}
		else
		{
			uint8_t t2=(t&0x7f);
			v.val|=(t2<<i);
			i+=7;
		}
	}
	while(t&0x80);
	return in;
}

inline std::istream& operator>>(std::istream& s, FLOAT& v)
{
	union float_reader
	{
		uint32_t dump;
		float value;
	};
	float_reader dummy;
	s.read((char*)&dummy.dump,4);
	dummy.dump=LittleEndianToHost32(dummy.dump);
	v.val=dummy.value;
	return s;
}

inline std::istream& operator>>(std::istream& s, DOUBLE& v)
{
	union double_reader
	{
		uint64_t dump;
		double value;
	};
	double_reader dummy;
	// "Wacky format" is 45670123. Thanks to Gnash for reversing :-)
	s.read(((char*)&dummy.dump)+4,4);
	s.read(((char*)&dummy.dump),4);
	dummy.dump=LittleEndianToHost64(dummy.dump);
	v.val=dummy.value;
	return s;
}

inline std::istream& operator>>(std::istream& s, RECORDHEADER& v)
{
	s >> v.CodeAndLen;
	if((v.CodeAndLen&0x3f)==0x3f)
		s >> v.Length;
	return s;
}

class BitStream
{
public:
	std::istream& f;
	unsigned char buffer;
	unsigned char pos;
public:
	BitStream(std::istream& in):f(in),pos(0){};
	unsigned int readBits(unsigned int num)
	{
		unsigned int ret=0;
		while(num)
		{
			if(!pos)
			{
				pos=8;
				f.read((char*)&buffer,1);
			}
			ret<<=1;
			ret|=(buffer>>(pos-1))&1;
			pos--;
			num--;
		}
		return ret;
	}
};

class FB
{
	int32_t buf;
	int size;
public:
	FB() { buf=0; }
	FB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			LOG(LOG_ERROR,_("Fixed point bit field wider than 32 bit not supported"));
		buf=stream.readBits(s);
		if(buf>>(s-1)&1)
		{
			for(int i=31;i>=s;i--)
				buf|=(1<<i);
		}
	}
	operator float() const
	{
		if(buf>=0)
		{
			int32_t b=buf;
			return b/65536.0f;
		}
		else
		{
			int32_t b=-buf;
			return -(b/65536.0f);
		}
		//return (buf>>16)+(buf&0xffff)/65536.0f;
	}
};

class UB
{
	uint32_t buf;
	int size;
public:
	UB() { buf=0; }
	UB(int s,BitStream& stream):size(s)
	{
/*		if(s%8)
			buf=new uint8_t[s/8+1];
		else
			buf=new uint8_t[s/8];
		int i=0;
		while(!s)
		{
			buf[i]=stream.readBits(imin(s,8));
			s-=imin(s,8);
			i++;
		}*/
		if(s>32)
			LOG(LOG_ERROR,_("Unsigned bit field wider than 32 bit not supported"));
		buf=stream.readBits(s);
	}
	operator int() const
	{
		return buf;
	}
};

class SB
{
	int32_t buf;
	int size;
public:
	SB() { buf=0; }
	SB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			LOG(LOG_ERROR,_("Signed bit field wider than 32 bit not supported"));
		buf=stream.readBits(s);
		if(buf>>(s-1)&1)
		{
			for(int i=31;i>=s;i--)
				buf|=(1<<i);
		}
	}
	operator int() const
	{
		return buf;
	}
};

class RECT
{
	friend std::ostream& operator<<(std::ostream& s, const RECT& r);
	friend std::istream& operator>>(std::istream& stream, RECT& v);
public:
	int Xmin;
	int Xmax;
	int Ymin;
	int Ymax;
public:
	RECT();
	RECT(int xmin, int xmax, int ymin, int ymax);
};

template<class T> class Vector2Tmpl;
typedef Vector2Tmpl<double> Vector2f;

class MATRIX
{
	friend std::istream& operator>>(std::istream& stream, MATRIX& v);
	friend std::ostream& operator<<(std::ostream& s, const MATRIX& r);
public:
	number_t ScaleX;
	number_t ScaleY;
	number_t RotateSkew0;
	number_t RotateSkew1;
	int TranslateX;
	int TranslateY;
public:
	MATRIX():ScaleX(1),ScaleY(1),RotateSkew0(0),RotateSkew1(0),TranslateX(0),TranslateY(0){}
	void get4DMatrix(float matrix[16]) const;
	void multiply2D(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	Vector2f multiply2D(const Vector2f& in) const;
	MATRIX multiplyMatrix(const MATRIX& r) const;
	bool operator!=(const MATRIX& r) const;
	MATRIX getInverted() const;
	bool isInvertible() const;
};

class GRADRECORD
{
	friend std::istream& operator>>(std::istream& s, GRADRECORD& v);
public:
	GRADRECORD(int v):version(v){}
	int version;
	UI8 Ratio;
	RGBA Color;
	bool operator<(const GRADRECORD& g) const
	{
		return Ratio<g.Ratio;
	}
};

class GRADIENT
{
	friend std::istream& operator>>(std::istream& s, GRADIENT& v);
public:
	GRADIENT(int v):version(v){}
	int version;
	int SpreadMode;
	int InterpolationMode;
	int NumGradient;
	std::vector<GRADRECORD> GradientRecords;
};

class FOCALGRADIENT
{
	friend std::istream& operator>>(std::istream& s, FOCALGRADIENT& v);
public:
	int version;
	int SpreadMode;
	int InterpolationMode;
	int NumGradient;
	std::vector<GRADRECORD> GradientRecords;
	float FocalPoint;
};

class FILLSTYLEARRAY;
class MORPHFILLSTYLE;

enum FILL_STYLE_TYPE { SOLID_FILL=0x00, LINEAR_GRADIENT=0x10, RADIAL_GRADIENT=0x12, FOCAL_RADIAL_GRADIENT=0x13, REPEATING_BITMAP=0x40,
			CLIPPED_BITMAP=0x41, NON_SMOOTHED_REPEATING_BITMAP=0x42, NON_SMOOTHED_CLIPPED_BITMAP=0x43};

class FILLSTYLE
{
public:
	FILLSTYLE(int v):version(v),Gradient(v){}
	int version;
	FILL_STYLE_TYPE FillStyleType;
	RGBA Color;
	MATRIX Matrix;
	GRADIENT Gradient;
	FOCALGRADIENT FocalGradient;
	Bitmap* bitmap;
	virtual ~FILLSTYLE(){}
};

class MORPHFILLSTYLE:public FILLSTYLE
{
public:
	MORPHFILLSTYLE():FILLSTYLE(1){}
	RGBA StartColor;
	RGBA EndColor;
	MATRIX StartGradientMatrix;
	MATRIX EndGradientMatrix;
	UI8 NumGradients;
	std::vector<UI8> StartRatios;
	std::vector<UI8> EndRatios;
	std::vector<RGBA> StartColors;
	std::vector<RGBA> EndColors;
	~MORPHFILLSTYLE(){}
};

class LINESTYLE
{
public:
	LINESTYLE(int v):version(v){}
	int version;
	UI16_SWF Width;
	RGBA Color;
};

class LINESTYLE2
{
public:
	LINESTYLE2(int v):version(v),FillType(v){}
	int version;
	UI16_SWF Width;
	UB StartCapStyle;
	UB JointStyle;
	UB HasFillFlag;
	UB NoHScaleFlag;
	UB NoVScaleFlag;
	UB PixelHintingFlag;
	UB NoClose;
	UB EndCapStyle;
	UI16_SWF MiterLimitFactor;
	RGBA Color;
	FILLSTYLE FillType;
};

class MORPHLINESTYLE
{
public:
	UI16_SWF StartWidth;
	UI16_SWF EndWidth;
	RGBA StartColor;
	RGBA EndColor;
};

class LINESTYLEARRAY
{
public:
	LINESTYLEARRAY(int v):version(v){}
	int version;
	void appendStyles(const LINESTYLEARRAY& r);
	UI8 LineStyleCount;
	std::list<LINESTYLE> LineStyles;
	std::list<LINESTYLE2> LineStyles2;
};

class MORPHLINESTYLEARRAY
{
public:
	UI8 LineStyleCount;
	MORPHLINESTYLE* LineStyles;
};

class FILLSTYLEARRAY
{
public:
	FILLSTYLEARRAY(int v):version(v){}
	int version;
	void appendStyles(const FILLSTYLEARRAY& r);
	UI8 FillStyleCount;
	std::list<FILLSTYLE> FillStyles;
};

class MORPHFILLSTYLEARRAY
{
public:
	UI8 FillStyleCount;
	MORPHFILLSTYLE* FillStyles;
};

class SHAPE;
class SHAPEWITHSTYLE;

class SHAPERECORD
{
public:
	SHAPE* parent;
	bool TypeFlag;
	bool StateNewStyles;
	bool StateLineStyle;
	bool StateFillStyle1;
	bool StateFillStyle0;
	bool StateMoveTo;

	uint32_t MoveBits;
	int32_t MoveDeltaX;
	int32_t MoveDeltaY;

	unsigned int FillStyle1;
	unsigned int FillStyle0;
	unsigned int LineStyle;

	//Edge record
	bool StraightFlag;
	uint32_t NumBits;
	bool GeneralLineFlag;
	bool VertLineFlag;
	int32_t DeltaX;
	int32_t DeltaY;

	int32_t ControlDeltaX;
	int32_t ControlDeltaY;
	int32_t AnchorDeltaX;
	int32_t AnchorDeltaY;

	SHAPERECORD(SHAPE* p,BitStream& bs);
};

class TEXTRECORD;

class GLYPHENTRY
{
public:
	UB GlyphIndex;
	SB GlyphAdvance;
	TEXTRECORD* parent;
	GLYPHENTRY(TEXTRECORD* p,BitStream& bs);
};

class DefineTextTag;

class TEXTRECORD
{
public:
	UB TextRecordType;
	UB StyleFlagsReserved;
	UB StyleFlagsHasFont;
	UB StyleFlagsHasColor;
	UB StyleFlagsHasYOffset;
	UB StyleFlagsHasXOffset;
	UI16_SWF FontID;
	RGBA TextColor;
	SI16_SWF XOffset;
	SI16_SWF YOffset;
	UI16_SWF TextHeight;
	UI8 GlyphCount;
	std::vector <GLYPHENTRY> GlyphEntries;
	DefineTextTag* parent;
	TEXTRECORD(DefineTextTag* p):parent(p){}
};

class SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPE& v);
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
public:
	SHAPE():fillOffset(0),lineOffset(0){}
	virtual ~SHAPE(){};
	UB NumFillBits;
	UB NumLineBits;
	unsigned int fillOffset;
	unsigned int lineOffset;
	std::vector<SHAPERECORD> ShapeRecords;
};

class SHAPEWITHSTYLE : public SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
public:
	SHAPEWITHSTYLE(int v):version(v),FillStyles(v),LineStyles(v){}
	const int version;
	FILLSTYLEARRAY FillStyles;
	LINESTYLEARRAY LineStyles;
};

class CXFORMWITHALPHA
{
	friend std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v);
private:
	UB HasAddTerms;
	UB HasMultTerms;
	UB NBits;
	SB RedMultTerm;
	SB GreenMultTerm;
	SB BlueMultTerm;
	SB AlphaMultTerm;
	SB RedAddTerm;
	SB GreenAddTerm;
	SB BlueAddTerm;
	SB AlphaAddTerm;
};

class CXFORM
{
};

class DROPSHADOWFILTER
{
public:
    RGBA DropShadowColor;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    FIXED8 Strength;
    bool InnerShadow;
    bool Knockout;
    bool CompositeSource;
    UB Passes;
};

class BLURFILTER
{
public:
	FIXED BlurX;
	FIXED BlurY;
	UB Passes;
};

class GLOWFILTER
{
public:
    RGBA GlowColor;
    FIXED BlurX;
    FIXED BlurY;
    FIXED8 Strength;
    bool InnerGlow;
    bool Knockout;
    bool CompositeSource;
    UB Passes;
};

class BEVELFILTER
{
public:
    RGBA ShadowColor;
    RGBA HighlightColor;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    FIXED8 Strength;
    bool InnerShadow;
    bool Knockout;
    bool CompositeSource;
    bool OnTop;
    UB Passes;
};

class GRADIENTGLOWFILTER
{
public:
    UI8 NumColors;
    std::vector<RGBA> GradientColors;
    std::vector<UI8> GradientRatio;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    FIXED8 Strength;
    bool InnerGlow;
    bool Knockout;
    bool CompositeSource;
    UB Passes;
};

class CONVOLUTIONFILTER
{
public:
    UI8 MatrixX;
    UI8 MatrixY;
    FLOAT Divisor;
    FLOAT Bias;
    std::vector<FLOAT> Matrix;
    RGBA DefaultColor;
    bool Clamp;
    bool PreserveAlpha;
};

class COLORMATRIXFILTER
{
public:
    FLOAT Matrix[20];
};

class GRADIENTBEVELFILTER
{
public:
    UI8 NumColors;
    std::vector<RGBA> GradientColors;
    std::vector<UI8> GradientRatio;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    FIXED8 Strength;
    bool InnerShadow;
    bool Knockout;
    bool CompositeSource;
    bool OnTop;
    UB Passes;
};

class FILTER
{
public:
	UI8 FilterID;
	DROPSHADOWFILTER DropShadowFilter;
	BLURFILTER BlurFilter;
	GLOWFILTER GlowFilter;
	BEVELFILTER BevelFilter;
	GRADIENTGLOWFILTER GradientGlowFilter;
	CONVOLUTIONFILTER ConvolutionFilter;
	COLORMATRIXFILTER ColorMatrixFilter;
	GRADIENTBEVELFILTER GradientBevelFilter;
};

class FILTERLIST
{
public:
	UI8 NumberOfFilters;
	std::vector<FILTER> Filters;
};

class BUTTONRECORD
{
public:
	BUTTONRECORD(int v):buttonVersion(v){}
	int buttonVersion;
	UB ButtonReserved;
	UB ButtonHasBlendMode;
	UB ButtonHasFilterList;
	UB ButtonStateHitTest;
	UB ButtonStateDown;
	UB ButtonStateOver;
	UB ButtonStateUp;
	UI16_SWF CharacterID;
	UI16_SWF PlaceDepth;
	MATRIX PlaceMatrix;
	CXFORMWITHALPHA	ColorTransform;
	FILTERLIST FilterList;
	UI8 BlendMode;

	bool isNull() const
	{
		return !(ButtonReserved | ButtonHasBlendMode | ButtonHasFilterList | ButtonStateHitTest | ButtonStateDown | ButtonStateOver | ButtonStateUp);
	}
};

class CLIPEVENTFLAGS
{
public:
	uint32_t toParse;
	bool isNull();
};

class CLIPACTIONRECORD
{
public:
	CLIPEVENTFLAGS EventFlags;
	UI32_SWF ActionRecordSize;
	bool isLast();
};

class CLIPACTIONS
{
public:
	UI16_SWF Reserved;
	CLIPEVENTFLAGS AllEventFlags;
	std::vector<CLIPACTIONRECORD> ClipActionRecords;
};

class RunState
{
public:
	int last_FP;
	unsigned int FP;
	unsigned int next_FP;
	bool stop_FP;
	bool explicit_FP;
	RunState();
};

ASObject* abstract_i(int32_t i);
ASObject* abstract_ui(uint32_t i);
ASObject* abstract_d(number_t i);

void stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns);

std::ostream& operator<<(std::ostream& s, const RECT& r);
std::ostream& operator<<(std::ostream& s, const RGB& r);
std::ostream& operator<<(std::ostream& s, const RGBA& r);
std::ostream& operator<<(std::ostream& s, const STRING& r);
std::ostream& operator<<(std::ostream& s, const nsNameAndKind& r);
std::ostream& operator<<(std::ostream& s, const multiname& r);
std::ostream& operator<<(std::ostream& s, const tiny_string& r) DLL_PUBLIC;
std::ostream& operator<<(std::ostream& s, const QName& r);

std::istream& operator>>(std::istream& s, RECT& v);
std::istream& operator>>(std::istream& s, CLIPEVENTFLAGS& v);
std::istream& operator>>(std::istream& s, CLIPACTIONRECORD& v);
std::istream& operator>>(std::istream& s, CLIPACTIONS& v);
std::istream& operator>>(std::istream& s, RGB& v);
std::istream& operator>>(std::istream& s, RGBA& v);
std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHFILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLE& v);
std::istream& operator>>(std::istream& stream, LINESTYLE2& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLE& v);
std::istream& operator>>(std::istream& stream, MORPHFILLSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPERECORD& v);
std::istream& operator>>(std::istream& stream, TEXTRECORD& v);
std::istream& operator>>(std::istream& stream, MATRIX& v);
std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v);
std::istream& operator>>(std::istream& stream, GLYPHENTRY& v);
std::istream& operator>>(std::istream& stream, STRING& v);
std::istream& operator>>(std::istream& stream, BUTTONRECORD& v);
std::istream& operator>>(std::istream& stream, RECORDHEADER& v);
std::istream& operator>>(std::istream& stream, FILTERLIST& v);
std::istream& operator>>(std::istream& stream, FILTER& v);
std::istream& operator>>(std::istream& stream, DROPSHADOWFILTER& v);
std::istream& operator>>(std::istream& stream, BLURFILTER& v);
std::istream& operator>>(std::istream& stream, GLOWFILTER& v);
std::istream& operator>>(std::istream& stream, BEVELFILTER& v);
std::istream& operator>>(std::istream& stream, GRADIENTGLOWFILTER& v);
std::istream& operator>>(std::istream& stream, CONVOLUTIONFILTER& v);
std::istream& operator>>(std::istream& stream, COLORMATRIXFILTER& v);
std::istream& operator>>(std::istream& stream, GRADIENTBEVELFILTER& v);


};
#endif
