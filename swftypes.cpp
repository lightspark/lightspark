/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <math.h>
#include "swf.h"
#include "backends/geometry.h"
#include "backends/rendering.h"
#include "scripting/class.h"
#include "exceptions.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME(ASObject);

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;
extern TLSDATA ParseThread* pt;

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

std::ostream& operator<<(std::ostream& s, const RGBA& r)
{
	s << "RGBA <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << ',' << (int)r.Alpha << '>';
	return s;
}

std::ostream& operator<<(std::ostream& s, const RGB& r)
{
	s << "RGB <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << '>';
	return s;
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

void MATRIX::getTranslation(int& x, int& y) const
{
	x=TranslateX;
	y=TranslateY;
}

std::ostream& operator<<(std::ostream& s, const MATRIX& r)
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
	assert_and_throw(version!=-1);

	assert_and_throw(r.version==version);
	if(version<4)
		LineStyles.insert(LineStyles.end(),r.LineStyles.begin(),r.LineStyles.end());
	else
		LineStyles2.insert(LineStyles2.end(),r.LineStyles2.begin(),r.LineStyles2.end());
	LineStyleCount = count;
}

std::istream& lightspark::operator>>(std::istream& s, LINESTYLEARRAY& v)
{
	assert_and_throw(v.version!=-1);
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		LOG(LOG_ERROR,"Line array extended not supported");
	if(v.version<4)
	{
		for(int i=0;i<v.LineStyleCount;i++)
		{
			LINESTYLE tmp;
			tmp.version=v.version;
			s >> tmp;
			v.LineStyles.push_back(tmp);
		}
	}
	else
	{
		for(int i=0;i<v.LineStyleCount;i++)
		{
			LINESTYLE2 tmp;
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
		LOG(LOG_ERROR,"Line array extended not supported");
	v.LineStyles=new MORPHLINESTYLE[v.LineStyleCount];
	for(int i=0;i<v.LineStyleCount;i++)
	{
		s >> v.LineStyles[i];
	}
	return s;
}

void FILLSTYLEARRAY::appendStyles(const FILLSTYLEARRAY& r)
{
	assert_and_throw(version!=-1);
	unsigned int count = FillStyleCount + r.FillStyleCount;

	FillStyles.insert(FillStyles.end(),r.FillStyles.begin(),r.FillStyles.end());
	FillStyleCount = count;
}

std::istream& lightspark::operator>>(std::istream& s, FILLSTYLEARRAY& v)
{
	assert_and_throw(v.version!=-1);
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(LOG_ERROR,"Fill array extended not supported");

	v.FillStyles.resize(v.FillStyleCount);
	list<FILLSTYLE>::iterator it=v.FillStyles.begin();
	for(;it!=v.FillStyles.end();it++)
	{
		it->version=v.version;
		s >> *it;
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, MORPHFILLSTYLEARRAY& v)
{
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(LOG_ERROR,"Fill array extended not supported");
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
		RGB t;
		in >> t;
		v.TextColor=t;
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
	GRADRECORD gr;
	gr.version=v.version;
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
	GRADRECORD gr;
	gr.version=v.version;
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

void FILLSTYLE::setVertexData(arrayElem* elem)
{
	if(FillStyleType==0x00)
	{
		//LOG(TRACE,"Fill color");
		elem->colors[0]=1;
		elem->colors[1]=0;
		elem->colors[2]=0;

		elem->texcoord[0]=float(Color.Red)/256.0f;
		elem->texcoord[1]=float(Color.Green)/256.0f;
		elem->texcoord[2]=float(Color.Blue)/256.0f;
		elem->texcoord[3]=float(Color.Alpha)/256.0f;
	}
	else if(FillStyleType==0x10)
	{
		//LOG(TRACE,"Fill gradient");
		elem->colors[0]=0;
		elem->colors[1]=1;
		elem->colors[2]=0;

		/*color_entry buffer[256];
		int grad_index=0;
		RGBA color_l(0,0,0,1);
		int index_l=0;
		RGBA color_r(Gradient.GradientRecords[0].Color);
		int index_r=Gradient.GradientRecords[0].Ratio;

		for(int i=0;i<256;i++)
		{
			float dist=i-index_l;
			dist/=(index_r-index_l);
			RGBA c=medianColor(color_l,color_r,dist);
			buffer[i].r=float(c.Red)/256.0f;
			buffer[i].g=float(c.Green)/256.0f;
			buffer[i].b=float(c.Blue)/256.0f;
			buffer[i].a=1;

			if(Gradient.GradientRecords[grad_index].Ratio==i)
			{
				grad_index++;
				color_l=color_r;
				index_l=index_r;
				color_r=Gradient.GradientRecords[grad_index].Color;
				index_r=Gradient.GradientRecords[grad_index].Ratio;
			}
		}

		glBindTexture(GL_TEXTURE_2D,rt->data_tex);
		glTexImage2D(GL_TEXTURE_2D,0,4,256,1,0,GL_RGBA,GL_FLOAT,buffer);*/
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"Reverting to fixed function");
		elem->colors[0]=1;
		elem->colors[1]=0;
		elem->colors[2]=0;

		elem->texcoord[0]=0.5;
		elem->texcoord[1]=0.5;
		elem->texcoord[2]=0;
		elem->texcoord[3]=1;
	}
}

void FILLSTYLE::setFragmentProgram() const
{
	//Let's abuse of glColor and glTexCoord to transport
	//custom information
	struct color_entry
	{
		float r,g,b,a;
	};

	//TODO: CHECK do we need to do this when the tex is not being used?
	rt->dataTex.bind();

	if(FillStyleType==0x00)
	{
		//LOG(TRACE,"Fill color");
		glColor4f(1,0,0,0);
		glTexCoord4f(float(Color.Red)/256.0f,
			float(Color.Green)/256.0f,
			float(Color.Blue)/256.0f,
			float(Color.Alpha)/256.0f);
	}
	else if(FillStyleType==0x10)
	{
		//LOG(TRACE,"Fill gradient");

#if 0
		color_entry buffer[256];
		unsigned int grad_index=0;
		RGBA color_l(0,0,0,1);
		int index_l=0;
		RGBA color_r(Gradient.GradientRecords[0].Color);
		int index_r=Gradient.GradientRecords[0].Ratio;

		glColor4f(0,1,0,0);

		for(int i=0;i<256;i++)
		{
			float dist=i-index_l;
			dist/=(index_r-index_l);
			RGBA c=medianColor(color_l,color_r,dist);
			buffer[i].r=float(c.Red)/256.0f;
			buffer[i].g=float(c.Green)/256.0f;
			buffer[i].b=float(c.Blue)/256.0f;
			buffer[i].a=1;

			assert_and_throw(grad_index<Gradient.GradientRecords.size());
			if(Gradient.GradientRecords[grad_index].Ratio==i)
			{
				color_l=color_r;
				index_l=index_r;
				color_r=Gradient.GradientRecords[grad_index].Color;
				index_r=Gradient.GradientRecords[grad_index].Ratio;
				grad_index++;
			}
		}

		glBindTexture(GL_TEXTURE_2D,rt->data_tex);
		glTexImage2D(GL_TEXTURE_2D,0,4,256,1,0,GL_RGBA,GL_FLOAT,buffer);
#else
		RGBA color_r(Gradient.GradientRecords[0].Color);
#endif

		//HACK: TODO: revamp gradient support
		glColor4f(1,0,0,0);
		glTexCoord4f(float(color_r.Red)/256.0f,
			float(color_r.Green)/256.0f,
			float(color_r.Blue)/256.0f,
			float(color_r.Alpha)/256.0f);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"Style not implemented");
		FILLSTYLE::fixedColor(0.5,0.5,0);
	}
}

void FILLSTYLE::fixedColor(float r, float g, float b)
{
	//TODO: CHECK: do we need to this here?
	rt->dataTex.bind();

	//Let's abuse of glColor and glTexCoord to transport
	//custom information
	glColor4f(1,0,0,0);
	glTexCoord4f(r,g,b,1);
}

std::istream& lightspark::operator>>(std::istream& s, FILLSTYLE& v)
{
	s >> v.FillStyleType;
	if(v.FillStyleType==0x00)
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
	else if(v.FillStyleType==0x10 || v.FillStyleType==0x12 || v.FillStyleType==0x13)
	{
		s >> v.GradientMatrix;
		v.Gradient.version=v.version;
		v.FocalGradient.version=v.version;
		if(v.FillStyleType==0x13)
			s >> v.FocalGradient;
		else
			s >> v.Gradient;
	}
	else if(v.FillStyleType==0x41 || v.FillStyleType==0x42 || v.FillStyleType==0x43)
	{
		s >> v.BitmapId >> v.BitmapMatrix;
	}
	else
	{
		LOG(LOG_ERROR,"Not supported fill style " << (int)v.FillStyleType << "... Aborting");
		throw ParseException("Not supported fill style");
	}
	return s;
}


std::istream& lightspark::operator>>(std::istream& s, MORPHFILLSTYLE& v)
{
	s >> v.FillStyleType;
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
		LOG(LOG_ERROR,"Not supported fill style " << (int)v.FillStyleType << "... Aborting");
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
			FILLSTYLEARRAY a;
			a.version=ps->FillStyles.version;
			bs.f >> a;
			p->fillOffset=ps->FillStyles.FillStyleCount;
			ps->FillStyles.appendStyles(a);

			LINESTYLEARRAY b;
			b.version=ps->LineStyles.version;
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
		UI16 t;
		s >> t;
		v.toParse=t;
	}
	else
	{
		s >> v.toParse;
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
	LOG(LOG_NOT_IMPLEMENTED,"Skipping " << v.ActionRecordSize << " of action data");
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
	//No double colons in the string
	name=tmp;
	ns="";
}

