#ifndef SWFTYPES_H
#define SWFTYPES_H

#include <stdint.h>
#include <iostream>
#include <fstream>
typedef uint8_t UI8;

class UI16
{
private:
	uint16_t val;
public:
	UI16():val(0){}
	UI16(uint16_t v):val(v){}
	operator uint16_t(){ return val; }
};

class UI32
{
private:
	uint32_t val;
public:
	UI32():val(0){}
	UI32(uint32_t v):val(v){}
	operator uint32_t(){ return val; }
};

typedef UI16 RECORDHEADER;

class RGB
{
public:
	UI8 Red;
	UI8 Green;
	UI8 Blue;
};

typedef UI32 SI32;

std::istream& operator>>(std::istream& s, RGB& v);

inline std::istream& operator>>(std::istream& s, UI16& v)
{
	s.read((char*)&v,2);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32& v)
{
	s.read((char*)&v,4);
	return s;
}

inline int min(int a,int b)
{
	return (a<b)?a:b;
}

class BitStream
{
private:
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

class UB
{
	uint32_t buf;
	int size;
public:
	UB() { buf=NULL; }
	UB(int s,BitStream& stream):size(s)
	{
/*		if(s%8)
			buf=new uint8_t[s/8+1];
		else
			buf=new uint8_t[s/8];
		int i=0;
		while(!s)
		{
			buf[i]=stream.readBits(min(s,8));
			s-=min(s,8);
			i++;
		}*/
		if(s>32)
			throw "UB too big\n";
		buf=stream.readBits(s);
	}
	operator int()
	{
		return buf;
	}
};

class SB
{
	int32_t buf;
	int size;
public:
	SB() { buf=NULL; }
	SB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			throw "SB too big\n";
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
	friend std::ostream& operator<<(const std::ostream& s, const RECT& r);
	friend std::istream& operator>>(std::istream& stream, RECT& v);
private:
	UB NBits;
	SB Xmin;
	SB Xmax;
	SB Ymin;
	SB Ymax;
public:
	RECT();
//	RECT(FILE* in);
};

class FILLSTYLE
{
public:
	UI8 FillStyleType;
	RGB Color;
};

class LINESTYLE
{
public:
	UI16 Width;
	RGB Color;
};

class LINESTYLEARRAY
{
public:
	UI8 LineStyleCount;
	LINESTYLE* LineStyles;
};

class FILLSTYLEARRAY
{
public:
	UI8 FillStyleCount;
	FILLSTYLE* FillStyles;
};

class SHAPEWITHSTYLE
{
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
private:
	FILLSTYLEARRAY FillStyles;
	LINESTYLEARRAY LineStyles;
};

std::ostream& operator<<(const std::ostream& s, const RECT& r);
std::ostream& operator<<(const std::ostream& s, const RGB& r);
std::istream& operator>>(std::istream& s, RECT& v);

std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLE& v);
#endif
