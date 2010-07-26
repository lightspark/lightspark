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

ByteArray::ByteArray(const ByteArray& b):ASObject(b),len(b.len),position(b.position)
{
	assert_and_throw(position==0);
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
	o->setGetterByQName("length","",Class<IFunction>::getFunction(_getLength));
	o->setGetterByQName("bytesAvailable","",Class<IFunction>::getFunction(_getBytesAvailable));
	o->setGetterByQName("position","",Class<IFunction>::getFunction(_getPosition));
	o->setSetterByQName("position","",Class<IFunction>::getFunction(_setPosition));
	o->setVariableByQName("readBytes","",Class<IFunction>::getFunction(readBytes));
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
		assert_and_throw(size<=len);
	}
	return bytes;
}

ASFUNCTIONBODY(ByteArray,_getPosition)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	return abstract_i(th->position);
}

ASFUNCTIONBODY(ByteArray,_setPosition)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	int pos=args[0]->toInt();
	th->position=pos;
	return NULL;
}

ASFUNCTIONBODY(ByteArray,_getLength)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	return abstract_i(th->len);
}

ASFUNCTIONBODY(ByteArray,_getBytesAvailable)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	return abstract_i(th->len-th->position);
}

ASFUNCTIONBODY(ByteArray,readBytes)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//Validate parameters
	assert_and_throw(argslen==3);
	assert_and_throw(args[0]->getPrototype()==Class<ByteArray>::getClass());

	ByteArray* out=Class<ByteArray>::cast(args[0]);
	int offset=args[1]->toInt();
	int length=args[2]->toInt();
	//TODO: Support offset
	if(offset!=0)
		throw UnsupportedException("offset in ByteArray::readBytes");

	uint8_t* buf=out->getBuffer(length);
	memcpy(buf,th->bytes+th->position,length);
	th->position+=length;

	return NULL;
}

objAndLevel ByteArray::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride)
{
	assert_and_throw(!skip_impl);
	assert_and_throw(implEnable);
	//It seems that various kind of implementation works only with the empty namespace
	assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride);

	assert_and_throw(index<len);
	ASObject* ret=abstract_i(bytes[index]);

	return objAndLevel(ret,0);
}

intptr_t ByteArray::getVariableByMultiname_i(const multiname& name)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::getVariableByMultiname_i(name);

	assert_and_throw(index<len);
	return bytes[index];
}

void ByteArray::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool find_back, bool skip_impl)
{
	if(!implEnable || skip_impl)
	{
		ASObject::setVariableByQName(name,ns,o,find_back,skip_impl);
		return;
	}
	
	unsigned int index=0;
	if(!Array::isValidQName(name,ns,index))
	{
		ASObject::setVariableByQName(name,ns,o,find_back,skip_impl);
		return;
	}
	throw UnsupportedException("ByteArray::setVariableByQName not completely implemented");
}

void ByteArray::setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o,enableOverride);

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
}

void ByteArray::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
	{
		ASObject::setVariableByMultiname_i(name,value);
		return;
	}

	throw UnsupportedException("ByteArray::setVariableByMultiname_i not completely implemented");
}

bool ByteArray::isEqual(ASObject* r)
{
	assert_and_throw(implEnable);
	/*if(r->getObjectType()!=T_OBJECT)
		return false;*/

	throw UnsupportedException("ByteArray::isEqual");
	return false;
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
			sys->currentVm->addEvent(this,Class<TimerEvent>::getInstanceS("timer"));
			//Do not spam timer events until this is done
			SynchronizationEvent* se=new SynchronizationEvent;
			bool added=sys->currentVm->addEvent(NULL,se);
			if(!added)
			{
				se->decRef();
				throw RunTimeException("Could not add event");
			}
			se->wait();
			se->decRef();
		}
	}
}

void Timer::threadAbort()
{
	running=false;
	throw UnsupportedException("Timer::threadAbort");
}

void Timer::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

ASFUNCTIONBODY(Timer,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	Timer* th=static_cast<Timer*>(obj);
	obj->setVariableByQName("start","",Class<IFunction>::getFunction(start));
	obj->setVariableByQName("reset","",Class<IFunction>::getFunction(reset));

	th->delay=args[0]->toInt();
	if(argslen>=2)
		th->repeatCount=args[1]->toInt();

	return NULL;
}

ASFUNCTIONBODY(Timer,start)
{
	Timer* th=static_cast<Timer*>(obj);
	th->running=true;
	th->incRef();
	sys->addJob(th);
	return NULL;
}

ASFUNCTIONBODY(Timer,reset)
{
	Timer* th=static_cast<Timer*>(obj);
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
		assert_and_throw(target->getPrototype());
		c=target->getPrototype();
	}
	else
		c=static_cast<Class_base*>(target);

	return Class<ASString>::getInstanceS(c->getQualifiedClassName());
}

ASFUNCTIONBODY(lightspark,getQualifiedSuperclassName)
{
	//CHECK: what to do is ns is empty
	ASObject* target=args[0];
	Class_base* c;
	if(target->getObjectType()!=T_CLASS)
	{
		assert_and_throw(target->getPrototype());
		c=target->getPrototype()->super;
	}
	else
		c=static_cast<Class_base*>(target)->super;

	assert_and_throw(c);

	return Class<ASString>::getInstanceS(c->getQualifiedClassName());
}

ASFUNCTIONBODY(lightspark,getDefinitionByName)
{
	assert_and_throw(args && argslen==1);
	const tiny_string& tmp=args[0]->toString();
	tiny_string name,ns;

	stringToQName(tmp,name,ns);

	LOG(LOG_CALLS,"Looking for definition of " << ns << " :: " << name);
	objAndLevel o=getGlobal()->getVariableByQName(name,ns);

	//TODO: should raise an exception, for now just return undefined	
	if(o.obj==NULL)
	{
		LOG(LOG_ERROR,"Definition for '" << ns << " :: " << name << "' not found.");
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

	assert_and_throw(o.obj->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,"Getting definition for " << ns << " :: " << name);
	o.obj->incRef();
	return o.obj;
}

ASFUNCTIONBODY(lightspark,getTimer)
{
	uint64_t ret=compat_msectiming() - sys->startTime;
	return abstract_i(ret);
}

Dictionary::~Dictionary()
{
	if(!sys->finalizingDestruction)
	{
		std::map<ASObject*,ASObject*>::iterator it=data.begin();
		for(;it!=data.end();it++)
			it->second->decRef();
	}
}

void Dictionary::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
}

void Dictionary::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Dictionary,_constructor)
{
	return NULL;
}

void Dictionary::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	assert_and_throw(implEnable);
	Dictionary::setVariableByMultiname(name,abstract_i(value),true);
}

void Dictionary::setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride)
{
	assert_and_throw(implEnable);
	if(name.name_type==multiname::NAME_OBJECT)
	{
		//We can use the [] operator, as the value is just a pointer and there is no side effect in creating one
		data[name.name_o]=o;
		//This is ugly, but at least we are sure that we own name_o
		multiname* tmp=const_cast<multiname*>(&name);
		tmp->name_o=NULL;
	}
	else if(name.name_type==multiname::NAME_STRING)
		data[Class<ASString>::getInstanceS(name.name_s)]=o;
	else if(name.name_type==multiname::NAME_INT)
		data[abstract_i(name.name_i)]=o;
	else
	{
		throw UnsupportedException("Unsupported name kind in Dictionary::setVariableByMultiname");
	}
}

void Dictionary::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(implEnable);
	assert_and_throw(name.name_type==multiname::NAME_OBJECT);
	map<ASObject*,ASObject*>::iterator it=data.find(name.name_o);
	assert_and_throw(it!=data.end());

	ASObject* ret=it->second;
	ret->decRef();

	data.erase(it);

	//This is ugly, but at least we are sure that we own name_o
	multiname* tmp=const_cast<multiname*>(&name);
	tmp->name_o=NULL;
}

objAndLevel Dictionary::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride)
{
	assert_and_throw(!skip_impl);
	assert_and_throw(implEnable);
	//It seems that various kind of implementation works only with the empty namespace
	assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");
	ASObject* ret=NULL;
	if(name.name_type==multiname::NAME_OBJECT)
	{
		//From specs, then === operator compare references when working on generic objects
		map<ASObject*,ASObject*>::iterator it=data.find(name.name_o);
		if(it==data.end())
			ret=new Undefined;
		else
		{
			ret=it->second;
			ret->incRef();
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
				ASString* s=Class<ASString>::cast(it->first);
				if(name.name_s == s->data.c_str())
				{
					//Value found
					ret=it->second;
					ret->incRef();
					return objAndLevel(ret,0);
				}
			}
		}
		//Value not found
		ret=new Undefined;
	}
	else
		throw UnsupportedException("Unsupported name kind on Dictionary::getVariableByMultiname");

	return objAndLevel(ret,0);
}

bool Dictionary::hasNext(unsigned int& index, bool& out)
{
	assert_and_throw(implEnable);
	out=index<data.size();
	index++;
	return true;
}

bool Dictionary::nextName(unsigned int index, ASObject*& out)
{
	assert_and_throw(implEnable);
	assert_and_throw(index<data.size());
	map<ASObject*,ASObject*>::iterator it=data.begin();
	for(unsigned int i=0;i<index;i++)
		it++;
	out=it->first;
	return true;
}

bool Dictionary::nextValue(unsigned int index, ASObject*& out)
{
	assert_and_throw(implEnable);
	throw UnsupportedException("Dictionary::nextValue not implmented");
/*	assert(index<data.size());
	map<ASObject*,ASObject*>::iterator it=data.begin();
	for(int i=0;i<index;i++)
		it++;
	out=it->first;
	return true;*/
	return false;
}

void Proxy::sinit(Class_base* c)
{
	//c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
}

void Proxy::setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride)
{
	assert_and_throw(implEnable);
	//If a variable named like this already exist, return that
	if(hasPropertyByMultiname(name) || !implEnable)
	{
		ASObject::setVariableByMultiname(name,o,enableOverride);
		return;
	}

	//Check if there is a custom setter defined, skipping implementation to avoid recursive calls
	objAndLevel proxySetter=getVariableByQName("setProperty",flash_proxy,true);

	if(proxySetter.obj==NULL)
	{
		ASObject::setVariableByMultiname(name,o,enableOverride);
		return;
	}

	assert_and_throw(proxySetter.obj->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(proxySetter.obj);

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* args[2];
	args[0]=Class<ASString>::getInstanceS(name.name_s);
	args[1]=o;
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,"Proxy::setProperty");
	ASObject* ret=f->call(this,args,2,getLevel());
	assert_and_throw(ret==NULL);
	implEnable=true;
}

objAndLevel Proxy::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride)
{
	assert_and_throw(!skip_impl);
	//It seems that various kind of implementation works only with the empty namespace
	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="" || hasPropertyByMultiname(name) || !implEnable)
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride);

	//Check if there is a custom getter defined, skipping implementation to avoid recursive calls
	objAndLevel o=getVariableByQName("getProperty",flash_proxy,true);

	if(o.obj==NULL)
		return ASObject::getVariableByMultiname(name,skip_impl,enableOverride);

	assert_and_throw(o.obj->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(o.obj);

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* arg=Class<ASString>::getInstanceS(name.name_s);
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,"Proxy::getProperty");
	ASObject* ret=f->call(this,&arg,1,getLevel());
	assert_and_throw(ret);
	implEnable=true;
	return objAndLevel(ret,0);
}
