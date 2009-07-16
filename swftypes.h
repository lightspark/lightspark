/**************************************************************************
    Lightspark, a free flash player implementation

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
#include <map>

#include "logger.h"
#include <stdlib.h>

#define ASFUNCTION(name) \
	static ISWFObject* name(ISWFObject* , arguments* args)
#define ASFUNCTIONBODY(c,name) \
	ISWFObject* c::name(ISWFObject* obj, arguments* args)

enum SWFOBJECT_TYPE { T_OBJECT=0, T_MOVIE, T_REGNUMBER, T_CONSTREF, T_INTEGER, T_NUMBER, T_FUNCTION,
	T_UNDEFINED, T_NULL, T_PLACEOBJECT, T_WRAPPED, T_STRING, T_DEFINABLE, T_BOOLEAN, T_ARRAY};

class arguments;
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
	std::string String;
public:
	STRING(){};
	STRING(const char* s):String(s)
	{
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
	operator const std::string&() const
	{
		return String;
	}
	int size()
	{
		return String.size();
	}
};

struct Qname
{
	std::string name;
	std::string ns;
	bool operator<(const Qname& r) const
	{
		return name<r.name;
	}
	Qname(const std::string& s):name(s){}
	Qname(const char* s):name(s){}
	Qname(const STRING& s):name(s){}
	Qname(const std::string& _ns, const std::string& _name):ns(_ns),name(_name){}
};

struct multiname
{
	std::string name;
	std::vector<std::string> ns;
};

class ISWFObject
{
protected:
	ISWFObject* parent;
	ISWFObject();
	std::map<Qname,ISWFObject*> Variables;
	std::map<Qname,IFunction*> Setters;
	std::map<Qname,IFunction*> Getters;
	std::vector<ISWFObject*> slots;
	typedef std::map<Qname,ISWFObject*>::iterator var_iterator;
	std::vector<var_iterator> slots_vars;
	int max_slot_index;
	bool binded;
	int ref_count;
public:
	IFunction* constructor;
	int class_index;
	std::string class_name;
	virtual ~ISWFObject();
	void incRef() {ref_count++;}
	void decRef()
	{
		ref_count--;
		//if(ref_count==0)
		//	delete this;
	}
	static void s_incRef(ISWFObject* o)
	{
		o->incRef();
	}
	static void s_decRef(ISWFObject* o)
	{
		o->decRef();
	}
	virtual IFunction* getSetterByName(const Qname& name, bool& found);
	virtual IFunction* setSetterByName(const Qname& name, IFunction* o);
	virtual IFunction* getGetterByName(const Qname& name, bool& found);
	virtual IFunction* setGetterByName(const Qname& name, IFunction* o);

	virtual ISWFObject* getVariableByMultiname(const multiname& name, bool& found);
	virtual ISWFObject* getVariableByString(const std::string& name, bool& found);
	virtual ISWFObject* getVariableByName(const Qname& name, bool& found);
	virtual ISWFObject* setVariableByName(const Qname& name, ISWFObject* o, bool force=false);
	virtual ISWFObject* getParent();
	virtual void _register();
	virtual ISWFObject* getSlot(int n);
	virtual void setSlot(int n,ISWFObject* o);
	virtual void initSlot(int n,ISWFObject* o, const Qname& name);
	virtual void dumpVariables();
	virtual int numVariables();
	std::string getNameAt(int i);

	bool isBinded() {return binded;}
	void bind(){ binded=true;}

	virtual SWFOBJECT_TYPE getObjectType() const=0;
	virtual std::string toString() const;
	virtual int toInt() const;
	virtual double toNumber() const;
	virtual IFunction* toFunction();
	virtual ISWFObject* clone()
	{
		LOG(ERROR,"Cloning object of type " << (int)getObjectType());
		abort();
	}

	virtual bool isEqual(const ISWFObject* r) const;
	virtual bool isLess(const ISWFObject* r) const;
	virtual bool isGreater(const ISWFObject* r) const;

	virtual void copyFrom(const ISWFObject* o)
	{
		LOG(ERROR,"Copy object of type " << (int)getObjectType() << " from object of type " << (int)o->getObjectType());
		abort();
	}
};

class ConstantReference : public ISWFObject
{
private:
	int index;
public:
	ConstantReference(int i):index(i){}
	SWFOBJECT_TYPE getObjectType() const{return T_CONSTREF;}
	std::string toString() const;
	int toInt() const;
	ISWFObject* clone()
	{
		return new ConstantReference(*this);
	}
	ISWFObject* instantiate();
};

class RegisterNumber : public ISWFObject
{
private:
	int index;
public:
	RegisterNumber(int i):index(i){ if(i>10) LOG(ERROR,"Register number too high"); }
	SWFOBJECT_TYPE getObjectType() const {return T_REGNUMBER;}
	std::string toString() const;
	//int toInt();
	ISWFObject* clone()
	{
		return new RegisterNumber(*this);
	}
	ISWFObject* instantiate();
};

class Null : public ISWFObject
{
public:
	SWFOBJECT_TYPE getObjectType() const {return T_NULL;}
	std::string toString() const;
	ISWFObject* clone()
	{
		return new Null;
	}
	bool isEqual(const ISWFObject* r) const;
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

class Integer : public ISWFObject
{
friend class Number;
friend class ABCVm;
private:
	int val;
public:
	Integer(int v):val(v){}
	Integer& operator=(int v){val=v; return *this; }
	SWFOBJECT_TYPE getObjectType()const {return T_INTEGER;}
	std::string toString() const;
	int toInt() const; 
	double toNumber() const;
	operator int() const{return val;} 
	bool isLess(const ISWFObject* r) const;
	bool isEqual(const ISWFObject* o) const;
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
	RGBA():Red(0),Green(0),Blue(0),Alpha(255){}
	RGBA(int r, int g, int b, int a):Red(r),Green(g),Blue(b),Alpha(a){}
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

inline std::istream& operator>>(std::istream& s, FLOAT& v)
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
public:
	int size;
	UB HasScale;
	UB NScaleBits;
	float ScaleX;
	float ScaleY;
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
	bool operator<(const GRADRECORD& g) const
	{
		return Ratio<g.Ratio;
	}
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

class FILLSTYLEARRAY;
class MORPHFILLSTYLE;

class FILLSTYLE
{
	friend std::istream& operator>>(std::istream& s, FILLSTYLEARRAY& v);
	friend std::istream& operator>>(std::istream& s, FILLSTYLE& v);
	friend std::istream& operator>>(std::istream& s, MORPHFILLSTYLE& v);
	friend class DefineTextTag;
	friend class DefineShape2Tag;
	friend class Shape;
private:
	int version;
	UI8 FillStyleType;
	RGBA Color;
	MATRIX GradientMatrix;
	GRADIENT Gradient;
	UI16 BitmapId;
	MATRIX BitmapMatrix;

public:
	virtual void setFragmentProgram() const;
	static void fixedColor(float r, float g, float b);
};

class MORPHFILLSTYLE:public FILLSTYLE
{
public:
	RGBA StartColor;
	RGBA EndColor;
	MATRIX StartGradientMatrix;
	MATRIX EndGradientMatrix;
	UI8 NumGradients;
	std::vector<UI8> StartRatios;
	std::vector<UI8> EndRatios;
	std::vector<RGBA> StartColors;
	std::vector<RGBA> EndColors;
};

class LINESTYLE
{
public:
	int version;
	UI16 Width;
	RGBA Color;
};

class LINESTYLE2
{
public:
	UI16 Width;
	UB StartCapStyle;
	UB JointStyle;
	UB HasFillFlag;
	UB NoHScaleFlag;
	UB NoVScaleFlag;
	UB PixelHintingFlag;
	UB NoClose;
	UB EndCapStyle;
	UI16 MiterLimitFactor;
	RGBA Color;
	FILLSTYLE FillType;
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
	LINESTYLE2* LineStyles2;
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
	friend class DefineShape4Tag;
	friend class DefineMorphShapeTag;
	friend class DefineTextTag;
	friend class DefineFontTag;
	friend class DefineFont2Tag;
	friend class DefineFont3Tag;
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

class CLIPEVENTFLAGS
{
public:
	UI32 toParse;
	bool isNull();
};

class CLIPACTIONRECORD
{
public:
	CLIPEVENTFLAGS EventFlags;
	UI32 ActionRecordSize;
	bool isLast();
};

class CLIPACTIONS
{
public:
	UI16 Reserved;
	CLIPEVENTFLAGS AllEventFlags;
	std::vector<CLIPACTIONRECORD> ClipActionRecords;
};

std::ostream& operator<<(std::ostream& s, const RECT& r);
std::ostream& operator<<(std::ostream& s, const RGB& r);
std::ostream& operator<<(std::ostream& s, const RGBA& r);
std::ostream& operator<<(std::ostream& s, const STRING& r);
std::ostream& operator<<(std::ostream& s, const multiname& r);
std::ostream& operator<<(std::ostream& s, const Qname& r);

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
#endif
