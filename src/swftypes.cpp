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

#define GL_GLEXT_PROTOTYPES

#include "swftypes.h"
#include "scripting/abc.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>
#include <cmath>
#include <cairo.h>

#include "exceptions.h"
#include "compat.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/flash/display/BitmapData.h"

using namespace std;
using namespace lightspark;

multiname::multiname(MemoryAccount* m):name_o(NULL),ns(reporter_allocator<nsNameAndKind>(m)),name_type(NAME_OBJECT),isAttribute(false)
{
}

tiny_string multiname::qualifiedString() const
{
	assert_and_throw(ns.size()==1);
	assert_and_throw(name_type==NAME_STRING);
	const tiny_string nsName=ns[0].getImpl().name;
	const tiny_string& name=getSys()->getStringFromUniqueId(name_s_id);
	if(nsName.empty())
		return name;
	else
	{
		tiny_string ret=nsName;
		ret+="::";
		ret+=name;
		return ret;
	}
}

tiny_string multiname::normalizedName() const
{
	switch(name_type)
	{
		case multiname::NAME_INT:
			return Integer::toString(name_i);
		case multiname::NAME_NUMBER:
			return Number::toString(name_d);
		case multiname::NAME_STRING:
			return getSys()->getStringFromUniqueId(name_s_id);
		case multiname::NAME_OBJECT:
			return name_o->toString();
		default:
			assert("Unexpected name kind" && false);
			//Should never reach this
			return "";
	}
}

uint32_t multiname::normalizedNameId() const
{
	switch(name_type)
	{
		case multiname::NAME_STRING:
			return name_s_id;
		case multiname::NAME_INT:
		case multiname::NAME_NUMBER:
		case multiname::NAME_OBJECT:
			return getSys()->getUniqueStringId(normalizedName());
		default:
			assert("Unexpected name kind" && false);
			//Should never reach this
			return -1;
	}
}

void multiname::setName(ASObject* n)
{
	if (name_type==NAME_OBJECT && name_o!=NULL) {
		name_o->decRef();
		name_o = NULL;
	}

	if(n->is<Integer>())
	{
		name_i=n->as<Integer>()->val;
		name_type = NAME_INT;
	}
	else if(n->is<UInteger>())
	{
		name_i=n->as<UInteger>()->val;
		name_type = NAME_INT;
	}
	else if(n->is<Number>())
	{
		name_d=n->as<Number>()->val;
		name_type = NAME_NUMBER;
	}
	else if(n->getObjectType()==T_QNAME)
	{
		ASQName* qname=static_cast<ASQName*>(n);
		name_s_id=getSys()->getUniqueStringId(qname->local_name);
		name_type = NAME_STRING;
	}
	else if(n->getObjectType()==T_STRING)
	{
		ASString* o=static_cast<ASString*>(n);
		name_s_id=getSys()->getUniqueStringId(o->data);
		name_type = NAME_STRING;
	}
	else
	{
		n->incRef();
		name_o=n;
		name_type = NAME_OBJECT;
	}
}

void multiname::resetNameIfObject()
{
	if(name_type==NAME_OBJECT && name_o)
	{
		name_o->decRef();
		name_o=NULL;
	}
}

bool multiname::toUInt(uint32_t& index) const
{
	switch(name_type)
	{
		//We try to convert this to an index, otherwise bail out
		case multiname::NAME_STRING:
		case multiname::NAME_OBJECT:
		{
			tiny_string str;
			if(name_type==multiname::NAME_STRING)
				str=getSys()->getStringFromUniqueId(name_s_id);
			else
				str=name_o->toString();

			if(str.empty())
				return false;
			index=0;
			for(auto i=str.begin(); i!=str.end(); ++i)
			{
				if(!i.isdigit())
					return false;

				index*=10;
				index+=i.digit_value();
			}
			break;
		}
		//This is already an int, so its good enough
		case multiname::NAME_INT:
			if(name_i < 0)
				return false;
			index=name_i;
			break;
		case multiname::NAME_NUMBER:
			if(!Number::isInteger(name_d) || name_d < 0)
				return false;
			index=name_d;
			break;
		default:
			// Not reached
			assert(false);
	}

	return true;
}

std::ostream& lightspark::operator<<(std::ostream& s, const QName& r)
{
	s << r.ns << ':' << r.name;
	return s;
}

std::ostream& lightspark::operator<<(std::ostream& s, const tiny_string& r)
{
	//s << r.buf would stop at the first '\0'
	s << std::string(r.buf,r.numBytes());
	return s;
}

std::ostream& lightspark::operator<<(std::ostream& s, const nsNameAndKind& r)
{
	const char* prefix;
	switch(r.getImpl().kind)
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
	s << prefix << r.getImpl().name;
	return s;
}

std::ostream& lightspark::operator<<(std::ostream& s, const multiname& r)
{
	for(unsigned int i=0;i<r.ns.size();i++)
	{
		s << '[' << r.ns[i] << "] ";
	}
	if(r.name_type==multiname::NAME_INT)
		s << r.name_i;
	else if(r.name_type==multiname::NAME_NUMBER)
		s << r.name_d;
	else if(r.name_type==multiname::NAME_STRING)
		s << getSys()->getStringFromUniqueId(r.name_s_id);
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

MATRIX::MATRIX(number_t sx, number_t sy, number_t sk0, number_t sk1, number_t tx, number_t ty)
{
	xx=sx;
	yy=sy;
	yx=sk0;
	xy=sk1;
	x0=tx;
	y0=ty;
}

MATRIX MATRIX::getInverted() const
{
	MATRIX ret(*this);
	cairo_status_t status=cairo_matrix_invert(&ret);
	if(status==CAIRO_STATUS_INVALID_MATRIX)
	{
		//Flash does not fail for non invertible matrices
		//but the result contains a few NaN and Infinite.
		//Just fill the result with NaN
		ret.xx=numeric_limits<double>::quiet_NaN();
		ret.yx=numeric_limits<double>::quiet_NaN();
		ret.xy=numeric_limits<double>::quiet_NaN();
		ret.yy=numeric_limits<double>::quiet_NaN();
		ret.x0=numeric_limits<double>::quiet_NaN();
		ret.y0=numeric_limits<double>::quiet_NaN();
	}
	return ret;
}

bool MATRIX::isInvertible() const
{
	const number_t den=xx*yy-yx*xy;
	return (fabs(den) > 1e-6);
}

void MATRIX::get4DMatrix(float matrix[16]) const
{
	memset(matrix,0,sizeof(float)*16);
	matrix[0]=xx;
	matrix[1]=yx;

	matrix[4]=xy;
	matrix[5]=yy;

	matrix[10]=1;

	matrix[12]=x0;
	matrix[13]=y0;
	matrix[15]=1;
}

Vector2f MATRIX::multiply2D(const Vector2f& in) const
{
	Vector2f out;
	multiply2D(in.x,in.y,out.x,out.y);
	return out;
}

void MATRIX::multiply2D(number_t xin, number_t yin, number_t& xout, number_t& yout) const
{
	xout=xin;
	yout=yin;
	cairo_matrix_transform_point(this, &xout, &yout);
}

MATRIX MATRIX::multiplyMatrix(const MATRIX& r) const
{
	MATRIX ret;
	//Do post multiplication
	cairo_matrix_multiply(&ret,&r,this);
	return ret;
}

bool MATRIX::operator!=(const MATRIX& r) const
{
	return xx!=r.xx || yx!=r.yx || xy!=r.xy || yy!=r.yy ||
		x0!=r.x0 || y0!=r.y0;
}

std::ostream& lightspark::operator<<(std::ostream& s, const MATRIX& r)
{
	s << "| " << r.xx << ' ' << r.yx << " |" << std::endl;
	s << "| " << r.xy << ' ' << r.yy << " |" << std::endl;
	s << "| " << (int)r.x0 << ' ' << (int)r.y0 << " |" << std::endl;
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
	assert(version!=0xff);

	assert_and_throw(r.version==version);
	if(version<4)
		LineStyles.insert(LineStyles.end(),r.LineStyles.begin(),r.LineStyles.end());
	else
		LineStyles2.insert(LineStyles2.end(),r.LineStyles2.begin(),r.LineStyles2.end());
}

std::istream& lightspark::operator>>(std::istream& s, LINESTYLEARRAY& v)
{
	assert(v.version!=0xff);
	UI8 LineStyleCount;
	s >> LineStyleCount;
	if(LineStyleCount==0xff)
		LOG(LOG_ERROR,_("Line array extended not supported"));
	if(v.version<4)
	{
		for(int i=0;i<LineStyleCount;i++)
		{
			LINESTYLE tmp(v.version);
			s >> tmp;
			v.LineStyles.push_back(tmp);
		}
	}
	else
	{
		for(int i=0;i<LineStyleCount;i++)
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
	UI8 LineStyleCount;
	s >> LineStyleCount;
	if(LineStyleCount==0xff)
		LOG(LOG_ERROR,_("Line array extended not supported"));
	assert(v.version==1 || v.version==2);
	if(v.version==1)
	{
		for(int i=0;i<LineStyleCount;i++)
		{
			MORPHLINESTYLE t;
			s >> t;
			v.LineStyles.emplace_back(t);
		}
	}
	else
	{
		for(int i=0;i<LineStyleCount;i++)
		{
			MORPHLINESTYLE2 t;
			s >> t;
			v.LineStyles2.emplace_back(t);
		}
	}
	return s;
}

void FILLSTYLEARRAY::appendStyles(const FILLSTYLEARRAY& r)
{
	assert(version!=0xff);

	FillStyles.insert(FillStyles.end(),r.FillStyles.begin(),r.FillStyles.end());
}

std::istream& lightspark::operator>>(std::istream& s, FILLSTYLEARRAY& v)
{
	assert(v.version!=0xff);
	UI8 FillStyleCount;
	s >> FillStyleCount;
	if(FillStyleCount==0xff)
		LOG(LOG_ERROR,_("Fill array extended not supported"));

	for(int i=0;i<FillStyleCount;i++)
	{
		FILLSTYLE t(v.version);
		s >> t;
		v.FillStyles.push_back(t);
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, MORPHFILLSTYLEARRAY& v)
{
	UI8 FillStyleCount;
	s >> FillStyleCount;
	if(FillStyleCount==0xff)
		LOG(LOG_ERROR,_("Fill array extended not supported"));
	for(int i=0;i<FillStyleCount;i++)
	{
		MORPHFILLSTYLE t;
		s >> t;
		v.FillStyles.emplace_back(t);
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
	bs.discard(5);
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

istream& lightspark::operator>>(istream& s, MORPHLINESTYLE2& v)
{
	s >> v.StartWidth >> v.EndWidth;
	BitStream bs(s);
	v.StartCapStyle = UB(2,bs);
	v.JoinStyle = UB(2,bs);
	v.HasFillFlag = UB(1,bs);
	v.NoHScaleFlag = UB(1,bs);
	v.NoVScaleFlag = UB(1,bs);
	v.PixelHintingFlag = UB(1,bs);
	bs.discard(5);
	v.NoClose = UB(1,bs);
	v.EndCapStyle = UB(2,bs);
	if(v.JoinStyle==2)
		s >> v.MiterLimitFactor;
	if(v.HasFillFlag==0)
		s >> v.StartColor >> v.EndColor;
	else
		s >> v.FillType;
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
	UI8 GlyphCount;
	in >> GlyphCount;
	v.GlyphEntries.clear();
	for(int i=0;i<GlyphCount;i++)
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
	int NumGradient=UB(4,bs);
	GRADRECORD gr(v.version);
	for(int i=0;i<NumGradient;i++)
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
				const DictionaryTag* dict=getParseThread()->getRootMovie()->dictionaryLookup(bitmapId);
				const BitmapTag* b = dynamic_cast<const BitmapTag*>(dict);
				if(!b)
				{
					LOG(LOG_ERROR,"Invalid bitmap ID " << bitmapId);
					throw ParseException("Invalid ID for bitmap");
				}
				v.bitmap = *b;
			}
			catch(RunTimeException& e)
			{
				//Thrown if the bitmapId does not exists in dictionary
				LOG(LOG_ERROR,"Exception in FillStyle parsing: " << e.what());
				v.bitmap.reset();
			}
		}
		else
		{
			//The bitmap might be invalid, the style should not be used
			v.bitmap.reset();
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
	if(v.FillStyleType==SOLID_FILL)
	{
		s >> v.StartColor >> v.EndColor;
	}
	else if(v.FillStyleType==LINEAR_GRADIENT || v.FillStyleType==RADIAL_GRADIENT)
	{
		s >> v.StartGradientMatrix >> v.EndGradientMatrix;
		UI8 NumGradients;
		s >> NumGradients;
		UI8 t;
		RGBA t2;
		for(int i=0;i<NumGradients;i++)
		{
			s >> t >> t2;
			v.StartRatios.push_back(t);
			v.StartColors.push_back(t2);
			s >> t >> t2;
			v.EndRatios.push_back(t);
			v.EndColors.push_back(t2);
		}
	}
	else if(v.FillStyleType==REPEATING_BITMAP
		|| v.FillStyleType==CLIPPED_BITMAP
		|| v.FillStyleType==NON_SMOOTHED_REPEATING_BITMAP
		|| v.FillStyleType==NON_SMOOTHED_CLIPPED_BITMAP)
	{
		UI16_SWF bitmapId;
		s >> bitmapId >> v.StartBitmapMatrix >> v.EndBitmapMatrix;
	}
	else
	{
		LOG(LOG_ERROR,_("Not supported fill style 0x") << hex << (int)v.FillStyleType << dec << _("... Aborting"));
	}
	return s;
}

GLYPHENTRY::GLYPHENTRY(TEXTRECORD* p,BitStream& bs):parent(p)
{
	GlyphIndex = UB(parent->parent->GlyphBits,bs);
	GlyphAdvance = SB(parent->parent->AdvanceBits,bs);
}

SHAPERECORD::SHAPERECORD(SHAPE* p,BitStream& bs):parent(p),MoveDeltaX(0),MoveDeltaY(0),DeltaX(0),DeltaY(0),TypeFlag(false),StateNewStyles(false),StateLineStyle(false),StateFillStyle1(false),StateFillStyle0(false),StateMoveTo(false)
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
			p->fillOffset=ps->FillStyles.FillStyles.size();
			ps->FillStyles.appendStyles(a);

			LINESTYLEARRAY b(ps->LineStyles.version);
			bs.f >> b;
			p->lineOffset=ps->LineStyles.LineStyles.size();
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
		v.xx=FB(NScaleBits,bs);
		v.yy=FB(NScaleBits,bs);
	}
	int HasRotate=UB(1,bs);
	if(HasRotate)
	{
		int NRotateBits=UB(5,bs);
		v.yx=FB(NRotateBits,bs);
		v.xy=FB(NRotateBits,bs);
	}
	int NTranslateBits=UB(5,bs);
	v.x0=SB(NTranslateBits,bs)/20;
	v.y0=SB(NTranslateBits,bs)/20;
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, BUTTONRECORD& v)
{
	assert_and_throw(v.buttonVersion==2);
	BitStream bs(stream);

	bs.discard(2);
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
	UI8 NumberOfFilters;
	stream >> NumberOfFilters;
	v.Filters.resize(NumberOfFilters);

	for(int i=0;i<NumberOfFilters;i++)
		stream >> v.Filters[i];
	
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, FILTER& v)
{
	UI8 FilterID;
	stream >> FilterID;
	switch(FilterID)
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
			LOG(LOG_ERROR,_("Unsupported Filter Id ") << (int)FilterID);
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
	bs.discard(5);

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
	bs.discard(5);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, BLURFILTER& v)
{
	stream >> v.BlurX;
	stream >> v.BlurY;
	BitStream bs(stream);
	v.Passes = UB(5,bs);
	bs.discard(3);

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
	bs.discard(4);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, GRADIENTGLOWFILTER& v)
{
	UI8 NumColors;
	stream >> NumColors;
	for(int i = 0; i < NumColors; i++)
	{
		RGBA color;
		stream >> color;
		v.GradientColors.push_back(color);
	}
	for(int i = 0; i < NumColors; i++)
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
	bs.discard(5);

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
	bs.discard(6);

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
	UI8 NumColors;
	stream >> NumColors;
	for(int i = 0; i < NumColors; i++)
	{
		RGBA color;
		stream >> color;
		v.GradientColors.push_back(color);
	}
	for(int i = 0; i < NumColors; i++)
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
	bs.discard(4);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& s, CLIPEVENTFLAGS& v)
{
	/* In SWF version <=5 this was UI16_SWF,
	 * but parsing will then stop before we get here
	 * to fallback on gnash
	 */
	UI32_SWF t;
	s >> t;
	v.toParse=t;
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
	UI16_SWF Reserved;
	s >> Reserved >> v.AllEventFlags;
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
	Number* ret=Class<Number>::getInstanceS(i);
	return ret;
}

ASObject* lightspark::abstract_i(int32_t i)
{
	Integer* ret=Class<Integer>::getInstanceS(i);
	return ret;
}

ASObject* lightspark::abstract_ui(uint32_t i)
{
	UInteger* ret=Class<UInteger>::getInstanceS(i);
	return ret;
}

void lightspark::stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns)
{
	//Ok, let's split our string into namespace and name part
	char* collon=tmp.strchrr(':');
	if(collon)
	{
		/* collon is not the first character and there is
		 * another collon before it */
		assert_and_throw(collon != tmp.raw_buf() && *(collon-1) == ':');
		uint32_t collon_offset = collon-tmp.raw_buf();
		ns = tmp.substr_bytes(0,collon_offset-1);
		name = tmp.substr_bytes(collon_offset+1,tmp.numChars()-collon_offset-1);
		return;
	}
	// No namespace, look for a package name
	char* dot = tmp.strchrr('.');
	if(dot)
	{
		uint32_t dot_offset = dot-tmp.raw_buf();
		ns = tmp.substr_bytes(0,dot_offset);
		name = tmp.substr_bytes(dot_offset+1,tmp.numChars()-dot_offset-1);
		return;
	}
	//No namespace or package in the string
	name=tmp;
	ns="";
}

RunState::RunState():last_FP(-1),FP(0),next_FP(0),stop_FP(0),explicit_FP(false)
{
}

tiny_string QName::getQualifiedName() const
{
	tiny_string ret;
	if(!ns.empty())
	{
		ret+=ns;
		ret+="::";
	}
	ret+=name;
	return ret;
}

QName::operator multiname() const
{
	multiname ret(NULL);
	ret.name_type = multiname::NAME_STRING;
	ret.name_s_id = getSys()->getUniqueStringId(name);
	ret.ns.push_back( nsNameAndKind(ns, NAMESPACE) );
	ret.isAttribute = false;
	return ret;
}

FILLSTYLE::FILLSTYLE(uint8_t v):Gradient(v),bitmap(getSys()->unaccountedMemory),version(v)
{
}

FILLSTYLE::FILLSTYLE(const FILLSTYLE& r):Matrix(r.Matrix),Gradient(r.Gradient),FocalGradient(r.FocalGradient),
	bitmap(r.bitmap),Color(r.Color),FillStyleType(r.FillStyleType),version(r.version)
{
}

FILLSTYLE::~FILLSTYLE()
{
}

nsNameAndKind::nsNameAndKind(const tiny_string& _name, NS_KIND _kind)
{
	nsNameAndKindImpl tmp(_name, _kind);
	getSys()->getUniqueNamespaceId(tmp, nsRealId, nsId);
	nameIsEmpty=_name.empty();
}

nsNameAndKind::nsNameAndKind(const char* _name, NS_KIND _kind)
{
	nsNameAndKindImpl tmp(_name, _kind);
	getSys()->getUniqueNamespaceId(tmp, nsRealId, nsId);
	nameIsEmpty=(_name[0]=='\0');
}

nsNameAndKind::nsNameAndKind(const tiny_string& _name, uint32_t _baseId, NS_KIND _kind)
{
	assert(_kind==PROTECTED_NAMESPACE);
	nsId=_baseId;
	nsNameAndKindImpl tmp(_name, _kind, nsId);
	uint32_t tmpId;
	getSys()->getUniqueNamespaceId(tmp, nsRealId, tmpId);
	assert(tmpId==_baseId);
	nameIsEmpty=_name.empty();
}

nsNameAndKind::nsNameAndKind(ABCContext* c, uint32_t nsContextIndex)
{
	const namespace_info& ns=c->constant_pool.namespaces[nsContextIndex];
	const tiny_string& nsName=c->getString(ns.name);
	nsNameAndKindImpl tmp(nsName, (NS_KIND)(int)ns.kind);
	//Give an id hint, in case the namespace is created in the map
	getSys()->getUniqueNamespaceId(tmp, c->namespaceBaseId+nsContextIndex, nsRealId, nsId);
	//Special handling for private namespaces, they are always compared by id
	if(ns.kind==PRIVATE_NAMESPACE)
		nsId=c->namespaceBaseId+nsContextIndex;
	nameIsEmpty=nsName.empty();
}

const nsNameAndKindImpl& nsNameAndKind::getImpl() const
{
	return getSys()->getNamespaceFromUniqueId(nsRealId);
}
