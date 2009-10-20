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
#include <assert.h>
#include "swf.h"
#include "geometry.h"

using namespace std;
extern __thread SystemState* sys;
extern __thread RenderThread* rt;
extern __thread ParseThread* pt;
extern __thread Manager* iManager;
extern __thread Manager* dManager;

/*tiny_string ConstantReference::toString() const
{
	return (const char*)rt->vm.getConstantByIndex(index);
}

double ConstantReference::toNumber() const
{
	string s=rt->vm.getConstantByIndex(index);
	double ret= strtod(s.c_str(),NULL);
	cout << "crto " << ret <<  " from " << s << endl;
	return ret;
}

ASObject* ConstantReference::clone()
{
	return new ASString((const char*)rt->vm.getConstantByIndex(index));
}

int ConstantReference::toInt() const
{
	LOG(ERROR,"Cannot convert ConstRef to Int");
	return 0;
}*/

tiny_string ASObject::toString() const
{
	cout << "Cannot convert object of type " << getObjectType() << " to String" << endl;
	abort();
	return "Cannot convert object to String";
}

bool ASObject::isGreater(const ASObject* r) const
{
	LOG(NOT_IMPLEMENTED,"Greater than comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	return false;
}

bool ASObject::isLess(const ASObject* r) const
{
	LOG(NOT_IMPLEMENTED,"Less than comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	abort();
	return false;
}

int multiname::count=0;

tiny_string multiname::qualifiedString()
{
	assert(ns.size()==1);
	assert(name_type==NAME_STRING);
	if(ns[0]=="")
		return name_s;
	else
	{
		tiny_string ret=ns[0];
		ret+="::";
		ret+=name_s;
		cout <<  ret << endl;
		abort();
		return ret;
	}
}

multiname::~multiname()
{
	count--;
}

bool Integer::isGreater(const ASObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return val>*i;
	}
	else
	{
		return ASObject::isGreater(o);
	}
}

bool Integer::isLess(const ASObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return val<*i;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		const Number* i=static_cast<const Number*>(o);
		return val<double(*i);
	}
	else
		return ASObject::isLess(o);
}

bool Integer::isEqual(const ASObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toInt();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toInt();
	else
	{
		return ASObject::isEqual(o);
	}
}

bool ASObject::isEqual(const ASObject* r) const
{
	LOG(NOT_IMPLEMENTED,"Equal comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	return false;
}

IFunction* ASObject::toFunction()
{
	LOG(ERROR,"Cannot convert object of type " << getObjectType() << " to Function");
	return NULL;
}


int ASObject::toInt() const
{
	LOG(ERROR,"Cannot convert object of type " << getObjectType() << " to Int");
	cout << "imanager " << iManager->available.size() << endl;
	cout << "dmanager " << dManager->available.size() << endl;
	abort();
	return 0;
}

double ASObject::toNumber() const
{
	LOG(ERROR,"Cannot convert object of type " << getObjectType() << " to float");
	abort();
	return 0;
}

obj_var* variables_map::findObjVar(const tiny_string& name, const tiny_string& ns, bool create)
{
	pair<var_iterator, var_iterator> ret=Variables.equal_range(name);
	if(ret.first->first==name)
	{
		//Check if this namespace is already present
		var_iterator& start=ret.first;
		for(start;start!=ret.second;start++)
		{
			if(start->second.first==ns)
				return &start->second.second;
		}
	}

	//Name not present, insert it if we have to create it
	if(create)
	{
		var_iterator inserted=Variables.insert(ret.first,make_pair(name, make_pair(ns, obj_var() ) ) );
		return &inserted->second.second;
	}
	else
		return NULL;
}

bool variables_map::hasProperty(const tiny_string& name)
{
	return Variables.find(name)!=Variables.end();
}

bool ASObject::hasProperty(const tiny_string& name)
{
	bool ret=Variables.hasProperty(name);
	if(!ret && super)
		ret=super->hasProperty(name);
	if(!ret && prototype)
		ret=prototype->hasProperty(name);

	return ret;
}

void ASObject::setGetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o)
{
	obj_var* obj=Variables.findObjVar(name,ns);
	if(obj->getter)
	{
		//ret.first->second.getter->decRef();
		//Should never happen
		abort();
	}
	obj->getter=o;
}

void ASObject::setSetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o)
{
	obj_var* obj=Variables.findObjVar(name,ns);
	if(obj->setter)
	{
		//ret.first->second.setter->decRef();
		//Should never happen
		abort();
	}
	obj->setter=o;
}

void ASObject::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o)
{
	obj_var* obj=Variables.findObjVar(name,ns);

	if(obj->setter)
	{
		//Call the setter
		LOG(CALLS,"Calling the setter");
		arguments args(1);
		args.set(0,o);
		//TODO: check
		o->incRef();
		obj->setter->call(this,&args);
		LOG(CALLS,"End of setter");
	}
	else
	{
		if(obj->var)
			obj->var->decRef();
		obj->var=o;
	}
}

void ASObject::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	setVariableByMultiname(name,abstract_i(value));
/*	if(name.namert)
		name.name=name.namert->toString();

	pair<map<Qname, ASObject*>::iterator,bool> ret=Variables.insert(pair<Qname,ASObject*>(name.name,o));
	if(!ret.second)
	{
		if(ret.first->second)
			ret.first->second->decRef();
		ret.first->second=o;
	}
	return o;*/
}

obj_var* variables_map::findObjVar(const multiname& mname, bool create)
{
	tiny_string name;
	if(mname.name_type==multiname::NAME_INT)
		name=tiny_string(mname.name_i);
	else if(mname.name_type==multiname::NAME_STRING)
		name=mname.name_s;

	pair<var_iterator, var_iterator> ret=Variables.equal_range(name);
	if(ret.first->first==name)
	{
		//Check if one the namespace is already present
		if(mname.ns.empty())
			abort();
		for(int i=0;i<mname.ns.size();i++)
		{
			const tiny_string& ns=mname.ns[i];
			var_iterator start=ret.first;
			for(start;start!=ret.second;start++)
			{
				if(start->second.first==ns)
					return &start->second.second;
			}
		}
	}

	//Name not present, insert it, if the multiname has a single ns and if we have to insert it
	if(create)
	{
		if(mname.ns.size()>1)
		{
			//Hack, insert with empty name
			//Here the object MUST exist
			var_iterator inserted=Variables.insert(ret.first,make_pair(name, make_pair("", obj_var() ) ) );
			return &inserted->second.second;
		}
		var_iterator inserted=Variables.insert(ret.first,make_pair(name, make_pair(mname.ns[0], obj_var() ) ) );
		return &inserted->second.second;
	}
	else
		return NULL;
}

ASFUNCTIONBODY(ASObject,_toString)
{
	return new ASString(obj->toString());
}

ASFUNCTIONBODY(ASObject,_constructor)
{
}

void ASObject::setVariableByMultiname(const multiname& name, ASObject* o)
{
	ASObject* owner=NULL;

	obj_var* obj;
	ASObject* cur=this;
	do
	{
		obj=cur->Variables.findObjVar(name,false);
		if(obj)
			owner=cur;
		cur=cur->super;
	}
	while(cur && owner==NULL);

	if(owner) //Variable already defined change it
	{
		if(obj->setter)
		{
			//Call the setter
			LOG(CALLS,"Calling the setter");
			arguments args(1);
			args.set(0,o);
			//TODO: check
			o->incRef();
			//

			obj->setter->call(owner,&args);
			LOG(CALLS,"End of setter");
		}
		else
		{
			if(obj->var)
				obj->var->decRef();
			obj->var=o;
		}
	}
	else //Insert it
	{
		obj=Variables.findObjVar(name,true);
		obj->var=o;
	}
}

intptr_t ASObject::getVariableByMultiname_i(const multiname& name, ASObject*& owner)
{
	ASObject* ret=getVariableByMultiname(name,owner);
	if(ret)
		return ret->toInt();
	else
		abort();
}

ASObject* ASObject::getVariableByMultiname(const multiname& name, ASObject*& owner)
{
	obj_var* obj=Variables.findObjVar(name,false);
	ASObject* ret;

	if(obj==NULL)
		owner=NULL;
	else
	{
		if(obj->getter)
		{
			//Call the getter
			LOG(CALLS,"Calling the getter");
			ASObject* ret=obj->getter->call(this,NULL);
			LOG(CALLS,"End of getter");
			owner=this;
			assert(ret);
			return ret;
		}
		else
		{
			owner=this;
			assert(obj->var);
			return obj->var;
		}
	}

	if(!owner)
	{
		//Check if we should do lazy definition
		if(name.name_s=="toString")
		{
			ret=new Function(ASObject::_toString);
			setVariableByQName("toString","",ret);
			owner=this;
		}
	}

	if(!owner && super)
		ret=super->getVariableByMultiname(name,owner);

	if(!owner && prototype)
		ret=prototype->getVariableByMultiname(name,owner);

	return ret;
}

ASObject* variables_map::getVariableByString(const std::string& name)
{
	//Slow linear lookup, should be avoided
	var_iterator it=Variables.begin();
	for(it;it!=Variables.end();it++)
	{
		string cur(it->second.first.raw_buf());
		if(!cur.empty())
			cur+='.';
		cur+=it->first.raw_buf();
		if(cur==name)
		{
			if(it->second.second.getter)
				abort();
			return it->second.second.var;
		}
	}
	
	return NULL;
}

ASObject* ASObject::getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner)
{
	obj_var* obj=Variables.findObjVar(name,ns,false);
	ASObject* ret;

	if(obj==NULL)
		owner=NULL;
	else
	{
		if(obj->getter)
		{
			//Call the getter
			LOG(CALLS,"Calling the getter");
			ret=obj->getter->call(this,NULL);
			LOG(CALLS,"End of getter");
			owner=this;
			return ret;
		}
		else
		{
			owner=this;
			return obj->var;
		}
	}

	if(!owner && super)
		ret=super->getVariableByQName(name,ns,owner);

	if(!owner && prototype)
		ret=prototype->getVariableByQName(name,ns,owner);

	return ret;
}

std::ostream& operator<<(std::ostream& s, const tiny_string& r)
{
	s << r.buf;
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

void variables_map::dumpVariables()
{
	var_iterator it=Variables.begin();
	for(it;it!=Variables.end();it++)
		cout << '[' << it->second.first << "] "<< it->first << endl;
}

tiny_string Integer::toString() const
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

void DictionaryDefinable::define(ASObject* g)
{
	abort();
/*	DictionaryTag* t=p->root->dictionaryLookup(dict_id);
	ASObject* o=dynamic_cast<ASObject*>(t);
	if(o==NULL)
	{
		//Should not happen in real live
		ASObject* ret=new ASObject;
		g->setVariableByName(p->Name,new ASObject);
	}
	else
	{
		ASObject* ret=o->clone();
		if(ret->constructor)
			ret->constructor->call(ret,NULL);
		p->setWrapped(ret);
		g->setVariableByName(p->Name,ret);
	}*/
}

variables_map::~variables_map()
{
	var_iterator it=Variables.begin();
	for(it;it!=Variables.end();it++)
	{
		if(it->second.second.var)
			it->second.second.var->decRef();
		if(it->second.second.setter)
			it->second.second.setter->decRef();
		if(it->second.second.getter)
			it->second.second.getter->decRef();
	}
}

ASObject::ASObject(const tiny_string& c, ASObject* v, Manager* m):
	prototype(NULL),super(NULL),parent(NULL),ref_count(1),
	constructor(NULL),class_index(-1),manager(m),type(T_OBJECT),class_name(c)
{
	mostDerived=(v)?v:this;
}

ASObject::ASObject(const ASObject& o):
	prototype(o.prototype),super(o.super),manager(NULL),parent(NULL),ref_count(1),
	constructor(NULL),class_index(o.class_index),type(o.type),
	mostDerived(this),class_name(o.class_name)
{
	parent=o.parent;
	constructor=o.constructor;
	if(constructor)
		constructor->incRef();

	if(super)
		super->incRef();
	if(prototype)
		prototype->incRef();
	
/*	std::map<Qname,ASObject*> Variables;	
	std::map<Qname,IFunction*> Setters;
	std::map<Qname,IFunction*> Getters;
	std::vector<ASObject*> slots;
	std::vector<var_iterator> slots_vars;*/
}

ASObject::~ASObject()
{
	if(super)
		super->decRef();
	if(prototype)
		prototype->decRef();
	if(constructor)
		constructor->decRef();
}

void variables_map::initSlot(int n,ASObject* o, const tiny_string& name, const tiny_string& ns)
{
	if(n>slots_vars.size())
		slots_vars.resize(n,Variables.end());

	typedef std::multimap<tiny_string,std::pair<tiny_string, obj_var> >::iterator var_iterator;
	pair<var_iterator, var_iterator> ret=Variables.equal_range(name);
	cout << ret.first->first << endl;
	if(ret.first->first==name)
	{
		//Check if this namespace is already present
		var_iterator& start=ret.first;
		for(start;start!=ret.second;start++)
		{
			if(start->second.first==ns)
			{
				slots_vars[n-1]=start;
				return;
			}
		}
	}

	//Name not present, no good
	abort();
}

void variables_map::setSlot(int n,ASObject* o)
{
	if(n-1<slots_vars.size())
	{
		if(slots_vars[n-1]!=Variables.end())
		{
			if(slots_vars[n-1]->second.second.setter)
				abort();
			slots_vars[n-1]->second.second.var->decRef();
			slots_vars[n-1]->second.second.var=o;
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

/*ASObject* RegisterNumber::clone()
{
	return rt->execContext->regs[index];
}

tiny_string RegisterNumber::toString() const
{
	char buf[20];
	snprintf(buf,20,"Register %i",index);
	return buf;
}*/

tiny_string variables_map::getNameAt(int index)
{
	if(index<Variables.size())
	{
		var_iterator it=Variables.begin();

		for(int i=0;i<index;i++)
			it++;

		return tiny_string(it->first);
	}
	else
	{
		LOG(ERROR,"Index too big");
		abort();
	}
}

int ASObject::numVariables()
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

ASObject* abstract_d(number_t i)
{
	Number* ret=dManager->get<Number>();
	ret->val=i;
	return ret;
}

ASObject* abstract_b(bool i)
{
	return new Boolean(i);
}

ASObject* abstract_i(intptr_t i)
{
	Integer* ret=iManager->get<Integer>();
	ret->val=i;
	return ret;
}
