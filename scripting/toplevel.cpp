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
#include <pcre.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>

#include "abc.h"
#include "toplevel.h"
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
	c->setGetterByQName("length","",Class<IFunction>::getFunction(_getLength));
	c->ASObject::setVariableByQName("pop","",Class<IFunction>::getFunction(_pop));
	c->ASObject::setVariableByQName("pop",AS3,Class<IFunction>::getFunction(_pop));
	c->ASObject::setVariableByQName("shift",AS3,Class<IFunction>::getFunction(shift));
	c->ASObject::setVariableByQName("unshift",AS3,Class<IFunction>::getFunction(unshift));
	c->ASObject::setVariableByQName("join",AS3,Class<IFunction>::getFunction(join));
	c->ASObject::setVariableByQName("push",AS3,Class<IFunction>::getFunction(_push));
	c->ASObject::setVariableByQName("sort",AS3,Class<IFunction>::getFunction(_sort));
//	c->ASObject::setVariableByQName("sortOn",AS3,Class<IFunction>::getFunction(sortOn));
	c->ASObject::setVariableByQName("concat",AS3,Class<IFunction>::getFunction(_concat));
	c->ASObject::setVariableByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf));
	c->ASObject::setVariableByQName("filter",AS3,Class<IFunction>::getFunction(filter));
	c->ASObject::setVariableByQName("splice",AS3,Class<IFunction>::getFunction(splice));
}

void Array::buildTraits(ASObject* o)
{
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
		if(ABCVm::strictEqualImpl(th->data[i].data,arg0))
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
	for(unsigned int i=0;i<ret->data.size();i++)
	{
		if(ret->data[i].type==DATA_OBJECT && ret->data[i].data)
			ret->data[i].data->incRef();
	}
	return ret;
}

ASFUNCTIONBODY(Array,_concat)
{
	Array* th=static_cast<Array*>(obj);
	Array* ret=Class<Array>::getInstanceS();
	ret->data=th->data;
	if(argslen==1 && args[0]->getObjectType()==T_ARRAY)
	{
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

bool Array::sortComparator(const data_slot& d1, const data_slot& d2)
{
	//Comparison is always in lexicographic order
	tiny_string s1;
	tiny_string s2;
	if(d1.type==DATA_INT)
		s1=tiny_string(d1.data_i);
	else if(d1.type==DATA_OBJECT && d1.data)
		s1=d1.data->toString();
	else
		s1="undefined";
	if(d2.type==DATA_INT)
		s2=tiny_string(d2.data_i);
	else if(d2.type==DATA_OBJECT && d2.data)
		s2=d2.data->toString();
	else
		s2="undefined";

	return s1<s2;
}

bool Array::sortComparatorWrapper::operator()(const data_slot& d1, const data_slot& d2)
{
	ASObject* objs[2];
	if(d1.type==DATA_INT)
		objs[0]=abstract_i(d1.data_i);
	else if(d1.type==DATA_OBJECT && d1.data)
	{
		objs[0]=d1.data;
		objs[0]->incRef();
	}
	else
		objs[0]=new Undefined;

	if(d2.type==DATA_INT)
		objs[1]=abstract_i(d2.data_i);
	else if(d2.type==DATA_OBJECT && d2.data)
	{
		objs[1]=d1.data;
		objs[1]->incRef();
	}
	else
		objs[1]=new Undefined;

	assert(comparator);
	ASObject* ret=comparator->call(NULL, objs, 2);
	assert_and_throw(ret);
	return (ret->toInt()<0); //Less
}

ASFUNCTIONBODY(Array,_sort)
{
	Array* th=static_cast<Array*>(obj);
	IFunction* comp=NULL;
	for(uint32_t i=0;i<argslen;i++)
	{
		if(args[i]->getObjectType()==T_FUNCTION) //Comparison func
		{
			assert_and_throw(comp==NULL);
			comp=static_cast<IFunction*>(args[i]);
		}
		else
		{
			if(args[i]->toInt()!=0) //Options?
				throw UnsupportedException("Array::sort not completely implemented");
		}
	}
	
	if(comp)
		sort(th->data.begin(),th->data.end(),sortComparatorWrapper(comp));
	else
		sort(th->data.begin(),th->data.end(),sortComparator);

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

ASObject* Array::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride, ASObject* base)
{
	if(skip_impl || !implEnable)
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride, base);
		
	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="")
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride, base);

	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride, base);

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
		return ret;
	}
	else
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride, base);
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

void Array::setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride, ASObject* base)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o,enableOverride,base);

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

void Array::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool skip_impl)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidQName(name,ns,index))
	{
		ASObject::setVariableByQName(name,ns,o,skip_impl);
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

ASObject* Array::getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl)
{
	assert_and_throw(implEnable);
	throw UnsupportedException("Array::getVariableByQName not completely implemented");
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

ASString::ASString(const char* s, uint32_t len):data(s, len)
{
	type=T_STRING;
}

ASFUNCTIONBODY(ASString,_constructor)
{
	ASString* th=static_cast<ASString*>(obj);
	if(args && args[0])
		th->data=args[0]->toString().raw_buf();
	return NULL;
}

ASFUNCTIONBODY(ASString,_getLength)
{
	ASString* th=static_cast<ASString*>(obj);
	return abstract_i(th->data.size());
}

void ASString::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("toString","",Class<IFunction>::getFunction(ASObject::_toString));
	c->setVariableByQName("split",AS3,Class<IFunction>::getFunction(split));
	c->setVariableByQName("substr",AS3,Class<IFunction>::getFunction(substr));
	c->setVariableByQName("replace",AS3,Class<IFunction>::getFunction(replace));
	c->setVariableByQName("concat",AS3,Class<IFunction>::getFunction(concat));
	c->setVariableByQName("match",AS3,Class<IFunction>::getFunction(match));
	c->setVariableByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf));
	c->setVariableByQName("charCodeAt",AS3,Class<IFunction>::getFunction(charCodeAt));
	c->setVariableByQName("charAt",AS3,Class<IFunction>::getFunction(charAt));
	c->setVariableByQName("slice",AS3,Class<IFunction>::getFunction(slice));
	c->setVariableByQName("toLowerCase",AS3,Class<IFunction>::getFunction(toLowerCase));
	c->setVariableByQName("toUpperCase",AS3,Class<IFunction>::getFunction(toUpperCase));
	c->setGetterByQName("length","",Class<IFunction>::getFunction(_getLength));
}

void ASString::buildTraits(ASObject* o)
{
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

ASFUNCTIONBODY(ASString,match)
{
	ASString* th=static_cast<ASString*>(obj);
	if(args[0]==NULL || args[0]->getObjectType()==T_NULL || args[0]->getObjectType()==T_UNDEFINED)
		return new Null;
	Array* ret=NULL;
	if(args[0]->getPrototype() && args[0]->getPrototype()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);

		const char* error;
		int offset;
		int options=0;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		pcre* pcreRE=pcre_compile(re->re.c_str(), 0, &error, &offset,NULL);
		if(error)
			return new Null;
		//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			return new Null;
		}
		assert_and_throw(capturingGroups<10);
		int ovector[30];
		int rc=pcre_exec(pcreRE, NULL, th->data.c_str(), th->data.size(), 0, 0, ovector, 30);
		if(rc<=0)
		{
			//No matches or error
			pcre_free(pcreRE);
			return new Null;
		}
		ret=Class<Array>::getInstanceS();
		if(re->global)
		{
			do
			{
				int offset=ovector[1];
				ret->push(Class<ASString>::getInstanceS(th->data.substr(ovector[0], ovector[1]-ovector[0])));
				rc=pcre_exec(pcreRE, NULL, th->data.c_str(), th->data.size(), offset, 0, ovector, 9);
			}
			while(rc>0);
		}
		else
			ret->push(Class<ASString>::getInstanceS(th->data.substr(ovector[0], ovector[1]-ovector[0])));
		pcre_free(pcreRE);
	}
	else
	{
		ret=Class<Array>::getInstanceS();
		const tiny_string& arg0=args[0]->toString();
		if(th->data.find(arg0.raw_buf())!=th->data.npos) //Match found
			ret->push(Class<ASString>::getInstanceS(arg0));
	}
	return ret;
}

ASFUNCTIONBODY(ASString,split)
{
	ASString* th=static_cast<ASString*>(obj);
	Array* ret=Class<Array>::getInstanceS();
	ASObject* delimiter=args[0];
	if(delimiter->getObjectType()==T_UNDEFINED)
	{
		ret->push(Class<ASString>::getInstanceS(th->data));
		return ret;
	}

	if(args[0]->getPrototype() && args[0]->getPrototype()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);

		const char* error;
		int offset;
		int options=0;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		pcre* pcreRE=pcre_compile(re->re.c_str(), 0, &error, &offset,NULL);
		if(error)
			return ret;
		//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			return ret;
		}
		assert_and_throw(capturingGroups<10);
		int ovector[30];
		offset=0;
		unsigned int end;
		do
		{
			int rc=pcre_exec(pcreRE, NULL, th->data.c_str(), th->data.size(), offset, 0, ovector, 30);
			end=ovector[0];
			if(rc<=0)
				end=th->data.size();
			ASString* s=Class<ASString>::getInstanceS(th->data.substr(offset,end-offset));
			ret->push(s);
			offset=ovector[1];
			//Insert capturing groups
			for(int i=1;i<rc;i++)
			{
				ASString* s=Class<ASString>::getInstanceS(th->data.substr(ovector[i*2],ovector[i*2+1]-ovector[i*2]));
				ret->push(s);
			}
		}
		while(end<th->data.size());
		pcre_free(pcreRE);
	}
	else
	{
		const tiny_string& del=args[0]->toString();
		unsigned int start=0;
		do
		{
			int match=th->data.find(del.raw_buf(),start);
			if(del.len()==0)
				match++;
			if(match==-1)
				match=th->data.size();
			ASString* s=Class<ASString>::getInstanceS(th->data.substr(start,(match-start)));
			ret->push(s);
			start=match+del.len();
		}
		while(start<th->data.size());
	}

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
	if(data[index].type==DATA_OBJECT)
		out=data[index].data;
	else if(data[index].type==DATA_INT)
	{
		out=abstract_i(data[index].data_i);
		out->fake_decRef();
	}
	else
		throw UnsupportedException("Unexpeted data type");
	return true;
}

bool Array::hasNext(unsigned int& index, bool& out)
{
	assert_and_throw(implEnable);
	out=index<data.size();
	index++;
	return true;
}

bool Array::nextName(unsigned int index, ASObject*& out)
{
	assert(index>0);
	index--;
	assert_and_throw(implEnable);
	assert_and_throw(index<data.size());
	out=abstract_i(index);
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
	for(unsigned int i=0;i<data.size();i++)
	{
		if(!(data[i]>='0' && data[i]<='9' && data[i]!='.')) //not a number
			return numeric_limits<double>::quiet_NaN();
	}
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

TRISTATE ASString::isLess(ASObject* r)
{
	//ECMA-262 11.8.5 algorithm
	assert_and_throw(implEnable);
	if(getObjectType()==T_STRING && r->getObjectType()==T_STRING)
	{
		ASString* rstr=Class<ASString>::cast(r);
		return (data<rstr->data)?TTRUE:TFALSE;
	}
	number_t a=toNumber();
	number_t b=r->toNumber();
	if(isnan(a) || isnan(b))
		return TUNDEFINED;
	//TODO: Should we special handle infinite values?
	return (a<b)?TTRUE:TFALSE;
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

TRISTATE Undefined::isLess(ASObject* r)
{
	//ECMA-262 complaiant
	//As undefined became NaN when converted to number the operation is undefined
	return TUNDEFINED;
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

ASFUNCTIONBODY(Integer,generator)
{
	//Int is specified as 32bit
	return abstract_i(args[0]->toInt()&0xffffffff);
}

TRISTATE Integer::isLess(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(o);
		return (val < i->toInt())?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		Number* i=static_cast<Number*>(o);
		return (val < i->toNumber())?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_STRING)
	{
		const ASString* s=static_cast<const ASString*>(o);
		//Check if the string may be converted to integer
		//TODO: check whole string?
		if(isdigit(s->data[0]))
		{
			int val2=atoi(s->data.c_str());
			return (val < val2)?TTRUE:TFALSE;
		}
		else
			return TFALSE;
	}
	else if(o->getObjectType()==T_BOOLEAN)
	{
		Boolean* i=static_cast<Boolean*>(o);
		return (val < i->toInt())?TTRUE:TFALSE;
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
		return tiny_string(buf,true);
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

void Integer::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
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

TRISTATE UInteger::isLess(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		uint32_t val1=val;
		int32_t val2=o->toInt();
		if(val2<0)
			return TFALSE;
		else
			return (val1<(uint32_t)val2)?TTRUE:TFALSE;
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

TRISTATE Number::isLess(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return (val<i->val)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		const Number* i=static_cast<const Number*>(o);
		return (val<i->val)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_UNDEFINED)
	{
		//Undefined is NaN, so the result is undefined
		return TUNDEFINED;
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

void Number::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	//Must create and link the number the hard way
	Number* ninf=new Number(-numeric_limits<double>::infinity());
	Number* pinf=new Number(numeric_limits<double>::infinity());
	ninf->setPrototype(c);
	pinf->setPrototype(c);
	c->setVariableByQName("NEGATIVE_INFINITY","",ninf);
	c->setVariableByQName("POSITIVE_INFINITY","",pinf);
}

Date::Date():year(-1),month(-1),date(-1),hour(-1),minute(-1),second(-1),millisecond(-1)
{
}

void Date::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("getTimezoneOffset","",Class<IFunction>::getFunction(getTimezoneOffset));
	c->setVariableByQName("valueOf","",Class<IFunction>::getFunction(valueOf));
	c->setVariableByQName("getTime",AS3,Class<IFunction>::getFunction(getTime));
	c->setVariableByQName("getFullYear","",Class<IFunction>::getFunction(getFullYear));
	c->setVariableByQName("getHours",AS3,Class<IFunction>::getFunction(getHours));
	c->setVariableByQName("getMinutes",AS3,Class<IFunction>::getFunction(getMinutes));
	c->setVariableByQName("getSeconds",AS3,Class<IFunction>::getFunction(getMinutes));
	//o->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(ASObject::_toString));
}

void Date::buildTraits(ASObject* o)
{
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
	bool overrideThis=true;
	//Only allow overriding if the type of args[0] is a subclass of closure_this
	if(!(th->closure_this && th->closure_this->prototype && args[0]->prototype && args[0]->prototype->isSubClass(th->closure_this->prototype)) ||
		args[0]->prototype==NULL)
	{
		overrideThis=false;
	}
	ASObject* ret=th->call(args[0],new_args,len,overrideThis);
	delete[] new_args;
	return ret;
}

ASFUNCTIONBODY(IFunction,_call)
{
	IFunction* th=static_cast<IFunction*>(obj);
	assert_and_throw(argslen>=1);
	ASObject** new_args=new ASObject*[argslen-1];
	for(unsigned int i=1;i<argslen;i++)
	{
		new_args[i-1]=args[i];
		new_args[i-1]->incRef();
	}

	args[0]->incRef();
	bool overrideThis=true;
	//Only allow overriding if the type of args[0] is a subclass of closure_this
	if(!(th->closure_this && th->closure_this->prototype && args[0]->prototype && args[0]->prototype->isSubClass(th->closure_this->prototype)) ||
		args[0]->prototype==NULL)
	{
		overrideThis=false;
	}
	ASObject* ret=th->call(args[0],new_args,argslen-1,overrideThis);
	delete[] new_args;
	return ret;
}

SyntheticFunction::SyntheticFunction(method_info* m):hit_count(0),mi(m),val(NULL)
{
//	class_index=-2;
}

ASObject* SyntheticFunction::call(ASObject* obj, ASObject* const* args, uint32_t numArgs, bool thisOverride)
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
	//We use the stored level or the object's level
	int realLevel=(closure_level!=-1)?closure_level:obj->getLevel();

	call_context* cc=new call_context(mi,realLevel,args,passedToLocals);
	uint32_t i=passedToLocals;
	cc->scope_stack=func_scope;
	for(unsigned int i=0;i<func_scope.size();i++)
		func_scope[i]->incRef();

	if(bound && closure_this && !thisOverride)
	{
		LOG(LOG_CALLS,"Calling with closure " << this);
		obj=closure_this;
	}

	cc->locals[0]=obj;
	obj->incRef();

	//Fixup missing parameters
	unsigned int missing_params=args_len-i;
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

ASObject* Function::call(ASObject* obj, ASObject* const* args, uint32_t num_args, bool thisOverride)
{
	ASObject* ret;
	if(bound && closure_this && !thisOverride)
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
	c->setVariableByQName("LOG10E","",abstract_d(0.4342944819032518));
	c->setVariableByQName("sqrt","",Class<IFunction>::getFunction(sqrt));
	c->setVariableByQName("atan2","",Class<IFunction>::getFunction(atan2));
	c->setVariableByQName("max","",Class<IFunction>::getFunction(_max));
	c->setVariableByQName("min","",Class<IFunction>::getFunction(_min));
	c->setVariableByQName("abs","",Class<IFunction>::getFunction(abs));
	c->setVariableByQName("sin","",Class<IFunction>::getFunction(sin));
	c->setVariableByQName("cos","",Class<IFunction>::getFunction(cos));
	c->setVariableByQName("log","",Class<IFunction>::getFunction(log));
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
	return abstract_d(::ceil(n));
}

ASFUNCTIONBODY(Math,log)
{
	double n=args[0]->toNumber();
	return abstract_d(::log(n));
}

ASFUNCTIONBODY(Math,floor)
{
	double n=args[0]->toNumber();
	return abstract_d(::floor(n));
}

ASFUNCTIONBODY(Math,round)
{
	double n=args[0]->toNumber();
	return abstract_d(::round(n));
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
	c->setVariableByQName("exec",AS3,Class<IFunction>::getFunction(exec));
	c->setVariableByQName("test",AS3,Class<IFunction>::getFunction(test));
	c->setGetterByQName("global","",Class<IFunction>::getFunction(_getGlobal));
}

void RegExp::buildTraits(ASObject* o)
{
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
		//TODO: add index field (not possible with current PCRECPP)
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

ASFUNCTIONBODY(ASString,charAt)
{
	ASString* th=static_cast<ASString*>(obj);
	int index=args[0]->toInt();
	int maxIndex=th->data.size();
	if(index<0 || index>=maxIndex)
		return Class<ASString>::getInstanceS();
	return Class<ASString>::getInstanceS(th->data.c_str()+index, 1);
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
	ASString* ret=Class<ASString>::getInstanceS(th->data);
	transform(th->data.begin(), th->data.end(), ret->data.begin(), ::tolower);
	return ret;
}

ASFUNCTIONBODY(ASString,toUpperCase)
{
	ASString* th=static_cast<ASString*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->data);
	transform(th->data.begin(), th->data.end(), ret->data.begin(), ::toupper);
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

void ASError::sinit(Class_base* c)
{
	c->setVariableByQName("getStackTrace",AS3,Class<IFunction>::getFunction(getStackTrace));
	c->setGetterByQName("errorID",AS3,Class<IFunction>::getFunction(_getErrorID));
	c->setGetterByQName("message",AS3,Class<IFunction>::getFunction(_getMessage));
	c->setSetterByQName("message",AS3,Class<IFunction>::getFunction(_setMessage));
	c->setGetterByQName("name",AS3,Class<IFunction>::getFunction(_getName));
	c->setSetterByQName("name",AS3,Class<IFunction>::getFunction(_setName));
}

void ASError::buildTraits(ASObject* o)
{
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

ASObject* Class_base::generator(ASObject* const* args, const unsigned int argslen)
{
	return ASObject::generator(NULL, args, argslen);
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
		ASObject* ret=constructor->call(target,args,argslen);
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

void IFunction::sinit(Class_base* c)
{
	c->setVariableByQName("call",AS3,Class<IFunction>::getFunction(IFunction::_call));
	c->setVariableByQName("apply",AS3,Class<IFunction>::getFunction(IFunction::apply));
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
			ASObject* interface_obj=getGlobal()->getVariableByMultiname(interfaces[i]);
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

void Class_base::linkInterface(Class_base* c) const
{
	if(class_index==-1)
	{
		//LOG(LOG_NOT_IMPLEMENTED,"Linking of builtin interface " << class_name << " not supported");
		return;
	}
	//Recursively link interfaces implemented by this interface
	for(unsigned int i=0;i<getInterfaces().size();i++)
		getInterfaces()[i]->linkInterface(c);

	assert_and_throw(context);

	//Link traits of this interface
	for(unsigned int j=0;j<context->instances[class_index].trait_count;j++)
	{
		traits_info* t=&context->instances[class_index].traits[j];
		context->linkTrait(c,t);
	}

	if(constructor)
	{
		LOG(LOG_CALLS,"Calling interface init for " << class_name);
		ASObject* ret=constructor->call(c,NULL,0);
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
	ASObject* ret=o->getVariableByQName(name,"");
	assert_and_throw(ret);
	ret->incRef();
	o->setVariableByQName(name,interfaceNs,ret);
}

void UInteger::sinit(Class_base* c)
{
	//TODO: add in the JIT support for unsigned number
	//Right now we pretend to be signed, to make comparisons work
	c->setVariableByQName("MAX_VALUE","",new UInteger(0x7fffffff));
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
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
		sys->classes.insert(std::make_pair(ClassName<IFunction>::name,ret));
		IFunction::sinit(ret);
	}
	else
		ret=static_cast<Class<IFunction>*>(it->second);

	ret->incRef();
	return ret;
}

//We follow the Boolean() algorithm, but return a concrete result, not a Boolean object
bool lightspark::Boolean_concrete(ASObject* obj)
{
	if(obj->getObjectType()==T_STRING)
	{
		LOG(LOG_CALLS,"String to bool");
		const tiny_string& s=obj->toString();
		if(s.len()==0)
			return false;
		else
			return true;
	}
	else if(obj->getObjectType()==T_BOOLEAN)
	{
		Boolean* b=static_cast<Boolean*>(obj);
		LOG(LOG_CALLS,"Boolean to bool " << b->val);
		return b->val;
	}
	else if(obj->getObjectType()==T_OBJECT)
	{
		LOG(LOG_CALLS,"Object to bool");
		return true;
	}
	else if(obj->getObjectType()==T_CLASS)
	{
		LOG(LOG_CALLS,"Class to bool");
		return true;
	}
	else if(obj->getObjectType()==T_ARRAY)
	{
		LOG(LOG_CALLS,"Array to bool");
		return true;
	}
	else if(obj->getObjectType()==T_UNDEFINED)
	{
		LOG(LOG_CALLS,"Undefined to bool");
		return false;
	}
	else if(obj->getObjectType()==T_NULL)
	{
		LOG(LOG_CALLS,"Null to bool");
		return false;
	}
	else if(obj->getObjectType()==T_NUMBER)
	{
		LOG(LOG_CALLS,"Number to bool");
		double val=obj->toNumber();
		if(val==0 || isnan(val))
			return false;
		else
			return true;
	}
	else if(obj->getObjectType()==T_INTEGER)
	{
		LOG(LOG_CALLS,"Integer to bool");
		int32_t val=obj->toInt();
		if(val==0)
			return false;
		else
			return true;
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"Boolean conversion for type " << obj->getObjectType() << endl);
		return false;
	}
}

ASFUNCTIONBODY(lightspark,parseInt)
{
	if(args[0]->getObjectType()==T_UNDEFINED)
		return new Undefined;
	else
		return abstract_i(atoi(args[0]->toString().raw_buf()));
}

ASFUNCTIONBODY(lightspark,parseFloat)
{
	if(args[0]->getObjectType()==T_UNDEFINED)
		return new Undefined;
	else
		return abstract_d(atof(args[0]->toString().raw_buf()));
}

ASFUNCTIONBODY(lightspark,isNaN)
{
	if(args[0]->getObjectType()==T_UNDEFINED)
		return abstract_b(true);
	else if(args[0]->getObjectType()==T_INTEGER)
		return abstract_b(false);
	else if(args[0]->getObjectType()==T_NUMBER)
	{
		if(isnan(args[0]->toNumber()))
			return abstract_b(true);
		else
			return abstract_b(false);
	}
	else if(args[0]->getObjectType()==T_BOOLEAN)
		return abstract_b(false);
	else if(args[0]->getObjectType()==T_STRING)
	{
		double n=args[0]->toNumber();
		return abstract_b(isnan(n));
	}
	else
		throw UnsupportedException("Weird argument for isNaN");
}

ASFUNCTIONBODY(lightspark,isFinite)
{
	if(args[0]->getObjectType()==T_NUMBER)
	{
		if(isfinite(args[0]->toNumber()))
			return abstract_b(true);
		else
			return abstract_b(false);
	}
	else
		throw UnsupportedException("Weird argument for isNaN");
}

ASFUNCTIONBODY(lightspark,unescape)
{
	ASString* th=static_cast<ASString*>(args[0]);
	string ret;
	ret.reserve(th->data.size());
	for(unsigned int i=0;i<th->data.size();i++)
	{
		if(th->data[i]=='%')
			throw UnsupportedException("Unescape not completely implemented");
		else
			ret.push_back(th->data[i]);
	}
	return Class<ASString>::getInstanceS(ret);
}

