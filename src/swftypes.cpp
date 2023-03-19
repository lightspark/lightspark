/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <glib.h>
#include <iomanip>
#include <algorithm>

#include "exceptions.h"
#include "compat.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "parsing/tags.h"


using namespace std;
using namespace lightspark;

const tiny_string multiname::qualifiedString(SystemState* sys, bool forDescribeType) const
{
	assert_and_throw(ns.size()>=1);
	assert_and_throw(name_type==NAME_STRING);
	if (forDescribeType && name_s_id == BUILTIN_STRINGS::ANY)
		return "*";
	const tiny_string nsName=sys->getStringFromUniqueId(ns[0].nsNameId);
	const tiny_string name=sys->getStringFromUniqueId(name_s_id);
	if(nsName.empty())
		return name;
	else
	{
		tiny_string ret=nsName;
		ret+="::";
		ret+=name;
		if (forDescribeType && ret.startsWith("__AS3__.vec::Vector$"))
		{
			tiny_string ret2 = "__AS3__.vec::Vector.<";
			ret2 += ret.substr(strlen("__AS3__.vec::Vector$"),ret.numChars());
			ret2 += ">";
			return ret2;
		}
		return ret;
	}
}

const tiny_string multiname::normalizedName(SystemState* sys) const
{
	switch(name_type)
	{
		case multiname::NAME_INT:
			return Integer::toString(name_i);
		case multiname::NAME_UINT:
			return UInteger::toString(name_ui);
		case multiname::NAME_NUMBER:
			return Number::toString(name_d);
		case multiname::NAME_STRING:
			return sys->getStringFromUniqueId(name_s_id);
		case multiname::NAME_OBJECT:
			return name_o ? name_o->toString() : "*";
		default:
			assert("Unexpected name kind" && false);
			//Should never reach this
			return "";
	}
}

uint32_t multiname::normalizedNameId(SystemState* sys) const
{
	switch(name_type)
	{
		case multiname::NAME_STRING:
			return name_s_id;
		case multiname::NAME_INT:
		case multiname::NAME_UINT:
		case multiname::NAME_NUMBER:
		case multiname::NAME_OBJECT:
			if (name_s_id != UINT32_MAX)
				return name_s_id;
			else
				return sys->getUniqueStringId(normalizedName(sys));
		default:
			assert("Unexpected name kind" && false);
			//Should never reach this
			return -1;
	}
}

const tiny_string multiname::normalizedNameUnresolved(SystemState* sys) const
{
	switch(name_type)
	{
		case multiname::NAME_INT:
			return Integer::toString(name_i);
		case multiname::NAME_UINT:
			return UInteger::toString(name_ui);
		case multiname::NAME_NUMBER:
			return Number::toString(name_d);
		case multiname::NAME_STRING:
			return sys->getStringFromUniqueId(name_s_id);
		case multiname::NAME_OBJECT:
			return name_o ? name_o->getClassName() : "*";
		default:
			assert("Unexpected name kind" && false);
			//Should never reach this
			return "";
	}
}

void multiname::setName(asAtom& n,ASWorker* w)
{
	if (name_type==NAME_OBJECT && name_o!=nullptr) {
		name_o->decRef();
		name_o = nullptr;
	}

	switch(asAtomHandler::getObjectType(n))
	{
	case T_INTEGER:
		name_i=asAtomHandler::getInt(n);
		name_type = NAME_INT;
		name_s_id = UINT32_MAX;
		isInteger=true;
		break;
	case T_UINTEGER:
		name_ui=asAtomHandler::getUInt(n);
		name_type = NAME_UINT;
		name_s_id = UINT32_MAX;
		isInteger=true;
		break;
	case T_NUMBER:
		name_d=asAtomHandler::toNumber(n);
		name_type = NAME_NUMBER;
		name_s_id = UINT32_MAX;
		isInteger=false;
		break;
	case T_BOOLEAN:
		name_i=asAtomHandler::toInt(n);
		name_type = NAME_INT;
		name_s_id = UINT32_MAX;
		isInteger=true;
		break;
	case T_QNAME:
		{
			ASQName* qname=asAtomHandler::as<ASQName>(n);
			name_s_id=qname->local_name;
			name_type = NAME_STRING;
			isInteger=Array::isIntegerWithoutLeadingZeros(w->getSystemState()->getStringFromUniqueId(name_s_id));
		}
		break;
	case T_STRING:
		{
			name_s_id=asAtomHandler::toStringId(n,w);
			name_type = NAME_STRING;
			isInteger=Array::isIntegerWithoutLeadingZeros(w->getSystemState()->getStringFromUniqueId(name_s_id));
		}
		break;
	case T_NULL:
		name_o=w->getSystemState()->getNullRef();
		name_type = NAME_OBJECT;
		name_s_id = UINT32_MAX;
		isInteger=false;
		break;
	case T_UNDEFINED:
		name_o=w->getSystemState()->getUndefinedRef();
		name_type = NAME_OBJECT;
		name_s_id = UINT32_MAX;
		isInteger=false;
		break;
	default:
		ASATOM_INCREF(n);
		name_o=asAtomHandler::getObject(n);
		name_type = NAME_OBJECT;
		name_s_id = UINT32_MAX;
		isInteger=false;
		break;
	}
}

void multiname::resetNameIfObject()
{
	if(name_type==NAME_OBJECT && name_o)
	{
		name_o->decRef();
		name_o=nullptr;
	}
}

bool multiname::toUInt(SystemState* sys, uint32_t& index, bool acceptStringFractions, bool *isNumber) const
{
	if (isNumber)
		*isNumber = false;
	switch(name_type)
	{
		//We try to convert this to an index, otherwise bail out
		case multiname::NAME_STRING:
		case multiname::NAME_OBJECT:
		{
			tiny_string str;
			if(name_type==multiname::NAME_STRING)
			{
				if (name_s_id < BUILTIN_STRINGS::LAST_BUILTIN_STRING &&
					(name_s_id < 0x30 || name_s_id > 0x39)) // char 0-9
					return false;
				str=sys->getStringFromUniqueId(name_s_id);
			}
			else
				str=name_o->toString();

			if(str.empty())
				return false;
			index=0;
			uint64_t parsed = 0;
			for(auto i=str.begin(); i!=str.end(); ++i)
			{
				if (*i == '.' && acceptStringFractions)
				{
					if (i==str.begin())
						return false;
					
					// Accept fractional part if it
					// is all zeros, e.g. "2.00"
					++i;
					for (; i!=str.end(); ++i)
						if (*i != '0')
						{
							if (isNumber)
								*isNumber = true;
							return false;
						}
					break;
				}
				else if(!i.isdigit())
					return false;
				parsed*=10;
				parsed+=i.digit_value();
				if (parsed > UINT32_MAX)
					break;
			}

			if (parsed > UINT32_MAX)
				return false;

			index = (uint32_t)parsed;
			break;
		}
		//This is already an int, so its good enough
		case multiname::NAME_INT:
			if(name_i < 0)
				return false;
			index=name_i;
			break;
		case multiname::NAME_UINT:
			index=name_ui;
			break;
		case multiname::NAME_NUMBER:
			if(!Number::isInteger(name_d) || name_d < 0 || name_d > UINT32_MAX)
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
	s << getSys()->getStringFromUniqueId(r.nsStringId) << ':' << getSys()->getStringFromUniqueId(r.nameId);
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
	switch(r.kind)
	{
		case NAMESPACE:
			prefix="ns:";
			break;
		case PACKAGE_NAMESPACE:
			prefix="pakns:";
			break;
		case PACKAGE_INTERNAL_NAMESPACE:
			prefix="pakintns:";
			break;
		case PROTECTED_NAMESPACE:
			prefix="protns:";
			break;
		case EXPLICIT_NAMESPACE:
			prefix="explns:";
			break;
		case STATIC_PROTECTED_NAMESPACE:
			prefix="staticprotns:";
			break;
		case PRIVATE_NAMESPACE:
			prefix="privns:";
			break;
		default:
			//Not reached
			assert("Unexpected namespace kind" && false);
			prefix="";
			break;
	}
	s << prefix << getSys()->getStringFromUniqueId(r.nsNameId);
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
	else if(r.name_type==multiname::NAME_UINT)
		s << r.name_ui;
	else if(r.name_type==multiname::NAME_NUMBER)
		s << r.name_d;
	else if(r.name_type==multiname::NAME_STRING)
		s << getSys()->getStringFromUniqueId(r.name_s_id);
	else
		s << r.name_o; //We print the hexadecimal value
	return s;
}

lightspark::RECT::RECT():Xmin(0),Xmax(0),Ymin(0),Ymax(0)
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

Vector2 MATRIX::multiply2D(const Vector2& in) const
{
	number_t xout;
	number_t yout;
	multiply2D(in.x,in.y,xout,yout);
	return Vector2(xout,yout);
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
	s << "| " << r.x0 << ' ' << r.y0 << " |" << std::endl;
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
	LineStyles2.insert(LineStyles2.end(),r.LineStyles2.begin(),r.LineStyles2.end());
}

std::istream& lightspark::operator>>(std::istream& s, LINESTYLEARRAY& v)
{
	assert(v.version!=0xff);
	UI8 LineStyleCount;
	s >> LineStyleCount;
	if(LineStyleCount==0xff)
		LOG(LOG_ERROR,"Line array extended not supported");
	if(v.version<4)
	{
		for(int i=0;i<LineStyleCount;i++)
		{
			LINESTYLE tmp(v.version);
			s >> tmp;
			v.LineStyles.push_back(tmp);
			LINESTYLE2 tmp2(v.version);
			tmp2.Color = tmp.Color;
			tmp2.Width = tmp.Width;
			v.LineStyles2.push_back(tmp2);
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
		LOG(LOG_ERROR,"Line array extended not supported");
	assert(v.version==1 || v.version==2);
	if(v.version==1)
	{
		for(int i=0;i<LineStyleCount;i++)
		{
			MORPHLINESTYLE t;
			s >> t;
			v.LineStyles.emplace_back(t);
			MORPHLINESTYLE2 tmp2;
			tmp2.StartWidth = t.StartWidth;
			tmp2.EndWidth = t.EndWidth;
			tmp2.StartColor = t.StartColor;
			tmp2.EndColor = t.EndColor;
			v.LineStyles2.push_back(tmp2);
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
	int fsc = FillStyleCount;
	if(FillStyleCount==0xff)
	{
		UI16_SWF ExtendedFillStyleCount;
		s >> ExtendedFillStyleCount;
		fsc = ExtendedFillStyleCount;
	}
	for(int i=0;i<fsc;i++)
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
		LOG(LOG_ERROR,"Fill array extended not supported");
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
	// this is not mentioned in the specs, but it seems if NumFillBits and NumLineBits are 0 in font tags, no shaperecords are read.
	if (v.forfont && v.NumFillBits == 0 && v.NumLineBits==0)
		return s;
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
		LOG(LOG_ERROR,"Reserved bits not so reserved");
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
					v.bitmap.reset();
					//throw ParseException("Invalid ID for bitmap");
				}
				else
					v.bitmap = b->getBitmap();
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
		LOG(LOG_ERROR,"Not supported fill style " << (int)v.FillStyleType);
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
	else if(v.FillStyleType==LINEAR_GRADIENT || v.FillStyleType==RADIAL_GRADIENT || v.FillStyleType==FOCAL_RADIAL_GRADIENT)
	{
		s >> v.StartGradientMatrix >> v.EndGradientMatrix;
		BitStream bs(s);
		v.SpreadMode = UB(2,bs);
		v.InterpolationMode = UB(2,bs);
		int NumGradients = UB(4,bs);
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
		if (v.FillStyleType==FOCAL_RADIAL_GRADIENT)
			s >> v.StartFocalPoint >> v.EndFocalPoint;
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
		LOG(LOG_ERROR,"Not supported fill style 0x" << hex << (int)v.FillStyleType << " at "<< s.tellg()<< dec <<  "... Aborting");
	}
	return s;
}

GLYPHENTRY::GLYPHENTRY(TEXTRECORD* p,BitStream& bs):parent(p)
{
	GlyphIndex = UB(parent->parent->GlyphBits,bs);
	GlyphAdvance = SB(parent->parent->AdvanceBits,bs);
}

SHAPERECORD::SHAPERECORD(SHAPE* p,BitStream& bs):parent(p),MoveBits(0),MoveDeltaX(0),MoveDeltaY(0),FillStyle1(0),FillStyle0(0),LineStyle(0),NumBits(0),DeltaX(0),DeltaY(0),ControlDeltaX(0),ControlDeltaY(0),AnchorDeltaX(0),AnchorDeltaY(0),TypeFlag(false),StateNewStyles(false),StateLineStyle(false),StateFillStyle1(false),StateFillStyle0(false),StateMoveTo(false),StraightFlag(false),GeneralLineFlag(false),VertLineFlag(false)
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
			FillStyle0=UB(parent->NumFillBits,bs);
		}
		if(StateFillStyle1)
		{
			FillStyle1=UB(parent->NumFillBits,bs);
		}
		if(StateLineStyle)
		{
			LineStyle=UB(parent->NumLineBits,bs);
		}
		// contrary to spec StateNewStyles is also allowed in DefineShapeTag
		if(StateNewStyles)// && parent->version >= 2)
		{
			SHAPEWITHSTYLE* ps=dynamic_cast<SHAPEWITHSTYLE*>(parent);
			if(ps==nullptr)
				throw ParseException("Malformed SWF file");
			bs.pos=0;
			FILLSTYLEARRAY a(ps->FillStyles.version);
			bs.f >> a;
			if (!a.FillStyles.empty())
			{
				p->fillOffset=ps->FillStyles.FillStyles.size();
				ps->FillStyles.appendStyles(a);
			}

			LINESTYLEARRAY b(ps->LineStyles.version);
			bs.f >> b;
			if (!b.LineStyles2.empty())
			{
				p->lineOffset=ps->LineStyles.LineStyles2.size();
				ps->LineStyles.appendStyles(b);
			}
			parent->NumFillBits=UB(4,bs);
			parent->NumLineBits=UB(4,bs);
		}
		if(StateFillStyle0 && (FillStyle0 != 0))
			FillStyle0+=p->fillOffset;
		if(StateFillStyle1 && (FillStyle1 != 0))
			FillStyle1+=p->fillOffset;
		if(StateLineStyle && (LineStyle != 0))
			LineStyle+=p->lineOffset;
	}
}

void CXFORMWITHALPHA::getParameters(number_t& redMultiplier,
				    number_t& greenMultiplier, 
				    number_t& blueMultiplier,
				    number_t& alphaMultiplier,
				    number_t& redOffset,
				    number_t& greenOffset,
				    number_t& blueOffset,
				    number_t& alphaOffset) const
{
	// multipliers are stored as values from 0.0 to 1.0
	if (HasMultTerms)
	{
		redMultiplier = RedMultTerm/256.0;
		greenMultiplier = GreenMultTerm/256.0;
		blueMultiplier = BlueMultTerm/256.0;
		alphaMultiplier = AlphaMultTerm/256.0;
		
	}
	else
	{
		redMultiplier = 1.0;
		greenMultiplier = 1.0;
		blueMultiplier = 1.0;
		alphaMultiplier = 1.0;
	}

	if (HasAddTerms)
	{
		redOffset = RedAddTerm;
		greenOffset = GreenAddTerm;
		blueOffset = BlueAddTerm;
		alphaOffset = AlphaAddTerm;
	}
	else
	{
		redOffset = 0;
		greenOffset = 0;
		blueOffset = 0;
		alphaOffset = 0;
	}
}

float CXFORMWITHALPHA::transformedAlpha(float alpha) const
{
	float ret = alpha;
	if (HasMultTerms)
		ret = alpha*AlphaMultTerm/256.;
	if (HasAddTerms)
		ret = alpha + AlphaAddTerm/256.;
	return dmin(dmax(ret, 0.), 1.);
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
	else
	{
		v.xx = 1;
		v.yy = 1;
	}
	int HasRotate=UB(1,bs);
	if(HasRotate)
	{
		int NRotateBits=UB(5,bs);
		v.yx=FB(NRotateBits,bs);
		v.xy=FB(NRotateBits,bs);
	}
	else
	{
		v.yx = 0;
		v.xy = 0;
	}
	int NTranslateBits=UB(5,bs);
	v.x0=SB(NTranslateBits,bs)/20.0;
	v.y0=SB(NTranslateBits,bs)/20.0;
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, BUTTONRECORD& v)
{
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

	stream >> v.CharacterID >> v.PlaceDepth >> v.PlaceMatrix;

	if (v.buttonVersion==2)
		stream >> v.ColorTransform;

	if(v.ButtonHasFilterList && v.buttonVersion==2)
		stream >> v.FilterList;

	if(v.ButtonHasBlendMode && v.buttonVersion==2)
		stream >> v.BlendMode;
	else
		v.BlendMode = 0;

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
			LOG(LOG_ERROR,"Unsupported Filter Id " << (int)v.FilterID);
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
	v.Passes = UB(5,bs);

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
	v.Passes = UB(5,bs);

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
	v.Passes = UB(4,bs);

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
	stream >> v.Angle;
	stream >> v.Distance;
	stream >> v.Strength;
	BitStream bs(stream);
	v.InnerGlow = UB(1,bs);
	v.Knockout = UB(1,bs);
	v.CompositeSource = UB(1,bs);
	v.OnTop = UB(1,bs);
	v.Passes = UB(4,bs);

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
	v.Passes = UB(4,bs);

	return stream;
}

std::istream& lightspark::operator>>(std::istream& s, CLIPEVENTFLAGS& v)
{
	BitStream bs(s);
	v.ClipEventKeyUp=UB(1,bs);
	v.ClipEventKeyDown=UB(1,bs);
	v.ClipEventMouseUp=UB(1,bs);
	v.ClipEventMouseDown=UB(1,bs);
	v.ClipEventMouseMove=UB(1,bs);
	v.ClipEventUnload=UB(1,bs);
	v.ClipEventEnterFrame=UB(1,bs);
	v.ClipEventLoad=UB(1,bs);
	v.ClipEventDragOver=UB(1,bs);
	v.ClipEventRollOut=UB(1,bs);
	v.ClipEventRollOver=UB(1,bs);
	v.ClipEventReleaseOutside=UB(1,bs);
	v.ClipEventRelease=UB(1,bs);
	v.ClipEventPress=UB(1,bs);
	v.ClipEventInitialize=UB(1,bs);
	v.ClipEventData=UB(1,bs);
	if (v.getSWFVersion() <= 5)
	{
		v.ClipEventConstruct=false;
		v.ClipEventKeyPress=false;
		v.ClipEventDragOut=false;
	}
	else
	{
		UB(5,bs);
		v.ClipEventConstruct=UB(1,bs);
		v.ClipEventKeyPress=UB(1,bs);
		v.ClipEventDragOut=UB(1,bs);
		UB(8,bs);
	}
	if (v.ClipEventUnload)
		LOG(LOG_NOT_IMPLEMENTED,"CLIPEVENTFLAG ClipEventUnload not handled");
	if (v.ClipEventDragOver)
		LOG(LOG_NOT_IMPLEMENTED,"CLIPEVENTFLAG ClipEventDragOver not handled");
	if (v.ClipEventData)
		LOG(LOG_NOT_IMPLEMENTED,"CLIPEVENTFLAG ClipEventData not handled");
	if (v.ClipEventKeyPress)
		LOG(LOG_NOT_IMPLEMENTED,"CLIPEVENTFLAG ClipEventKeyPress not handled");
	if (v.ClipEventDragOut)
		LOG(LOG_NOT_IMPLEMENTED,"CLIPEVENTFLAG ClipEventDragOut not handled");
	return s;
}

bool CLIPEVENTFLAGS::isNull()
{
	return
			!ClipEventKeyUp &&
			!ClipEventKeyDown &&
			!ClipEventMouseUp &&
			!ClipEventMouseDown &&
			!ClipEventMouseMove &&
			!ClipEventUnload &&
			!ClipEventEnterFrame &&
			!ClipEventLoad &&
			!ClipEventDragOver &&
			!ClipEventRollOut &&
			!ClipEventRollOver &&
			!ClipEventReleaseOutside &&
			!ClipEventRelease &&
			!ClipEventPress &&
			!ClipEventInitialize &&
			!ClipEventData &&
			!ClipEventConstruct &&
			!ClipEventKeyPress &&
			!ClipEventDragOut;
}

std::istream& lightspark::operator>>(std::istream& s, CLIPACTIONRECORD& v)
{
	s >> v.EventFlags;
	if(v.EventFlags.isNull())
		return s;
	s >> v.ActionRecordSize;
	uint32_t len = v.ActionRecordSize;
	if (v.EventFlags.ClipEventKeyPress)
	{
		s >> v.KeyCode;
		len -= 1;
	}
	if (v.datatag)
	{
		v.startactionpos=v.datatag->numbytes+v.dataskipbytes;
		v.actions.resize(len+v.startactionpos);
		memcpy(v.actions.data(),v.datatag->bytes,v.datatag->numbytes);
	}
	else
		v.actions.resize(len);
	s.read((char*)(v.actions.data()+v.startactionpos),len);
	return s;
}

bool CLIPACTIONRECORD::isLast()
{
	return EventFlags.isNull();
}

std::istream& lightspark::operator>>(std::istream& s, CLIPACTIONS& v)
{
	uint32_t startpos=s.tellg();
	UI16_SWF Reserved;
	s >> Reserved >> v.AllEventFlags;
	while(1)
	{
		CLIPACTIONRECORD t(v.AllEventFlags.getSWFVersion(),v.dataskipbytes+(uint32_t(s.tellg())-startpos),v.datatag);
		// use datatag only on first clipaction
		v.datatag=nullptr;
		s >> t;
		if(t.isLast())
			break;
		v.ClipActionRecords.push_back(t);
	}
	return s;
}
ASObject* lightspark::abstract_s(ASWorker* wrk)
{
	ASString* ret= Class<ASString>::getInstanceSNoArgs(wrk);
	ret->stringId = BUILTIN_STRINGS::EMPTY;
	ret->hasId = true;
	ret->datafilled=true;
	return ret;
}
ASObject* lightspark::abstract_s(ASWorker* wrk, const char* s, uint32_t len)
{
	ASString* ret= Class<ASString>::getInstanceSNoArgs(wrk);
	ret->data = std::string(s,len);
	ret->stringId = UINT32_MAX;
	ret->hasId = false;
	ret->datafilled=true;
	return ret;
}
ASObject* lightspark::abstract_s(ASWorker* wrk, const char* s, int numbytes, int numchars, bool issinglebyte, bool hasNull)
{
	ASString* ret= Class<ASString>::getInstanceSNoArgs(wrk);
	ret->data.setValue(s,numbytes,numchars,issinglebyte,hasNull,true);
	ret->stringId = UINT32_MAX;
	ret->hasId = false;
	ret->datafilled=true;
	return ret;
}
ASObject* lightspark::abstract_s(ASWorker* wrk, const char* s)
{
	ASString* ret= Class<ASString>::getInstanceSNoArgs(wrk);
	ret->data = s;
	ret->stringId = UINT32_MAX;
	ret->hasId = false;
	ret->datafilled=true;
	return ret;
}
ASObject* lightspark::abstract_s(ASWorker* wrk, const tiny_string& s)
{
	ASString* ret= Class<ASString>::getInstanceSNoArgs(wrk);
	ret->data = s;
	ret->stringId = UINT32_MAX;
	ret->hasId = false;
	ret->datafilled=true;
	return ret;
}
ASObject* lightspark::abstract_s(ASWorker* wrk, uint32_t stringId)
{
	ASString* ret= Class<ASString>::getInstanceSNoArgs(wrk);
	ret->stringId = stringId;
	ret->hasId = true;
	ret->datafilled=false;
	return ret;
}
ASObject* lightspark::abstract_d(ASWorker* wrk, number_t i)
{
	Number* ret=Class<Number>::getInstanceSNoArgs(wrk);
	ret->dval = i;
	ret->isfloat = true;
	return ret;
}
ASObject* lightspark::abstract_d_constant(ASWorker* wrk, number_t i)
{
	Number* ret=new (wrk->getSystemState()->unaccountedMemory) Number(wrk,Class<Number>::getRef(wrk->getSystemState()).getPtr());
	ret->dval = i;
	ret->isfloat = true;
	ret->setRefConstant();
	return ret;
}
ASObject* lightspark::abstract_di(ASWorker* wrk, int64_t i)
{
	Number* ret=Class<Number>::getInstanceSNoArgs(wrk);
	ret->ival = i;
	ret->isfloat = false;
	return ret;
}
ASObject* lightspark::abstract_i(ASWorker* wrk, int32_t i)
{
	Integer* ret=Class<Integer>::getInstanceSNoArgs(wrk);
	ret->val = i;
	return ret;
}

ASObject* lightspark::abstract_ui(ASWorker* wrk, uint32_t i)
{
	UInteger* ret=Class<UInteger>::getInstanceSNoArgs(wrk);
	ret->val = i;
	return ret;
}
ASObject* lightspark::abstract_null(SystemState *sys)
{
	return sys->getNullRef();
}
ASObject* lightspark::abstract_undefined(SystemState *sys)
{
	return sys->getUndefinedRef();
}
ASObject *lightspark::abstract_b(SystemState *sys, bool v)
{
	if(v==true)
		return sys->getTrueRef();
	else
		return sys->getFalseRef();
}

void lightspark::stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns)
{
	//Ok, let's split our string into namespace and name part
	char* collon=tmp.strchr(':');
	if(collon)
	{
		/* collon is not the first character and there is
		 * another collon before it */
		if (collon == tmp.raw_buf() || *(collon+1) != ':')
		{
			name=tmp;
			ns="";
			return;
		}
		uint32_t collon_offset = collon-tmp.raw_buf();
		ns = tmp.substr_bytes(0,collon_offset);
		name = tmp.substr_bytes(collon_offset+2,tmp.numBytes()-collon_offset-2);
		return;
	}
	// No namespace, look for a package name
	char* dot = tmp.strchrr('.');
	if(dot)
	{
		uint32_t dot_offset = dot-tmp.raw_buf();
		ns = tmp.substr_bytes(0,dot_offset);
		name = tmp.substr_bytes(dot_offset+1,tmp.numBytes()-dot_offset-1);
		return;
	}
	//No namespace or package in the string
	name=tmp;
	ns="";
}

RunState::RunState():last_FP(-1),FP(0),next_FP(0),stop_FP(false),explicit_FP(false),creatingframe(false),frameadvanced(false)
{
}

tiny_string QName::getQualifiedName(SystemState *sys,bool forDescribeType) const
{
	tiny_string ret;
	if(nsStringId != BUILTIN_STRINGS::EMPTY)
	{
		ret+=sys->getStringFromUniqueId(nsStringId);
		ret+="::";
	}
	ret+=sys->getStringFromUniqueId(nameId);
	if (forDescribeType && ret.startsWith("__AS3__.vec::Vector$"))
	{
		tiny_string ret2 = "__AS3__.vec::Vector.<";
		ret2 += ret.substr(strlen("__AS3__.vec::Vector$"),ret.numChars());
		ret2 += ">";
		return ret2;
	}
	return ret;
}

QName::operator multiname() const
{
	multiname ret(nullptr);
	ret.name_type = multiname::NAME_STRING;
	ret.name_s_id = nameId;
	ret.ns.emplace_back(getSys(),nsStringId, NAMESPACE);
	ret.isAttribute = false;
	return ret;
}

FILLSTYLE::FILLSTYLE(uint8_t v):Gradient(v),version(v)
{
}

FILLSTYLE::FILLSTYLE(const FILLSTYLE& r):Matrix(r.Matrix),Gradient(r.Gradient),FocalGradient(r.FocalGradient),
	bitmap(r.bitmap),ShapeBounds(r.ShapeBounds),Color(r.Color),FillStyleType(r.FillStyleType),version(r.version)
{
}

FILLSTYLE::~FILLSTYLE()
{
}

FILLSTYLE& FILLSTYLE::operator=(FILLSTYLE r)
{
	Matrix = r.Matrix;
	Gradient = r.Gradient;
	FocalGradient = r.FocalGradient;
	bitmap = r.bitmap;
	ShapeBounds = r.ShapeBounds;
	Color = r.Color;
	FillStyleType = r.FillStyleType;
	version = r.version;
	return *this;
}

LINESTYLE2::LINESTYLE2(const LINESTYLE2& r):StartCapStyle(r.StartCapStyle),JointStyle(r.JointStyle),HasFillFlag(r.HasFillFlag),
	NoHScaleFlag(r.NoHScaleFlag),NoVScaleFlag(r.NoVScaleFlag),PixelHintingFlag(r.PixelHintingFlag),
	Width(r.Width),MiterLimitFactor(r.MiterLimitFactor),Color(r.Color),
	FillType(r.FillType),version(r.version)
{
}

LINESTYLE2::~LINESTYLE2()
{
}

LINESTYLE2& LINESTYLE2::operator=(LINESTYLE2 r)
{
	StartCapStyle=r.StartCapStyle;
	JointStyle=r.JointStyle;
	HasFillFlag=r.HasFillFlag;
	NoHScaleFlag=r.NoHScaleFlag;
	NoVScaleFlag=r.NoVScaleFlag;
	PixelHintingFlag=r.PixelHintingFlag;
	NoClose=r.NoClose;
	EndCapStyle=r.EndCapStyle;
	Width=r.Width;
	MiterLimitFactor=r.MiterLimitFactor;
	Color=r.Color;
	FillType=r.FillType;
	version=r.version;
	return *this;
}

nsNameAndKind::nsNameAndKind(SystemState* sys,const tiny_string& _name, NS_KIND _kind)
{
	nsNameId = sys->getUniqueStringId(_name);
	nsNameAndKindImpl tmp(nsNameId, _kind);
	sys->getUniqueNamespaceId(tmp, nsRealId, nsId);
	kind = _kind;
}

nsNameAndKind::nsNameAndKind(SystemState* sys,const char* _name, NS_KIND _kind)
{
	nsNameId = sys->getUniqueStringId(_name);
	nsNameAndKindImpl tmp(nsNameId, _kind);
	sys->getUniqueNamespaceId(tmp, nsRealId, nsId);
	kind = _kind;
}

nsNameAndKind::nsNameAndKind(SystemState* sys,uint32_t _nameId, NS_KIND _kind)
{
	nsNameAndKindImpl tmp(_nameId, _kind);
	sys->getUniqueNamespaceId(tmp, nsRealId, nsId);
	nsNameId = _nameId;
	kind = _kind;
}
nsNameAndKind::nsNameAndKind(SystemState* sys,uint32_t _nameId, NS_KIND _kind,RootMovieClip* root)
{
	assert(_kind==PROTECTED_NAMESPACE);
	nsNameAndKindImpl tmp(_nameId, _kind,root);
	sys->getUniqueNamespaceId(tmp, nsRealId, nsId);
	nsNameId = _nameId;
	kind = _kind;
}
nsNameAndKind::nsNameAndKind(SystemState* sys, uint32_t _nameId, uint32_t _baseId, NS_KIND _kind,RootMovieClip* root)
{
	assert(_kind==PROTECTED_NAMESPACE);
	nsId=_baseId;
	nsNameAndKindImpl tmp(_nameId, _kind, root, nsId);
	uint32_t tmpId;
	sys->getUniqueNamespaceId(tmp, nsRealId, tmpId);
	assert(tmpId==_baseId);
	nsNameId = _nameId;
	kind = _kind;
}

nsNameAndKind::nsNameAndKind(ABCContext* c, uint32_t nsContextIndex)
{
	const namespace_info& ns=c->constant_pool.namespaces[nsContextIndex];
	nsNameId=c->getString(ns.name);
	nsNameAndKindImpl tmp(nsNameId, (NS_KIND)(int)ns.kind,((NS_KIND)(int)ns.kind)==PROTECTED_NAMESPACE ? c->root : nullptr);
	//Give an id hint, in case the namespace is created in the map
	c->root->getSystemState()->getUniqueNamespaceId(tmp, c->namespaceBaseId+nsContextIndex, nsRealId, nsId);
	//Special handling for private namespaces, they are always compared by id
	if(ns.kind==PRIVATE_NAMESPACE)
		nsId=c->namespaceBaseId+nsContextIndex;
	kind = (NS_KIND)(int)ns.kind;
}

nsNameAndKindImpl::nsNameAndKindImpl(uint32_t _nameId, NS_KIND _kind, RootMovieClip *r, uint32_t b)
  : nameId(_nameId),kind(_kind),baseId(b),root(r)
{
	if (kind != NAMESPACE &&
	    kind != PACKAGE_NAMESPACE &&
	    kind != PACKAGE_INTERNAL_NAMESPACE &&
	    kind != PROTECTED_NAMESPACE &&
	    kind != EXPLICIT_NAMESPACE &&
	    kind != STATIC_PROTECTED_NAMESPACE &&
	    kind != PRIVATE_NAMESPACE)
	{
		//I have seen empty namespace with kind 0. For other
		//namespaces we should not get here.
		if (nameId != BUILTIN_STRINGS::EMPTY)
			LOG(LOG_ERROR, "Invalid namespace kind, converting to public namespace");
		kind = NAMESPACE;
	}
}

RGB::RGB(const tiny_string& colorstr):Red(0),Green(0),Blue(0)
{
	if (colorstr.empty())
		return;

	const char *s = colorstr.raw_buf();
	if (s[0] == '#')
		s++;

	gint64 color = g_ascii_strtoll(s, nullptr, 16);
	Red = (color >> 16) & 0xFF;
	Green = (color >> 8) & 0xFF;
	Blue = color & 0xFF;
}

tiny_string RGB::toString() const
{
	ostringstream ss;
	ss << "#" << std::hex << std::setfill('0') << 
		std::setw(2) << (int)Red << 
		std::setw(2) << (int)Green << 
		std::setw(2) << (int)Blue;

	return ss.str();
}

std::istream& lightspark::operator>>(std::istream& stream, SOUNDINFO& v)
{
	BitStream bs(stream);
	UB(2,bs); // reserved
	v.SyncStop = UB(1,bs);
	v.SyncNoMultiple = UB(1,bs);
	v.HasEnvelope = UB(1,bs);
	v.HasLoops = UB(1,bs);
	v.HasOutPoint = UB(1,bs);
	v.HasInPoint = UB(1,bs);
	if (v.HasInPoint)
		stream >> v.InPoint;
	if (v.HasOutPoint)
		stream >> v.OutPoint;
	if (v.HasLoops)
		stream >> v.LoopCount;
	if (v.HasEnvelope)
	{
		stream >> v.EnvPoints;
		for (unsigned int i = 0; i < v.EnvPoints;i++)
		{
			SOUNDENVELOPE env;
			stream >> env.Pos44;
			stream >> env.LeftLevel;
			stream >> env.RightLevel;
			v.SoundEnvelope.push_back(env);
		}
	}
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, BUTTONCONDACTION& v)
{
	stream >> v.CondActionSize;
	BitStream bs(stream);
	v.CondIdleToOverDown = UB(1,bs);
	v.CondOutDownToIdle = UB(1,bs);
	v.CondOutDownToOverDown = UB(1,bs);
	v.CondOverDownToOutDown = UB(1,bs);
	v.CondOverDownToOverUp = UB(1,bs);
	v.CondOverUpToOverDown = UB(1,bs);
	v.CondOverUpToIdle = UB(1,bs);
	v.CondIdleToOverUp = UB(1,bs);
	v.CondKeyPress = UB(7,bs);
	v.CondOverDownToIdle = UB(1,bs);

	return stream;
}

SHAPE::~SHAPE()
{
	for (auto it = scaledtexturecache.begin(); it != scaledtexturecache.begin(); it++)
	{
		delete (*it).second;
	}
}
