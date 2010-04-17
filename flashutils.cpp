/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "abc.h"
#include "flashutils.h"
#include "asobjects.h"
#include "class.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME(ByteArray);
REGISTER_CLASS_NAME(Timer);
REGISTER_CLASS_NAME(Dictionary);
REGISTER_CLASS_NAME(Proxy);

ByteArray::ByteArray():bytes(NULL),len(0),position(0)
{
}

ByteArray::ByteArray(const ByteArray& b):IInterface(b),len(b.len),position(b.position)
{
	assert(position==0);
	bytes=new uint8_t[len];
	memcpy(bytes,b.bytes,len);
}

ByteArray::~ByteArray()
{
	delete[] bytes;
}

void ByteArray::sinit(Class_base* c)
{
}

void ByteArray::buildTraits(ASObject* o)
{
	o->setGetterByQName("length","",new Function(_getLength));
	o->setGetterByQName("bytesAvailable","",new Function(_getBytesAvailable));
	o->setGetterByQName("position","",new Function(_getPosition));
	o->setSetterByQName("position","",new Function(_setPosition));
	o->setVariableByQName("readBytes","",new Function(readBytes));
}

uint8_t* ByteArray::getBuffer(unsigned int size)
{
	if(bytes==NULL)
	{
		len=size;
		bytes=new uint8_t[len];
	}
	else
	{
		assert(size<=len);
	}
	return bytes;
}

ASFUNCTIONBODY(ByteArray,_getPosition)
{
	ByteArray* th=static_cast<ByteArray*>(obj->implementation);
	return abstract_i(th->position);
}

ASFUNCTIONBODY(ByteArray,_setPosition)
{
	ByteArray* th=static_cast<ByteArray*>(obj->implementation);
	int pos=args[0]->toInt();
	th->position=pos;
	return NULL;
}

ASFUNCTIONBODY(ByteArray,_getLength)
{
	ByteArray* th=static_cast<ByteArray*>(obj->implementation);
	return abstract_i(th->len);
}

ASFUNCTIONBODY(ByteArray,_getBytesAvailable)
{
	ByteArray* th=static_cast<ByteArray*>(obj->implementation);
	return abstract_i(th->len-th->position);
}

ASFUNCTIONBODY(ByteArray,readBytes)
{
	ByteArray* th=static_cast<ByteArray*>(obj->implementation);
	//Validate parameters
	assert(argslen==3);
	assert(args[0]->prototype==Class<ByteArray>::getClass());

	ByteArray* out=Class<ByteArray>::cast(args[0]->implementation);
	int offset=args[1]->toInt();
	int length=args[2]->toInt();
	//TODO: Support offset
	if(offset!=0)
		abort();

	uint8_t* buf=out->getBuffer(length);
	memcpy(buf,th->bytes+th->position,length);
	th->position+=length;

	return NULL;
}

bool ByteArray::getVariableByMultiname(const multiname& name, ASObject*& out)
{
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return false;

	assert(index<len);
	out=abstract_i(bytes[index]);

	return true;
}

bool ByteArray::getVariableByMultiname_i(const multiname& name, intptr_t& out)
{
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return false;

	assert(index<len);
	out=bytes[index];

	return true;
}

bool ByteArray::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o)
{
	unsigned int index=0;
	if(!Array::isValidQName(name,ns,index))
		return false;

	abort();
}

bool ByteArray::setVariableByMultiname(const multiname& name, ASObject* o)
{
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return false;

	if(index>=len)
	{
		//Resize the array
		uint8_t* buf=new uint8_t[index+1];
		memcpy(buf,bytes,len);
		delete[] bytes;
		len=index+1;
		bytes=buf;
	}

	if(o->getObjectType()!=T_UNDEFINED)
		bytes[index]=o->toInt();
	else
		bytes[index]=0;
	return true;
}

bool ByteArray::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return false;

	abort();
}

bool ByteArray::isEqual(bool& ret, ASObject* r)
{
	if(r->getObjectType()!=T_OBJECT)
		return false;
	
	abort();
}

void ByteArray::acquireBuffer(uint8_t* buf, int bufLen)
{
	if(bytes)
		delete[] bytes;
	bytes=buf;
	len=bufLen;
	position=0;
}

void Timer::execute()
{
	while(running)
	{
		compat_msleep(delay);
		if(running)
		{
			sys->currentVm->addEvent(this,Class<TimerEvent>::getInstanceS(true,"timer"));
			//Do not spam timer events until this is done
			SynchronizationEvent* se=new SynchronizationEvent;
			sys->currentVm->addEvent(NULL,se);
			se->wait();
			delete se;
		}
	}
}

void Timer::sinit(Class_base* c)
{
	c->setConstructor(new Function(_constructor));
}

ASFUNCTIONBODY(Timer,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	Timer* th=static_cast<Timer*>(obj->implementation);
	obj->setVariableByQName("start","",new Function(start));
	obj->setVariableByQName("reset","",new Function(reset));

	th->delay=args[0]->toInt();
	if(argslen>=2)
		th->repeatCount=args[1]->toInt();

	return NULL;
}

ASFUNCTIONBODY(Timer,start)
{
	Timer* th=static_cast<Timer*>(obj->implementation);
	th->running=true;
	sys->addJob(th);
	return NULL;
}

ASFUNCTIONBODY(Timer,reset)
{
	Timer* th=static_cast<Timer*>(obj->implementation);
	th->running=false;
	return NULL;
}

ASFUNCTIONBODY(lightspark,getQualifiedClassName)
{
	//CHECK: what to do is ns is empty
	ASObject* target=args[0];
	Class_base* c;
	if(target->getObjectType()!=T_CLASS)
	{
		assert(target->prototype);
		c=target->prototype;
	}
	else
		c=static_cast<Class_base*>(target);

	return Class<ASString>::getInstanceS(true,c->getQualifiedClassName())->obj;
}

ASFUNCTIONBODY(lightspark,getQualifiedSuperclassName)
{
	//CHECK: what to do is ns is empty
	ASObject* target=args[0];
	Class_base* c;
	if(target->getObjectType()!=T_CLASS)
	{
		assert(target->prototype);
		c=target->prototype->super;
	}
	else
		c=static_cast<Class_base*>(target)->super;

	assert(c);

	return Class<ASString>::getInstanceS(true,c->getQualifiedClassName())->obj;
}

ASFUNCTIONBODY(lightspark,getDefinitionByName)
{
	assert(args && argslen==1);
	const tiny_string& tmp=args[0]->toString();
	tiny_string name,ns;

	stringToQName(tmp,name,ns);

	LOG(LOG_CALLS,"Looking for definition of " << ns << " :: " << name);
	objAndLevel o=getGlobal()->getVariableByQName(name,ns);

	//assert(owner);
	//TODO: should raise an exception, for now just return undefined
	if(o.obj==NULL)
	{
		LOG(LOG_NOT_IMPLEMENTED,"NOT found");
		return new Undefined;
	}

	//Check if the object has to be defined
	if(o.obj->getObjectType()==T_DEFINABLE)
	{
		LOG(LOG_CALLS,"We got an object not yet valid");
		Definable* d=static_cast<Definable*>(o.obj);
		d->define(getGlobal());
		o=getGlobal()->getVariableByQName(name,ns);
	}

	assert(o.obj->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,"Getting definition for " << ns << " :: " << name);
	return o.obj;
}

ASFUNCTIONBODY(lightspark,getTimer)
{
	uint64_t ret=compat_msectiming() - sys->startTime;
	return abstract_i(ret);
}

void Dictionary::sinit(Class_base* c)
{
	c->setConstructor(new Function(_constructor));
}

void Dictionary::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Dictionary,_constructor)
{
	return NULL;
}

bool Dictionary::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	return setVariableByMultiname(name,abstract_i(value));
}

bool Dictionary::setVariableByMultiname(const multiname& name, ASObject* o)
{
	if(name.name_type==multiname::NAME_OBJECT)
	{
		//We can use the [] operator, as the value is just a pointer and there is no side effect in creating one
		data[name.name_o]=o;
		//This is ugly, but at least we are sure that we own name_o
		multiname* tmp=const_cast<multiname*>(&name);
		tmp->name_o=NULL;
		return true;
	}
	else if(name.name_type==multiname::NAME_STRING)
	{
		data[Class<ASString>::getInstanceS(true,name.name_s)->obj]=o;
		return true;
	}
	else
		abort();
}

bool Dictionary::deleteVariableByMultiname(const multiname& name)
{
	assert(name.name_type==multiname::NAME_OBJECT);
	map<ASObject*,ASObject*>::iterator it=data.find(name.name_o);
	assert(it!=data.end());

	ASObject* ret=it->second;
	ret->decRef();

	data.erase(it);

	//This is ugly, but at least we are sure that we own name_o
	multiname* tmp=const_cast<multiname*>(&name);
	tmp->name_o=NULL;
	return true;
}

bool Dictionary::getVariableByMultiname(const multiname& name, ASObject*& out)
{
	if(name.name_type==multiname::NAME_OBJECT)
	{
		//From specs, then === operator compare references when working on generic objects
		map<ASObject*,ASObject*>::iterator it=data.find(name.name_o);
		if(it==data.end())
			out=new Undefined;
		else
		{
			ASObject* ret=it->second;
			ret->incRef();
			out=ret;
			//This is ugly, but at least we are sure that we own name_o
			multiname* tmp=const_cast<multiname*>(&name);
			tmp->name_o=NULL;
		}
	}
	else if(name.name_type==multiname::NAME_STRING)
	{
		//Ok, we need to do the slow lookup on every object and check for === comparison
		map<ASObject*, ASObject*>::iterator it=data.begin();
		for(;it!=data.end();it++)
		{
			if(it->first->getObjectType()==T_STRING)
			{
				ASString* s=Class<ASString>::cast(it->first->implementation);
				if(name.name_s == s->data.c_str())
				{
					//Value found
					ASObject* ret=it->second;
					ret->incRef();
					out=ret;
					return true;
				}
			}
		}
		//Value not found
		out=new Undefined;
	}
	else
		abort();

	return true;
}

bool Dictionary::hasNext(unsigned int& index, bool& out)
{
	out=index<data.size();
	index++;
	return true;
}

bool Dictionary::nextName(unsigned int index, ASObject*& out)
{
	assert(index<data.size());
	map<ASObject*,ASObject*>::iterator it=data.begin();
	for(unsigned int i=0;i<index;i++)
		it++;
	out=it->first;
	return true;
}

bool Dictionary::nextValue(unsigned int index, ASObject*& out)
{
	abort();
/*	assert(index<data.size());
	map<ASObject*,ASObject*>::iterator it=data.begin();
	for(int i=0;i<index;i++)
		it++;
	out=it->first;
	return true;*/
}

void Proxy::sinit(Class_base* c)
{
	//c->constructor=new Function(_constructor);
	c->setConstructor(NULL);
}

bool Proxy::setVariableByMultiname(const multiname& name, ASObject* v)
{
	//If a variable named like this already exist, return that
	if(obj->hasPropertyByMultiname(name))
		return false;
	if(suppress)
		return false;

	//Check if there is a custom setter defined, skipping implementation to avoid recursive calls
	objAndLevel o=obj->getVariableByQName("setProperty",flash_proxy,true);

	if(o.obj==NULL)
		return false;

	assert(o.obj->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(o.obj);

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* args[2];
	args[0]=Class<ASString>::getInstanceS(true,name.name_s)->obj;
	args[1]=v;
	//We now suppress special handling
	suppress=true;
	LOG(LOG_CALLS,"Proxy::setProperty");
	f->call(obj,args,2,obj->getLevel());
	suppress=false;
	return true;
}

bool Proxy::getVariableByMultiname(const multiname& name, ASObject*& out)
{
	if(obj->hasPropertyByMultiname(name))
		return false;
	if(suppress)
		return false;

	//Check if there is a custom getter defined, skipping implementation to avoid recursive calls
	objAndLevel o=obj->getVariableByQName("getProperty",flash_proxy,true);

	if(o.obj==NULL)
		return false;

	assert(o.obj->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(o.obj);

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* arg=Class<ASString>::getInstanceS(true,name.name_s)->obj;
	//We now suppress special handling
	suppress=true;
	LOG(LOG_CALLS,"Proxy::getProperty");
	out=f->call(obj,&arg,1,obj->getLevel());
	assert(out);
	suppress=false;
	return true;
}
