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

#define GL_GLEXT_PROTOTYPES

#include "swftypes.h"
#include "scripting/abc.h"
#include "logger.h"
#include <string.h>
#include <algorithm>
#include <stdlib.h>
#include <cmath>
#include "swf.h"
#include "backends/geometry.h"
#include "backends/rendering.h"
#include "scripting/class.h"
#include "exceptions.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

extern TLSDATA ParseThread* pt;

tiny_string& tiny_string::operator+=(const char* s)
{
	if(type==READONLY)
	{
		char* tmp=buf;
		makePrivateCopy(tmp);
	}
	uint32_t addedLen=strlen(s);
	stringSize+=addedLen;
	if(type==STATIC && stringSize > STATIC_SIZE)
	{
		createBuffer(stringSize);
		strcpy(buf,_buf_static);
	}
	else if(type==DYNAMIC && addedLen!=0)
		resizeBuffer(stringSize);
	strcat(buf,s);
	return *this;
}

tiny_string& tiny_string::operator+=(const tiny_string& r)
{
	if(type==READONLY)
	{
		char* tmp=buf;
		makePrivateCopy(tmp);
	}
	uint32_t addedLen=r.stringSize-1;
	stringSize+=addedLen;
	if(type==STATIC && stringSize > STATIC_SIZE)
	{
		createBuffer(stringSize);
		strcpy(buf,_buf_static);
	}
	else if(type==DYNAMIC && addedLen!=0)
		resizeBuffer(stringSize);
	strcat(buf,r.buf);
	return *this;
}

const tiny_string tiny_string::operator+(const tiny_string& r) const
{
	tiny_string ret(*this);
	ret+=r;
	return ret;
}

tiny_string tiny_string::substr(uint32_t start, uint32_t end) const
{
	tiny_string ret;
	//start and end are assumed to be valid
	assert(end < stringSize);
	int subSize=end-start+1;
	if(subSize > STATIC_SIZE)
		ret.createBuffer(subSize);
	strncpy(ret.buf,buf+start,end-start);
	ret.buf[end-start]=0;
	return ret;
}

tiny_string multiname::qualifiedString() const
{
	assert_and_throw(ns.size()==1);
	assert_and_throw(name_type==NAME_STRING);
	//TODO: what if the ns is empty
	if(false && ns[0].name=="")
		return name_s;
	else
	{
		tiny_string ret=ns[0].name;
		ret+="::";
		ret+=name_s;
		return ret;
	}
}

tiny_string multiname::normalizedName() const
{
	switch(name_type)
	{
		case multiname::NAME_INT:
			return tiny_string(name_i);
		case multiname::NAME_NUMBER:
			return tiny_string(name_d);
		case multiname::NAME_STRING:
			return name_s;
		case multiname::NAME_OBJECT:
			return name_o->toString();
		default:
			assert("Unexpected name kind" && false);
			//Should never reach this
			return "";
	}
}

std::ostream& lightspark::operator<<(std::ostream& s, const QName& r)
{
	s << r.ns << ':' << r.name;
	return s;
}

std::ostream& lightspark::operator<<(std::ostream& s, const tiny_string& r)
{
	s << r.buf;
	return s;
}

std::ostream& lightspark::operator<<(std::ostream& s, const multiname& r)
{
	for(unsigned int i=0;i<r.ns.size();i++)
	{
		string prefix;
		switch(r.ns[i].kind)
		{
			case 0x08:
				prefix="ns:";
				break;
			case 0x16:
				prefix="pakns:";
				break;
			case 0x17:
				prefix="pakintns:";
				break;
			case 0x18:
				prefix="protns:";
				break;
			case 0x19:
				prefix="explns:";
				break;
			case 0x1a:
				prefix="staticprotns:";
				break;
			case 0x05:
				prefix="privns:";
				break;
		}
		s << '[' << prefix << r.ns[i].name << "] ";
	}
	if(r.name_type==multiname::NAME_INT)
		s << r.name_i;
	else if(r.name_type==multiname::NAME_NUMBER)
		s << r.name_d;
	else if(r.name_type==multiname::NAME_STRING)
		s << r.name_s;
	else
		s << r.name_o; //We print the hexadecimal value
	return s;
}

lightspark::RECT::RECT()
{
}

lightspark::RECT::RECT(int a, int b, int c, int d):Xmin(a),Xmax(b),Ymin(c),Ymax(d)
{
}

std::ostream& lightspark::operator<<(std::ostream& s, const RECT& r)
{
	s << '{' << (int)r.Xmin << ',' << r.Xmax << ',' << r.Ymin << ',' << r.Ymax << '}';
	return s;
}

ostream& lightspark::operator<<(ostream& s, const STRING& t)
{
	for(unsigned int i=0;i<t.String.size();i++)
		s << t.String[i];
	return s;
}

std::ostream& lightspark::operator<<(std::ostream& s, const RGBA& r)
{
	s << "RGBA <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << ',' << (int)r.Alpha << '>';
	return s;
}

std::ostream& lightspark::operator<<(std::ostream& s, const RGB& r)
{
	s << "RGB <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << '>';
	return s;
}

MATRIX MATRIX::getInverted() const
{
	MATRIX ret;
	const number_t den=ScaleX*ScaleY+RotateSkew0*RotateSkew1;
	ret.ScaleX=ScaleY/den;
	ret.RotateSkew1=-RotateSkew1/den;
	ret.RotateSkew0=-RotateSkew0/den;
	ret.ScaleY=ScaleX/den;
	ret.TranslateX=(RotateSkew1*TranslateY-ScaleY*TranslateX)/den;
	ret.TranslateY=(RotateSkew0*TranslateX-ScaleX*TranslateY)/den;
	return ret;
}

void MATRIX::get4DMatrix(float matrix[16]) const
{
	memset(matrix,0,sizeof(float)*16);
	matrix[0]=ScaleX;
	matrix[1]=RotateSkew0;

	matrix[4]=RotateSkew1;
	matrix[5]=ScaleY;

	matrix[10]=1;

	matrix[12]=TranslateX;
	matrix[13]=TranslateY;
	matrix[15]=1;
}

void MATRIX::multiply2D(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	xout=xin*ScaleX + yin*RotateSkew1 + TranslateX;
	yout=xin*RotateSkew0 + yin*ScaleY + TranslateY;
}

MATRIX MATRIX::multiplyMatrix(const MATRIX& r) const
{
	MATRIX ret;
	ret.ScaleX=ScaleX*r.ScaleX + RotateSkew1*r.RotateSkew0;
	ret.RotateSkew1=ScaleX*r.RotateSkew1 + RotateSkew1*r.ScaleY;
	ret.RotateSkew0=RotateSkew0*r.ScaleX + ScaleY*r.RotateSkew0;
	ret.ScaleY=RotateSkew0*r.RotateSkew1 + ScaleY*r.ScaleY;
	ret.TranslateX=ScaleX*r.TranslateX + RotateSkew1*r.TranslateY + TranslateX;
	ret.TranslateY=RotateSkew0*r.TranslateX + ScaleY*r.TranslateY + TranslateY;
	return ret;
}

const bool MATRIX::operator!=(const MATRIX& r) const
{
	return ScaleX!=r.ScaleX ||
		RotateSkew1!=r.RotateSkew1 ||
		RotateSkew0!=r.RotateSkew0 ||
		ScaleY!=r.ScaleY ||
		TranslateX!=r.TranslateX ||
		TranslateY!=r.TranslateY;
}

std::ostream& lightspark::operator<<(std::ostream& s, const MATRIX& r)
{
	s << "| " << r.ScaleX << ' ' << r.RotateSkew0 << " |" << std::endl;
	s << "| " << r.RotateSkew1 << ' ' << r.ScaleY << " |" << std::endl;
	s << "| " << (int)r.TranslateX << ' ' << (int)r.TranslateY << " |" << std::endl;
	return s;
}

std::istream& lightspark::operator>>(std::istream& stream, STRING& v)
{
	v.String.clear();
	UI8 c;
	do
	{
		stream >> c;
		if(c==0)
			break;
		v.String.push_back(c);
	}
	while(c!=0);
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, RECT& v)
{
	BitStream s(stream);
	int nbits=UB(5,s);
	v.Xmin=SB(nbits,s);
	v.Xmax=SB(nbits,s);
	v.Ymin=SB(nbits,s);
	v.Ymax=SB(nbits,s);
	return stream;
}

std::istream& lightspark::operator>>(std::istream& s, RGB& v)
{
	s >> v.Red >> v.Green >> v.Blue;
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, RGBA& v)
{
	s >> v.Red >> v.Green >> v.Blue >> v.Alpha;
	return s;
}

void LINESTYLEARRAY::appendStyles(const LINESTYLEARRAY& r)
{
	unsigned int count = LineStyleCount + r.LineStyleCount;
	assert(version!=-1);

	assert_and_throw(r.version==version);
	if(version<4)
		LineStyles.insert(LineStyles.end(),r.LineStyles.begin(),r.LineStyles.end());
	else
		LineStyles2.insert(LineStyles2.end(),r.LineStyles2.begin(),r.LineStyles2.end());
	LineStyleCount = count;
}

std::istream& lightspark::operator>>(std::istream& s, LINESTYLEARRAY& v)
{
	assert(v.version!=-1);
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		LOG(LOG_ERROR,_("Line array extended not supported"));
	if(v.version<4)
	{
		for(int i=0;i<v.LineStyleCount;i++)
		{
			LINESTYLE tmp(v.version);
			s >> tmp;
			v.LineStyles.push_back(tmp);
		}
	}
	else
	{
		for(int i=0;i<v.LineStyleCount;i++)
		{
			LINESTYLE2 tmp(v.version);
			s >> tmp;
			v.LineStyles2.push_back(tmp);
		}
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, MORPHLINESTYLEARRAY& v)
{
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		LOG(LOG_ERROR,_("Line array extended not supported"));
	v.LineStyles=new MORPHLINESTYLE[v.LineStyleCount];
	for(int i=0;i<v.LineStyleCount;i++)
	{
		s >> v.LineStyles[i];
	}
	return s;
}

void FILLSTYLEARRAY::appendStyles(const FILLSTYLEARRAY& r)
{
	assert(version!=-1);
	unsigned int count = FillStyleCount + r.FillStyleCount;

	FillStyles.insert(FillStyles.end(),r.FillStyles.begin(),r.FillStyles.end());
	FillStyleCount = count;
}

std::istream& lightspark::operator>>(std::istream& s, FILLSTYLEARRAY& v)
{
	assert(v.version!=-1);
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(LOG_ERROR,_("Fill array extended not supported"));

	for(int i=0;i<v.FillStyleCount;i++)
	{
		FILLSTYLE t(v.version);
		s >> t;
		v.FillStyles.push_back(t);
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, MORPHFILLSTYLEARRAY& v)
{
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(LOG_ERROR,_("Fill array extended not supported"));
	v.FillStyles=new MORPHFILLSTYLE[v.FillStyleCount];
	for(int i=0;i<v.FillStyleCount;i++)
	{
		s >> v.FillStyles[i];
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, SHAPE& v)
{
	BitStream bs(s);
	v.NumFillBits=UB(4,bs);
	v.NumLineBits=UB(4,bs);
	do
	{
		v.ShapeRecords.push_back(SHAPERECORD(&v,bs));
	}
	while(v.ShapeRecords.back().TypeFlag || v.ShapeRecords.back().StateNewStyles || v.ShapeRecords.back().StateLineStyle || 
			v.ShapeRecords.back().StateFillStyle1 || v.ShapeRecords.back().StateFillStyle0 || 
			v.ShapeRecords.back().StateMoveTo);
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, SHAPEWITHSTYLE& v)
{
	v.FillStyles.version=v.version;
	v.LineStyles.version=v.version;
	s >> v.FillStyles >> v.LineStyles;
	BitStream bs(s);
	v.NumFillBits=UB(4,bs);
	v.NumLineBits=UB(4,bs);
	do
	{
		v.ShapeRecords.push_back(SHAPERECORD(&v,bs));
	}
	while(v.ShapeRecords.back().TypeFlag || v.ShapeRecords.back().StateNewStyles || v.ShapeRecords.back().StateLineStyle || 
			v.ShapeRecords.back().StateFillStyle1 || v.ShapeRecords.back().StateFillStyle0 || 
			v.ShapeRecords.back().StateMoveTo);
	return s;
}

istream& lightspark::operator>>(istream& s, LINESTYLE2& v)
{
	s >> v.Width;
	BitStream bs(s);
	v.StartCapStyle=UB(2,bs);
	v.JointStyle=UB(2,bs);
	v.HasFillFlag=UB(1,bs);
	v.NoHScaleFlag=UB(1,bs);
	v.NoVScaleFlag=UB(1,bs);
	v.PixelHintingFlag=UB(1,bs);
	UB(5,bs);
	v.NoClose=UB(1,bs);
	v.EndCapStyle=UB(2,bs);
	if(v.JointStyle==2)
		s >> v.MiterLimitFactor;
	if(v.HasFillFlag)
		s >> v.FillType;
	else
		s >> v.Color;

	return s;
}

istream& lightspark::operator>>(istream& s, LINESTYLE& v)
{
	s >> v.Width;
	if(v.version==2 || v.version==1)
	{
		RGB tmp;
		s >> tmp;
		v.Color=tmp;
	}
	else
		s >> v.Color;
	return s;
}

istream& lightspark::operator>>(istream& s, MORPHLINESTYLE& v)
{
	s >> v.StartWidth >> v.EndWidth >> v.StartColor >> v.EndColor;
	return s;
}

std::istream& lightspark::operator>>(std::istream& in, TEXTRECORD& v)
{
	BitStream bs(in);
	v.TextRecordType=UB(1,bs);
	v.StyleFlagsReserved=UB(3,bs);
	if(v.StyleFlagsReserved)
		LOG(LOG_ERROR,_("Reserved bits not so reserved"));
	v.StyleFlagsHasFont=UB(1,bs);
	v.StyleFlagsHasColor=UB(1,bs);
	v.StyleFlagsHasYOffset=UB(1,bs);
	v.StyleFlagsHasXOffset=UB(1,bs);
	if(!v.TextRecordType)
		return in;
	if(v.StyleFlagsHasFont)
		in >> v.FontID;
	if(v.StyleFlagsHasColor)
	{
		if(v.parent->version==1)
		{
			RGB t;
			in >> t;
			v.TextColor=t;
		}
		else if(v.parent->version==2)
		{
			RGBA t;
			in >> t;
			v.TextColor=t;
		}
		else
			assert(false);
	}
	if(v.StyleFlagsHasXOffset)
		in >> v.XOffset;
	if(v.StyleFlagsHasYOffset)
		in >> v.YOffset;
	if(v.StyleFlagsHasFont)
		in >> v.TextHeight;
	in >> v.GlyphCount;
	v.GlyphEntries.clear();
	for(int i=0;i<v.GlyphCount;i++)
	{
		v.GlyphEntries.push_back(GLYPHENTRY(&v,bs));
	}

	return in;
}

std::istream& lightspark::operator>>(std::istream& s, GRADRECORD& v)
{
	s >> v.Ratio;
	if(v.version==1 || v.version==2)
	{
		RGB tmp;
		s >> tmp;
		v.Color=tmp;
	}
	else
		s >> v.Color;

	return s;
}

std::istream& lightspark::operator>>(std::istream& s, GRADIENT& v)
{
	BitStream bs(s);
	v.SpreadMode=UB(2,bs);
	v.InterpolationMode=UB(2,bs);
	v.NumGradient=UB(4,bs);
	GRADRECORD gr(v.version);
	for(int i=0;i<v.NumGradient;i++)
	{
		s >> gr;
		v.GradientRecords.push_back(gr);
	}
	sort(v.GradientRecords.begin(),v.GradientRecords.end());
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, FOCALGRADIENT& v)
{
	BitStream bs(s);
	v.SpreadMode=UB(2,bs);
	v.InterpolationMode=UB(2,bs);
	v.NumGradient=UB(4,bs);
	GRADRECORD gr(v.version);
	for(int i=0;i<v.NumGradient;i++)
	{
		s >> gr;
		v.GradientRecords.push_back(gr);
	}
	sort(v.GradientRecords.begin(),v.GradientRecords.end());
	//TODO: support FocalPoint
	s.read((char*)&v.FocalPoint,2);
	return s;
}

inline RGBA medianColor(const RGBA& a, const RGBA& b, float factor)
{
	return RGBA(a.Red+(b.Red-a.Red)*factor,
		a.Green+(b.Green-a.Green)*factor,
		a.Blue+(b.Blue-a.Blue)*factor,
		a.Alpha+(b.Alpha-a.Alpha)*factor);
}

std::istream& lightspark::operator>>(std::istream& s, FILLSTYLE& v)
{
	UI8 tmp;
	s >> tmp;
	v.FillStyleType=(FILL_STYLE_TYPE)(int)tmp;
	if(v.FillStyleType==SOLID_FILL)
	{
		if(v.version==1 || v.version==2)
		{
			RGB tmp;
			s >> tmp;
			v.Color=tmp;
		}
		else
			s >> v.Color;
	}
	else if(v.FillStyleType==LINEAR_GRADIENT || v.FillStyleType==RADIAL_GRADIENT || v.FillStyleType==FOCAL_RADIAL_GRADIENT)
	{
		s >> v.Matrix;
		v.FocalGradient.version=v.version;
		if(v.FillStyleType==FOCAL_RADIAL_GRADIENT)
			s >> v.FocalGradient;
		else
			s >> v.Gradient;
	}
	else if(v.FillStyleType==REPEATING_BITMAP || v.FillStyleType==CLIPPED_BITMAP || v.FillStyleType==NON_SMOOTHED_REPEATING_BITMAP || 
			v.FillStyleType==NON_SMOOTHED_CLIPPED_BITMAP)
	{
		UI16_SWF bitmapId;
		s >> bitmapId >> v.Matrix;
		//Lookup the bitmap in the dictionary
		if(bitmapId!=65535)
		{
			try
			{
				DictionaryTag* dict=pt->root->dictionaryLookup(bitmapId);
				v.bitmap=dynamic_cast<Bitmap*>(dict);
				if(v.bitmap==NULL)
				{
					LOG(LOG_ERROR,"Invalid bitmap ID " << bitmapId);
					throw ParseException("Invalid ID for bitmap");
				}
			}
			catch(RunTimeException& e)
			{
				//Thrown if the bitmapId does not exists in dictionary
				LOG(LOG_ERROR,"Exception in FillStyle parsing: " << e.what());
				v.bitmap=NULL;
			}
		}
		else
		{
			//The bitmap might be invalid, the style should not be used
			v.bitmap=NULL;
		}
	}
	else
	{
		LOG(LOG_ERROR,_("Not supported fill style ") << (int)v.FillStyleType);
		throw ParseException("Not supported fill style");
	}
	return s;
}


std::istream& lightspark::operator>>(std::istream& s, MORPHFILLSTYLE& v)
{
	UI8 tmp;
	s >> tmp;
	v.FillStyleType=(FILL_STYLE_TYPE)(int)tmp;
	if(v.FillStyleType==0x00)
	{
		s >> v.StartColor >> v.EndColor;
	}
	else if(v.FillStyleType==0x10 || v.FillStyleType==0x12)
	{
		s >> v.StartGradientMatrix >> v.EndGradientMatrix;
		s >> v.NumGradients;
		UI8 t;
		RGBA t2;
		for(int i=0;i<v.NumGradients;i++)
		{
			s >> t >> t2;
			v.StartRatios.push_back(t);
			v.StartColors.push_back(t2);
			s >> t >> t2;
			v.EndRatios.push_back(t);
			v.EndColors.push_back(t2);
		}
	}
	else
	{
		LOG(LOG_ERROR,_("Not supported fill style ") << (int)v.FillStyleType << _("... Aborting"));
	}
	return s;
}

GLYPHENTRY::GLYPHENTRY(TEXTRECORD* p,BitStream& bs):parent(p)
{
	GlyphIndex = UB(parent->parent->GlyphBits,bs);
	GlyphAdvance = SB(parent->parent->AdvanceBits,bs);
}

SHAPERECORD::SHAPERECORD(SHAPE* p,BitStream& bs):parent(p),TypeFlag(false),StateNewStyles(false),StateLineStyle(false),StateFillStyle1(false),
	StateFillStyle0(false),StateMoveTo(false),MoveDeltaX(0),MoveDeltaY(0),DeltaX(0),DeltaY(0)
{
	TypeFlag = UB(1,bs);
	if(TypeFlag)
	{
		StraightFlag=UB(1,bs);
		NumBits=UB(4,bs);
		if(StraightFlag)
		{

			GeneralLineFlag=UB(1,bs);
			if(!GeneralLineFlag)
				VertLineFlag=UB(1,bs);

			if(GeneralLineFlag || !VertLineFlag)
			{
				DeltaX=SB(NumBits+2,bs);
			}
			if(GeneralLineFlag || VertLineFlag)
			{
				DeltaY=SB(NumBits+2,bs);
			}
		}
		else
		{
			
			ControlDeltaX=SB(NumBits+2,bs);
			ControlDeltaY=SB(NumBits+2,bs);
			AnchorDeltaX=SB(NumBits+2,bs);
			AnchorDeltaY=SB(NumBits+2,bs);
			
		}
	}
	else
	{
		StateNewStyles = UB(1,bs);
		StateLineStyle = UB(1,bs);
		StateFillStyle1 = UB(1,bs);
		StateFillStyle0 = UB(1,bs);
		StateMoveTo = UB(1,bs);
		if(StateMoveTo)
		{
			MoveBits = UB(5,bs);
			MoveDeltaX = SB(MoveBits,bs);
			MoveDeltaY = SB(MoveBits,bs);
		}
		if(StateFillStyle0)
		{
			FillStyle0=UB(parent->NumFillBits,bs)+p->fillOffset;
		}
		if(StateFillStyle1)
		{
			FillStyle1=UB(parent->NumFillBits,bs)+p->fillOffset;
		}
		if(StateLineStyle)
		{
			LineStyle=UB(parent->NumLineBits,bs)+p->lineOffset;
		}
		if(StateNewStyles)
		{
			SHAPEWITHSTYLE* ps=dynamic_cast<SHAPEWITHSTYLE*>(parent);
			if(ps==NULL)
				throw ParseException("Malformed SWF file");
			bs.pos=0;
			FILLSTYLEARRAY a(ps->FillStyles.version);
			bs.f >> a;
			p->fillOffset=ps->FillStyles.FillStyleCount;
			ps->FillStyles.appendStyles(a);

			LINESTYLEARRAY b(ps->LineStyles.version);
			bs.f >> b;
			p->lineOffset=ps->LineStyles.LineStyleCount;
			ps->LineStyles.appendStyles(b);

			parent->NumFillBits=UB(4,bs);
			parent->NumLineBits=UB(4,bs);
		}
	}
}

std::istream& lightspark::operator>>(std::istream& stream, CXFORMWITHALPHA& v)
{
	BitStream bs(stream);
	v.HasAddTerms=UB(1,bs);
	v.HasMultTerms=UB(1,bs);
	v.NBits=UB(4,bs);
	if(v.HasMultTerms)
	{
		v.RedMultTerm=SB(v.NBits,bs);
		v.GreenMultTerm=SB(v.NBits,bs);
		v.BlueMultTerm=SB(v.NBits,bs);
		v.AlphaMultTerm=SB(v.NBits,bs);
	}
	if(v.HasAddTerms)
	{
		v.RedAddTerm=SB(v.NBits,bs);
		v.GreenAddTerm=SB(v.NBits,bs);
		v.BlueAddTerm=SB(v.NBits,bs);
		v.AlphaAddTerm=SB(v.NBits,bs);
	}
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, MATRIX& v)
{
	BitStream bs(stream);
	int HasScale=UB(1,bs);
	if(HasScale)
	{
		int NScaleBits=UB(5,bs);
		v.ScaleX=FB(NScaleBits,bs);
		v.ScaleY=FB(NScaleBits,bs);
	}
	int HasRotate=UB(1,bs);
	if(HasRotate)
	{
		int NRotateBits=UB(5,bs);
		v.RotateSkew0=FB(NRotateBits,bs);
		v.RotateSkew1=FB(NRotateBits,bs);
	}
	int NTranslateBits=UB(5,bs);
	v.TranslateX=SB(NTranslateBits,bs)/20;
	v.TranslateY=SB(NTranslateBits,bs)/20;
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, BUTTONRECORD& v)
{
	assert_and_throw(v.buttonVersion==2);
	BitStream bs(stream);

	UB(2,bs);
	v.ButtonHasBlendMode=UB(1,bs);
	v.ButtonHasFilterList=UB(1,bs);
	v.ButtonStateHitTest=UB(1,bs);
	v.ButtonStateDown=UB(1,bs);
	v.ButtonStateOver=UB(1,bs);
	v.ButtonStateUp=UB(1,bs);

	if(v.isNull())
		return stream;

	stream >> v.CharacterID >> v.PlaceDepth >> v.PlaceMatrix >> v.ColorTransform;

	if(v.ButtonHasFilterList)
		stream >> v.FilterList;

	if(v.ButtonHasBlendMode)
		stream >> v.BlendMode;

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, FILTERLIST& v)
{
	stream >> v.NumberOfFilters;
	v.Filters.resize(v.NumberOfFilters);

	for(int i=0;i<v.NumberOfFilters;i++)
		stream >> v.Filters[i];
	
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, FILTER& v)
{
	stream >> v.FilterID;
	switch(v.FilterID)
	{
		case 0:
			stream >> v.DropShadowFilter;
			break;
		case 1:
			stream >> v.BlurFilter;
			break;
		case 2:
			stream >> v.GlowFilter;
			break;
		case 3:
			stream >> v.BevelFilter;
			break;
		case 4:
			stream >> v.GradientGlowFilter;
			break;
		case 5:
			stream >> v.ConvolutionFilter;
			break;
		case 6:
			stream >> v.ColorMatrixFilter;
			break;
		case 7:
			stream >> v.GradientBevelFilter;
			break;
		default:
			LOG(LOG_ERROR,_("Unsupported Filter Id ") << (int)v.FilterID);
			throw ParseException("Unsupported Filter Id");
	}
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, GLOWFILTER& v)
{
	stream >> v.GlowColor;
	stream >> v.BlurX;
	stream >> v.BlurY;
	stream >> v.Strength;
	BitStream bs(stream);
	v.InnerGlow = UB(1,bs);
	v.Knockout = UB(1,bs);
	v.CompositeSource = UB(1,bs);
	UB(5,bs);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, DROPSHADOWFILTER& v)
{
	stream >> v.DropShadowColor;
	stream >> v.BlurX;
	stream >> v.BlurY;
	stream >> v.Angle;
	stream >> v.Distance;
	stream >> v.Strength;
	BitStream bs(stream);
	v.InnerShadow = UB(1,bs);
	v.Knockout = UB(1,bs);
	v.CompositeSource = UB(1,bs);
	UB(5,bs);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, BLURFILTER& v)
{
	stream >> v.BlurX;
	stream >> v.BlurY;
	BitStream bs(stream);
	v.Passes = UB(5,bs);
	UB(3,bs);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, BEVELFILTER& v)
{
	stream >> v.ShadowColor;
	stream >> v.HighlightColor;
	stream >> v.BlurX;
	stream >> v.BlurY;
	stream >> v.Angle;
	stream >> v.Distance;
	stream >> v.Strength;
	BitStream bs(stream);
	v.InnerShadow = UB(1,bs);
	v.Knockout = UB(1,bs);
	v.CompositeSource = UB(1,bs);
	v.OnTop = UB(1,bs);
	UB(4,bs);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, GRADIENTGLOWFILTER& v)
{
	stream >> v.NumColors;
	for(int i = 0; i < v.NumColors; i++)
	{
		RGBA color;
		stream >> color;
		v.GradientColors.push_back(color);
	}
	for(int i = 0; i < v.NumColors; i++)
	{
		UI8 ratio;
		stream >> ratio;
		v.GradientRatio.push_back(ratio);
	}
	stream >> v.BlurX;
	stream >> v.BlurY;
	stream >> v.Strength;
	BitStream bs(stream);
	v.InnerGlow = UB(1,bs);
	v.Knockout = UB(1,bs);
	v.CompositeSource = UB(1,bs);
	UB(5,bs);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, CONVOLUTIONFILTER& v)
{
	stream >> v.MatrixX;
	stream >> v.MatrixY;
	stream >> v.Divisor;
	stream >> v.Bias;
	for(int i = 0; i < v.MatrixX * v.MatrixY; i++)
	{
		FLOAT f;
		stream >> f;
		v.Matrix.push_back(f);
	}
	stream >> v.DefaultColor;
	BitStream bs(stream);
	v.Clamp = UB(1,bs);
	v.PreserveAlpha = UB(1,bs);
	UB(6,bs);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, COLORMATRIXFILTER& v)
{
	for (int i = 0; i < 20; i++)
		stream >> v.Matrix[i];

    return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, GRADIENTBEVELFILTER& v)
{
	stream >> v.NumColors;
	for(int i = 0; i < v.NumColors; i++)
	{
		RGBA color;
		stream >> color;
		v.GradientColors.push_back(color);
	}
	for(int i = 0; i < v.NumColors; i++)
	{
		UI8 ratio;
		stream >> ratio;
		v.GradientRatio.push_back(ratio);
	}
	stream >> v.BlurX;
	stream >> v.BlurY;
	stream >> v.Angle;
	stream >> v.Distance;
	stream >> v.Strength;
	BitStream bs(stream);
	v.InnerShadow = UB(1,bs);
	v.Knockout = UB(1,bs);
	v.CompositeSource = UB(1,bs);
	v.OnTop = UB(1,bs);
	UB(4,bs);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& s, CLIPEVENTFLAGS& v)
{
	if(pt->version<=5)
	{
		UI16_SWF t;
		s >> t;
		v.toParse=t;
	}
	else
	{
		UI32_SWF t;
		s >> t;
		v.toParse=t;
	}
	return s;
}

bool CLIPEVENTFLAGS::isNull()
{
	return toParse==0;
}

std::istream& lightspark::operator>>(std::istream& s, CLIPACTIONRECORD& v)
{
	s >> v.EventFlags;
	if(v.EventFlags.isNull())
		return s;
	s >> v.ActionRecordSize;
	LOG(LOG_NOT_IMPLEMENTED,_("Skipping ") << v.ActionRecordSize << _(" of action data"));
	ignore(s,v.ActionRecordSize);
	return s;
}

bool CLIPACTIONRECORD::isLast()
{
	return EventFlags.isNull();
}

std::istream& lightspark::operator>>(std::istream& s, CLIPACTIONS& v)
{
	s >> v.Reserved >> v.AllEventFlags;
	while(1)
	{
		CLIPACTIONRECORD t;
		s >> t;
		if(t.isLast())
			break;
		v.ClipActionRecords.push_back(t);
	}
	return s;
}

ASObject* lightspark::abstract_d(number_t i)
{
	Number* ret=getVm()->number_manager->get<Number>();
	ret->val=i;
	return ret;
}

ASObject* lightspark::abstract_b(bool i)
{
	return Class<Boolean>::getInstanceS(i);
}

ASObject* lightspark::abstract_i(intptr_t i)
{
	Integer* ret=getVm()->int_manager->get<Integer>();
	ret->val=i;
	return ret;
}

void lightspark::stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns)
{
	//Ok, let's split our string into namespace and name part
	for(int i=tmp.len()-1;i>0;i--)
	{
		if(tmp[i]==':')
		{
			assert_and_throw(tmp[i-1]==':');
			ns=tmp.substr(0,i-1);
			name=tmp.substr(i+1,tmp.len());
			return;
		}
	}
	// No namespace, look for a package name
	for(int i=tmp.len()-1;i>0;i--)
	{
		if(tmp[i]=='.')
		{
			ns=tmp.substr(0,i);
			name=tmp.substr(i+1,tmp.len());
			return;
		}
	}
	//No namespace or package in the string
	name=tmp;
	ns="";
}

RunState::RunState():last_FP(-1),FP(0),next_FP(0),stop_FP(0),explicit_FP(false)
{
}

