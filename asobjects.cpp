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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include <list>
#include <algorithm>
//#include <libxml/parser.h>
#include <pcrecpp.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <cmath>

#include "abc.h"
#include "asobjects.h"
#include "flashevents.h"
#include "swf.h"
#include "compat.h"
#include "class.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;

REGISTER_CLASS_NAME(Array);
REGISTER_CLASS_NAME2(ASQName,"QName");
REGISTER_CLASS_NAME(Namespace);
REGISTER_CLASS_NAME(IInterface);
REGISTER_CLASS_NAME(Date);
REGISTER_CLASS_NAME(RegExp);
REGISTER_CLASS_NAME(Math);
REGISTER_CLASS_NAME(ASString);

Array::Array()
{
	type=T_ARRAY;
	//constructor=new Function(_constructor);
	//ASObject::setVariableByQName("toString","",new Function(ASObject::_toString));
}

void Array::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

void Array::buildTraits(ASObject* o)
{
	o->setGetterByQName("length","",new Function(_getLength));
	o->ASObject::setVariableByQName("pop","",new Function(_pop));
	o->ASObject::setVariableByQName("shift","",new Function(shift));
	o->ASObject::setVariableByQName("unshift","",new Function(unshift));
	o->ASObject::setVariableByQName("join",AS3,new Function(join));
	o->ASObject::setVariableByQName("push",AS3,new Function(_push));
	o->ASObject::setVariableByQName("sort",AS3,new Function(_sort));
	o->ASObject::setVariableByQName("concat",AS3,new Function(_concat));
}

ASFUNCTIONBODY(Array,_constructor)
{
	Array* th=static_cast<Array*>(obj->implementation);

	if(args)
	{
		if(args->size()==1)
		{
			int size=args->at(0)->toInt();
			LOG(LOG_CALLS,"Creating array of length " << size);
			th->resize(size);
		}
		else
		{
			LOG(LOG_CALLS,"Called Array constructor");
			th->resize(args->size());
			for(int i=0;i<args->size();i++)
			{
				th->set(i,args->at(i));
				args->at(i)->incRef();
			}
		}
	}
	return NULL;
}

ASFUNCTIONBODY(Array,_getLength)
{
	Array* th=static_cast<Array*>(obj->implementation);
	return abstract_i(th->data.size());
}

ASFUNCTIONBODY(Array,shift)
{
	Array* th=static_cast<Array*>(obj->implementation);
	if(th->data.empty())
		return new Undefined;
	ASObject* ret;
	if(th->data[0].type==STACK_OBJECT)
		ret=th->data[0].data;
	else
		abort();
	th->data.erase(th->data.begin());
	return ret;
}

ASFUNCTIONBODY(Array,join)
{
		Array* th=static_cast<Array*>(obj->implementation);
		ASObject* del=args->at(0);
		string ret;
		for(int i=0;i<th->size();i++)
		{
			ret+=th->at(i)->toString().raw_buf();
			if(i!=th->size()-1)
				ret+=del->toString().raw_buf();
		}
		return Class<ASString>::getInstanceS(true,ret)->obj;
}

ASFUNCTIONBODY(Array,_concat)
{
	Array* th=static_cast<Array*>(obj->implementation);
	Array* ret=Class<Array>::getInstanceS(true);
	ret->data=th->data;
	if(args->size()>=1 && args->at(0)->getObjectType()==T_ARRAY)
	{
		assert(args->size()==1);
		Array* tmp=Class<Array>::cast(args->at(0)->implementation);
		ret->data.insert(ret->data.end(),tmp->data.begin(),tmp->data.end());
	}
	else
	{
		//Insert the arguments in the array
		ret->data.insert(th->data.end(),args->data.begin(),args->data.end());
		ret->data.reserve(ret->data.size()+args->size());
		for(int i=0;i<args->size();i++)
			ret->push(args->at(i));
	}

	//All the elements in the new array should be increffed, as args will be deleted and
	//this array could die too
	for(int i=0;i<ret->data.size();i++)
	{
		
		if(ret->data[i].type==STACK_OBJECT)
			ret->data[i].data->incRef();
	}
	
	return ret->obj;
}

ASFUNCTIONBODY(Array,_pop)
{
	Array* th=static_cast<Array*>(obj->implementation);
	ASObject* ret;
	if(th->data.back().type==STACK_OBJECT)
		ret=th->data.back().data;
	else
		abort();
	th->data.pop_back();
	return ret;
}

ASFUNCTIONBODY(Array,_sort)
{
	LOG(LOG_NOT_IMPLEMENTED,"Array::sort not really implemented");
	return obj;
}

ASFUNCTIONBODY(Array,unshift)
{
	Array* th=static_cast<Array*>(obj->implementation);
	if(args->size()!=1)
	{
		LOG(LOG_ERROR,"Multiple unshift");
		abort();
	}
	th->data.insert(th->data.begin(),args->at(0));
	args->at(0)->incRef();
	return new Integer(th->size());
}

ASFUNCTIONBODY(Array,_push)
{
	Array* th=static_cast<Array*>(obj->implementation);
	if(args->size()!=1)
	{
		LOG(LOG_ERROR,"Multiple push");
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
	LOG(LOG_NOT_IMPLEMENTED,"Called MovieClipLoader constructor");
	return NULL;
}

ASFUNCTIONBODY(ASMovieClipLoader,addListener)
{
	LOG(LOG_NOT_IMPLEMENTED,"Called MovieClipLoader::addListener");
	return NULL;
}

ASXML::ASXML()
{
	xml_buf=new char[1024*20];
	xml_index=0;
}

ASFUNCTIONBODY(ASXML,constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"Called XML constructor");
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
	ASXML* th=static_cast<ASXML*>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Called ASXML::load " << args->at(0)->toString());
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

bool Array::isEqual(bool& ret, ASObject* r)
{
	if(r->getObjectType()!=T_ARRAY)
		ret=false;
	else
	{
		const Array* ra=static_cast<const Array*>(r->implementation);
		int size=data.size();
		if(size!=ra->size())
			ret=false;

		for(int i=0;i<size;i++)
		{
			if(data[i].type!=STACK_OBJECT)
				abort();
			if(!data[i].data->isEqual(ra->at(i)))
				ret=false;
		}
		ret=true;
	}
	return true;
}

bool Array::getVariableByMultiname_i(const multiname& name, intptr_t& out)
{
	int index=0;
	if(!isValidMultiname(name,index))
		return false;

	if(index<data.size())
	{
		switch(data[index].type)
		{
			case STACK_OBJECT:
			{
				assert(data[index].data!=NULL);
				if(data[index].data->getObjectType()==T_INTEGER)
				{
					Integer* i=static_cast<Integer*>(data[index].data);
					out=i->toInt();
				}
				else if(data[index].data->getObjectType()==T_NUMBER)
				{
					Number* i=static_cast<Number*>(data[index].data);
					out=i->toInt();
				}
				else
					abort();
				break;
			}
			case STACK_INT:
				out=data[index].data_i;
				break;
		}
		return true;
	}
	else
		return false;
}

bool Array::getVariableByMultiname(const multiname& name, ASObject*& out)
{
	int index=0;
	if(!isValidMultiname(name,index))
		return false;

	if(index<data.size())
	{
		switch(data[index].type)
		{
			case STACK_OBJECT:
				out=data[index].data;
				if(out==NULL)
				{
					out=new Undefined;
					data[index].data=out;
				}
				break;
			case STACK_INT:
				//cout << "Not efficent" << endl;
				out=abstract_i(data[index].data_i);
				out->fake_decRef();
				break;
		}
		return true;
	}
	else
		return false;
}

bool Array::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	int index=0;
	if(!isValidMultiname(name,index))
		return false;

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
	data[index].data_i=value;
	data[index].type=STACK_INT;

	return true;
}

bool Array::isValidMultiname(const multiname& name, int& index)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	assert(name.ns.size()!=0);
	if(name.ns[0].name!="")
		return false;

	index=0;
	int len;
	switch(name.name_type)
	{
		//We try to convert this to an index, otherwise bail out
		case multiname::NAME_STRING:
			len=name.name_s.len();
			assert(len);
			for(int i=0;i<len;i++)
			{
				if(name.name_s[i]<'0' || name.name_s[i]>'9')
					return false;

				index*=10;
				index+=(name.name_s[i]-'0');
			}
			break;
		//This is already an int, so its good enough
		case multiname::NAME_INT:
			index=name.name_i;
			break;
		case multiname::NAME_NUMBER:
			//TODO: check that this is really an integer
			index=name.name_d;
			break;
		default:
			abort();
	}
	return true;
}

bool Array::setVariableByMultiname(const multiname& name, ASObject* o)
{
	int index=0;
	if(!isValidMultiname(name,index))
		return false;

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
	return true;
}

bool Array::isValidQName(const tiny_string& name, const tiny_string& ns, int& index)
{
	if(ns!="")
		return false;
	assert(name.len()!=0);
	index=0;
	//First we try to convert the string name to an index, at the first non-digit
	//we bail out
	for(int i=0;i<name.len();i++)
	{
		if(!isdigit(name[i]))
			return false;

		index*=10;
		index+=(name[i]-'0');
	}
	return true;
}

bool Array::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o)
{
	int index=0;
	if(!isValidQName(name,ns,index))
		return false;

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
	return true;
}

bool Array::getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& out)
{
	abort();
	return NULL;
/*	ASObject* ret;
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

ASString::ASString()
{
	type=T_STRING;
}

ASString::ASString(const string& s):data(s)
{
	type=T_STRING;
}

ASString::ASString(const tiny_string& s):data(s.raw_buf())
{
	type=T_STRING;
}

ASString::ASString(const char* s):data(s)
{
	type=T_STRING;
}

/*ASFUNCTIONBODY(ASString,_constructor)
{
}*/

ASFUNCTIONBODY(ASString,_getLength)
{
	ASString* th=static_cast<ASString*>(obj->implementation);
	return abstract_i(th->data.size());
}

void ASString::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
//	c->constructor=new Function(_constructor);
}

void ASString::buildTraits(ASObject* o)
{
	o->setVariableByQName("toString","",new Function(ASObject::_toString));
	o->setVariableByQName("split",AS3,new Function(split));
	o->setVariableByQName("substr",AS3,new Function(substr));
	o->setVariableByQName("replace",AS3,new Function(replace));
	o->setVariableByQName("concat",AS3,new Function(concat));
	o->setVariableByQName("indexOf",AS3,new Function(indexOf));
	o->setVariableByQName("charCodeAt",AS3,new Function(charCodeAt));
	o->setVariableByQName("slice",AS3,new Function(slice));
	o->setVariableByQName("toLowerCase",AS3,new Function(toLowerCase));
	o->setGetterByQName("length","",new Function(_getLength));
}

Array::~Array()
{
	for(int i=0;i<data.size();i++)
	{
		if(data[i].type==STACK_OBJECT && data[i].data)
			data[i].data->decRef();
	}
}

ASFUNCTIONBODY(ASString,split)
{
	ASString* th=static_cast<ASString*>(obj->implementation);
	Array* ret=Class<Array>::getInstanceS(true);
	ASObject* delimiter=args->at(0);
	if(delimiter->getObjectType()==T_STRING)
	{
		ASString* del=static_cast<ASString*>(delimiter->implementation);
		int start=0;
		do
		{
			int match=th->data.find(del->data,start);
			if(match==-1)
				match=th->data.size();
			ASString* s=Class<ASString>::getInstanceS(true,(th->data.substr(start,match)));
			ret->push(s->obj);
			start=match+del->data.size();
		}
		while(start<th->data.size());
	}
	else
		abort();

	return ret->obj;
}

ASFUNCTIONBODY(ASString,substr)
{
	ASString* th=static_cast<ASString*>(obj->implementation);
	int start=args->at(0)->toInt();
	if(start<0)
		start=th->data.size()+start;

	int len=0x7fffffff;
	if(args->size()==2)
		len=args->at(1)->toInt();

	return Class<ASString>::getInstanceS(true,th->data.substr(start,len))->obj;
}

bool Array::toString(tiny_string& ret)
{
	ret=toString();
	return true;
}

tiny_string Array::toString() const
{
	string ret;
	for(int i=0;i<data.size();i++)
	{
		if(data[i].type==STACK_OBJECT)
		{
			if(data[i].data)
				ret+=data[i].data->toString().raw_buf();
		}
		else if(data[i].type==STACK_INT)
		{
			char buf[20];
			snprintf(buf,20,"%i",data[i].data_i);
			ret+=buf;
		}
		else
			abort();

		if(i!=data.size()-1)
			ret+=',';
	}
	return ret.c_str();
}

bool Array::nextValue(int index, ASObject*& out)
{
	assert(index<data.size());
	assert(data[index].type==STACK_OBJECT);
	out=data[index].data;
	return true;
}

bool Array::hasNext(int& index, bool& out)
{
	out=index<data.size();
	index++;
	return true;
}

tiny_string Boolean::toString() const
{
	return (val)?"true":"false";
}

tiny_string ASString::toString() const
{
	return data.c_str();
}

bool ASString::toString(tiny_string& ret)
{
	ret=toString();
	return true;
}

ASFUNCTIONBODY(Undefined,call)
{
	LOG(LOG_CALLS,"Undefined function");
	return NULL;
}

tiny_string Undefined::toString() const
{
	return "null";
}

bool ASString::isEqual(bool& ret, ASObject* r)
{
	if(r->getObjectType()==T_STRING)
	{
		const ASString* s=static_cast<const ASString*>(r->implementation);
		if(s->data==data)
			ret=true;
		else
			ret=false;
	}
	else
		ret=false;

	return true;
}

bool Boolean::isEqual(ASObject* r)
{
	if(r->getObjectType()==T_BOOLEAN)
	{
		const Boolean* b=static_cast<const Boolean*>(r);
		return b->val==val;
	}
	else
	{
		return ASObject::isEqual(r);
	}
}

bool Undefined::isEqual(ASObject* r)
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
}

ASFUNCTIONBODY(Integer,_toString)
{
	Integer* th=static_cast<Integer*>(obj);
	int radix=10;
	char buf[20];
	if(args->size()==1)
		radix=args->at(0)->toUInt();
	assert(radix==10 || radix==16);
	if(radix==10)
		snprintf(buf,20,"%i",th->val);
	else if(radix==16)
		snprintf(buf,20,"%x",th->val);

	return Class<ASString>::getInstanceS(true,buf)->obj;
}

tiny_string Integer::toString() const
{
	char buf[20];
	if(val<0)
	{
		//This can be a slow path, as it not used for array access
		snprintf(buf,20,"%i",val);
		return buf;
	}
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

bool Number::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toNumber();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toNumber();
	else
	{
		return ASObject::isLess(o);
	}
}

bool Number::isGreater(ASObject* o)
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
		return ASObject::isGreater(o);
	}
}

bool Number::isLess(ASObject* o)
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
		return ASObject::isLess(o);
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
}

void Date::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

void Date::buildTraits(ASObject* o)
{
	o->setVariableByQName("getTimezoneOffset","",new Function(getTimezoneOffset));
	o->setVariableByQName("valueOf","",new Function(valueOf));
	o->setVariableByQName("getTime",AS3,new Function(getTime));
	o->setVariableByQName("getFullYear","",new Function(getFullYear));
	o->setVariableByQName("getHours",AS3,new Function(getHours));
	o->setVariableByQName("getMinutes",AS3,new Function(getMinutes));
	o->setVariableByQName("getSeconds",AS3,new Function(getMinutes));
	//o->setVariableByQName("toString",AS3,new Function(ASObject::_toString));
}

ASFUNCTIONBODY(Date,_constructor)
{
	Date* th=static_cast<Date*>(obj->implementation);
	th->year=1969;
	th->month=1;
	th->date=1;
	th->hour=0;
	th->minute=0;
	th->second=0;
	th->millisecond=0;
	return NULL;
}

ASFUNCTIONBODY(Date,getTimezoneOffset)
{
	LOG(LOG_NOT_IMPLEMENTED,"getTimezoneOffset");
	return new Number(120);
}

ASFUNCTIONBODY(Date,getFullYear)
{
	Date* th=static_cast<Date*>(obj->implementation);
	return new Number(th->year);
}

ASFUNCTIONBODY(Date,getHours)
{
	Date* th=static_cast<Date*>(obj->implementation);
	return new Number(th->hour);
}

ASFUNCTIONBODY(Date,getMinutes)
{
	Date* th=static_cast<Date*>(obj->implementation);
	return new Number(th->minute);
}

ASFUNCTIONBODY(Date,getTime)
{
	Date* th=static_cast<Date*>(obj->implementation);
	return new Number(th->toInt());
}

ASFUNCTIONBODY(Date,valueOf)
{
	Date* th=static_cast<Date*>(obj->implementation);
	return new Number(th->toInt());
}

bool Date::toInt(int& ret)
{
	ret=toInt();
	return true;
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

bool Date::toString(tiny_string& ret)
{
	ret=toString();
	return true;
}

tiny_string Date::toString() const
{
	return "Wed Dec 31 16:00:00 GMT-0800 1969";
}

IFunction* SyntheticFunction::toFunction()
{
	return this;
}

IFunction* Function::toFunction()
{
	return this;
}

SyntheticFunction::SyntheticFunction(method_info* m):mi(m),hit_count(0),val(NULL)
{
//	class_index=-2;
}

ASObject* SyntheticFunction::fast_call(ASObject* obj, ASObject** args, int numArgs, int level)
{
	if(mi->body==NULL)
	{
//		LOG(LOG_NOT_IMPLEMENTED,"Not initialized function");
		return NULL;
	}

	//Validate that the initial scope depth is correct
	assert(mi->body->init_scope_depth==func_scope.size());

	if(hit_count==3 || sys->useInterpreter==false)
	{
		//We passed the hot function threshold, synt the function
		val=mi->synt_method();
	}

	if(mi->needsArgs())
		abort();

	int args_len=mi->numArgs();
	int passedToLocals=min(numArgs,args_len);
	int passedToRest=(numArgs > args_len)?(numArgs-mi->numArgs()):0;
	int realLevel=(bound)?closure_level:level;

	call_context* cc=new call_context(mi,realLevel,args,passedToLocals);
	int i=passedToLocals;
	cc->scope_stack=func_scope;
	for(int i=0;i<func_scope.size();i++)
		func_scope[i]->incRef();

	if(bound && closure_this)
	{
		LOG(LOG_CALLS,"Calling with closure " << this);
		obj=closure_this;
	}

	cc->locals[0]=obj;
	obj->incRef();

	//Fixup missing parameters
	int missing_params=args_len-i;
	assert(missing_params<=mi->option_count);
	int starting_options=mi->option_count-missing_params;

	for(int j=starting_options;j<mi->option_count;j++)
	{
		cc->locals[i+1]=mi->getOptional(j);
		i++;
	}
	
	assert(i==mi->numArgs());

	if(mi->needsRest()) //TODO
	{
		assert(passedToRest==0);
		Array* rest=Class<Array>::getInstanceS(true);
		cc->locals[i+1]=rest->obj;
	}
	ASObject* ret;
	if(hit_count<3 && sys->useInterpreter)
	{
		//This is not an hot function, execute it using the intepreter
		ret=ABCVm::executeFunction(this,cc);
	}
	else
		ret=val(cc);


	delete cc;
	hit_count++;
	return ret;
}

ASObject* SyntheticFunction::call(ASObject* obj, arguments* args, int level)
{
	ASObject** args_fast;
	int args_len;
	if(args)
	{
		args_len=args->size();
		args_fast=new ASObject*[args_len];
		for(int i=0;i<args_len;i++)
		{
			args_fast[i]=args->at(i);
			args_fast[i]->incRef();
		}
	}
	else
	{
		args_fast=NULL;
		args_len=0;
	}
	ASObject* ret=fast_call(obj,args_fast,args_len,level);
	delete[] args_fast;
	return ret;
}

ASObject* Function::fast_call(ASObject* obj, ASObject** args,int num_args, int level)
{
	arguments arg(num_args);
	for(int i=0;i<num_args;i++)
		arg.set(i,args[i]);
	return call(obj,&arg,level);
}

ASObject* Function::call(ASObject* obj, arguments* args, int level)
{
	if(bound && closure_this)
	{
		LOG(LOG_CALLS,"Calling with closure " << this);
		return val(closure_this,args);
	}
	else
		return val(obj,args);
}

void Math::sinit(Class_base* c)
{
	c->setVariableByQName("PI","",new Number(M_PI));
	c->setVariableByQName("sqrt","",new Function(sqrt));
	c->setVariableByQName("atan2","",new Function(atan2));
	c->setVariableByQName("max","",new Function(_max));
	c->setVariableByQName("min","",new Function(_min));
	c->setVariableByQName("abs","",new Function(abs));
	c->setVariableByQName("sin","",new Function(sin));
	c->setVariableByQName("cos","",new Function(cos));
	c->setVariableByQName("floor","",new Function(floor));
	c->setVariableByQName("ceil","",new Function(ceil));
	c->setVariableByQName("round","",new Function(round));
	c->setVariableByQName("random","",new Function(random));
	c->setVariableByQName("pow","",new Function(pow));
}

ASFUNCTIONBODY(Math,atan2)
{
	double n1=args->at(0)->toNumber();
	double n2=args->at(1)->toNumber();
	return abstract_d(::atan2(n1,n2));
}

ASFUNCTIONBODY(Math,_max)
{
	double n1=args->at(0)->toNumber();
	double n2=args->at(1)->toNumber();
	return abstract_d(dmax(n1,n2));
}

ASFUNCTIONBODY(Math,_min)
{
	double n1=args->at(0)->toNumber();
	double n2=args->at(1)->toNumber();
	return abstract_d(dmin(n1,n2));
}

ASFUNCTIONBODY(Math,cos)
{
	//Angle is in radians
	double n=args->at(0)->toNumber();
	return abstract_d(::cos(n));
}

ASFUNCTIONBODY(Math,sin)
{
	//Angle is in radians
	double n=args->at(0)->toNumber();
	return abstract_d(::sin(n));
}

ASFUNCTIONBODY(Math,abs)
{
	double n=args->at(0)->toNumber();
	return abstract_d(::abs(n));
}

ASFUNCTIONBODY(Math,ceil)
{
	double n=args->at(0)->toNumber();
	return abstract_i(::ceil(n));
}

ASFUNCTIONBODY(Math,floor)
{
	double n=args->at(0)->toNumber();
	return abstract_i(::floor(n));
}

ASFUNCTIONBODY(Math,round)
{
	double n=args->at(0)->toNumber();
	return abstract_i(::round(n));
}

ASFUNCTIONBODY(Math,sqrt)
{
	double n=args->at(0)->toNumber();
	return abstract_d(::sqrt(n));
}

ASFUNCTIONBODY(Math,pow)
{
	double x=args->at(0)->toNumber();
	double y=args->at(1)->toNumber();
	return abstract_d(::pow(x,y));
}

ASFUNCTIONBODY(Math,random)
{
	double ret=rand();
	ret/=RAND_MAX;
	return abstract_d(ret);
}

tiny_string Null::toString() const
{
	return "null";
}

bool Null::isEqual(ASObject* r)
{
	if(r->getObjectType()==T_NULL)
		return true;
	else if(r->getObjectType()==T_UNDEFINED)
		return true;
	else
		return false;
}

RegExp::RegExp():global(false),ignoreCase(false),lastIndex(0)
{
}

void RegExp::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

void RegExp::buildTraits(ASObject* o)
{
//	o->setVariableByQName("exec",AS3,new Function(exec));
	o->setVariableByQName("test",AS3,new Function(test));
}

ASFUNCTIONBODY(RegExp,_constructor)
{
	RegExp* th=static_cast<RegExp*>(obj->implementation);
	th->re=args->at(0)->toString().raw_buf();
	if(args->size()>1)
	{
		const tiny_string& flags=args->at(1)->toString();
		for(int i=0;i<flags.len();i++)
		{
			switch(flags[i])
			{
				case 'g':
					th->global=true;
					break;
				case 'i':
					th->ignoreCase=true;
					break;
				case 's':
				case 'm':
				case 'x':
					abort();
			}
		}
	}
	return NULL;
}

ASFUNCTIONBODY(RegExp,exec)
{
	RegExp* th=static_cast<RegExp*>(obj->implementation);
/*	cout << "Returning tracer2" <<endl;
	return new DebugTracer("RegExp::exec");*/
	return new Null;
}

ASFUNCTIONBODY(RegExp,test)
{
	RegExp* th=static_cast<RegExp*>(obj->implementation);
	pcrecpp::RE_Options opt;
	opt.set_caseless(th->ignoreCase);

	pcrecpp::RE pcreRE(th->re,opt);
	assert(th->lastIndex==0);
	const tiny_string& arg0=args->at(0)->toString();

	bool ret=pcreRE.PartialMatch(arg0.raw_buf());
	return new Boolean(ret);
}

ASFUNCTIONBODY(ASString,slice)
{
	ASString* th=static_cast<ASString*>(obj->implementation);
	int startIndex=args->at(0)->toInt();
	int endIndex=args->at(1)->toInt();
	return Class<ASString>::getInstanceS(true,th->data.substr(startIndex,endIndex))->obj;
}

ASFUNCTIONBODY(ASString,charCodeAt)
{
	LOG(LOG_NOT_IMPLEMENTED,"ASString::charCodeAt not really implemented");
	ASString* th=static_cast<ASString*>(obj->implementation);
	int index=args->at(0)->toInt();
	return new Integer(th->data[index]);
}

ASFUNCTIONBODY(ASString,indexOf)
{
	LOG(LOG_NOT_IMPLEMENTED,"ASString::indexOf not really implemented");
	ASString* th=static_cast<ASString*>(obj->implementation);
	return new Integer(-1);
}

ASFUNCTIONBODY(ASString,toLowerCase)
{
	ASString* th=static_cast<ASString*>(obj->implementation);
	ASString* ret=Class<ASString>::getInstanceS(true);
	ret->data=th->data;
	transform(th->data.begin(), th->data.end(), ret->data.begin(), ::tolower);
	return ret->obj;
}

ASFUNCTIONBODY(ASString,replace)
{
	const ASString* th=static_cast<const ASString*>(obj->implementation);
	ASString* ret=Class<ASString>::getInstanceS(true,th->data);
	string replaceWith(args->at(1)->toString().raw_buf());
	//We have to escape '\\' because that is interpreted by pcrecpp
	int index=0;
	do
	{
		index=replaceWith.find("\\",index);
		if(index==-1) //No result
			break;
		replaceWith.replace(index,1,"\\\\");

		//Increment index to jump over the added character
		index+=2;
	}
	while(index<ret->data.size());

	assert(args->size()==2 && args->at(1)->getObjectType()==T_STRING);

	if(args->at(0)->prototype==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args->at(0)->implementation);

		pcrecpp::RE_Options opt;
		opt.set_caseless(re->ignoreCase);
		pcrecpp::RE pcreRE(re->re,opt);
		if(re->global)
			pcreRE.GlobalReplace(replaceWith,&ret->data);
		else
			pcreRE.Replace(replaceWith,&ret->data);
	}
	else if(args->at(0)->getObjectType()==T_STRING)
	{
		ASString* s=static_cast<ASString*>(args->at(0)->implementation);
		int index=0;
		do
		{
			index=ret->data.find(s->data,index);
			if(index==-1) //No result
				break;
			ret->data.replace(index,s->data.size(),replaceWith);
			index+=(replaceWith.size()-s->data.size());

		}
		while(index<ret->data.size());
	}
	else
		abort();

	return ret->obj;
}

ASFUNCTIONBODY(ASString,concat)
{
	LOG(LOG_NOT_IMPLEMENTED,"ASString::concat not really implemented");
	ASString* th=static_cast<ASString*>(obj->implementation);
	th->data+=args->at(0)->toString().raw_buf();
	th->obj->incRef();
	return th->obj;
}

Class_base::~Class_base()
{
	if(constructor)
		constructor->decRef();
}

void Class_base::addImplementedInterface(const multiname& i)
{
	interfaces.push_back(i);
}

void Class_base::addImplementedInterface(Class_base* i)
{
	interfaces_added.push_back(i);
}

tiny_string Class_base::toString() const
{
	tiny_string ret="[Class ";
	ret+=class_name;
	ret+="]";
	return ret;
}

IInterface* Class_inherit::getInstance(bool construct, arguments* args)
{
	//TODO: Ask our super for the interface to ret
	assert(super);
	//Our super should not construct, we are going to do it ourselves
	IInterface* ret=super->getInstance(false,args);
	ret->obj->max_level=max_level;
	ret->obj->prototype->decRef();
	ret->obj->prototype=this;
	ret->obj->actualPrototype=this;
	//As we are the prototype we should incRef ourself
	incRef();
	if(construct)
		ret->obj->handleConstruction(args,true,true);
	return ret;
}

void Class_inherit::buildInstanceTraits(ASObject* o) const
{
	assert(class_index!=-1);
	//The class is declared in the script and has an index
	LOG(LOG_CALLS,"Building instance traits");

	//To insert the trait in the rigth level we have to change the max_level
	int oldlevel=o->max_level;
	o->max_level=max_level;

	context->buildInstanceTraits(o,class_index);

	o->max_level=oldlevel;
}

Class_object* Class_object::getClass()
{
	//We check if we are registered in the class map
	//if not we register ourselves (see also Class<T>::getClass)
	std::map<tiny_string, Class_base*>::iterator it=sys->classes.find("Class");
	if(it==sys->classes.end()) //This class is not yet in the map, create it
	{
		Class_object* ret=new Class_object();
		sys->classes.insert(std::make_pair("Class",ret));
		return ret;
	}
	else
		return static_cast<Class_object*>(it->second);
}

const std::vector<Class_base*>& Class_base::getInterfaces() const
{
	if(!interfaces.empty())
	{
		//Recursively get interfaces implemented by this interface
		for(int i=0;i<interfaces.size();i++)
		{
			ASObject* interface_obj=getGlobal()->getVariableByMultiname(interfaces[i]).obj;
			assert(interface_obj && interface_obj->getObjectType()==T_CLASS);
			Class_base* inter=static_cast<Class_base*>(interface_obj);

			interfaces_added.push_back(inter);
			//Probe the interface for its interfaces
			inter->getInterfaces();
		}
		//Clean the interface vector to save some space
		interfaces.clear();
	}
	return interfaces_added;
}

void Class_base::linkInterface(ASObject* obj) const
{
	if(class_index==-1)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Linking of builtin interface " << class_name << " not supported");
		return;
	}
	//Recursively link interfaces implemented by this interface
	for(int i=0;i<getInterfaces().size();i++)
		getInterfaces()[i]->linkInterface(obj);

	assert(context);

	//Link traits of this interface
	for(int j=0;j<context->instances[class_index].trait_count;j++)
	{
		traits_info* t=&context->instances[class_index].traits[j];
		context->linkTrait(obj,t);
	}

	if(constructor)
	{
		LOG(LOG_CALLS,"Calling interface init for " << class_name);
		constructor->call(obj,NULL,obj->max_level);
	}
}

bool Class_base::isSubClass(const Class_base* cls) const
{
	if(cls==this)
		return true;

	//Now check the interfaces
	for(int i=0;i<getInterfaces().size();i++)
	{
		if(getInterfaces()[i]->isSubClass(cls))
			return true;
	}

	//Now ask the super
	if(super && super->isSubClass(cls))
		return true;
	return false;
}

tiny_string Class_base::getQualifiedClassName() const
{
	if(class_index==-1)
		return class_name;
	else
	{
		assert(context);
		int name_index=context->instances[class_index].name;
		assert(name_index);
		const multiname* mname=context->getMultiname(name_index,NULL);
		return mname->qualifiedString();
	}
}

void ASQName::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(ASQName,_constructor)
{
	ASQName* th=static_cast<ASQName*>(obj->implementation);
	if(args->size()!=2)
		abort();

	assert(args->at(0)->getObjectType()==T_STRING || args->at(0)->getObjectType()==T_NAMESPACE);
	assert(args->at(1)->getObjectType()==T_STRING);

	switch(args->at(0)->getObjectType())
	{
		case T_STRING:
		{
			ASString* s=static_cast<ASString*>(args->at(0)->implementation);
			th->uri=s->data;
			break;
		}
		case T_NAMESPACE:
		{
			Namespace* n=static_cast<Namespace*>(args->at(0)->implementation);
			th->uri=n->uri;
			break;
		}
	}
	th->local_name=args->at(1)->toString();
}

void Namespace::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

void Namespace::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Namespace,_constructor)
{
	assert(args==NULL);
}

void InterfaceClass::lookupAndLink(ASObject* o, const tiny_string& name, const tiny_string& interfaceNs)
{
	ASObject* ret=o->getVariableByQName(name,"").obj;
	assert(ret);
	o->setVariableByQName(name,interfaceNs,ret);
}
