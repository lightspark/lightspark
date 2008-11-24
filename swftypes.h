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
				std::cout << "read byte  @" << this << std::endl;
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
	uint32_t buf;
	int size;
public:
	FB() { buf=0; }
	FB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			throw "FB too big\n";
		buf=stream.readBits(s);
	}
	operator float()
	{
		return (buf>>16)+(buf&0xffff)/65535.0f; //TODO: Oppure 65536?
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

class SHAPEWITHSTYLE;

class SHAPERECORD
{
public:
	SHAPEWITHSTYLE* parent;
	UB TypeFlag;
	UB StateNewStyles;
	UB StateLineStyle;
	UB StateFillStyle1;
	UB StateFillStyle0;
	UB StateMoveTo;

	UB MoveBits;
	UB MoveDeltaX;
	UB MoveDeltaY;

	UB FillStyle1;

	//Edge record
	UB StraightFlag;
	UB NumBits;
	UB GeneralLineFlag;
	UB VertLineFlag;
	SB DeltaX;
	SB DeltaY;

	SB ControlDeltaX;
	SB ControlDeltaY;
	SB AnchorDeltaX;
	SB AnchorDeltaY;

	SHAPERECORD* next;

	SHAPERECORD():next(0){};
	SHAPERECORD(SHAPEWITHSTYLE* p,BitStream& bs);
};

class SHAPEWITHSTYLE
{
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
	friend class SHAPERECORD;
private:
	FILLSTYLEARRAY FillStyles;
	LINESTYLEARRAY LineStyles;
	UB NumFillBits;
	UB NumLineBits;
	SHAPERECORD ShapeRecords;
public:
//	SHAPEWITHSTYLE(){}
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

class STRING
{
};

class CLIPACTIONS
{
};

class MATRIX
{
	friend std::istream& operator>>(std::istream& stream, MATRIX& v);
private:
	int size;
	UB HasScale;
	UB NScaleBits;
	FB ScaleX;
	FB ScaleY;
	UB HasRotate;
	UB NRotateBits;
	FB RotateSkew0;
	FB RotateSkew1;
	UB NTranslateBits;
	SB TranslateX;
	SB TranslateY;
public:
	MATRIX():size(0){}
	//int getSize(){return size;}
};

class CXFORM
{
};

std::ostream& operator<<(const std::ostream& s, const RECT& r);
std::ostream& operator<<(const std::ostream& s, const RGB& r);
std::istream& operator>>(std::istream& s, RECT& v);

std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPERECORD& v);
std::istream& operator>>(std::istream& stream, MATRIX& v);
std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v);
#endif
