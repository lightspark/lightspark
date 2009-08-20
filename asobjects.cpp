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
	setVariableByName("width",&width);
	setVariableByName("height",&height);
}

ASArray::ASArray()
{
	type=T_ARRAY;
	constructor=new Function(_constructor);
	setVariableByName("Call",new Function(ASArray::Array));
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
	th->setGetterByName("length",new Function(_getLength));
	//th->setVariableByName(Qname(AS3,"push"),new Function(_push));
	th->setVariableByName("push",new Function(_push));
	th->setVariableByName("pop",new Function(_pop));
	th->setVariableByName("shift",new Function(shift));
	th->setVariableByName("unshift",new Function(unshift));
	//th->setVariableByName(Qname(AS3,"shift"),new Function(shift));
	//th->length.incRef();
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

/*void ASMovieClipLoader::_register()
{
	setVariableByName("constructor",new Function(constructor));
	setVariableByName("addListener",new Function(addListener));
}*/

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

/*void ASXML::_register()
{
	setVariableByName("constructor",new Function(constructor));
	setVariableByName("load",new Function(load));
}*/

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

		data[index].data=o;
		data[index].type=STACK_OBJECT;
	}
	else
		ASObject::setVariableByMultiname(name,o);
}

void ASArray::setVariableByName(const Qname& name, ISWFObject* o)
{
	bool number=true;
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

		data[index]=o;
	}
	else
		ASObject::setVariableByName(name,o);
}

ISWFObject* ASArray::getVariableByName(const Qname& name, ISWFObject*& owner)
{
	ISWFObject* ret;
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

	return ret;
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
			setVariableByName("toString",ret);
			owner=this;
		}
	}

	if(!owner && super)
		ret=super->getVariableByMultiname(name,owner);

	if(!owner && prototype)
		ret=prototype->getVariableByMultiname(name,owner);

	return ret;
}

ISWFObject* ASObject::getVariableByName(const Qname& name, ISWFObject*& owner)
{
	ISWFObject* ret=ISWFObject::getVariableByName(name,owner);
	if(!owner && super)
		ret=super->getVariableByName(name,owner);

	if(!owner && prototype)
		ret=prototype->getVariableByName(name,owner);

	return ret;
}

ASObject::ASObject(Manager* m):
	debug_id(0),prototype(NULL),super(NULL),ISWFObject(m)
{
	type=T_OBJECT;
}

ASObject::ASObject():
	debug_id(0),prototype(NULL),super(NULL)
{
	type=T_OBJECT;
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
	setVariableByName("Call",new Function(ASString::String));
	setVariableByName("toString",new Function(ASObject::_toString));
}

ASString::ASString(const string& s):data(s)
{
	type=T_STRING;
	setVariableByName("Call",new Function(ASString::String));
	setVariableByName("toString",new Function(ASObject::_toString));
}

ASArray::~ASArray()
{
	for(int i=0;i<data.size();i++)
	{
		if(data[i].type==STACK_OBJECT && data[i].data)
			data[i].data->decRef();
	}
}

ASFUNCTIONBODY(ASString,String)
{
	ASString* th=dynamic_cast<ASString*>(obj);
	if(args->at(0)->getObjectType()==T_NUMBER)
	{
		Number* n=dynamic_cast<Number*>(args->at(0));
		ostringstream oss;
		oss << setprecision(8) << fixed << *n;

		th->data=oss.str();
		return th;
	}
	if(args->at(0)->getObjectType()==T_ARRAY)
	{
		cout << "array" << endl;
		cout << args->at(0)->toString() << endl;
		return new ASString(args->at(0)->toString());
	}
	if(args->at(0)->getObjectType()==T_UNDEFINED)
	{
		return new ASString("undefined");
	}
	else
	{
		LOG(CALLS,"Cannot convert " << args->at(0)->getObjectType() << " to String");
		abort();
	}
}

string ASArray::toString() const
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
	return ret;
}

string ASObject::toString() const
{
	cerr << getObjectType() << endl;
	return "Object";
}

string Boolean::toString() const
{
	return (val)?"true":"false";
}

string ASString::toString() const
{
	return data.data();
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

string Undefined::toString() const
{
	return string("undefined");
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
	setVariableByName(".Call",new Function(call));
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

bool Number::isLess(const ISWFObject* o) const
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=dynamic_cast<const Integer*>(o);
		return val<*i;
	}
	else
	{
		return ISWFObject::isLess(o);
	}
}

string Number::toString() const
{
	char buf[20];
	snprintf(buf,20,"%g",val);
	return string(buf);
}

Date::Date():year(-1),month(-1),date(-1),hour(-1),minute(-1),second(-1),millisecond(-1)
{
	constructor=new Function(_constructor);
}

ASFUNCTIONBODY(Date,_constructor)
{
	Date* th=static_cast<Date*>(obj);
	th->setVariableByName("getTimezoneOffset",new Function(getTimezoneOffset));
	th->setVariableByName("valueOf",new Function(valueOf));
	th->setVariableByName(Qname(AS3,"getTime"),new Function(getTime));
	th->setVariableByName(Qname(AS3,"getHours"),new Function(getHours));
	th->setVariableByName(Qname(AS3,"getMinutes"),new Function(getMinutes));
	th->setVariableByName(Qname(AS3,"getSeconds"),new Function(getMinutes));
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

	ISWFObject* ret;
	if(!bound)
	{
		obj->incRef();
		ret=val(obj,args,cc);
	}
	else
	{
		LOG(CALLS,"Calling with closure");
		closure_this->incRef();
		ret=val(closure_this,args,cc);
	}
	delete cc;
	return ret;
}

ISWFObject* Function::call(ISWFObject* obj, arguments* args)
{
	if(!bound)
		return val(obj,args);
	else
	{
		LOG(CALLS,"Calling with closure");
		LOG(CALLS,"args 0 " << args->at(0));
		return val(closure_this,args);
	}
}

Math::Math()
{
	setVariableByName("PI",new Number(M_PI));
	setVariableByName("sqrt",new Function(sqrt));
	setVariableByName("atan2",new Function(atan2));
	setVariableByName("floor",new Function(floor));
	setVariableByName("random",new Function(random));
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

ASFUNCTIONBODY(Math,floor)
{
	Number* n=dynamic_cast<Number*>(args->at(0));
	if(n)
		return new Number(::floor(*n));
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

string Null::toString() const
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
