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

#include <list>
#include <algorithm>
//#include <libxml/parser.h>
#include <pcrecpp.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>

#include "abc.h"
#include "asobjects.h"
#include "flashevents.h"
#include "swf.h"
#include "compat.h"
#include "class.h"
#include "exceptions.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;

REGISTER_CLASS_NAME(Array);
REGISTER_CLASS_NAME2(ASQName,"QName");
REGISTER_CLASS_NAME2(IFunction,"Function");
REGISTER_CLASS_NAME2(UInteger,"uint");
REGISTER_CLASS_NAME(Boolean);
REGISTER_CLASS_NAME(Integer);
REGISTER_CLASS_NAME(Number);
REGISTER_CLASS_NAME(Namespace);
REGISTER_CLASS_NAME(Date);
REGISTER_CLASS_NAME(RegExp);
REGISTER_CLASS_NAME(Math);
REGISTER_CLASS_NAME(ASString);
REGISTER_CLASS_NAME(ASError);

Array::Array()
{
	type=T_ARRAY;
	//constructor=new Function(_constructor);
	//ASObject::setVariableByQName("toString","",new Function(ASObject::_toString));
}

void Array::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

void Array::buildTraits(ASObject* o)
{
	o->setGetterByQName("length","",Class<IFunction>::getFunction(_getLength));
	o->ASObject::setVariableByQName("pop","",Class<IFunction>::getFunction(_pop));
	o->ASObject::setVariableByQName("pop",AS3,Class<IFunction>::getFunction(_pop));
	o->ASObject::setVariableByQName("shift",AS3,Class<IFunction>::getFunction(shift));
	o->ASObject::setVariableByQName("unshift",AS3,Class<IFunction>::getFunction(unshift));
	o->ASObject::setVariableByQName("join",AS3,Class<IFunction>::getFunction(join));
	o->ASObject::setVariableByQName("push",AS3,Class<IFunction>::getFunction(_push));
	o->ASObject::setVariableByQName("sort",AS3,Class<IFunction>::getFunction(_sort));
//	o->ASObject::setVariableByQName("sortOn",AS3,Class<IFunction>::getFunction(sortOn));
	o->ASObject::setVariableByQName("concat",AS3,Class<IFunction>::getFunction(_concat));
	o->ASObject::setVariableByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf));
	o->ASObject::setVariableByQName("filter",AS3,Class<IFunction>::getFunction(filter));
	o->ASObject::setVariableByQName("splice",AS3,Class<IFunction>::getFunction(splice));
}

ASFUNCTIONBODY(Array,_constructor)
{
	Array* th=static_cast<Array*>(obj);

	if(argslen==1)
	{
		int size=args[0]->toInt();
		LOG(LOG_CALLS,"Creating array of length " << size);
		th->resize(size);
	}
	else
	{
		LOG(LOG_CALLS,"Called Array constructor");
		th->resize(argslen);
		for(unsigned int i=0;i<argslen;i++)
		{
			th->set(i,args[i]);
			args[i]->incRef();
		}
	}
	return NULL;
}

ASFUNCTIONBODY(Array,_getLength)
{
	Array* th=static_cast<Array*>(obj);
	return abstract_i(th->data.size());
}

ASFUNCTIONBODY(Array,shift)
{
	Array* th=static_cast<Array*>(obj);
	if(th->data.empty())
		return new Undefined;
	ASObject* ret;
	if(th->data[0].type==DATA_OBJECT)
		ret=th->data[0].data;
	else
		throw UnsupportedException("Array::shift not completely implemented");
	th->data.erase(th->data.begin());
	return ret;
}

ASFUNCTIONBODY(Array,splice)
{
	Array* th=static_cast<Array*>(obj);
	
	int startIndex=args[0]->toInt();
	int deleteCount=args[1]->toUInt();
	int totalSize=th->data.size();
	
	//A negative startIndex is relative to the end
	assert_and_throw(abs(startIndex)<totalSize);
	startIndex=(startIndex+totalSize)%totalSize;
	assert_and_throw((startIndex+deleteCount)<=totalSize);
	
	Array* ret=Class<Array>::getInstanceS();
	ret->data.reserve(deleteCount);

	for(int i=0;i<deleteCount;i++)
		ret->data.push_back(th->data[startIndex+i]);
	
	th->data.erase(th->data.begin()+startIndex,th->data.begin()+startIndex+deleteCount);

	//Insert requested values starting at startIndex
	for(unsigned int i=2,n=0;i<argslen;i++,n++)
	{
		args[i]->incRef();
		th->data.insert(th->data.begin()+startIndex+n,data_slot(args[i]));
	}

	return ret;
}

ASFUNCTIONBODY(Array,join)
{
	Array* th=static_cast<Array*>(obj);
	ASObject* del=args[0];
	string ret;
	for(int i=0;i<th->size();i++)
	{
		ret+=th->at(i)->toString().raw_buf();
		if(i!=th->size()-1)
			ret+=del->toString().raw_buf();
	}
	return Class<ASString>::getInstanceS(ret);
}

ASFUNCTIONBODY(Array,indexOf)
{
	Array* th=static_cast<Array*>(obj);
	assert_and_throw(argslen==1);
	int ret=-1;
	ASObject* arg0=args[0];
	for(unsigned int i=0;i<th->data.size();i++)
	{
		assert_and_throw(th->data[i].type==DATA_OBJECT);
		if(ABCVm::strictEquals(th->data[i].data,arg0))
		{
			ret=i;
			break;
		}
	}
	return abstract_i(ret);
}

ASFUNCTIONBODY(Array,filter)
{
	//TODO: really implement
	Array* th=static_cast<Array*>(obj);
	//assert_and_throw(th->data.size()==0);
	LOG(LOG_NOT_IMPLEMENTED,"Array::filter STUB");
	Array* ret=Class<Array>::getInstanceS();
	ret->data=th->data;
	return ret;
}

ASFUNCTIONBODY(Array,_concat)
{
	Array* th=static_cast<Array*>(obj);
	Array* ret=Class<Array>::getInstanceS();
	ret->data=th->data;
	if(argslen>=1 && args[0]->getObjectType()==T_ARRAY)
	{
		assert_and_throw(argslen==1);
		Array* tmp=Class<Array>::cast(args[0]);
		ret->data.insert(ret->data.end(),tmp->data.begin(),tmp->data.end());
	}
	else
	{
		//Insert the arguments in the array
		ret->data.reserve(ret->data.size()+argslen);
		for(unsigned int i=0;i<argslen;i++)
			ret->push(args[i]);
	}

	//All the elements in the new array should be increffed, as args will be deleted and
	//this array could die too
	for(unsigned int i=0;i<ret->data.size();i++)
	{
		if(ret->data[i].type==DATA_OBJECT)
			ret->data[i].data->incRef();
	}
	
	return ret;
}

ASFUNCTIONBODY(Array,_pop)
{
	Array* th=static_cast<Array*>(obj);
	ASObject* ret;
	if(th->data.back().type==DATA_OBJECT)
		ret=th->data.back().data;
	else
		throw UnsupportedException("Array::pop not completely implemented");
	th->data.pop_back();
	return ret;
}

ASFUNCTIONBODY(Array,_sort)
{
	Array* th=static_cast<Array*>(obj);
	if(th->data.size()>1)
		throw UnsupportedException("Array::sort not completely implemented");
	LOG(LOG_NOT_IMPLEMENTED,"Array::sort not really implemented");
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(Array,sortOn)
{
//	Array* th=static_cast<Array*>(obj);
/*	if(th->data.size()>1)
		throw UnsupportedException("Array::sort not completely implemented");
	LOG(LOG_NOT_IMPLEMENTED,"Array::sort not really implemented");*/
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(Array,unshift)
{
	Array* th=static_cast<Array*>(obj);
	for(uint32_t i=0;i<argslen;i++)
	{
		th->data.insert(th->data.begin(),data_slot(args[i],DATA_OBJECT));
		args[i]->incRef();
	}
	return abstract_i(th->size());;
}

ASFUNCTIONBODY(Array,_push)
{
	Array* th=static_cast<Array*>(obj);
	for(unsigned int i=0;i<argslen;i++)
	{
		th->push(args[i]);
		args[i]->incRef();
	}
	return abstract_i(th->size());
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
	LOG(LOG_NOT_IMPLEMENTED,"Called ASXML::load " << args[0]->toString());
	throw UnsupportedException("ASXML::load not completely implemented");
}

bool Array::isEqual(ASObject* r)
{
	assert_and_throw(implEnable);
	if(r->getObjectType()!=T_ARRAY)
		return false;
	else
	{
		const Array* ra=static_cast<const Array*>(r);
		int size=data.size();
		if(size!=ra->size())
			return false;

		for(int i=0;i<size;i++)
		{
			if(data[i].type!=DATA_OBJECT)
				throw UnsupportedException("Array::isEqual not completely implemented");
			if(!data[i].data->isEqual(ra->at(i)))
				return false;
		}
		return true;
	}
}

intptr_t Array::getVariableByMultiname_i(const multiname& name)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::getVariableByMultiname_i(name);

	if(index<data.size())
	{
		switch(data[index].type)
		{
			case DATA_OBJECT:
			{
				assert(data[index].data!=NULL);
				if(data[index].data->getObjectType()==T_INTEGER)
				{
					Integer* i=static_cast<Integer*>(data[index].data);
					return i->toInt();
				}
				else if(data[index].data->getObjectType()==T_NUMBER)
				{
					Number* i=static_cast<Number*>(data[index].data);
					return i->toInt();
				}
				else
					throw UnsupportedException("Array::getVariableByMultiname_i not completely implemented");
			}
			case DATA_INT:
				return data[index].data_i;
		}
	}
	
	return ASObject::getVariableByMultiname_i(name);
}

objAndLevel Array::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride)
{
	if(skip_impl || !implEnable)
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride);
		
	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="")
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride);

	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride);

	if(index<data.size())
	{
		ASObject* ret=NULL;
		switch(data[index].type)
		{
			case DATA_OBJECT:
				ret=data[index].data;
				if(ret==NULL)
				{
					ret=new Undefined;
					data[index].data=ret;
				}
				break;
			case DATA_INT:
				ret=abstract_i(data[index].data_i);
				ret->fake_decRef();
				break;
		}
		return objAndLevel(ret,0);
	}
	else
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride);
}

void Array::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
	{
		ASObject::setVariableByMultiname_i(name,value);
		return;
	}

	if(index>=data.capacity())
	{
		//Heuristic, we increse the array 20%
		int new_size=imax(index+1,data.size()*2);
		data.reserve(new_size);
	}
	if(index>=data.size())
		resize(index+1);

	if(data[index].type==DATA_OBJECT && data[index].data)
		data[index].data->decRef();
	data[index].data_i=value;
	data[index].type=DATA_INT;
}

bool Array::isValidMultiname(const multiname& name, unsigned int& index)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	assert_and_throw(name.ns.size()!=0);
	if(name.ns[0].name!="")
		return false;

	index=0;
	int len;
	switch(name.name_type)
	{
		//We try to convert this to an index, otherwise bail out
		case multiname::NAME_STRING:
			len=name.name_s.len();
			assert_and_throw(len);
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
			throw UnsupportedException("Array::isValidMultiname not completely implemented");
	}
	return true;
}

void Array::setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o,enableOverride);

	if(index>=data.capacity())
	{
		//Heuristic, we increse the array 20%
		int new_size=imax(index+1,data.size()*2);
		data.reserve(new_size);
	}
	if(index>=data.size())
		resize(index+1);

	if(data[index].type==DATA_OBJECT && data[index].data)
		data[index].data->decRef();

	if(o->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(o);
		data[index].data_i=i->val;
		data[index].type=DATA_INT;
		o->decRef();
	}
	else
	{
		data[index].data=o;
		data[index].type=DATA_OBJECT;
	}
}

bool Array::isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index)
{
	if(ns!="")
		return false;
	assert_and_throw(name.len()!=0);
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

void Array::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool find_back, bool skip_impl)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidQName(name,ns,index))
	{
		ASObject::setVariableByQName(name,ns,o,find_back,skip_impl);
		return;
	}

	if(index>=data.capacity())
	{
		//Heuristic, we increse the array 20%
		int new_size=imax(index+1,data.size()*2);
		data.reserve(new_size);
	}
	if(index>=data.size())
		resize(index+1);

	if(data[index].type==DATA_OBJECT && data[index].data)
		data[index].data->decRef();

	if(o->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(o);
		data[index].data_i=i->val;
		data[index].type=DATA_INT;
		o->decRef();
	}
	else
	{
		data[index].data=o;
		data[index].type=DATA_OBJECT;
	}
}

objAndLevel Array::getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl)
{
	assert_and_throw(implEnable);
	throw UnsupportedException("Array::getVariableByQName not completely implemented");
	return objAndLevel(NULL,0);
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

ASString::ASString(const char* s, uint32_t len):data(s, len)
{
	type=T_STRING;
}

/*ASFUNCTIONBODY(ASString,_constructor)
{
}*/

ASFUNCTIONBODY(ASString,_getLength)
{
	ASString* th=static_cast<ASString*>(obj);
	return abstract_i(th->data.size());
}

void ASString::sinit(Class_base* c)
{
	//c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setConstructor(NULL);
}

void ASString::buildTraits(ASObject* o)
{
	o->setVariableByQName("toString","",Class<IFunction>::getFunction(ASObject::_toString));
	o->setVariableByQName("split",AS3,Class<IFunction>::getFunction(split));
	o->setVariableByQName("substr",AS3,Class<IFunction>::getFunction(substr));
	o->setVariableByQName("replace",AS3,Class<IFunction>::getFunction(replace));
	o->setVariableByQName("concat",AS3,Class<IFunction>::getFunction(concat));
	o->setVariableByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf));
	o->setVariableByQName("charCodeAt",AS3,Class<IFunction>::getFunction(charCodeAt));
	o->setVariableByQName("slice",AS3,Class<IFunction>::getFunction(slice));
	o->setVariableByQName("toLowerCase",AS3,Class<IFunction>::getFunction(toLowerCase));
	o->setGetterByQName("length","",Class<IFunction>::getFunction(_getLength));
}

Array::~Array()
{
	if(sys && !sys->finalizingDestruction)
	{
		for(unsigned int i=0;i<data.size();i++)
		{
			if(data[i].type==DATA_OBJECT && data[i].data)
				data[i].data->decRef();
		}
	}
}

ASFUNCTIONBODY(ASString,split)
{
	ASString* th=static_cast<ASString*>(obj);
	Array* ret=Class<Array>::getInstanceS();
	ASObject* delimiter=args[0];
	if(delimiter->getObjectType()==T_STRING)
	{
		ASString* del=static_cast<ASString*>(delimiter);
		unsigned int start=0;
		do
		{
			int match=th->data.find(del->data,start);
			if(match==-1)
				match=th->data.size();
			ASString* s=Class<ASString>::getInstanceS(th->data.substr(start,(match-start)));
			ret->push(s);
			start=match+del->data.size();
		}
		while(start<th->data.size());
	}
	else
		throw UnsupportedException("Array::split not completely implemented");

	return ret;
}

ASFUNCTIONBODY(ASString,substr)
{
	ASString* th=static_cast<ASString*>(obj);
	int start=args[0]->toInt();
	if(start<0)
		start=th->data.size()+start;

	int len=0x7fffffff;
	if(argslen==2)
		len=args[1]->toInt();

	return Class<ASString>::getInstanceS(th->data.substr(start,len));
}

tiny_string Array::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	if(debugMsg)
		return ASObject::toString(debugMsg);
	return toString_priv();
}

tiny_string Array::toString_priv() const
{
	string ret;
	for(unsigned int i=0;i<data.size();i++)
	{
		if(data[i].type==DATA_OBJECT)
		{
			if(data[i].data)
				ret+=data[i].data->toString().raw_buf();
		}
		else if(data[i].type==DATA_INT)
		{
			char buf[20];
			snprintf(buf,20,"%i",data[i].data_i);
			ret+=buf;
		}
		else
			throw UnsupportedException("Array::toString not completely implemented");

		if(i!=data.size()-1)
			ret+=',';
	}
	return ret;
}

bool Array::nextValue(unsigned int index, ASObject*& out)
{
	assert_and_throw(implEnable);
	assert_and_throw(index<data.size());
	assert_and_throw(data[index].type==DATA_OBJECT);
	out=data[index].data;
	return true;
}

bool Array::hasNext(unsigned int& index, bool& out)
{
	assert_and_throw(implEnable);
	out=index<data.size();
	index++;
	return true;
}

void Array::outofbounds() const
{
	throw ParseException("Array access out of bounds");
}

tiny_string Boolean::toString(bool debugMsg)
{
	return (val)?"true":"false";
}

tiny_string ASString::toString_priv() const
{
	return data;
}

tiny_string ASString::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	return toString_priv();
}

double ASString::toNumber()
{
	assert_and_throw(implEnable);
	//TODO: implement conversion that checks for validity
	return atof(data.c_str());
}

int32_t ASString::toInt()
{
	assert_and_throw(implEnable);
	if(data.empty() || !isdigit(data[0]))
		return 0;
	return atoi(data.c_str());
}

bool ASString::isEqual(ASObject* r)
{
	assert_and_throw(implEnable);
	//TODO: check conversion
	if(r->getObjectType()==T_STRING)
	{
		const ASString* s=static_cast<const ASString*>(r);
		return s->data==data;
	}
	else
		return false;
}

bool ASString::isLess(ASObject* r)
{
	assert_and_throw(implEnable);
	//TODO: Implement ECMA-262 11.8.5 algorithm
	//Number comparison has the priority over strings
	if(r->getObjectType()==T_INTEGER)
	{
		number_t a=toNumber();
		number_t b=r->toNumber();
		return a<b;
	}
	throw UnsupportedException("String::isLess not completely implemented");
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

Undefined::Undefined()
{
	type=T_UNDEFINED;
}

ASFUNCTIONBODY(Undefined,call)
{
	LOG(LOG_CALLS,"Undefined function");
	return NULL;
}

tiny_string Undefined::toString(bool debugMsg)
{
	return "undefined";
}

bool Undefined::isLess(ASObject* r)
{
	//ECMA-262 complaiant
	//As undefined became NaN when converted to number the operation is undefined
	//And undefined must return false
	return false;
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

int Undefined::toInt()
{
	return 0;
}

double Undefined::toNumber()
{
	return numeric_limits<double>::quiet_NaN();
}

ASFUNCTIONBODY(Integer,_toString)
{
	Integer* th=static_cast<Integer*>(obj);
	int radix=10;
	char buf[20];
	if(argslen==1)
		radix=args[0]->toUInt();
	assert_and_throw(radix==10 || radix==16);
	if(radix==10)
		snprintf(buf,20,"%i",th->val);
	else if(radix==16)
		snprintf(buf,20,"%x",th->val);

	return Class<ASString>::getInstanceS(buf);
}

bool Integer::isLess(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(o);
		return val < i->toInt();
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		Number* i=static_cast<Number*>(o);
		return val < i->toNumber();
	}
	else if(o->getObjectType()==T_STRING)
	{
		const ASString* s=static_cast<const ASString*>(o);
		//Check if the string may be converted to integer
		//TODO: check whole string?
		if(isdigit(s->data[0]))
		{
			int val2=atoi(s->data.c_str());
			return val < val2;
		}
		else
			return false;
	}
	else if(o->getObjectType()==T_BOOLEAN)
	{
		Boolean* i=static_cast<Boolean*>(o);
		return val < i->toInt();
	}
	else
		return ASObject::isLess(o);
}

bool Integer::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toInt();
	else if(o->getObjectType()==T_UINTEGER)
	{
		//CHECK: somehow wrong
		return val==o->toInt();
	}
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toInt();
	else
	{
		return ASObject::isEqual(o);
	}
}

tiny_string Integer::toString(bool debugMsg)
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
	return tiny_string(cur,true); //Create a copy
}

tiny_string UInteger::toString(bool debugMsg)
{
	char buf[20];
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
	return tiny_string(cur,true); //Create a copy
}

bool UInteger::isLess(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		uint32_t val1=val;
		int32_t val2=o->toInt();
		if(val2<0)
			return false;
		else
			return val1<(uint32_t)val2;
	}
	else
		throw UnsupportedException("UInteger::isLess is not completely implemented");
}

bool Number::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toNumber();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toNumber();
	else
	{
		return ASObject::isEqual(o);
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
	else if(o->getObjectType()==T_UNDEFINED)
	{
		//Undefined is NaN, so the result is undefined and so false
		return false;
	}
	else
	{
		return ASObject::isLess(o);
	}
}

tiny_string Number::toString(bool debugMsg)
{
	char buf[20];
	snprintf(buf,20,"%g",val);
	return tiny_string(buf,true);
}

Date::Date():year(-1),month(-1),date(-1),hour(-1),minute(-1),second(-1),millisecond(-1)
{
}

void Date::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

void Date::buildTraits(ASObject* o)
{
	o->setVariableByQName("getTimezoneOffset","",Class<IFunction>::getFunction(getTimezoneOffset));
	o->setVariableByQName("valueOf","",Class<IFunction>::getFunction(valueOf));
	o->setVariableByQName("getTime",AS3,Class<IFunction>::getFunction(getTime));
	o->setVariableByQName("getFullYear","",Class<IFunction>::getFunction(getFullYear));
	o->setVariableByQName("getHours",AS3,Class<IFunction>::getFunction(getHours));
	o->setVariableByQName("getMinutes",AS3,Class<IFunction>::getFunction(getMinutes));
	o->setVariableByQName("getSeconds",AS3,Class<IFunction>::getFunction(getMinutes));
	//o->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(ASObject::_toString));
}

ASFUNCTIONBODY(Date,_constructor)
{
	Date* th=static_cast<Date*>(obj);
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
	return abstract_d(120);
}

ASFUNCTIONBODY(Date,getFullYear)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->year);
}

ASFUNCTIONBODY(Date,getHours)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->hour);
}

ASFUNCTIONBODY(Date,getMinutes)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->minute);
}

ASFUNCTIONBODY(Date,getTime)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->toInt());
}

ASFUNCTIONBODY(Date,valueOf)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->toInt());
}

int Date::toInt()
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

tiny_string Date::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	return toString_priv();
}

tiny_string Date::toString_priv() const
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

IFunction::IFunction():closure_this(NULL),closure_level(-1),bound(false),overriden_by(NULL)
{
	type=T_FUNCTION;
}

ASFUNCTIONBODY(IFunction,apply)
{
	IFunction* th=static_cast<IFunction*>(obj);
	assert_and_throw(argslen==2);

	//Validate parameters
	assert_and_throw(args[1]->getObjectType()==T_ARRAY);
	Array* array=Class<Array>::cast(args[1]);

	int len=array->size();
	ASObject** new_args=new ASObject*[len];
	for(int i=0;i<len;i++)
	{
		new_args[i]=array->at(i);
		new_args[i]->incRef();
	}

	args[0]->incRef();
	ASObject* ret=th->call(args[0],new_args,len,0);
	delete[] new_args;
	return ret;
}

SyntheticFunction::SyntheticFunction(method_info* m):hit_count(0),mi(m),val(NULL)
{
//	class_index=-2;
}

ASObject* SyntheticFunction::call(ASObject* obj, ASObject* const* args, uint32_t numArgs, int level)
{
	const int hit_threshold=10;
	if(mi->body==NULL)
	{
//		LOG(LOG_NOT_IMPLEMENTED,"Not initialized function");
		return NULL;
	}

	//Temporarily disable JITting
	if(sys->useJit && (hit_count==hit_threshold || sys->useInterpreter==false))
	{
		//We passed the hot function threshold, synt the function
		val=mi->synt_method();
		assert_and_throw(val);
	}

	//Prepare arguments
	uint32_t args_len=mi->numArgs();
	int passedToLocals=imin(numArgs,args_len);
	uint32_t passedToRest=(numArgs > args_len)?(numArgs-mi->numArgs()):0;
	int realLevel=(bound)?closure_level:level;
	if(realLevel==-1) //If realLevel is not set, keep the object level
		realLevel=obj->getLevel();

	call_context* cc=new call_context(mi,realLevel,args,passedToLocals);
	uint32_t i=passedToLocals;
	cc->scope_stack=func_scope;
	for(unsigned int i=0;i<func_scope.size();i++)
		func_scope[i]->incRef();

	if(bound && closure_this)
	{
		LOG(LOG_CALLS,"Calling with closure " << this);
		obj=closure_this;
	}

	cc->locals[0]=obj;
	obj->incRef();

	//Fixup missing parameters
	unsigned int missing_params=args_len-i;
	assert(missing_params<=mi->option_count);
	assert_and_throw(missing_params<=mi->option_count);
	int starting_options=mi->option_count-missing_params;

	for(unsigned int j=starting_options;j<mi->option_count;j++)
	{
		cc->locals[i+1]=mi->getOptional(j);
		i++;
	}

	assert_and_throw(i==args_len);

	assert_and_throw(mi->needsArgs()==false || mi->needsRest()==false);
	if(mi->needsRest()) //TODO
	{
		Array* rest=Class<Array>::getInstanceS();
		rest->resize(passedToRest);
		for(uint32_t j=0;j<passedToRest;j++)
			rest->set(j,args[passedToLocals+j]);

		cc->locals[i+1]=rest;
	}
	else if(mi->needsArgs())
	{
		Array* argumentsArray=Class<Array>::getInstanceS();
		argumentsArray->resize(args_len+passedToRest);
		for(uint32_t j=0;j<args_len;j++)
		{
			cc->locals[j+1]->incRef();
			argumentsArray->set(j,cc->locals[j+1]);
		}
		for(uint32_t j=0;j<passedToRest;j++)
			argumentsArray->set(j+args_len,args[passedToLocals+j]);
		//Add ourself as the callee property
		incRef();
		argumentsArray->setVariableByQName("callee","",this);

		cc->locals[i+1]=argumentsArray;
	}
	//Parameters are ready

	//As we are changing execution context (e.g. 'this' and level), reset the level of the current
	//object and add the new 'this' and level to the stack
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	tl.cur_this->resetLevel();

	getVm()->pushObjAndLevel(obj,realLevel);
	//Set the current level
	obj->setLevel(realLevel);

	ASObject* ret;
	if(val==NULL && sys->useInterpreter)
	{
		//This is not an hot function, execute it using the intepreter
		ret=ABCVm::executeFunction(this,cc);
	}
	else
		ret=val(cc);

	//Now pop this context and reset the level correctly
	tl=getVm()->popObjAndLevel();
	assert_and_throw(tl.cur_this==obj);
	assert_and_throw(tl.cur_this->getLevel()==realLevel);
	obj->resetLevel();

	tl=getVm()->getCurObjAndLevel();
	tl.cur_this->setLevel(tl.cur_level);

	delete cc;
	hit_count++;
	return ret;
}

ASObject* Function::call(ASObject* obj, ASObject* const* args, uint32_t num_args, int level)
{
	ASObject* ret;
	if(bound && closure_this)
	{
		LOG(LOG_CALLS,"Calling with closure " << this);
		obj->decRef();
		obj=closure_this;
		obj->incRef();
	}
	ret=val(obj,args,num_args);

	for(uint32_t i=0;i<num_args;i++)
		args[i]->decRef();
	obj->decRef();
	return ret;
}

void Math::sinit(Class_base* c)
{
	c->setVariableByQName("PI","",abstract_d(M_PI));
	c->setVariableByQName("sqrt","",Class<IFunction>::getFunction(sqrt));
	c->setVariableByQName("atan2","",Class<IFunction>::getFunction(atan2));
	c->setVariableByQName("max","",Class<IFunction>::getFunction(_max));
	c->setVariableByQName("min","",Class<IFunction>::getFunction(_min));
	c->setVariableByQName("abs","",Class<IFunction>::getFunction(abs));
	c->setVariableByQName("sin","",Class<IFunction>::getFunction(sin));
	c->setVariableByQName("cos","",Class<IFunction>::getFunction(cos));
	c->setVariableByQName("floor","",Class<IFunction>::getFunction(floor));
	c->setVariableByQName("ceil","",Class<IFunction>::getFunction(ceil));
	c->setVariableByQName("round","",Class<IFunction>::getFunction(round));
	c->setVariableByQName("random","",Class<IFunction>::getFunction(random));
	c->setVariableByQName("pow","",Class<IFunction>::getFunction(pow));
}

ASFUNCTIONBODY(Math,atan2)
{
	double n1=args[0]->toNumber();
	double n2=args[1]->toNumber();
	return abstract_d(::atan2(n1,n2));
}

ASFUNCTIONBODY(Math,_max)
{
	double n1=args[0]->toNumber();
	double n2=args[1]->toNumber();
	return abstract_d(dmax(n1,n2));
}

ASFUNCTIONBODY(Math,_min)
{
	double n1=args[0]->toNumber();
	double n2=args[1]->toNumber();
	return abstract_d(dmin(n1,n2));
}

ASFUNCTIONBODY(Math,cos)
{
	//Angle is in radians
	double n=args[0]->toNumber();
	return abstract_d(::cos(n));
}

ASFUNCTIONBODY(Math,sin)
{
	//Angle is in radians
	double n=args[0]->toNumber();
	return abstract_d(::sin(n));
}

ASFUNCTIONBODY(Math,abs)
{
	double n=args[0]->toNumber();
	return abstract_d(::abs(n));
}

ASFUNCTIONBODY(Math,ceil)
{
	double n=args[0]->toNumber();
	return abstract_i(::ceil(n));
}

ASFUNCTIONBODY(Math,floor)
{
	double n=args[0]->toNumber();
	return abstract_i(::floor(n));
}

ASFUNCTIONBODY(Math,round)
{
	double n=args[0]->toNumber();
	return abstract_i(::round(n));
}

ASFUNCTIONBODY(Math,sqrt)
{
	double n=args[0]->toNumber();
	return abstract_d(::sqrt(n));
}

ASFUNCTIONBODY(Math,pow)
{
	double x=args[0]->toNumber();
	double y=args[1]->toNumber();
	return abstract_d(::pow(x,y));
}

ASFUNCTIONBODY(Math,random)
{
	double ret=rand();
	ret/=RAND_MAX;
	return abstract_d(ret);
}

tiny_string Null::toString(bool debugMsg)
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

RegExp::RegExp():global(false),ignoreCase(false),extended(false),lastIndex(0)
{
}

void RegExp::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

void RegExp::buildTraits(ASObject* o)
{
	o->setVariableByQName("exec",AS3,Class<IFunction>::getFunction(exec));
	o->setVariableByQName("test",AS3,Class<IFunction>::getFunction(test));
	o->setGetterByQName("global","",Class<IFunction>::getFunction(_getGlobal));
}

ASFUNCTIONBODY(RegExp,_constructor)
{
	RegExp* th=static_cast<RegExp*>(obj);
	th->re=args[0]->toString().raw_buf();
	if(argslen>1)
	{
		const tiny_string& flags=args[1]->toString();
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
				case 'x':
					th->extended=true;
					break;
				case 's':
				case 'm':
					throw UnsupportedException("RegExp not completely implemented");

			}
		}
	}
	return NULL;
}

ASFUNCTIONBODY(RegExp,_getGlobal)
{
	RegExp* th=static_cast<RegExp*>(obj);
	return abstract_b(th->global);
}

ASFUNCTIONBODY(RegExp,exec)
{
	RegExp* th=static_cast<RegExp*>(obj);
	pcrecpp::RE_Options opt;
	opt.set_caseless(th->ignoreCase);
	opt.set_extended(th->extended);

	pcrecpp::RE pcreRE(th->re,opt);
	assert_and_throw(th->lastIndex==0);
	const tiny_string& arg0=args[0]->toString();
	LOG(LOG_CALLS,"re: " << th->re);
	int numberOfCaptures=pcreRE.NumberOfCapturingGroups();
	LOG(LOG_CALLS,"capturing groups " << numberOfCaptures);
	assert_and_throw(numberOfCaptures!=-1);
	//The array of captured groups
	pcrecpp::Arg** captures=new pcrecpp::Arg*[numberOfCaptures];
	//The array of strings
	string* s=new string[numberOfCaptures];
	for(int i=0;i<numberOfCaptures;i++)
		captures[i]=new pcrecpp::Arg(&s[i]);

	int consumed;
	bool matched=pcreRE.DoMatch(arg0.raw_buf(),pcrecpp::RE::ANCHOR_START,&consumed,captures,numberOfCaptures);
	ASObject* ret;
	if(matched)
	{
		Array* a=Class<Array>::getInstanceS();
		for(int i=0;i<numberOfCaptures;i++)
			a->push(Class<ASString>::getInstanceS(s[i]));
		args[0]->incRef();
		a->setVariableByQName("input","",args[0]);
		ret=a;
		//TODO: add index field (not possible with current PCRECPP
	}
	else
		ret=new Null;

	delete[] captures;
	delete[] s;

	return ret;
}

ASFUNCTIONBODY(RegExp,test)
{
	RegExp* th=static_cast<RegExp*>(obj);
	pcrecpp::RE_Options opt;
	opt.set_caseless(th->ignoreCase);
	opt.set_extended(th->extended);

	pcrecpp::RE pcreRE(th->re,opt);
	assert_and_throw(th->lastIndex==0);
	const tiny_string& arg0=args[0]->toString();

	bool ret=pcreRE.PartialMatch(arg0.raw_buf());
	return abstract_b(ret);
}

ASFUNCTIONBODY(ASString,slice)
{
	ASString* th=static_cast<ASString*>(obj);
	int startIndex=0;
	if(argslen>=1)
		startIndex=args[0]->toInt();
	int endIndex=0x7fffffff;
	if(argslen>=2)
		endIndex=args[1]->toInt();
	return Class<ASString>::getInstanceS(th->data.substr(startIndex,endIndex));
}

ASFUNCTIONBODY(ASString,charCodeAt)
{
	//TODO: should return utf16
	LOG(LOG_CALLS,"ASString::charCodeAt not really implemented");
	ASString* th=static_cast<ASString*>(obj);
	unsigned int index=args[0]->toInt();
	assert_and_throw(index>=0 && index<th->data.size());
	return abstract_i(th->data[index]);
}

ASFUNCTIONBODY(ASString,indexOf)
{
	ASString* th=static_cast<ASString*>(obj);
	const tiny_string& arg0=args[0]->toString();
	int startIndex=0;
	if(argslen>1)
		startIndex=args[1]->toInt();
	
	assert_and_throw(startIndex==0);
	bool found=false;
	unsigned int i;
	for(i=startIndex;i<th->data.size();i++)
	{
		if(th->data[i]==arg0[0])
		{
			found=true;
			for(int j=1;j<arg0.len();j++)
			{
				if(th->data[i+j]!=arg0[j])
				{
					found=false;
					break;
				}
			}
		}
		if(found)
			break;
	}

	if(!found)
		return abstract_i(-1);
	else
		return abstract_i(i);
}

ASFUNCTIONBODY(ASString,toLowerCase)
{
	ASString* th=static_cast<ASString*>(obj);
	ASString* ret=Class<ASString>::getInstanceS();
	ret->data=th->data;
	transform(th->data.begin(), th->data.end(), ret->data.begin(), ::tolower);
	return ret;
}

ASFUNCTIONBODY(ASString,replace)
{
	const ASString* th=static_cast<const ASString*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->data);
	string replaceWith(args[1]->toString().raw_buf());
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
	while(index<(int)ret->data.size());

	assert_and_throw(argslen==2 && args[1]->getObjectType()==T_STRING);

	if(args[0]->getPrototype()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);

		pcrecpp::RE_Options opt;
		opt.set_caseless(re->ignoreCase);
		opt.set_extended(re->extended);
		pcrecpp::RE pcreRE(re->re,opt);
		if(re->global)
			pcreRE.GlobalReplace(replaceWith,&ret->data);
		else
			pcreRE.Replace(replaceWith,&ret->data);
	}
	else if(args[0]->getObjectType()==T_STRING)
	{
		ASString* s=static_cast<ASString*>(args[0]);
		int index=0;
		do
		{
			index=ret->data.find(s->data,index);
			if(index==-1) //No result
				break;
			ret->data.replace(index,s->data.size(),replaceWith);
			index+=(replaceWith.size()-s->data.size());

		}
		while(index<(int)ret->data.size());
	}
	else
		throw UnsupportedException("String::replace not completely implemented");

	return ret;
}

ASFUNCTIONBODY(ASString,concat)
{
	ASString* th=static_cast<ASString*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->data);
	for(unsigned int i=0;i<argslen;i++)
		ret->data+=args[i]->toString().raw_buf();

	return ret;
}

ASFUNCTIONBODY(ASError,getStackTrace)
{
	ASError* th=static_cast<ASError*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->toString(true));
	LOG(LOG_NOT_IMPLEMENTED,"Error.getStackTrace not yet implemented.");
	return ret;
}

tiny_string ASError::toString(bool debugMsg)
{
	return message.len() > 0 ? message : "Error";
}

ASFUNCTIONBODY(ASError,_getErrorID)
{
	ASError* th=static_cast<ASError*>(obj);
	return abstract_i(th->errorID);
}

ASFUNCTIONBODY(ASError,_setName)
{
	ASError* th=static_cast<ASError*>(obj);
	assert_and_throw(argslen==1);
	th->name = args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(ASError,_getName)
{
	ASError* th=static_cast<ASError*>(obj);
	return Class<ASString>::getInstanceS(th->name);
}

ASFUNCTIONBODY(ASError,_setMessage)
{
	ASError* th=static_cast<ASError*>(obj);
	assert_and_throw(argslen==1);
	th->message = args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(ASError,_getMessage)
{
	ASError* th=static_cast<ASError*>(obj);
	return Class<ASString>::getInstanceS(th->message);
}

void ASError::buildTraits(ASObject* o)
{
	o->setVariableByQName("getStackTrace",AS3,Class<IFunction>::getFunction(getStackTrace));
	o->setGetterByQName("errorID",AS3,Class<IFunction>::getFunction(_getErrorID));
	o->setGetterByQName("message",AS3,Class<IFunction>::getFunction(_getMessage));
	o->setSetterByQName("message",AS3,Class<IFunction>::getFunction(_setMessage));
	o->setGetterByQName("name",AS3,Class<IFunction>::getFunction(_getName));
	o->setSetterByQName("name",AS3,Class<IFunction>::getFunction(_setName));
}

Class_base::Class_base(const tiny_string& name):use_protected(false),constructor(NULL),referencedObjectsMutex("referencedObjects"),super(NULL),
	context(NULL),class_name(name),class_index(-1),max_level(0)
{
	type=T_CLASS;
}

Class_base::~Class_base()
{
	if(constructor)
		constructor->decRef();

	if(super)
		super->decRef();
	
	//Destroy all the object reference by us
	if(!referencedObjects.empty())
	{
		cout << "Class " << class_name << " references " << referencedObjects.size() << endl;
		set<ASObject*>::iterator it=referencedObjects.begin();
		for(;it!=referencedObjects.end();it++)
			delete *it;
	}
	
}

void Class_base::addImplementedInterface(const multiname& i)
{
	interfaces.push_back(i);
}

void Class_base::addImplementedInterface(Class_base* i)
{
	interfaces_added.push_back(i);
}

tiny_string Class_base::toString(bool debugMsg)
{
	tiny_string ret="[Class ";
	ret+=class_name;
	ret+="]";
	return ret;
}

void Class_base::recursiveBuild(ASObject* target)
{
	if(super)
		super->recursiveBuild(target);

	LOG(LOG_TRACE,"Building traits for " << class_name);
	target->setLevel(max_level);
	buildInstanceTraits(target);

	//Link the interfaces for this level
	const vector<Class_base*>& interfaces=getInterfaces();
	for(unsigned int i=0;i<interfaces.size();i++)
	{
		LOG(LOG_CALLS,"Linking with interface " << interfaces[i]->class_name);
		interfaces[i]->linkInterface(target);
	}
}

void Class_base::setConstructor(IFunction* c)
{
	assert_and_throw(constructor==NULL);
	constructor=c;
}

void Class_base::handleConstruction(ASObject* target, ASObject* const* args, unsigned int argslen, bool buildAndLink)
{
/*	if(getActualPrototype()->class_index==-2)
	{
		abort();
		//We have to build the method traits
		SyntheticFunction* sf=static_cast<SyntheticFunction*>(this);
		LOG(LOG_CALLS,"Building method traits");
		for(int i=0;i<sf->mi->body->trait_count;i++)
			sf->mi->context->buildTrait(this,&sf->mi->body->traits[i]);
		sf->call(this,args,max_level);
	}*/
	if(buildAndLink)
	{
	#ifndef NDEBUG
		assert_and_throw(!target->initialized);
	#endif
		//HACK: suppress implementation handling of variables just now
		bool bak=target->implEnable;
		target->implEnable=false;
		recursiveBuild(target);
		//And restore it
		target->implEnable=bak;
		assert_and_throw(target->getLevel()==max_level);
	#ifndef NDEBUG
		target->initialized=true;
	#endif
	}

	//As constructors are not binded, we should change here the level
	assert_and_throw(max_level==target->getLevel());
	if(constructor)
	{
		LOG(LOG_CALLS,"Calling Instance init " << class_name);
		target->incRef();
		ASObject* ret=constructor->call(target,args,argslen,max_level);
		assert_and_throw(ret==NULL);
	}
}

void Class_base::acquireObject(ASObject* ob)
{
	Locker l(referencedObjectsMutex);
	bool ret=referencedObjects.insert(ob).second;
	assert_and_throw(ret);
}

void Class_base::abandonObject(ASObject* ob)
{
	Locker l(referencedObjectsMutex);
	set<ASObject>::size_type ret=referencedObjects.erase(ob);
	if(ret!=1)
	{
		LOG(LOG_ERROR,"Failure in reference counting");
	}
}

void Class_base::cleanUp()
{
	Variables.destroyContents();
	if(constructor)
	{
		constructor->decRef();
		constructor=NULL;
	}

	if(super)
	{
		super->decRef();
		super=NULL;
	}
}

ASObject* Class_inherit::getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
{
	ASObject* ret=NULL;
	if(tag)
	{
		ret=tag->instance();
		assert_and_throw(ret);
	}
	else
	{
		assert_and_throw(super);
		//Our super should not construct, we are going to do it ourselves
		ret=super->getInstance(false,NULL,0);
	}
	//We override the prototype
	ret->setPrototype(this);
	if(construct)
		handleConstruction(ret,args,argslen,true);
	return ret;
}

void Class_inherit::buildInstanceTraits(ASObject* o) const
{
	assert_and_throw(class_index!=-1);
	//The class is declared in the script and has an index
	LOG(LOG_CALLS,"Building instance traits");

	context->buildInstanceTraits(o,class_index);
}

Class_object* Class_object::getClass()
{
	//We check if we are registered in the class map
	//if not we register ourselves (see also Class<T>::getClass)
	std::map<tiny_string, Class_base*>::iterator it=sys->classes.find("Class");
	Class_object* ret=NULL;
	if(it==sys->classes.end()) //This class is not yet in the map, create it
	{
		ret=new Class_object();
		sys->classes.insert(std::make_pair("Class",ret));
	}
	else
		ret=static_cast<Class_object*>(it->second);

	ret->incRef();
	return ret;
}

Class_function* Class_function::getClass()
{
	//We check if we are registered in the class map
	//if not we register ourselves (see also Class<T>::getClass)
	std::map<tiny_string, Class_base*>::iterator it=sys->classes.find("Function");
	Class_function* ret=NULL;
	if(it==sys->classes.end()) //This class is not yet in the map, create it
	{
		ret=new Class_function();
		sys->classes.insert(std::make_pair("Function",ret));
	}
	else
		ret=static_cast<Class_function*>(it->second);

	ret->incRef();
	return ret;
}

const std::vector<Class_base*>& Class_base::getInterfaces() const
{
	if(!interfaces.empty())
	{
		//Recursively get interfaces implemented by this interface
		for(unsigned int i=0;i<interfaces.size();i++)
		{
			ASObject* interface_obj=getGlobal()->getVariableByMultiname(interfaces[i]).obj;
			assert_and_throw(interface_obj && interface_obj->getObjectType()==T_CLASS);
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
		//LOG(LOG_NOT_IMPLEMENTED,"Linking of builtin interface " << class_name << " not supported");
		return;
	}
	//Recursively link interfaces implemented by this interface
	for(unsigned int i=0;i<getInterfaces().size();i++)
		getInterfaces()[i]->linkInterface(obj);

	assert_and_throw(context);

	//Link traits of this interface
	for(unsigned int j=0;j<context->instances[class_index].trait_count;j++)
	{
		traits_info* t=&context->instances[class_index].traits[j];
		context->linkTrait(obj,t);
	}

	if(constructor)
	{
		LOG(LOG_CALLS,"Calling interface init for " << class_name);
		ASObject* ret=constructor->call(obj,NULL,0,max_level);
		assert_and_throw(ret==NULL);
	}
}

bool Class_base::isSubClass(const Class_base* cls) const
{
	check();
	if(cls==this)
		return true;

	//Now check the interfaces
	for(unsigned int i=0;i<getInterfaces().size();i++)
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
		assert_and_throw(context);
		int name_index=context->instances[class_index].name;
		assert_and_throw(name_index);
		const multiname* mname=context->getMultiname(name_index,NULL);
		return mname->qualifiedString();
	}
}

void ASQName::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

ASFUNCTIONBODY(ASQName,_constructor)
{
	ASQName* th=static_cast<ASQName*>(obj);
	if(argslen!=2)
		throw UnsupportedException("ArgumentError");

	assert_and_throw(args[0]->getObjectType()==T_STRING || args[0]->getObjectType()==T_NAMESPACE);
	assert_and_throw(args[1]->getObjectType()==T_STRING);

	switch(args[0]->getObjectType())
	{
		case T_STRING:
		{
			ASString* s=static_cast<ASString*>(args[0]);
			th->uri=s->data;
			break;
		}
		case T_NAMESPACE:
		{
			Namespace* n=static_cast<Namespace*>(args[0]);
			th->uri=n->uri;
			break;
		}
		default:
			throw UnsupportedException("QName not completely implemented");
	}
	th->local_name=args[1]->toString();
	return NULL;
}

void Namespace::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

void Namespace::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Namespace,_constructor)
{
	assert_and_throw(argslen==0);
	return NULL;
}

void InterfaceClass::lookupAndLink(ASObject* o, const tiny_string& name, const tiny_string& interfaceNs)
{
	ASObject* ret=o->getVariableByQName(name,"").obj;
	assert_and_throw(ret);
	ret->incRef();
	o->setVariableByQName(name,interfaceNs,ret);
}

void UInteger::sinit(Class_base* c)
{
	//TODO: add in the JIT support for unsigned number
	//Right now we pretend to be signed, to make comparisons work
	c->setVariableByQName("MAX_VALUE","",new UInteger(0x7fffffff));
}

bool UInteger::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		//CHECK: somehow wrong
		return val==o->toUInt();
	}
	else if(o->getObjectType()==T_UINTEGER)
		return val==o->toUInt();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toUInt();
	else
	{
		return ASObject::isEqual(o);
	}
}

Class<IFunction>* Class<IFunction>::getClass()
{
	std::map<tiny_string, Class_base*>::iterator it=sys->classes.find(ClassName<IFunction>::name);
	Class<IFunction>* ret=NULL;
	if(it==sys->classes.end()) //This class is not yet in the map, create it
	{
		ret=new Class<IFunction>;
		IFunction::sinit(ret);
		sys->classes.insert(std::make_pair(ClassName<IFunction>::name,ret));
	}
	else
		ret=static_cast<Class<IFunction>*>(it->second);

	ret->incRef();
	return ret;
}
