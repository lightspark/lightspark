/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
#define SWFTYPES_H 1

#include "compat.h"
#include <iostream>
#include <vector>
#include <list>
#include <cairo.h>

#include "logger.h"
#include <cstdlib>
#include <cassert>
#include "exceptions.h"
#include "smartrefs.h"
#include "tiny_string.h"
#include "memory_support.h"
#include "scripting/flash/display/BitmapContainer.h"

#ifdef BIG_ENDIAN
#include <algorithm>
#endif

class memorystream;

namespace lightspark
{

enum SWFOBJECT_TYPE { T_OBJECT=0, T_INTEGER=1, T_NUMBER=2, T_FUNCTION=3, T_UNDEFINED=4, T_NULL=5, T_STRING=6, 
	/*UNUSED=7,*/ T_BOOLEAN=8, T_ARRAY=9, T_CLASS=10, T_QNAME=11, T_NAMESPACE=12, T_UINTEGER=13, T_PROXY=14, T_TEMPLATE=15};

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

enum FILE_TYPE { FT_UNKNOWN=0, FT_SWF, FT_COMPRESSED_SWF, FT_LZMA_COMPRESSED_SWF, FT_PNG, FT_JPEG, FT_GIF };

typedef double number_t;

class ASObject;
class ABCContext;
class namespace_info;

struct multiname;
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
	operator multiname() const;
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
friend memorystream& operator>>(memorystream& in, u32& v);
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

struct nsNameAndKindImpl
{
	tiny_string name;
	NS_KIND kind;
	uint32_t baseId;
	nsNameAndKindImpl(const tiny_string& _name, NS_KIND _kind, uint32_t b=-1):name(_name),kind(_kind),baseId(b){}
	nsNameAndKindImpl(const char* _name, NS_KIND _kind, uint32_t b=-1):name(_name),kind(_kind),baseId(b){}
	bool operator<(const nsNameAndKindImpl& r) const
	{
		if(kind==r.kind)
			return name < r.name;
		else
			return kind < r.kind;
	}
	bool operator>(const nsNameAndKindImpl& r) const
	{
		if(kind==r.kind)
			return name > r.name;
		else
			return kind > r.kind;
	}
};

struct nsNameAndKind
{
	uint32_t nsId;
	uint32_t nsRealId;
	bool nameIsEmpty;
	nsNameAndKind(const tiny_string& _name, NS_KIND _kind);
	nsNameAndKind(const char* _name, NS_KIND _kind);
	nsNameAndKind(ABCContext * c, uint32_t nsContextIndex);
	/*
	 * Special constructor for protected namespace, which have
	 * different representationId
	 */
	nsNameAndKind(const tiny_string& _name, uint32_t _baseId, NS_KIND _kind);
	/*
	 * Special version to create the empty bultin namespace
	 */
	nsNameAndKind(uint32_t id):nsId(id),nsRealId(id),nameIsEmpty(true)
	{
		assert(nsId==0);
	}
	bool operator<(const nsNameAndKind& r) const
	{
		return nsId < r.nsId;
	}
	bool operator>(const nsNameAndKind& r) const
	{
		return nsId > r.nsId;
	}
	bool operator==(const nsNameAndKind& r) const
	{
		return nsId==r.nsId;
	}
	const nsNameAndKindImpl& getImpl() const;
	bool hasEmptyName() const
	{
		return nameIsEmpty;
	}
};

struct multiname: public memory_reporter
{
	union
	{
		uint32_t name_s_id;
		int32_t name_i;
		number_t name_d;
		ASObject* name_o;
	};
	std::vector<nsNameAndKind, reporter_allocator<nsNameAndKind>> ns;
	enum NAME_TYPE {NAME_STRING,NAME_INT,NAME_NUMBER,NAME_OBJECT};
	NAME_TYPE name_type;
	bool isAttribute;
	multiname(MemoryAccount* m);
	/*
		Returns a string name whatever is the name type
	*/
	tiny_string normalizedName() const;
	/*
	 * 	Return a string id whatever is the name type
	 */
	uint32_t normalizedNameId() const;
	tiny_string qualifiedString() const;
	/* sets name_type, name_s/name_d based on the object n */
	void setName(ASObject* n);
	void resetNameIfObject();
	bool isQName() const { return ns.size() == 1; }
	bool toUInt(uint32_t& out) const;
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
	UI32_SWF Length;
	UI16_SWF CodeAndLen;
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
	RGB(uint32_t color):Red((color>>16)&0xFF),Green((color>>8)&0xFF),Blue(color&0xFF){}
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
	RGBA(uint32_t color, int a):Red((color>>16)&0xFF),Green((color>>8)&0xFF),Blue(color&0xFF),Alpha(a){}
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
	v.val=GINT16_FROM_LE(v.val);
	return s;
}

inline std::istream & operator>>(std::istream &s, SI16_FLV& v)
{
	s.read((char*)&v.val,2);
	v.val=GINT16_FROM_BE(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16_SWF& v)
{
	s.read((char*)&v.val,2);
	v.val=GINT16_FROM_LE(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16_FLV& v)
{
	s.read((char*)&v.val,2);
	v.val=GINT16_FROM_BE(v.val);
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
	v.val=GINT32_FROM_LE(v.val);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32_FLV& v)
{
	s.read((char*)&v.val,4);
	v.val=GINT32_FROM_BE(v.val);
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
	dummy.dump=GINT32_FROM_LE(dummy.dump);
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
	dummy.dump=GINT64_FROM_LE(dummy.dump);
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
	BitStream(std::istream& in):f(in),buffer(0),pos(0){};
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
	// discards 'num' bits (padding)
	void discard(unsigned int num)
	{
		readBits(num);
	}
};

class FB
{
	int32_t buf;
	int size;
public:
	FB():buf(0),size(0){}
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
public:
	UB():buf(0) {}
	UB(int s,BitStream& stream)
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
public:
	SB():buf(0) {}
	SB(int s,BitStream& stream)
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

class MATRIX: public cairo_matrix_t
{
	friend std::istream& operator>>(std::istream& stream, MATRIX& v);
	friend std::ostream& operator<<(std::ostream& s, const MATRIX& r);
public:
	MATRIX(number_t sx=1, number_t sy=1, number_t sk0=0, number_t sk1=0, number_t tx=0, number_t ty=0);
	void get4DMatrix(float matrix[16]) const;
	void multiply2D(number_t xin, number_t yin, number_t& xout, number_t& yout) const;
	Vector2f multiply2D(const Vector2f& in) const;
	MATRIX multiplyMatrix(const MATRIX& r) const;
	bool operator!=(const MATRIX& r) const;
	MATRIX getInverted() const;
	bool isInvertible() const;
	number_t getTranslateX() const
	{
		return x0;
	}
	number_t getTranslateY() const
	{
		return y0;
	}
	number_t getScaleX() const
	{
		number_t ret=sqrt(xx*xx + yx*yx);
		if(xx>0)
			return ret;
		else
			return -ret;
	}
	number_t getScaleY() const
	{
		number_t ret=sqrt(yy*yy + xy*xy);
		if(yy>0)
			return ret;
		else
			return -ret;
	}
	number_t getRotation() const
	{
		return atan(xx/yx)*180/M_PI;
	}
	/*
	 * Implement flash style premultiply matrix operators
	 */
	void rotate(number_t angle)
	{
		cairo_matrix_t tmp;
		cairo_matrix_init_rotate(&tmp,angle);
		cairo_matrix_multiply(this,this,&tmp);
	}
	void scale(number_t sx, number_t sy)
	{
		cairo_matrix_t tmp;
		cairo_matrix_init_scale(&tmp,sx,sy);
		cairo_matrix_multiply(this,this,&tmp);
	}
	void translate(number_t dx, number_t dy)
	{
		cairo_matrix_t tmp;
		cairo_matrix_init_translate(&tmp,dx,dy);
		cairo_matrix_multiply(this,this,&tmp);
	}
};

class GRADRECORD
{
	friend std::istream& operator>>(std::istream& s, GRADRECORD& v);
public:
	GRADRECORD(uint8_t v):version(v){}
	RGBA Color;
	uint8_t version;
	UI8 Ratio;
	bool operator<(const GRADRECORD& g) const
	{
		return Ratio<g.Ratio;
	}
};

class GRADIENT
{
	friend std::istream& operator>>(std::istream& s, GRADIENT& v);
public:
	GRADIENT(uint8_t v):SpreadMode(0),InterpolationMode(0),version(v) {}
	int SpreadMode;
	int InterpolationMode;
	std::vector<GRADRECORD> GradientRecords;
	uint8_t version;
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
	FILLSTYLE(uint8_t v);
	FILLSTYLE(const FILLSTYLE& r);
	virtual ~FILLSTYLE();
	MATRIX Matrix;
	GRADIENT Gradient;
	FOCALGRADIENT FocalGradient;
	BitmapContainer bitmap;
	RGBA Color;
	FILL_STYLE_TYPE FillStyleType;
	uint8_t version;
};

class MORPHFILLSTYLE:public FILLSTYLE
{
public:
	MORPHFILLSTYLE():FILLSTYLE(1){}
	MATRIX StartGradientMatrix;
	MATRIX EndGradientMatrix;
	MATRIX StartBitmapMatrix;
	MATRIX EndBitmapMatrix;
	std::vector<UI8> StartRatios;
	std::vector<UI8> EndRatios;
	std::vector<RGBA> StartColors;
	std::vector<RGBA> EndColors;
	RGBA StartColor;
	RGBA EndColor;
	~MORPHFILLSTYLE(){}
};

class LINESTYLE
{
public:
	LINESTYLE(uint8_t v):version(v){}
	RGBA Color;
	UI16_SWF Width;
	uint8_t version;
};

class LINESTYLE2
{
public:
	LINESTYLE2(uint8_t v):FillType(v),version(v){}
	UB StartCapStyle;
	UB JointStyle;
	UB HasFillFlag;
	UB NoHScaleFlag;
	UB NoVScaleFlag;
	UB PixelHintingFlag;
	UB NoClose;
	UB EndCapStyle;
	UI16_SWF Width;
	UI16_SWF MiterLimitFactor;
	RGBA Color;
	FILLSTYLE FillType;
	uint8_t version;
};

class MORPHLINESTYLE
{
public:
	UI16_SWF StartWidth;
	UI16_SWF EndWidth;
	RGBA StartColor;
	RGBA EndColor;
};

class MORPHLINESTYLE2: public MORPHLINESTYLE
{
public:
	UB StartCapStyle;
	MORPHFILLSTYLE FillType;
	UB JoinStyle;
	UB HasFillFlag;
	UB NoHScaleFlag;
	UB NoVScaleFlag;
	UB PixelHintingFlag;
	UB NoClose;
	UB EndCapStyle;
	UI16_SWF MiterLimitFactor;
};

class LINESTYLEARRAY
{
public:
	LINESTYLEARRAY(uint8_t v):version(v){}
	void appendStyles(const LINESTYLEARRAY& r);
	std::list<LINESTYLE> LineStyles;
	std::list<LINESTYLE2> LineStyles2;
	uint8_t version;
};

class MORPHLINESTYLEARRAY
{
public:
	MORPHLINESTYLEARRAY(uint8_t v):version(v){}
	std::list<MORPHLINESTYLE> LineStyles;
	std::list<MORPHLINESTYLE2> LineStyles2;
	const uint8_t version;
};

class FILLSTYLEARRAY
{
public:
	FILLSTYLEARRAY(uint8_t v):version(v){}
	void appendStyles(const FILLSTYLEARRAY& r);
	std::list<FILLSTYLE> FillStyles;
	uint8_t version;
};

class MORPHFILLSTYLEARRAY
{
public:
	std::list<MORPHFILLSTYLE> FillStyles;
};

class SHAPE;
class SHAPEWITHSTYLE;

class SHAPERECORD
{
public:
	SHAPE* parent;

	uint32_t MoveBits;
	int32_t MoveDeltaX;
	int32_t MoveDeltaY;

	unsigned int FillStyle1;
	unsigned int FillStyle0;
	unsigned int LineStyle;

	//Edge record
	uint32_t NumBits;
	int32_t DeltaX;
	int32_t DeltaY;

	int32_t ControlDeltaX;
	int32_t ControlDeltaY;
	int32_t AnchorDeltaX;
	int32_t AnchorDeltaY;

	bool TypeFlag;
	bool StateNewStyles;
	bool StateLineStyle;
	bool StateFillStyle1;
	bool StateFillStyle0;
	bool StateMoveTo;
	bool StraightFlag;
	bool GeneralLineFlag;
	bool VertLineFlag;
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
	std::vector <GLYPHENTRY> GlyphEntries;
	DefineTextTag* parent;
	UB TextRecordType;
	UB StyleFlagsReserved;
	UB StyleFlagsHasFont;
	UB StyleFlagsHasColor;
	UB StyleFlagsHasYOffset;
	UB StyleFlagsHasXOffset;
	RGBA TextColor;
	SI16_SWF XOffset;
	SI16_SWF YOffset;
	UI16_SWF TextHeight;
	UI16_SWF FontID;
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
	SHAPEWITHSTYLE(uint8_t v):FillStyles(v),LineStyles(v),version(v){}
	FILLSTYLEARRAY FillStyles;
	LINESTYLEARRAY LineStyles;
	const uint8_t version;
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
    UB Passes;
    FIXED8 Strength;
    bool InnerShadow;
    bool Knockout;
    bool CompositeSource;
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
    UB Passes;
    FIXED8 Strength;
    bool InnerGlow;
    bool Knockout;
    bool CompositeSource;
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
    UB Passes;
    FIXED8 Strength;
    bool InnerShadow;
    bool Knockout;
    bool CompositeSource;
    bool OnTop;
};

class GRADIENTGLOWFILTER
{
public:
    std::vector<RGBA> GradientColors;
    std::vector<UI8> GradientRatio;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    UB Passes;
    FIXED8 Strength;
    bool InnerGlow;
    bool Knockout;
    bool CompositeSource;
};

class CONVOLUTIONFILTER
{
public:
    FLOAT Divisor;
    FLOAT Bias;
    std::vector<FLOAT> Matrix;
    RGBA DefaultColor;
    UI8 MatrixX;
    UI8 MatrixY;
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
    std::vector<RGBA> GradientColors;
    std::vector<UI8> GradientRatio;
    FIXED BlurX;
    FIXED BlurY;
    FIXED Angle;
    FIXED Distance;
    UB Passes;
    FIXED8 Strength;
    bool InnerShadow;
    bool Knockout;
    bool CompositeSource;
    bool OnTop;
};

class FILTER
{
public:
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
	MATRIX PlaceMatrix;
	FILTERLIST FilterList;
	CXFORMWITHALPHA	ColorTransform;
	UI16_SWF CharacterID;
	UI16_SWF PlaceDepth;
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
	std::vector<CLIPACTIONRECORD> ClipActionRecords;
	CLIPEVENTFLAGS AllEventFlags;
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

inline double twipsToPixels(double twips) { return twips/20.0; }

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
std::istream& operator>>(std::istream& stream, MORPHLINESTYLE2& v);
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
#endif /* SWFTYPES_H */
