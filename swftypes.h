/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef SWFTYPES_H
#define SWFTYPES_H

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "logger.h"

class STRING;

enum SWFOBJECT_TYPE { T_OBJECT=0, T_MOVIE, T_REGNUMBER, T_CONSTREF, T_INTEGER, T_DOUBLE, T_FUNCTION,
	T_UNDEFINED, T_NULL, T_PLACEOBJECT, T_WRAPPED, T_STRING};

class SWFObject;
class IFunction;
class UI32
{
friend std::istream& operator>>(std::istream& s, UI32& v);
private:
	uint32_t val;
public:
	UI32():val(0){}
	UI32(uint32_t v):val(v){}
	operator uint32_t(){ return val; }
};

class UI16
{
friend std::istream& operator>>(std::istream& s, UI16& v);
private:
	uint16_t val;
public:
	UI16():val(0){}
	UI16(uint16_t v):val(v){}
	operator uint16_t() const { return val; }
	operator UI32() const { return val; }
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
	operator UI16(){ return val; }
};

class STRING
{
friend std::ostream& operator<<(std::ostream& s, const STRING& r);
friend std::istream& operator>>(std::istream& stream, STRING& v);
friend class ASString;
private:
	std::vector<UI8> String;
public:
	STRING(){};
	STRING(const char* s)
	{
		do
		{
			String.push_back(*s);
			s++;
		}
		while(*s!=0);
	}
	bool operator==(const STRING& s)
	{
		if(String.size()!=s.String.size())
			return false;
		for(int i=0;i<String.size();i++)
		{
			if(String[i]!=s.String[i])
				return false;
		}
		return true;
	}
	STRING operator+(const STRING& s)
	{
		STRING ret(*this);
		for(int i=0;i<s.String.size();i++)
			ret.String.push_back(s.String[i]);
		return ret;
	}
	bool isNull() const
	{
		return !String.size();
	}
};

class ISWFObject;

class SWFObject
{
private:
	ISWFObject* data;
	bool owner;
	bool binded;
	STRING name;
	//virtual bool xequals(const SWFObject& r);
public:
	SWFObject();
	SWFObject(const SWFObject& o);
	SWFObject(ISWFObject* d, bool b=false);
	ISWFObject* operator->() const { return data; }
	bool isDefined(); 
	SWFObject& operator=(const SWFObject& r);
	bool equals(const SWFObject& r);
	bool isLess(const SWFObject& r);
	bool isGreater(const SWFObject& r);
	STRING getName() const;
	void setName(const STRING& n);
	//void bind(){ binded=true;}
	
	ISWFObject* getData() const;
};

class ISWFObject
{
public:
	virtual SWFOBJECT_TYPE getObjectType()=0;
	virtual STRING toString();
	virtual int toInt();
	virtual float toFloat();
	virtual IFunction* toFunction();
	virtual SWFObject getVariableByName(const STRING& name)=0;
	virtual void setVariableByName(const STRING& name, const SWFObject& o)=0;
	virtual ISWFObject* clone()
	{
		LOG(ERROR,"Cloning object of type " << (int)getObjectType());
	}
	virtual SWFObject instantiate()
	{
		return SWFObject(clone(),true);
	}
	virtual ISWFObject* getParent()=0;
	virtual void _register()=0;
};

class ISWFObject_impl:public ISWFObject
{
private:
	int getVariableIndexByName(const STRING& name);
protected:
	ISWFObject* parent;
	ISWFObject_impl();
	std::vector<SWFObject> Variables;
public:
	SWFObject getVariableByName(const STRING& name);
	void setVariableByName(const STRING& name, const SWFObject& o);
	ISWFObject* getParent();
	void _register();
};

class ConstantReference : public ISWFObject_impl
{
private:
	int index;
public:
	ConstantReference(int i):index(i){}
	SWFOBJECT_TYPE getObjectType(){return T_CONSTREF;}
	STRING toString();
	int toInt();
	ISWFObject* clone()
	{
		return new ConstantReference(*this);
	}
	SWFObject instantiate();
};

class RegisterNumber : public ISWFObject_impl
{
private:
	int index;
public:
	RegisterNumber(int i):index(i){ if(i>10) LOG(ERROR,"Register number too high"); }
	SWFOBJECT_TYPE getObjectType(){return T_REGNUMBER;}
	STRING toString();
	//int toInt();
	ISWFObject* clone()
	{
		return new RegisterNumber(*this);
	}
	SWFObject instantiate();
};

class Undefined : public ISWFObject_impl
{
public:
	SWFOBJECT_TYPE getObjectType(){return T_UNDEFINED;}
	STRING toString();
	ISWFObject* clone()
	{
		return new Undefined;
	}
};

class Null : public ISWFObject_impl
{
public:
	SWFOBJECT_TYPE getObjectType(){return T_NULL;}
	STRING toString();
	ISWFObject* clone()
	{
		return new Null;
	}
};

class Double : public ISWFObject_impl
{
private:
	double val;
public:
	Double(double v):val(v){}
	SWFOBJECT_TYPE getObjectType(){return T_DOUBLE;}
	STRING toString();
	int toInt(); 
	float toFloat();
	ISWFObject* clone()
	{
		return new Double(*this);
	}
};

class arguments;

class IFunction: public ISWFObject_impl
{
public:
	virtual SWFObject call(ISWFObject* obj, arguments* args)=0;
};

class Function : public IFunction
{
public:
	typedef SWFObject (*as_function)(const SWFObject&, arguments*);
	Function(as_function v):val(v){}
	SWFOBJECT_TYPE getObjectType(){return T_FUNCTION;}
	SWFObject call(ISWFObject* obj, arguments* args);
	IFunction* toFunction();
private:
	as_function val;
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

class Integer : public ISWFObject_impl
{
private:
	int val;
public:
	Integer(int v):val(v){}
	SWFOBJECT_TYPE getObjectType(){return T_INTEGER;}
	STRING toString();
	int toInt(); 
	float toFloat();
	ISWFObject* clone()
	{
		return new Integer(*this);
	}
};

class SI16
{
friend std::istream& operator>>(std::istream& s, SI16& v);
private:
	int16_t val;
public:
	SI16():val(0){}
	SI16(int16_t v):val(v){}
	operator int16_t(){ return val; }
};

typedef UI16 RECORDHEADER;

class RGB
{
public:
	RGB(){};
	RGB(int r,int g, int b):Red(r),Green(g),Blue(b){};
	UI8 Red;
	UI8 Green;
	UI8 Blue;
};

class RGBA
{
public:
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
};

typedef UI32 SI32;

typedef UI8 LANGCODE;

std::istream& operator>>(std::istream& s, RGB& v);

inline std::istream& operator>>(std::istream& s, UI8& v)
{
	s.read((char*)&v.val,1);
	return s;
}

inline std::istream& operator>>(std::istream& s, SI16& v)
{
	s.read((char*)&v.val,2);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI16& v)
{
	s.read((char*)&v.val,2);
	return s;
}

inline std::istream& operator>>(std::istream& s, UI32& v)
{
	s.read((char*)&v.val,4);
	return s;
}

inline std::istream& operator>>(std::istream& s, DOUBLE& v)
{
	// "Wacky format" is 45670123. Thanks to Ghash :-)
	s.read(((char*)&v.val)+4,4);
	s.read(((char*)&v.val),4);
	return s;
}

inline int min(int a,int b)
{
	return (a<b)?a:b;
}

inline int max(int a,int b)
{
	return (a>b)?a:b;
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
	uint32_t buf;
	int size;
public:
	FB() { buf=0; }
	FB(int s,BitStream& stream):size(s)
	{
		if(s>32)
			LOG(ERROR,"Fixed point bit field wider than 32 bit not supported");
		buf=stream.readBits(s);
	}
	operator float() const
	{
		return (buf>>16)+(buf&0xffff)/65535.0f; //TODO: Oppure 65536?
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
			buf[i]=stream.readBits(min(s,8));
			s-=min(s,8);
			i++;
		}*/
		if(s>32)
			LOG(ERROR,"Unsigned bit field wider than 32 bit not supported");
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
			LOG(ERROR,"Signed bit field wider than 32 bit not supported");
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
	UB NBits;
	SB Xmin;
	SB Xmax;
	SB Ymin;
	SB Ymax;
public:
	RECT();
//	RECT(FILE* in);
};

class MATRIX
{
	friend std::istream& operator>>(std::istream& stream, MATRIX& v);
	friend std::ostream& operator<<(std::ostream& s, const MATRIX& r);
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
	void get4DMatrix(float matrix[16]);
	//int getSize(){return size;}
};

class GRADRECORD
{
public:
	int version;
	UI8 Ratio;
	RGBA Color;
};

class GRADIENT
{
public:
	int version;
	UB SpreadMode;
	UB InterpolationMode;
	UB NumGradient;
	std::vector<GRADRECORD> GradientRecords;
};

class FILLSTYLE
{
public:
	int version;
	UI8 FillStyleType;
	RGBA Color;
	MATRIX GradientMatrix;
	GRADIENT Gradient;
	UI16 BitmapId;
	MATRIX BitmapMatrix;
};

class MORPHFILLSTYLE
{
public:
	UI8 FillStyleType;
	RGBA StartColor;
	RGBA EndColor;
};

class LINESTYLE
{
public:
	int version;
	UI16 Width;
	RGBA Color;
};

class MORPHLINESTYLE
{
public:
	UI16 StartWidth;
	UI16 EndWidth;
	RGBA StartColor;
	RGBA EndColor;
};

class LINESTYLEARRAY
{
public:
	int version;
	UI8 LineStyleCount;
	LINESTYLE* LineStyles;
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
	int version;
	UI8 FillStyleCount;
	FILLSTYLE* FillStyles;
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
	UB TypeFlag;
	UB StateNewStyles;
	UB StateLineStyle;
	UB StateFillStyle1;
	UB StateFillStyle0;
	UB StateMoveTo;

	UB MoveBits;
	SB MoveDeltaX;
	SB MoveDeltaY;

	UB FillStyle1;
	UB FillStyle0;
	UB LineStyle;

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
	UI16 FontID;
	RGBA TextColor;
	SI16 XOffset;
	SI16 YOffset;
	UI16 TextHeight;
	UI8 GlyphCount;
	std::vector <GLYPHENTRY> GlyphEntries;
	DefineTextTag* parent;
	TEXTRECORD(DefineTextTag* p):parent(p){}
};

class Shape;

class SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPE& v);
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
	friend class SHAPERECORD;
	friend class DefineShapeTag;
	friend class DefineShape2Tag;
	friend class DefineShape3Tag;
	friend class DefineMorphShapeTag;
	friend class DefineTextTag;
	friend class DefineFontTag;
	friend class DefineFont2Tag;
private:
	UB NumFillBits;
	UB NumLineBits;
	SHAPERECORD ShapeRecords;
};

class SHAPEWITHSTYLE : public SHAPE
{
	friend std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
public:
	int version;
	FILLSTYLEARRAY FillStyles;
	LINESTYLEARRAY LineStyles;
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

class CLIPACTIONS
{
};

class CXFORM
{
};

class FILTERLIST
{
};

class BUTTONRECORD
{
public:
	UB ButtonReserved;
	UB ButtonHasBlendMode;
	UB ButtonHasFilterList;
	UB ButtonStateHitTest;
	UB ButtonStateDown;
	UB ButtonStateOver;
	UB ButtonStateUp;
	UI16 CharacterID;
	UI16 PlaceDepth;
	MATRIX PlaceMatrix;
	CXFORMWITHALPHA	ColorTransform;
	FILTERLIST FilterList;
	UI8 BlendMode;

	bool isNull() const
	{
		return !(ButtonReserved | ButtonHasBlendMode | ButtonHasFilterList | ButtonStateHitTest | ButtonStateDown | ButtonStateOver | ButtonStateUp);
	}
};

std::ostream& operator<<(std::ostream& s, const RECT& r);
std::ostream& operator<<(std::ostream& s, const RGB& r);
std::ostream& operator<<(std::ostream& s, const RGBA& r);
std::ostream& operator<<(std::ostream& s, const STRING& r);
std::istream& operator>>(std::istream& s, RECT& v);

std::istream& operator>>(std::istream& s, RGB& v);
std::istream& operator>>(std::istream& s, RGBA& v);
std::istream& operator>>(std::istream& stream, SHAPEWITHSTYLE& v);
std::istream& operator>>(std::istream& stream, SHAPE& v);
std::istream& operator>>(std::istream& stream, FILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHFILLSTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, MORPHLINESTYLEARRAY& v);
std::istream& operator>>(std::istream& stream, LINESTYLE& v);
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
#endif
