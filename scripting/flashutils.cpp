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
#include "asobject.h"
#include "class.h"
#include "compat.h"
#include <sstream>

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.utils");

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
	c->setGetterByQName("length","",Class<IFunction>::getFunction(_getLength),true);
	c->setGetterByQName("bytesAvailable","",Class<IFunction>::getFunction(_getBytesAvailable),true);
	c->setGetterByQName("position","",Class<IFunction>::getFunction(_getPosition),true);
	c->setSetterByQName("position","",Class<IFunction>::getFunction(_setPosition),true);
	c->setMethodByQName("readBytes","",Class<IFunction>::getFunction(readBytes),true);
}

void ByteArray::buildTraits(ASObject* o)
{
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

ASObject* ByteArray::getVariableByMultiname(const multiname& name, bool skip_impl, ASObject* base)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(skip_impl || !Array::isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,skip_impl,base);

	assert_and_throw(index<len);
	ASObject* ret=abstract_i(bytes[index]);

	return ret;
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

void ByteArray::setVariableByMultiname(const multiname& name, ASObject* o, ASObject* base)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o,base);

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

void Timer::tick()
{
	TimerEvent* e=Class<TimerEvent>::getInstanceS("timer");
	sys->currentVm->addEvent(this,e);
	e->decRef();
	if(repeatCount==0)
		sys->addWait(delay,this);
	else
	{
		repeatCount--;
		if(repeatCount)
			sys->addWait(delay,this);
	}
}

void Timer::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
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
	sys->addWait(th->delay,th);
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
	multiname name;
	name.name_type=multiname::NAME_STRING;
	name.ns.push_back(nsNameAndKind("",NAMESPACE)); //TODO: set type

	stringToQName(tmp,name.name_s,name.ns[0].name);

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ASObject* target;
	ASObject* o=getGlobal()->getVariableAndTargetByMultiname(name,target);

	//TODO: should raise an exception, for now just return undefined	
	if(o==NULL)
	{
		LOG(LOG_ERROR,_("Definition for '") << name << _("' not found."));
		return new Undefined;
	}

	//Check if the object has to be defined
	if(o->getObjectType()==T_DEFINABLE)
	{
		LOG(LOG_CALLS,_("We got an object not yet valid"));
		Definable* d=static_cast<Definable*>(o);
		d->define(target);
		o=target->getVariableByMultiname(name);
	}

	assert_and_throw(o->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,_("Getting definition for ") << name);
	o->incRef();
	return o;
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
		for(;it!=data.end();++it)
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
	Dictionary::setVariableByMultiname(name,abstract_i(value));
}

void Dictionary::setVariableByMultiname(const multiname& name, ASObject* o, ASObject* base)
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
	else if(name.name_type==multiname::NAME_NUMBER)
		data[abstract_d(name.name_d)]=o;
	else
	{
		throw UnsupportedException("Unsupported name kind in Dictionary::setVariableByMultiname");
	}
}

void Dictionary::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(implEnable);
	
	map<ASObject*, ASObject*>::iterator it;
	getIteratorByMultiname(name, it);
	
	assert_and_throw(it != data.end());
	
	it->second->decRef();
	data.erase(it);

	//This is ugly, but at least we are sure that we own name_o
	multiname* tmp=const_cast<multiname*>(&name);
	tmp->name_o=NULL;
}

void Dictionary::getIteratorByMultiname(const multiname& name, map<ASObject*, ASObject*>::iterator& iter)
{
	assert_and_throw(implEnable);
	//It seems that various kind of implementation works only with the empty namespace
	assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");
	if(name.name_type==multiname::NAME_OBJECT)
	{
		//From specs, then === operator compare references when working on generic objects
		map<ASObject*,ASObject*>::iterator it=data.find(name.name_o);
		if(it != data.end())
		{
			iter = it;
			//This is ugly, but at least we are sure that we own name_o
			multiname* tmp=const_cast<multiname*>(&name);
			tmp->name_o=NULL;
			return;
		}
	}
	else if(name.name_type==multiname::NAME_STRING)
	{
		//Ok, we need to do the slow lookup on every object and check for === comparison
		map<ASObject*, ASObject*>::iterator it=data.begin();
		for(;it!=data.end();++it)
		{
			if(it->first->getObjectType()==T_STRING)
			{
				ASString* s=Class<ASString>::cast(it->first);
				if(name.name_s == s->data.c_str())
				{
					//Value found
					iter = it;
					return;
				}
			}
		}
	}
	else if(name.name_type==multiname::NAME_INT)
	{
		//Ok, we need to do the slow lookup on every object and check for === comparison
		map<ASObject*, ASObject*>::iterator it=data.begin();
		for(;it!=data.end();++it)
		{
			SWFOBJECT_TYPE type=it->first->getObjectType();
			if(type==T_INTEGER || type==T_UINTEGER || type==T_NUMBER)
			{
				if(name.name_i == it->first->toNumber())
				{
					//Value found
					iter = it;
					return;
				}
			}
		}
	}
	else if(name.name_type==multiname::NAME_NUMBER)
	{
		//Ok, we need to do the slow lookup on every object and check for === comparison
		map<ASObject*, ASObject*>::iterator it=data.begin();
		for(;it!=data.end();++it)
		{
			SWFOBJECT_TYPE type=it->first->getObjectType();
			if(type==T_INTEGER || type==T_UINTEGER || type==T_NUMBER)
			{
				if(name.name_d == it->first->toNumber())
				{
					//Value found
					iter = it;
					return;
				}
			}
		}
	}
	else
	{
		throw UnsupportedException("Unsupported name kind on Dictionary::getVariableByMultiname");
	}
	iter = data.end();
}


ASObject* Dictionary::getVariableByMultiname(const multiname& name, bool skip_impl, ASObject* base)
{
	assert_and_throw(!skip_impl);
	assert_and_throw(implEnable);
	
	map<ASObject*, ASObject*>::iterator it;
	getIteratorByMultiname(name, it);
	
	if (it == data.end())
		return new Undefined;
		
	it->second->incRef();
	return it->second;
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
	assert(index>0);
	index--;
	assert_and_throw(implEnable);
	assert_and_throw(index<data.size());
	map<ASObject*,ASObject*>::iterator it=data.begin();
	for(unsigned int i=0;i<index;i++)
		++it;
	out=it->first;
	return true;
}

bool Dictionary::nextValue(unsigned int index, ASObject*& out)
{
	assert_and_throw(implEnable);
	assert(index<data.size());
	map<ASObject*,ASObject*>::iterator it=data.begin();
	for(unsigned int i=0;i<index;i++)
		++it;
	out=it->second;
	return true;
}

tiny_string Dictionary::toString(bool debugMsg)
{
	if(debugMsg)
		return ASObject::toString(debugMsg);
		
	std::stringstream retstr;
	retstr << "{";
	map<ASObject*,ASObject*>::iterator it=data.begin();
	while(it != data.end())
	{
		if(it != data.begin())
			retstr << ", ";
		retstr << "{" << it->first->toString() << ", " << it->second->toString() << "}";
		++it;
	}
	retstr << "}";
	
	return retstr.str();
}

void Proxy::sinit(Class_base* c)
{
	//c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
}

void Proxy::setVariableByMultiname(const multiname& name, ASObject* o, ASObject* base)
{
	//If a variable named like this already exist, return that
	if(hasPropertyByMultiname(name) || !implEnable)
	{
		ASObject::setVariableByMultiname(name,o,base);
		return;
	}

	//Check if there is a custom setter defined, skipping implementation to avoid recursive calls
	multiname setPropertyName;
	setPropertyName.name_type=multiname::NAME_STRING;
	setPropertyName.name_s="setProperty";
	setPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	ASObject* proxySetter=getVariableByMultiname(setPropertyName,true);

	if(proxySetter==NULL)
	{
		ASObject::setVariableByMultiname(name,o);
		return;
	}

	assert_and_throw(proxySetter->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(proxySetter);

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* args[2];
	args[0]=Class<ASString>::getInstanceS(name.name_s);
	args[1]=o;
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,_("Proxy::setProperty"));
	incRef();
	ASObject* ret=f->call(this,args,2);
	assert_and_throw(ret==NULL);
	implEnable=true;
}

ASObject* Proxy::getVariableByMultiname(const multiname& name, bool skip_impl, ASObject* base)
{
	//It seems that various kind of implementation works only with the empty namespace
	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="" || hasPropertyByMultiname(name) || !implEnable || skip_impl)
		return ASObject::getVariableByMultiname(name,skip_impl,base);

	//Check if there is a custom getter defined, skipping implementation to avoid recursive calls
	multiname getPropertyName;
	getPropertyName.name_type=multiname::NAME_STRING;
	getPropertyName.name_s="getProperty";
	getPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	ASObject* o=getVariableByMultiname(getPropertyName,true);

	if(o==NULL)
		return ASObject::getVariableByMultiname(name,skip_impl,base);

	assert_and_throw(o->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(o);

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* arg=Class<ASString>::getInstanceS(name.name_s);
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,_("Proxy::getProperty"));
	incRef();
	ASObject* ret=f->call(this,&arg,1);
	assert_and_throw(ret);
	implEnable=true;
	return ret;
}

bool Proxy::hasNext(unsigned int& index, bool& out)
{
	assert_and_throw(implEnable);
	LOG(LOG_CALLS, _("Proxy::hasNext"));
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameIndexName;
	nextNameIndexName.name_type=multiname::NAME_STRING;
	nextNameIndexName.name_s="nextNameIndex";
	nextNameIndexName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	ASObject* o=getVariableByMultiname(nextNameIndexName,true);
	assert_and_throw(o && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o);
	ASObject* arg=abstract_i(index);
	incRef();
	ASObject* ret=f->call(this,&arg,1);
	index=ret->toInt();
	ret->decRef();
	out=(index!=0);
	return true;
}

bool Proxy::nextName(unsigned int index, ASObject*& out)
{
	assert(index>0);
	assert_and_throw(implEnable);
	LOG(LOG_CALLS, _("Proxy::nextName"));
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameName;
	nextNameName.name_type=multiname::NAME_STRING;
	nextNameName.name_s="nextName";
	nextNameName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	ASObject* o=getVariableByMultiname(nextNameName,true);
	assert_and_throw(o && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o);
	ASObject* arg=abstract_i(index);
	incRef();
	out=f->call(this,&arg,1);
	return true;
}

ASFUNCTIONBODY(lightspark,setInterval)
{
	assert_and_throw(argslen >= 2);
	//incRef the function
	args[0]->incRef();

	//Build arguments array
	ASObject* callbackArgs[argslen-2];
	uint32_t i;
	for(i=0; i<argslen-2; i++)
	{
		callbackArgs[i] = args[i+2];
		//incRef all passed arguments
		args[i+2]->incRef();
	}

	//Add interval through manager
	uint32_t id = sys->intervalManager->setInterval(args[0], callbackArgs, argslen-2, new Null, args[1]->toInt());
	return abstract_i(id);
}

ASFUNCTIONBODY(lightspark,setTimeout)
{
	assert_and_throw(argslen >= 2);
	//incRef the function
	args[0]->incRef();

	//Build arguments array
	ASObject* callbackArgs[argslen-2];
	uint32_t i;
	for(i=0; i<argslen-2; i++)
	{
		callbackArgs[i] = args[i+2];
		//incRef all passed arguments
		args[i+2]->incRef();
	}

	//Add timeout through manager
	uint32_t id = sys->intervalManager->setTimeout(args[0], callbackArgs, argslen-2, new Null, args[1]->toInt());
	return abstract_i(id);
}

ASFUNCTIONBODY(lightspark,clearInterval)
{
	assert_and_throw(argslen == 1);
	sys->intervalManager->clearInterval(args[0]->toInt(), IntervalRunner::INTERVAL, true);
	return NULL;
}

ASFUNCTIONBODY(lightspark,clearTimeout)
{
	assert_and_throw(argslen == 1);
	sys->intervalManager->clearInterval(args[0]->toInt(), IntervalRunner::TIMEOUT, true);
	return NULL;
}

IntervalRunner::IntervalRunner(IntervalRunner::INTERVALTYPE _type, uint32_t _id, ASObject* _callback, ASObject** _args, const unsigned int _argslen, 
		ASObject* _obj, uint32_t _interval):
	type(_type), id(_id), callback(_callback),argslen(_argslen),obj(_obj),interval(_interval)
{
	args = new ASObject*[argslen];
	uint32_t i;
	for(i=0; i<argslen; i++)
	{
		args[i] = _args[i];
	}
}

IntervalRunner::~IntervalRunner()
{
	delete[] args;
}

void IntervalRunner::tick() 
{
	//incRef all arguments
	uint32_t i;
	for(i=0; i < argslen; i++)
	{
		args[i]->incRef();
	}
	//Incref the this object
	obj->incRef();
	FunctionEvent* event = new FunctionEvent(static_cast<IFunction*>(callback), obj, args, argslen);
	getVm()->addEvent(NULL,event);
	event->decRef();
	if(type == TIMEOUT)
	{
		//TODO: IntervalRunner deletes itself. Is this allowed?
		//Delete ourselves from the active intervals list
		sys->intervalManager->clearInterval(id, TIMEOUT, false);
		//No actions may be performed after this point
	}
}

IntervalManager::IntervalManager() : currentID(0)
{
	sem_init(&mutex, 0, 1);
}

IntervalManager::~IntervalManager()
{
	//Run through all running intervals and remove their tickjob, delete their intervalRunner and erase their entry
	std::map<uint32_t,IntervalRunner*>::iterator it = runners.begin();
	while(it != runners.end())
	{
		sys->removeJob((*it).second);
		delete((*it).second);
		runners.erase(it);
	}
	sem_destroy(&mutex);
}

uint32_t IntervalManager::setInterval(ASObject* callback, ASObject** args, const unsigned int argslen, ASObject* obj, uint32_t interval)
{
	sem_wait(&mutex);

	uint32_t id = getFreeID();
	IntervalRunner* runner = new IntervalRunner(IntervalRunner::INTERVAL, id, callback, args, argslen, obj, interval);

	//Add runner as tickjob
	sys->addTick(interval, runner);
	//Add runner to map
	runners[id] = runner;
	//Increment currentID
	currentID++;

	sem_post(&mutex);
	return currentID-1;
}
uint32_t IntervalManager::setTimeout(ASObject* callback, ASObject** args, const unsigned int argslen, ASObject* obj, uint32_t interval)
{
	sem_wait(&mutex);

	uint32_t id = getFreeID();
	IntervalRunner* runner = new IntervalRunner(IntervalRunner::TIMEOUT, id, callback, args, argslen, obj, interval);

	//Add runner as waitjob
	sys->addWait(interval, runner);
	//Add runner to map
	runners[id] = runner;
	//increment currentID
	currentID++;

	sem_post(&mutex);
	return currentID-1;
}

uint32_t IntervalManager::getFreeID()
{
	//At the first run every currentID will be available. But eventually the currentID will wrap around.
	//Thats why we need to check if the currentID isn't used yet
	while(runners.count(currentID) != 0)
		currentID++;
	return currentID;
}

void IntervalManager::clearInterval(uint32_t id, IntervalRunner::INTERVALTYPE type, bool removeJob)
{
	sem_wait(&mutex);
	
	std::map<uint32_t,IntervalRunner*>::iterator it = runners.find(id);
	//If the entry exists and the types match, remove its tickjob, delete its intervalRunner and erase their entry
	if(it != runners.end() && (*it).second->getType() == type)
	{
		if(removeJob)
		{
			sys->removeJob((*it).second);
		}
		delete (*it).second;
		runners.erase(it);
	}

	sem_post(&mutex);
}
