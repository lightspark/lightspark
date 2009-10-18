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

#include <list>
#include <algorithm>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <math.h>

#include "abc.h"
#include "asobjects.h"
#include "flashevents.h"
#include "swf.h"

using namespace std;

extern __thread SystemState* sys;

ASStage::ASStage():width(640),height(480)
{
	setVariableByQName("width","",&width);
	setVariableByQName("height","",&height);
}

ASArray::ASArray()
{
	type=T_ARRAY;
	constructor=new Function(_constructor);
	ASObject::setVariableByQName("Call","",new Function(ASArray::Array));
	ASObject::setVariableByQName("toString","",new Function(ASObject::_toString));
}

ASFUNCTIONBODY(ASArray,Array)
{
	ISWFObject* ret=new ASArray();
	ret->constructor->call(ret,args);
	return ret;
}

ASFUNCTIONBODY(ASArray,_constructor)
{
	ASArray* th=static_cast<ASArray*>(obj);
	th->setGetterByQName("length","",new Function(_getLength));
	th->ASObject::setVariableByQName("pop","",new Function(_pop));
	th->ASObject::setVariableByQName("shift","",new Function(shift));
	th->ASObject::setVariableByQName("unshift","",new Function(unshift));
	th->ASObject::setVariableByQName("join",AS3,new Function(join));
	th->ASObject::setVariableByQName("push",AS3,new Function(_push));
	//th->setVariableByName("push",new Function(_push));
	//th->setVariableByName(Qname(AS3,"shift"),new Function(shift));
	if(args==NULL)
		return NULL;

	if(args->size()!=0)
	{
		LOG(CALLS,"Called Array constructor");
		th->resize(args->size());
		for(int i=0;i<args->size();i++)
		{
			th->set(i,args->at(i));
			args->at(i)->incRef();
		}
	}
}

ASFUNCTIONBODY(ASArray,_getLength)
{
	ASArray* th=static_cast<ASArray*>(obj);
	return abstract_i(th->data.size());
}

ASFUNCTIONBODY(ASArray,shift)
{
	ASArray* th=static_cast<ASArray*>(obj);
	if(th->data.empty())
		return new Undefined;
	ISWFObject* ret;
	if(th->data[0].type==STACK_OBJECT)
		ret=th->data[0].data;
	else
		abort();
	th->data.erase(th->data.begin());
	return ret;
}

ASFUNCTIONBODY(ASArray,join)
{
		ASArray* th=static_cast<ASArray*>(obj);
		ISWFObject* del=args->at(0);
		string ret;
		for(int i=0;i<th->size();i++)
		{
			ret+=th->at(i)->toString();
			if(i!=th->size()-1)
				ret+=del->toString();
		}
		return new ASString(ret);
}

ASFUNCTIONBODY(ASArray,_pop)
{
	ASArray* th=static_cast<ASArray*>(obj);
	ISWFObject* ret;
	if(th->data.back().type==STACK_OBJECT)
		ret=th->data.back().data;
	else
		abort();
	th->data.pop_back();
	return ret;
}

ASFUNCTIONBODY(ASArray,unshift)
{
	ASArray* th=static_cast<ASArray*>(obj);
	if(args->size()!=1)
	{
		LOG(ERROR,"Multiple unshift");
		abort();
	}
	th->data.insert(th->data.begin(),args->at(0));
	args->at(0)->incRef();
	return new Integer(th->size());
}

ASFUNCTIONBODY(ASArray,_push)
{
	ASArray* th=static_cast<ASArray*>(obj);
	if(args->size()!=1)
	{
		LOG(ERROR,"Multiple push");
		abort();
	}
	th->push(args->at(0));
	args->at(0)->incRef();
	return new Integer(th->size());
}

ASMovieClipLoader::ASMovieClipLoader()
{
}

ASFUNCTIONBODY(ASMovieClipLoader,constructor)
{
	LOG(NOT_IMPLEMENTED,"Called MovieClipLoader constructor");
	return NULL;
}

ASFUNCTIONBODY(ASMovieClipLoader,addListener)
{
	LOG(NOT_IMPLEMENTED,"Called MovieClipLoader::addListener");
	return NULL;
}

ASXML::ASXML()
{
	xml_buf=new char[1024*20];
	xml_index=0;
}

ASFUNCTIONBODY(ASXML,constructor)
{
	LOG(NOT_IMPLEMENTED,"Called XML constructor");
	return NULL;
}


size_t ASXML::write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	ASXML* th=(ASXML*)userp;
	memcpy(th->xml_buf+th->xml_index,buffer,size*nmemb);
	th->xml_index+=size*nmemb;
	return size*nmemb;
}

ASFUNCTIONBODY(ASXML,load)
{
	ASXML* th=dynamic_cast<ASXML*>(obj);
	LOG(NOT_IMPLEMENTED,"Called ASXML::load " << args->at(0)->toString());
/*	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	string base("www.youtube.com");
	string url=base+args->at(0)->toString();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, (string(url)).c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, obj);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	xmlDocPtr doc=xmlReadMemory(th->xml_buf,th->xml_index,(string(url)).c_str(),NULL,0);

	bool found;
	IFunction* on_load=obj->getVariableByName("onLoad",found)->toFunction();
	arguments a;
	a.push(new Integer(1));
	on_load->call(NULL,&a);*/
	return new Integer(1);
}

bool ASArray::isEqual(const ISWFObject* r) const
{
	if(r->getObjectType()!=T_ARRAY)
		return false;
	else
	{
		const ASArray* ra=static_cast<const ASArray*>(r);
		int size=data.size();
		if(size!=ra->size())
			return false;

		for(int i=0;i<size;i++)
		{
			if(data[i].type!=STACK_OBJECT)
				abort();
			if(!data[i].data->isEqual(ra->at(i)))
				return false;
		}
		return true;
	}
}

intptr_t ASArray::getVariableByMultiname_i(const multiname& name, ISWFObject*& owner)
{
	intptr_t ret;
	owner=NULL;
	int index=0;

	switch(name.name_type)
	{
		case multiname::NAME_STRING:
			for(int i=0;i<name.name_s.len();i++)
			{
				char a=name.name_s[i];
				if(a>='0' && a<='9')
				{
					index*=10;
					index+=(a-'0');
				}
				else
				{
					index=-1;
					break;
				}
			}
			break;
		case multiname::NAME_INT:
			index=name.name_i;
			break;
	}

	if(index!=-1 && index<data.size())
	{
			switch(data[index].type)
			{
				case STACK_OBJECT:
					ret=data[index].data->toInt();
					break;
				case STACK_INT:
					ret=data[index].data_i;
					break;
			}
			owner=this;
	}
	else
		ret=ASObject::getVariableByMultiname_i(name,owner);

	return ret;
}

ISWFObject* ASArray::getVariableByMultiname(const multiname& name, ISWFObject*& owner)
{
	ISWFObject* ret;
	owner=NULL;
	int index=0;

	switch(name.name_type)
	{
		case multiname::NAME_STRING:
			for(int i=0;i<name.name_s.len();i++)
			{
				char a=name.name_s[i];
				if(a>='0' && a<='9')
				{
					index*=10;
					index+=(a-'0');
				}
				else
				{
					index=-1;
					break;
				}
			}
			break;
		case multiname::NAME_INT:
			index=name.name_i;
			break;
	}

	if(index!=-1 && index<data.size())
	{
			switch(data[index].type)
			{
				case STACK_OBJECT:
					ret=data[index].data;
					if(ret==NULL)
					{
						ret=new Undefined;
						data[index].data=ret;
					}
					break;
				case STACK_INT:
					//cout << "Not efficent" << endl;
					ret=abstract_i(data[index].data_i);
					ret->fake_decRef();
					break;
			}
			owner=this;
	}
	else
		ret=ASObject::getVariableByMultiname(name,owner);

	return ret;
}

void ASArray::setVariableByMultiname_i(multiname& name, intptr_t value)
{
	int index=0;
	switch(name.name_type)
	{
		case multiname::NAME_STRING:
			for(int i=0;i<name.name_s.len();i++)
			{
				char a=name.name_s[i];
				if(a>='0' && a<='9')
				{
					index*=10;
					index+=(a-'0');
				}
				else
				{
					ASObject::setVariableByMultiname(name,abstract_i(value));
					return;
				}
			}
			break;
		case multiname::NAME_INT:
			index=name.name_i;
			break;
	}

	int size=data.size();
	if(index==size)
		data.push_back(data_slot(value));
	else if(index>size)
	{
		data.resize(index);
		data.push_back(data_slot(value));
	}
	else
	{
		if(data[index].type==STACK_OBJECT && data[index].data)
			data[index].data->decRef();

		data[index].data_i=value;
		data[index].type=STACK_INT;
	}
}

void ASArray::setVariableByMultiname(multiname& name, ISWFObject* o)
{
	int index=0;
	if(name.name_type==multiname::NAME_STRING)
	{
		for(int i=0;i<name.name_s.len();i++)
		{
			if(!isdigit(name.name_s[i]))
			{
				index=-1;
				break;
			}

		}
		if(index==0)
			index=atoi(name.name_s);
	}
	else if(name.name_type==multiname::NAME_INT)
		index=name.name_i;

	if(index!=-1)
	{
		if(index>=data.capacity())
		{
			//Heuristic, we increse the array 20%
			int new_size=max(index+1,data.size()*2);
			data.reserve(new_size);
		}
		if(index>=data.size())
			resize(index+1);

		if(data[index].type==STACK_OBJECT && data[index].data)
			data[index].data->decRef();

		if(o->getObjectType()==T_INTEGER)
		{
			Integer* i=static_cast<Integer*>(o);
			data[index].data_i=i->val;
			data[index].type=STACK_INT;
			o->decRef();
		}
		else
		{
			data[index].data=o;
			data[index].type=STACK_OBJECT;
		}
	}
	else
		ASObject::setVariableByMultiname(name,o);
}

void ASArray::setVariableByQName(const tiny_string& name, const tiny_string& ns, ISWFObject* o)
{
	int index=0;
	for(int i=0;i<name.len();i++)
	{
		if(!isdigit(name[i]))
		{
			index=-1;
			break;
		}

	}
	if(index==0)
		index=atoi((const char*)name);

	if(index!=-1)
	{
		if(index>=data.capacity())
		{
			//Heuristic, we increse the array 20%
			int new_size=max(index+1,data.size()*2);
			data.reserve(new_size);
		}
		if(index>=data.size())
			resize(index+1);

		if(data[index].type==STACK_OBJECT && data[index].data)
			data[index].data->decRef();

		if(o->getObjectType()==T_INTEGER)
		{
			Integer* i=static_cast<Integer*>(o);
			data[index].data_i=i->val;
			data[index].type=STACK_INT;
			o->decRef();
		}
		else
		{
			data[index].data=o;
			data[index].type=STACK_OBJECT;
		}
	}
	else
		ASObject::setVariableByQName(name,ns,o);
}

ISWFObject* ASArray::getVariableByQName(const tiny_string& name, const tiny_string& ns, ISWFObject*& owner)
{
	abort();
/*	ISWFObject* ret;
	bool number=true;
	owner=NULL;
	for(int i=0;i<name.name.size();i++)
	{
		if(!isdigit(name.name[i]))
		{
			number=false;
			break;
		}

	}
	if(number)
	{
		int index=atoi(name.name.c_str());
		if(index<data.size())
		{
			if(data[index].type!=STACK_OBJECT)
				abort();
			ret=data[index].data;
			owner=this;
		}
	}

	if(!owner)
		ret=ASObject::getVariableByName(name,owner);

	return ret;*/
}

ISWFObject* ASObject::getVariableByMultiname(const multiname& name, ISWFObject*& owner)
{
	ISWFObject* ret=ISWFObject::getVariableByMultiname(name,owner);

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

void ASObject::setVariableByMultiname(multiname& name, ISWFObject* o)
{
	ISWFObject* owner=NULL;

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

ISWFObject* ASObject::getVariableByQName(const tiny_string& name, const tiny_string& ns, ISWFObject*& owner)
{
	ISWFObject* ret=ISWFObject::getVariableByQName(name,ns,owner);
	if(!owner && super)
		ret=super->getVariableByQName(name,ns,owner);

	if(!owner && prototype)
		ret=prototype->getVariableByQName(name,ns,owner);

	return ret;
}

ASObject::ASObject(Manager* m):
	debug_id(0),prototype(NULL),super(NULL),ISWFObject(m)
{
	type=T_OBJECT;
	class_name="Object";
}

ASObject::ASObject():
	debug_id(0),prototype(NULL),super(NULL)
{
	type=T_OBJECT;
	class_name="Object";
}

ASObject::ASObject(const tiny_string& c, ISWFObject* v):
	debug_id(0),prototype(NULL),super(NULL)
{
	type=T_OBJECT;
	class_name=c;
	mostDerived=v;
}

ASObject::~ASObject()
{
	if(super)
		super->decRef();
	if(prototype)
		prototype->decRef();
}

ASFUNCTIONBODY(ASObject,_toString)
{
	return new ASString(obj->toString());
}

ASFUNCTIONBODY(ASObject,_constructor)
{
}

ASString::ASString()
{
	type=T_STRING;
	setVariableByQName("Call","",new Function(ASString::String));
	setVariableByQName("toString","",new Function(ASObject::_toString));
	setVariableByQName("split",AS3,new Function(split));
	setVariableByQName("replace",AS3,new Function(replace));
	setVariableByQName("indexOf",AS3,new Function(indexOf));
	setVariableByQName("charCodeAt",AS3,new Function(charCodeAt));
	setGetterByQName("length","",new Function(_getLength));
}

ASString::ASString(const string& s):data(s)
{
	type=T_STRING;
	setVariableByQName("Call","",new Function(ASString::String));
	setVariableByQName("toString","",new Function(ASObject::_toString));
	setVariableByQName("split",AS3,new Function(split));
	setVariableByQName("replace",AS3,new Function(replace));
	setVariableByQName("indexOf",AS3,new Function(indexOf));
	setVariableByQName("charCodeAt",AS3,new Function(charCodeAt));
	setGetterByQName("length","",new Function(_getLength));
}

ASString::ASString(const tiny_string& s):data((const char*)s)
{
	type=T_STRING;
	setVariableByQName("Call","",new Function(ASString::String));
	setVariableByQName("toString","",new Function(ASObject::_toString));
	setVariableByQName("split",AS3,new Function(split));
	setVariableByQName("replace",AS3,new Function(replace));
	setVariableByQName("indexOf",AS3,new Function(indexOf));
	setVariableByQName("charCodeAt",AS3,new Function(charCodeAt));
	setGetterByQName("length","",new Function(_getLength));
}

ASString::ASString(const char* s):data(s)
{
	type=T_STRING;
	setVariableByQName("Call","",new Function(ASString::String));
	setVariableByQName("toString","",new Function(ASObject::_toString));
	setVariableByQName("split",AS3,new Function(split));
	setVariableByQName("replace",AS3,new Function(replace));
	setGetterByQName("length","",new Function(_getLength));
}

ASFUNCTIONBODY(ASString,_getLength)
{
	ASString* th=static_cast<ASString*>(obj);
	return abstract_i(th->data.size());
}
ASArray::~ASArray()
{
	for(int i=0;i<data.size();i++)
	{
		if(data[i].type==STACK_OBJECT && data[i].data)
			data[i].data->decRef();
	}
}

ASFUNCTIONBODY(ASString,split)
{
	ASString* th=static_cast<ASString*>(obj);
	cout << th->data << endl;
	ASArray* ret=new ASArray();
	ret->_constructor(ret,NULL);
	ISWFObject* delimiter=args->at(0);
	if(delimiter->getObjectType()==T_STRING)
	{
		ASString* del=static_cast<ASString*>(delimiter);
		int start=0;
		do
		{
			int match=th->data.find(del->data,start);
			if(match==-1)
				match=th->data.size();
			ASString* s=new ASString(th->data.substr(start,match));
			ret->push(s);
			start=match+del->data.size();
		}
		while(start<th->data.size());
	}
	else
		abort();

	return ret;
}

ASFUNCTIONBODY(ASString,String)
{
	if(args->at(0)->getObjectType()==T_INTEGER)
	{
		Integer* n=static_cast<Integer*>(args->at(0));
		ostringstream oss;
		oss << setprecision(8) << fixed << *n;

		return new ASString(oss.str());
	}
	else if(args->at(0)->getObjectType()==T_NUMBER)
	{
		Number* n=static_cast<Number*>(args->at(0));
		ostringstream oss;
		oss << setprecision(8) << fixed << *n;

		return new ASString(oss.str());
	}
	else if(args->at(0)->getObjectType()==T_ARRAY)
	{
		cout << "array" << endl;
		cout << args->at(0)->toString() << endl;
		return new ASString(args->at(0)->toString());
	}
	else if(args->at(0)->getObjectType()==T_UNDEFINED)
	{
		return new ASString("undefined");
	}
	else
	{
		LOG(CALLS,"Cannot convert " << args->at(0)->getObjectType() << " to String");
		abort();
	}
}

tiny_string ASArray::toString() const
{
	string ret;
	for(int i=0;i<data.size();i++)
	{
		if(data[i].type!=STACK_OBJECT)
			abort();
		ret+=data[i].data->toString();
		if(i!=data.size()-1)
			ret+=',';
	}
	return ret.c_str();
}

tiny_string ASObject::toString() const
{
	cerr << getObjectType() << endl;
	return "Object";
}

tiny_string Boolean::toString() const
{
	return (val)?"true":"false";
}

tiny_string ASString::toString() const
{
	return data.c_str();
}

double ASString::toNumber() const
{
	LOG(ERROR,"Cannot convert string " << data << " to float");
	return 0;
}

ASFUNCTIONBODY(Undefined,call)
{
	LOG(CALLS,"Undefined function");
}

tiny_string Undefined::toString() const
{
	return "null";
}

bool ASString::isEqual(const ISWFObject* r) const
{
	if(r->getObjectType()==T_STRING)
	{
		const ASString* s=dynamic_cast<const ASString*>(r);
		if(s->data==data)
			return true;
		else
			return false;
	}
	else
		return false;
}

bool Boolean::isEqual(const ISWFObject* r) const
{
	if(r->getObjectType()==T_BOOLEAN)
	{
		const Boolean* b=static_cast<const Boolean*>(r);
		return b->val==val;
	}
	else
	{
		return ISWFObject::isEqual(r);
	}
}

bool Undefined::isEqual(const ISWFObject* r) const
{
	if(r->getObjectType()==T_UNDEFINED)
		return true;
	if(r->getObjectType()==T_NULL)
		return true;
	else
		return false;
}

Undefined::Undefined()
{
	type=T_UNDEFINED;
//	setVariableByName(".Call",new Function(call));
}

/*Number::Number(const ISWFObject* obj)
{
	const Integer* i=dynamic_cast<const Integer*>(obj);
	if(i)
	{
		val=i->val;
		return;
	}
	const Number* n=dynamic_cast<const Number*>(obj);
	if(n)
	{
		val=n->val;
		return;
	}

	cout << obj->getObjectType() << endl;
	abort();
}*/

void Number::copyFrom(const ISWFObject* o)
{
	if(o->getObjectType()==T_NUMBER)
	{
		const Number* n=static_cast<const Number*>(o);
		val=n->val;
	}
	else if(o->getObjectType()==T_INTEGER)
	{
		const Integer* n=static_cast<const Integer*>(o);
		val=n->val;
	}
	else
	{
		LOG(ERROR,"Copying Number from type " << o->getObjectType() << " is not supported");
		abort();
	}
	
	LOG(TRACE,"Set to " << val);
}

bool Number::isEqual(const ISWFObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toNumber();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toNumber();
	else
	{
		return ISWFObject::isLess(o);
	}
}

bool Number::isGreater(const ISWFObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return val>i->val;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		const Number* i=static_cast<const Number*>(o);
		return val>i->val;
	}
	else
	{
		return ISWFObject::isGreater(o);
	}
}

bool Number::isLess(const ISWFObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return val<i->val;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		const Number* i=static_cast<const Number*>(o);
		return val<i->val;
	}
	else
	{
		return ISWFObject::isLess(o);
	}
}

tiny_string Number::toString() const
{
	char buf[20];
	snprintf(buf,20,"%g",val);
	return buf;
}

Date::Date():year(-1),month(-1),date(-1),hour(-1),minute(-1),second(-1),millisecond(-1)
{
	constructor=new Function(_constructor);
}

ASFUNCTIONBODY(Date,_constructor)
{
	Date* th=static_cast<Date*>(obj);
	th->setVariableByQName("getTimezoneOffset","",new Function(getTimezoneOffset));
	th->setVariableByQName("valueOf","",new Function(valueOf));
	th->setVariableByQName("getTime",AS3,new Function(getTime));
	th->setVariableByQName("getFullYear","",new Function(getFullYear));
	//th->setVariableByName(Qname(AS3,"getFullYear"),new Function(getFullYear));
	th->setVariableByQName("getHours",AS3,new Function(getHours));
	th->setVariableByQName("getMinutes",AS3,new Function(getMinutes));
	th->setVariableByQName("getSeconds",AS3,new Function(getMinutes));
	th->setVariableByQName("toString",AS3,new Function(ASObject::_toString));
	th->year=1990;
	th->month=1;
	th->date=1;
	th->hour=0;
	th->minute=0;
	th->second=0;
	th->millisecond=0;
}

ASFUNCTIONBODY(Date,getTimezoneOffset)
{
	LOG(NOT_IMPLEMENTED,"getTimezoneOffset");
	return new Number(120);
}

ASFUNCTIONBODY(Date,getFullYear)
{
	Date* th=static_cast<Date*>(obj);
	return new Number(th->year);
}

ASFUNCTIONBODY(Date,getHours)
{
	Date* th=static_cast<Date*>(obj);
	return new Number(th->hour);
}

ASFUNCTIONBODY(Date,getMinutes)
{
	Date* th=static_cast<Date*>(obj);
	return new Number(th->minute);
}

ASFUNCTIONBODY(Date,getTime)
{
	Date* th=static_cast<Date*>(obj);
	return new Number(th->toInt());
}

ASFUNCTIONBODY(Date,valueOf)
{
	Date* th=dynamic_cast<Date*>(obj);
	return new Number(th->toInt());
}

int Date::toInt() const
{
	int ret=0;
	//TODO: leap year
	ret+=(year-1990)*365*24*3600*1000;
	//TODO: month length
	ret+=(month-1)*30*24*3600*1000;
	ret+=(date-1)*24*3600*1000;
	ret+=hour*3600*1000;
	ret+=minute*60*1000;
	ret+=second*1000;
	ret+=millisecond;
	return ret;
}

tiny_string Date::toString() const
{
	return "Wed Apr 12 15:30:17 GMT-0700 2006";
}

IFunction* SyntheticFunction::toFunction()
{
	return this;
}

IFunction* Function::toFunction()
{
	return this;
}

SyntheticFunction::SyntheticFunction(method_info* m):mi(m)
{
	class_index=-2;
	val=m->synt_method();
}

ISWFObject* SyntheticFunction::fast_call(ISWFObject* obj, ISWFObject** args, int numArgs)
{
	if(mi->needsArgs())
		abort();

	call_context* cc=new call_context(mi,args,numArgs);
	cc->scope_stack=func_scope;
	for(int i=0;i<func_scope.size();i++)
		func_scope[i]->incRef();

	if(bound)
	{
		LOG(CALLS,"Calling with closure " << this);
		obj=closure_this;
	}

	cc->locals[0]=obj;
	for(int i=numArgs;i<mi->numArgs();i++)
	{
		cout << "filling arg " << i << endl;
		cc->locals[i+1]=new Undefined;
	}

	if(mi->needsRest()) //TODO
	{
		ASArray* rest=new ASArray();
		rest->_constructor(rest,NULL);
		cc->locals[mi->numArgs()+1]=rest;
	}
	obj->incRef();
	ISWFObject* ret=val(obj,NULL,cc);

	delete cc;
	return ret;
}

ISWFObject* SyntheticFunction::call(ISWFObject* obj, arguments* args)
{
	if(val==NULL)
	{
		LOG(NOT_IMPLEMENTED,"Not initialized function");
		return NULL;
	}

	call_context* cc=new call_context(mi);
	cc->scope_stack=func_scope;
	for(int i=0;i<func_scope.size();i++)
		func_scope[i]->incRef();

	if(bound)
	{
		LOG(CALLS,"Calling with closure " << this);
		obj=closure_this;
	}

	cc->locals[0]=obj;
	int i=0;
	if(args)
	{
		for(i;i<args->size();i++)
		{
			cc->locals[i+1]=args->at(i);
			cc->locals[i+1]->incRef();
		}
	}
	for(i;i<mi->numArgs();i++)
	{
		cout << "filling arg " << i << endl;
		cc->locals[i+1]=new Undefined;
	}

	if(mi->needsRest()) //TODO
	{
		ASArray* rest=new ASArray();
		rest->_constructor(rest,NULL);
		cc->locals[mi->numArgs()+1]=rest;
		/*llvm::Value* rest=Builder.CreateCall(ex->FindFunctionNamed("createRest"));
		constant = llvm::ConstantInt::get(int_type, param_count+1);
		t=Builder.CreateGEP(locals,constant);
		Builder.CreateStore(rest,t);*/
	}

	obj->incRef();
	ISWFObject* ret=val(obj,args,cc);

	delete cc;
	return ret;
}

ISWFObject* Function::fast_call(ISWFObject* obj, ISWFObject** args,int num_args)
{
	arguments arg(num_args,false);
	for(int i=0;i<num_args;i++)
		arg.set(i,args[i]);
	return call(obj,&arg);
}

ISWFObject* Function::call(ISWFObject* obj, arguments* args)
{
	if(!bound)
		return val(obj,args);
	else
	{
		LOG(CALLS,"Calling with closure " << this);
		LOG(CALLS,"args 0 " << args->at(0));
		return val(closure_this,args);
	}
}

Math::Math()
{
	setVariableByQName("PI","",new Number(M_PI));
	setVariableByQName("sqrt","",new Function(sqrt));
	setVariableByQName("atan2","",new Function(atan2));
	setVariableByQName("abs","",new Function(abs));
	setVariableByQName("floor","",new Function(floor));
	setVariableByQName("random","",new Function(random));
}

ASFUNCTIONBODY(Math,atan2)
{
	Number* n1=dynamic_cast<Number*>(args->at(0));
	Number* n2=dynamic_cast<Number*>(args->at(1));
	if(n1 && n2)
		return new Number(::atan2(*n1,*n2));
	else
	{
		LOG(CALLS, "Invalid argument type ("<<args->at(0)->getObjectType()<<','<<args->at(1)->getObjectType()<<')' );
		abort();
	}
}

ASFUNCTIONBODY(Math,abs)
{
	Number* n=dynamic_cast<Number*>(args->at(0));
	if(n)
	{
		Integer* ret=new Integer(::abs(*n));
		return ret;
	}
	else
	{
		LOG(TRACE,"Invalid argument");
		abort();
	}
}

ASFUNCTIONBODY(Math,floor)
{
	Number* n=dynamic_cast<Number*>(args->at(0));
	if(n)
	{
		Integer* ret=new Integer(::floor(*n));
		return ret;
	}
	else
	{
		LOG(TRACE,"Invalid argument");
		abort();
	}
}

ASFUNCTIONBODY(Math,sqrt)
{
	Number* n=dynamic_cast<Number*>(args->at(0));
	if(n)
		return new Number(::sqrt(*n));
	else
	{
		LOG(TRACE,"Invalid argument");
		abort();
	}
}

ASFUNCTIONBODY(Math,random)
{
	double ret=rand();
	ret/=RAND_MAX;
	return new Integer(ret);
}

tiny_string Null::toString() const
{
	return "null";
}

bool Null::isEqual(const ISWFObject* r) const
{
	if(r->getObjectType()==T_NULL)
		return true;
	else if(r->getObjectType()==T_UNDEFINED)
		return true;
	else
		return false;
}

RegExp::RegExp()
{
	constructor=new Function(_constructor);
}

ASFUNCTIONBODY(RegExp,_constructor)
{
	RegExp* th=static_cast<RegExp*>(obj);
	th->re=args->at(0)->toString();
	if(args->size()>1)
		th->flags=args->at(1)->toString();
}

ASFUNCTIONBODY(ASString,charCodeAt)
{
	LOG(NOT_IMPLEMENTED,"ASString::charCodeAt not really implemented");
	ASString* th=static_cast<ASString*>(obj);
	int index=args->at(0)->toInt();
	return new Integer(th->data[index]);
}

ASFUNCTIONBODY(ASString,indexOf)
{
	LOG(NOT_IMPLEMENTED,"ASString::indexOf not really implemented");
	ASString* th=static_cast<ASString*>(obj);
	return new Integer(-1);
}

ASFUNCTIONBODY(ASString,replace)
{
	LOG(NOT_IMPLEMENTED,"ASString::replace not really implemented");
	ASString* th=static_cast<ASString*>(obj);
	return new ASString(th->data);
}
