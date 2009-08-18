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

#define GL_GLEXT_PROTOTYPES
#include "swftypes.h"
#include "tags.h"
#include "logger.h"
#include "actions.h"
#include <string.h>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include "swf.h"
#include "geometry.h"

using namespace std;
extern __thread SystemState* sys;
extern __thread RenderThread* rt;
extern __thread ParseThread* pt;
extern __thread Manager* iManager;
extern __thread Manager* dManager;

string ConstantReference::toString() const
{
	return rt->vm.getConstantByIndex(index);
}

/*double ConstantReference::toNumber() const
{
	string s=rt->vm.getConstantByIndex(index);
	double ret= strtod(s.c_str(),NULL);
	cout << "crto " << ret <<  " from " << s << endl;
	return ret;
}*/

ISWFObject* ConstantReference::clone()
{
	return new ASString(rt->vm.getConstantByIndex(index));
}

int ConstantReference::toInt() const
{
	LOG(ERROR,"Cannot convert ConstRef to Int");
	return 0;
}

string ISWFObject::toString() const
{
	cout << "Cannot convert object of type " << getObjectType() << " to String" << endl;
	abort();
	return "Cannot convert object to String";
}

bool ISWFObject::isGreater(const ISWFObject* r) const
{
	LOG(NOT_IMPLEMENTED,"Greater than comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	return false;
}

bool ISWFObject::isLess(const ISWFObject* r) const
{
	LOG(NOT_IMPLEMENTED,"Less than comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	return false;
}

int multiname::count=0;

multiname::~multiname()
{
	count--;
}

bool Integer::isGreater(const ISWFObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return val>*i;
	}
	else
	{
		return ISWFObject::isGreater(o);
	}
}

bool Integer::isLess(const ISWFObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return val<*i;
	}
	else
		return ISWFObject::isLess(o);
}

bool Integer::isEqual(const ISWFObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toInt();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toInt();
	else
	{
		return ISWFObject::isEqual(o);
	}
}

bool ISWFObject::isEqual(const ISWFObject* r) const
{
	LOG(NOT_IMPLEMENTED,"Equal comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	return false;
}

IFunction* ISWFObject::toFunction()
{
	LOG(ERROR,"Cannot convert object of type " << getObjectType() << " to Function");
	return NULL;
}


int ISWFObject::toInt() const
{
	LOG(ERROR,"Cannot convert object of type " << getObjectType() << " to Int");
	cout << "imanager " << iManager->available.size() << endl;
	cout << "dmanager " << dManager->available.size() << endl;
	abort();
	return 0;
}

double ISWFObject::toNumber() const
{
	LOG(ERROR,"Cannot convert object of type " << getObjectType() << " to float");
	abort();
	return 0;
}

void ISWFObject::setGetterByName(const Qname& name, IFunction* o)
{
	pair<map<Qname, obj_var>::iterator,bool> ret=Variables.insert(make_pair(name,obj_var(NULL,o,NULL)));
	if(!ret.second)
	{
		if(ret.first->second.getter)
		{
			//ret.first->second.getter->decRef();
			//Should never happen
			abort();
		}
		ret.first->second.getter=o;
	}
}

void ISWFObject::setSetterByName(const Qname& name, IFunction* o)
{
	pair<map<Qname, obj_var>::iterator,bool> ret=Variables.insert(make_pair(name,obj_var(NULL,NULL,o)));
	if(!ret.second)
	{
		if(ret.first->second.setter)
		{
			//ret.first->second.setter->decRef();
			//Should never happen
			abort();
		}
		ret.first->second.setter=o;
	}
}

void ISWFObject::setVariableByName(const Qname& name, ISWFObject* o)
{
	pair<map<Qname, obj_var>::iterator,bool> ret=Variables.insert(make_pair(name,obj_var(o)));
	if(!ret.second)
	{
		if(ret.first->second.setter)
			abort();
	
		if(ret.first->second.var)
			ret.first->second.var->decRef();
		ret.first->second.var=o;
	}
}

void ISWFObject::setVariableByMultiname_i(multiname& name, intptr_t value)
{
	abort();
/*	if(name.namert)
		name.name=name.namert->toString();

	pair<map<Qname, ISWFObject*>::iterator,bool> ret=Variables.insert(pair<Qname,ISWFObject*>(name.name,o));
	if(!ret.second)
	{
		if(ret.first->second)
			ret.first->second->decRef();
		ret.first->second=o;
	}
	return o;*/
}

void ISWFObject::setVariableByMultiname(multiname& name, ISWFObject* o)
{
	pair<map<Qname, obj_var>::iterator,bool> ret;
	if(name.name_type==multiname::NAME_INT)
		ret=Variables.insert(make_pair(name.name_i,obj_var(o)));
	else if(name.name_type==multiname::NAME_STRING)
		ret=Variables.insert(make_pair((const char*)name.name_s,obj_var(o)));

	if(!ret.second)
	{
		if(ret.first->second.setter)
		{
			//Call the setter
			LOG(CALLS,"Calling the setter");
			arguments args(1);
			args.set(0,o);
			//TODO: check
			o->incRef();
			//

			ret.first->second.setter->call(this,&args);
			LOG(CALLS,"End of setter");
		}
		else
		{
			if(ret.first->second.var)
				ret.first->second.var->decRef();
			ret.first->second.var=o;
		}
	}
}

ISWFObject* ISWFObject::getVariableByMultiname(const multiname& name, ISWFObject*& owner)
{
	map<Qname,obj_var>::iterator it;
	if(name.name_type==multiname::NAME_INT)
		it=Variables.find(name.name_i);
	else if(name.name_type==multiname::NAME_STRING)
		it=Variables.find((const char*)name.name_s);
	if(it!=Variables.end())
	{
		if(name.ns.empty())
			owner=this;
		else
			owner=NULL;

		for(int i=0;i<name.ns.size();i++)
		{
			if(it->first.ns==name.ns[i])
			{
				owner=this;
				break;
			}
		}
		//Check to see if a proper getter method is available
		if(it->second.getter)
		{
			//Call the getter
			LOG(CALLS,"Calling the getter");
			//arguments args;
			//args.push(value);
			//value->incRef();
			ISWFObject* ret=it->second.getter->call(this,NULL);
			LOG(CALLS,"End of getter");
			return ret;
		}
		else
			return it->second.var;
	}
	else
	{
		owner=NULL;
		return NULL;
	}
}

ISWFObject* ISWFObject::getVariableByString(const std::string& name, ISWFObject*& owner)
{
	//Slow linear lookup, should be avoided
	map<Qname,obj_var>::iterator it=Variables.begin();
	for(it;it!=Variables.end();it++)
	{
		string cur=it->first.ns;
		if(!cur.empty())
			cur+='.';
		cur+=it->first.name;
		if(cur==name)
		{
			owner=this;
			if(it->second.getter)
				abort();
			return it->second.var;
		}
	}
	
	owner=NULL;
	return NULL;
}

ISWFObject* ISWFObject::getVariableByName(const Qname& name, ISWFObject*& owner)
{
	map<Qname,obj_var>::iterator it=Variables.find(name);
	if(it!=Variables.end())
	{
		owner=this;
		if(it->second.getter)
			abort();
		return it->second.var;
	}
	else
	{
		owner=NULL;
		return NULL;
	}
}

std::ostream& operator<<(std::ostream& s, const Qname& r)
{
	string prefix;
	switch(r.nskind)
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
	s << '[' << prefix << r.ns << "] " << r.name;
	return s;
}

std::ostream& operator<<(std::ostream& s, const multiname& r)
{
	for(int i=0;i<r.ns.size();i++)
	{
		string prefix;
		switch(r.nskind[i])
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
		s << '[' << prefix << r.ns[i] << "] ";
	}
	if(r.name_type==multiname::NAME_INT)
		s << r.name_i;
	else if(r.name_type==multiname::NAME_STRING)
		s << r.name_s;
	return s;
}

void ISWFObject::dumpVariables()
{
	map<Qname,obj_var>::iterator it=Variables.begin();
	for(it;it!=Variables.end();it++)
		cout << '[' << it->first.ns << "] "<< it->first.name << endl;
}

string Integer::toString() const
{
	char buf[20];
	if(val<0)
		abort();
	buf[19]=0;
	char* cur=buf+19;

	int v=val;
	do
	{
		cur--;
		*cur='0'+(v%10);
		v/=10;
	}
	while(v!=0);
	return cur;
}

int Integer::toInt() const
{
	return val;
}

double Integer::toNumber() const
{
	return val;
}

RECT::RECT()
{
}

std::ostream& operator<<(std::ostream& s, const RECT& r)
{
	s << '{' << (int)r.Xmin << ',' << r.Xmax << ',' << r.Ymin << ',' << r.Ymax << '}';
	return s;
}

ostream& operator<<(ostream& s, const STRING& t)
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

void MATRIX::get4DMatrix(float matrix[16])
{
	memset(matrix,0,sizeof(float)*16);
	float sX=1,sY=1;
	if(HasScale)
	{
		sX=ScaleX;
		sY=ScaleY;
	}
	matrix[0]=sX;
	matrix[1]=RotateSkew0;

	matrix[4]=RotateSkew1;
	matrix[5]=sY;

	matrix[10]=1;

	matrix[12]=TranslateX;
	matrix[13]=TranslateY;
	matrix[15]=1;
}

std::ostream& operator<<(std::ostream& s, const MATRIX& r)
{
	float scaleX=1,scaleY=1;
	if(r.HasScale)
	{
		scaleX=r.ScaleX;
		scaleY=r.ScaleY;
	}
	s << "| " << scaleX << ' ' << r.RotateSkew0 << " |" << std::endl;
	s << "| " << r.RotateSkew1 << ' ' << scaleY << " |" << std::endl;
	s << "| " << (int)r.TranslateX << ' ' << (int)r.TranslateY << " |" << std::endl;
	return s;
}

std::istream& operator>>(std::istream& stream, STRING& v)
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

std::istream& operator>>(std::istream& stream, RECT& v)
{
	BitStream s(stream);
	v.NBits=UB(5,s);
	v.Xmin=SB(v.NBits,s);
	v.Xmax=SB(v.NBits,s);
	v.Ymin=SB(v.NBits,s);
	v.Ymax=SB(v.NBits,s);
	return stream;
}

std::istream& operator>>(std::istream& s, RGB& v)
{
	s >> v.Red >> v.Green >> v.Blue;
	return s;
}

std::istream& operator>>(std::istream& s, RGBA& v)
{
	s >> v.Red >> v.Green >> v.Blue >> v.Alpha;
	return s;
}

std::istream& operator>>(std::istream& s, LINESTYLEARRAY& v)
{
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		LOG(ERROR,"Line array extended not supported");
	if(v.version<4)
	{
		v.LineStyles=new LINESTYLE[v.LineStyleCount];
		for(int i=0;i<v.LineStyleCount;i++)
		{
			v.LineStyles[i].version=v.version;
			s >> v.LineStyles[i];
		}
	}
	else
	{
		v.LineStyles2=new LINESTYLE2[v.LineStyleCount];
		for(int i=0;i<v.LineStyleCount;i++)
			s >> v.LineStyles2[i];
	}
	return s;
}

std::istream& operator>>(std::istream& s, MORPHLINESTYLEARRAY& v)
{
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		LOG(ERROR,"Line array extended not supported");
	v.LineStyles=new MORPHLINESTYLE[v.LineStyleCount];
	for(int i=0;i<v.LineStyleCount;i++)
	{
		s >> v.LineStyles[i];
	}
	return s;
}

std::istream& operator>>(std::istream& s, FILLSTYLEARRAY& v)
{
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(ERROR,"Fill array extended not supported");
	v.FillStyles=new FILLSTYLE[v.FillStyleCount];
	for(int i=0;i<v.FillStyleCount;i++)
	{
		v.FillStyles[i].version=v.version;
		s >> v.FillStyles[i];
	}
	return s;
}

std::istream& operator>>(std::istream& s, MORPHFILLSTYLEARRAY& v)
{
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(ERROR,"Fill array extended not supported");
	v.FillStyles=new MORPHFILLSTYLE[v.FillStyleCount];
	for(int i=0;i<v.FillStyleCount;i++)
	{
		s >> v.FillStyles[i];
	}
	return s;
}

std::istream& operator>>(std::istream& s, SHAPE& v)
{
	BitStream bs(s);
	v.NumFillBits=UB(4,bs);
	v.NumLineBits=UB(4,bs);
	v.ShapeRecords=SHAPERECORD(&v,bs);
	if(v.ShapeRecords.TypeFlag + v.ShapeRecords.StateNewStyles+v.ShapeRecords.StateLineStyle+v.ShapeRecords.StateFillStyle1+
			v.ShapeRecords.StateFillStyle0+v.ShapeRecords.StateMoveTo)
	{
		SHAPERECORD* cur=&(v.ShapeRecords);
		while(1)
		{
			cur->next=new SHAPERECORD(&v,bs);
			cur=cur->next;
			if(cur->TypeFlag+cur->StateNewStyles+cur->StateLineStyle+cur->StateFillStyle1+cur->StateFillStyle0+cur->StateMoveTo==0)
				break;
		}
	}
	return s;
}

std::istream& operator>>(std::istream& s, SHAPEWITHSTYLE& v)
{
	v.FillStyles.version=v.version;
	v.LineStyles.version=v.version;
	s >> v.FillStyles >> v.LineStyles;
	BitStream bs(s);
	v.NumFillBits=UB(4,bs);
	v.NumLineBits=UB(4,bs);
	v.ShapeRecords=SHAPERECORD(&v,bs);
	if(v.ShapeRecords.TypeFlag+v.ShapeRecords.StateNewStyles+v.ShapeRecords.StateLineStyle+v.ShapeRecords.StateFillStyle1+
			v.ShapeRecords.StateFillStyle0+v.ShapeRecords.StateMoveTo)
	{
		SHAPERECORD* cur=&(v.ShapeRecords);
		while(1)
		{
			cur->next=new SHAPERECORD(&v,bs);
			cur=cur->next;
			if(cur->TypeFlag+cur->StateNewStyles+cur->StateLineStyle+cur->StateFillStyle1+cur->StateFillStyle0+cur->StateMoveTo==0)
				break;
		}
	}
	return s;
}

istream& operator>>(istream& s, LINESTYLE2& v)
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
}

istream& operator>>(istream& s, LINESTYLE& v)
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

istream& operator>>(istream& s, MORPHLINESTYLE& v)
{
	s >> v.StartWidth >> v.EndWidth >> v.StartColor >> v.EndColor;
	return s;
}

std::istream& operator>>(std::istream& in, TEXTRECORD& v)
{
	BitStream bs(in);
	v.TextRecordType=UB(1,bs);
	v.StyleFlagsReserved=UB(3,bs);
	if(v.StyleFlagsReserved)
		LOG(ERROR,"Reserved bits not so reserved");
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

std::istream& operator>>(std::istream& s, GRADRECORD& v)
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

std::istream& operator>>(std::istream& s, GRADIENT& v)
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
		LOG(NOT_IMPLEMENTED,"Reverting to fixed function");
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

	glBindTexture(GL_TEXTURE_2D,rt->data_tex);

	if(FillStyleType==0x00)
	{
		//LOG(TRACE,"Fill color");
		glColor3f(1,0,0);
		glTexCoord4f(float(Color.Red)/256.0f,
			float(Color.Green)/256.0f,
			float(Color.Blue)/256.0f,
			float(Color.Alpha)/256.0f);
	}
	else if(FillStyleType==0x10)
	{
		//LOG(TRACE,"Fill gradient");
		glColor3f(0,1,0);

		color_entry buffer[256];
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
		glTexImage2D(GL_TEXTURE_2D,0,4,256,1,0,GL_RGBA,GL_FLOAT,buffer);
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Reverting to fixed function");
		FILLSTYLE::fixedColor(0.5,0.5,0);
	}
}

void FILLSTYLE::fixedColor(float r, float g, float b)
{
	glBindTexture(GL_TEXTURE_2D,rt->data_tex);

	//Let's abuse of glColor and glTexCoord to transport
	//custom information
	glColor3f(1,0,0);
	glTexCoord4f(r,g,b,1);
}

std::istream& operator>>(std::istream& s, FILLSTYLE& v)
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
	else if(v.FillStyleType==0x10 || v.FillStyleType==0x12)
	{
		s >> v.GradientMatrix;
		v.Gradient.version=v.version;
		s >> v.Gradient;
	}
	else if(v.FillStyleType==0x41 || v.FillStyleType==0x42 || v.FillStyleType==0x43)
	{
		s >> v.BitmapId >> v.BitmapMatrix;
	}
	else
	{
		LOG(ERROR,"Not supported fill style " << (int)v.FillStyleType << "... Aborting");
	}
	return s;
}


std::istream& operator>>(std::istream& s, MORPHFILLSTYLE& v)
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
		LOG(ERROR,"Not supported fill style " << (int)v.FillStyleType << "... Aborting");
	}
	return s;
}

GLYPHENTRY::GLYPHENTRY(TEXTRECORD* p,BitStream& bs):parent(p)
{
	GlyphIndex = UB(parent->parent->GlyphBits,bs);
	GlyphAdvance = SB(parent->parent->AdvanceBits,bs);
}

SHAPERECORD::SHAPERECORD(SHAPE* p,BitStream& bs):parent(p),next(0)
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
		if(StateNewStyles)
		{
			SHAPEWITHSTYLE* ps=static_cast<SHAPEWITHSTYLE*>(parent);
			bs.pos=0;
			FILLSTYLEARRAY a;
			bs.f >> ps->FillStyles;
			LINESTYLEARRAY b;
			bs.f >> ps->LineStyles;
			parent->NumFillBits=UB(4,bs);
			parent->NumLineBits=UB(4,bs);
		}
	}
}

std::istream& operator>>(std::istream& stream, CXFORMWITHALPHA& v)
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

std::istream& operator>>(std::istream& stream, MATRIX& v)
{
	BitStream bs(stream);
	v.HasScale=UB(1,bs);
	if(v.HasScale)
	{
		v.NScaleBits=UB(5,bs);
		v.ScaleX=FB(v.NScaleBits,bs);
		v.ScaleY=FB(v.NScaleBits,bs);
	}
	else
	{
		v.ScaleX=1;
		v.ScaleY=1;
	}
	v.HasRotate=UB(1,bs);
	if(v.HasRotate)
	{
		v.NRotateBits=UB(5,bs);
		v.RotateSkew0=FB(v.NRotateBits,bs);
		v.RotateSkew1=FB(v.NRotateBits,bs);
	}
	v.NTranslateBits=UB(5,bs);
	v.TranslateX=SB(v.NTranslateBits,bs);
	v.TranslateY=SB(v.NTranslateBits,bs);
	return stream;
}

std::istream& operator>>(std::istream& stream, BUTTONRECORD& v)
{
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

	if(v.ButtonHasFilterList | v.ButtonHasBlendMode)
		LOG(ERROR,"Button record not yet totally supported");

	return stream;
}

void DictionaryDefinable::define(ISWFObject* g)
{
/*	DictionaryTag* t=p->root->dictionaryLookup(dict_id);
	ISWFObject* o=dynamic_cast<ISWFObject*>(t);
	if(o==NULL)
	{
		//Should not happen in real live
		ISWFObject* ret=new ASObject;
		g->setVariableByName(p->Name,new ASObject);
	}
	else
	{
		ISWFObject* ret=o->clone();
		if(ret->constructor)
			ret->constructor->call(ret,NULL);
		p->setWrapped(ret);
		g->setVariableByName(p->Name,ret);
	}*/
}

ISWFObject::ISWFObject():parent(NULL),max_slot_index(0),ref_count(1),constructor(NULL),debug(0),
	class_index(-1),manager(NULL),type(T_INVALID)
{
}

/*void ISWFObject::decRef()
{
//	if(ref_count==0)
//		abort();
	ref_count--;
	if(ref_count==0)
	{
		if(manager)
			manager->put(this);
		else
			delete this;
	}
}*/

ISWFObject::ISWFObject(const ISWFObject& o):ref_count(1),debug(0),manager(NULL)
{
	parent=o.parent;
	constructor=o.constructor;
	if(constructor)
		constructor->incRef();
	
/*	std::map<Qname,ISWFObject*> Variables;	
	std::map<Qname,IFunction*> Setters;
	std::map<Qname,IFunction*> Getters;
	std::vector<ISWFObject*> slots;
	std::vector<var_iterator> slots_vars;
	int max_slot_index;
	bool binded;*/
}

ISWFObject* ISWFObject::clone()
{
	abort();
}

ISWFObject::~ISWFObject()
{
	if(constructor)
		constructor->decRef();

	map<Qname,obj_var>::iterator it=Variables.begin();
	for(it;it!=Variables.end();it++)
	{
		if(it->second.var)
			it->second.var->decRef();
		if(it->second.setter)
			it->second.setter->decRef();
		if(it->second.getter)
			it->second.getter->decRef();
	}
}

void ISWFObject::initSlot(int n,ISWFObject* o,const Qname& s)
{
	if(n-1<slots_vars.size())
		slots_vars[n-1]=Variables.find(s);
	else
	{
		slots_vars.resize(n,Variables.end());
		slots_vars[n-1]=Variables.find(s);
	}
}

void ISWFObject::setSlot(int n,ISWFObject* o)
{
	if(n-1<slots_vars.size())
	{
		if(slots_vars[n-1]!=Variables.end())
		{
			slots_vars[n-1]->second.var->decRef();
			slots_vars[n-1]->second.var=o;
		}
		else
			abort();
	}
	else
	{
		LOG(ERROR,"Setting slot out of range");
		abort();
		//slots_vars.resize(n);
		//slots[n-1]=o;
	}
}

ISWFObject* RegisterNumber::clone()
{
	return rt->execContext->regs[index];
}

string RegisterNumber::toString() const
{
	char buf[20];
	snprintf(buf,20,"Register %i",index);
	return STRING(buf);
}

string ISWFObject::getNameAt(int index)
{
	if(index<Variables.size())
	{
		map<Qname,obj_var>::iterator it=Variables.begin();

		for(int i=0;i<index;i++)
			it++;

		return it->first.name;
	}
	else
	{
		LOG(ERROR,"Index too big");
		abort();
	}
}

int ISWFObject::numVariables()
{
	return Variables.size();
}

std::istream& operator>>(std::istream& s, CLIPEVENTFLAGS& v)
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

std::istream& operator>>(std::istream& s, CLIPACTIONRECORD& v)
{
	s >> v.EventFlags;
	if(v.EventFlags.isNull())
		return s;
	s >> v.ActionRecordSize;
	LOG(NOT_IMPLEMENTED,"Skipping " << v.ActionRecordSize << " of action data");
	ignore(s,v.ActionRecordSize);
	return s;
}

bool CLIPACTIONRECORD::isLast()
{
	return EventFlags.isNull();
}

std::istream& operator>>(std::istream& s, CLIPACTIONS& v)
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

ISWFObject* abstract_d(number_t i)
{
	Number* ret=dManager->get<Number>();
	ret->val=i;
	return ret;
}

ISWFObject* abstract_b(bool i)
{
	return new Boolean(i);
}

ISWFObject* abstract_i(intptr_t i)
{
	Integer* ret=iManager->get<Integer>();
	ret->val=i;
	return ret;
}

void tiny_string::fromInt(int i)
{
	buf[19]=0;
	start=19;

	if(i<0)
		abort();

	do
	{
		start--;
		*(buf+start)='0'+(i%10);
		i/=10;
	}
	while(i!=0);
}

/*void Manager::put(ISWFObject* o)
{
	if(available.size()>10)
		delete o;
	else
		available.push_back(o);
}*/

