/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "parsing/amf3_generator.h"
#include <sstream>

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.utils");

REGISTER_CLASS_NAME(ByteArray);
REGISTER_CLASS_NAME(Timer);
REGISTER_CLASS_NAME(Dictionary);
REGISTER_CLASS_NAME(Proxy);

ByteArray::ByteArray(uint8_t* b, uint32_t l):bytes(b),len(l),position(0)
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
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_getLength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_setLength),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesAvailable","",Class<IFunction>::getFunction(_getBytesAvailable),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(_getPosition),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(_setPosition),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(_getDefaultObjectEncoding),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(_setDefaultObjectEncoding),SETTER_METHOD,false);
	sys->staticByteArrayDefaultObjectEncoding = ObjectEncoding::DEFAULT;
	c->setDeclaredMethodByQName("readBytes","",Class<IFunction>::getFunction(readBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readByte","",Class<IFunction>::getFunction(readByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readInt","",Class<IFunction>::getFunction(readInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readObject","",Class<IFunction>::getFunction(readObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUTFBytes","",Class<IFunction>::getFunction(writeUTFBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeBytes","",Class<IFunction>::getFunction(writeBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeByte","",Class<IFunction>::getFunction(writeByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeInt","",Class<IFunction>::getFunction(writeInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeObject","",Class<IFunction>::getFunction(writeObject),NORMAL_METHOD,true);
//	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(ByteArray::_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(ByteArray::_toString),NORMAL_METHOD,true);
}

void ByteArray::buildTraits(ASObject* o)
{
}

uint8_t* ByteArray::getBuffer(unsigned int size, bool enableResize)
{
	if(bytes==NULL)
	{
		len=size;
		bytes=new uint8_t[len];
	}
	else if(enableResize==false)
	{
		assert_and_throw(size<=len);
	}
	else if(len<size) // && enableResize==true
	{
		//Resize the buffer
		uint8_t* bytes2=new uint8_t[size];
		memcpy(bytes2,bytes,len);
		len=size;
		delete[] bytes;
		bytes=bytes2;
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

ASFUNCTIONBODY(ByteArray,_getDefaultObjectEncoding)
{
	return abstract_i(sys->staticNetConnectionDefaultObjectEncoding);
}

ASFUNCTIONBODY(ByteArray,_setDefaultObjectEncoding)
{
	assert_and_throw(argslen == 1);
	int32_t value = args[0]->toInt();
	if(value == 0)
		sys->staticByteArrayDefaultObjectEncoding = ObjectEncoding::AMF0;
	else if(value == 3)
		sys->staticByteArrayDefaultObjectEncoding = ObjectEncoding::AMF3;
	else
		throw RunTimeException("Invalid object encoding");
	return NULL;
}

ASFUNCTIONBODY(ByteArray,_setLength)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==1);
	uint32_t newLen=args[0]->toInt();
	if(newLen==th->len) //Nothing to do
		return NULL;
	uint8_t* newBytes=new uint8_t[newLen];
	if(th->len<newLen)
	{
		//Extend
		memcpy(newBytes,th->bytes,th->len);
		memset(newBytes+th->len,0,newLen-th->len);
	}
	else
	{
		//Truncate
		memcpy(newBytes,th->bytes,newLen);
	}
	delete[] th->bytes;
	th->bytes=newBytes;
	th->len=newLen;
	//TODO: check position
	th->position=0;
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
	assert_and_throw(argslen>=1 && argslen<=3);
	assert_and_throw(args[0]->getPrototype()==Class<ByteArray>::getClass());

	ByteArray* out=Class<ByteArray>::cast(args[0]);
	uint32_t offset=0;
	uint32_t length=0;
	if(argslen>=2)
		offset=args[1]->toInt();
	if(argslen==3)
		length=args[2]->toInt();
	//TODO: Support offset (offset is in the destination!)
	if(offset!=0 || length==0)
		throw UnsupportedException("offset in ByteArray::readBytes");

	//Error checks
	if(length > th->len)
	{
		LOG(LOG_ERROR,"ByteArray::readBytes not enough data");
		//TODO: throw AS exceptions
		return NULL;
	}
	uint8_t* buf=out->getBuffer(length,true);
	memcpy(buf,th->bytes+th->position,length);
	th->position+=length;

	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeUTFBytes)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//Validate parameters
	assert_and_throw(argslen==1);
	assert_and_throw(args[0]->getObjectType()==T_STRING);
	ASString* str=Class<ASString>::cast(args[0]);
	th->getBuffer(th->position+str->data.size()+1,true);
	memcpy(th->bytes+th->position,str->data.c_str(),str->data.size()+1);

	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeObject)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//Validate parameters
	assert_and_throw(argslen==1);
	//TODO: support AMF0
	//TODO: support custom serialization
	map<tiny_string, uint32_t> stringMap;
	map<const ASObject*, uint32_t> objMap;
	args[0]->serialize(th, stringMap, objMap);

	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeBytes)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//Validate parameters
	assert_and_throw(argslen>=1 && argslen<=3);
	assert_and_throw(args[0]->getPrototype()==Class<ByteArray>::getClass());
	ByteArray* out=Class<ByteArray>::cast(args[0]);
	uint32_t offset=0;
	uint32_t length=0;
	if(argslen>=2)
		offset=args[1]->toInt();
	if(argslen==3)
		length=args[2]->toInt();

	//If the length is 0 the whole buffer must be copied
	if(length == 0)
		length=(out->getLength()-offset);
	else if(length > (out->getLength()-offset))
	{
		LOG(LOG_ERROR,"ByteArray::writeBytes not enough data");
		//TODO: throw AS exceptions
		return NULL;
	}
	uint8_t* buf=out->getBuffer(offset+length,false);
	th->getBuffer(th->position+length,true);
	memcpy(th->bytes+th->position,buf+offset,length);
	th->position+=length;

	return NULL;
}

void ByteArray::writeByte(uint8_t b)
{
	getBuffer(position+1,true);
	bytes[position++] = b;
}

ASFUNCTIONBODY(ByteArray,writeByte)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==1);

	int32_t value=args[0]->toInt();

	th->writeByte(value&0xff);

	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeInt)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==1);

	int32_t value=args[0]->toInt();

	th->getBuffer(th->position+4,true);
	memcpy(th->bytes+th->position,&value,4);
	th->position+=4;

	return NULL;
}

bool ByteArray::readByte(uint8_t& b)
{
	if (len <= position)
		return false;

	b=bytes[position++];
	return true;
}

bool ByteArray::readU29(int32_t& ret)
{
	ret=0;
	for(uint32_t i=0;i<4;i++)
	{
		if (len <= position)
			return false;

		uint8_t tmp=bytes[position++];
		if(i<3)
		{
			ret|=((tmp&0x7f) << i*7);
			if((tmp&0x80)==0)
				break;
		}
		else
		{
			ret|=(tmp << 21);
			//Sign extend
			if(tmp&0x80)
				ret|=0xe0000000;
		}
	}
	return true;
}

ASFUNCTIONBODY(ByteArray, readByte)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	uint8_t ret;
	if(!th->readByte(ret))
	{
		LOG(LOG_ERROR,"ByteArray::readByte not enough data");
		//TODO: throw AS exceptions
		return NULL;
	}
	return abstract_i(ret);
}

ASFUNCTIONBODY(ByteArray,readInt)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	if(th->len < th->position+4)
	{
		LOG(LOG_ERROR,"ByteArray::readInt not enough data");
		//TODO: throw AS exceptions
		return NULL;
	}

	int32_t ret;
	memcpy(&ret,th->bytes+th->position,4);
	th->position+=4;

	return abstract_i(ret);
}

ASFUNCTIONBODY(ByteArray,readObject)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);
	if(th->bytes==NULL)
	{
		LOG(LOG_ERROR,"No data to read");
		return NULL;
	}
	std::vector<ASObject*> ret;
	Amf3Deserializer d(th);
	try
	{
		d.generateObjects(ret);
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Exception caught while parsing AMF3: " << e.cause);
		//TODO: throw AS exception
	}

	if(ret.size()==0)
	{
		LOG(LOG_ERROR,"No objects in the AMF3 data. Returning Undefined");
		return new Undefined;
	}
	if(ret.size()>1)
	{
		LOG(LOG_ERROR,"More than one object in the AMF3 data. Returning the first");
		for(uint32_t i=1;i<ret.size();i++)
			ret[i]->decRef();
	}
	return ret[0];
}

ASFUNCTIONBODY(ByteArray,_toString)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//TODO: check for Byte Order Mark
	return Class<ASString>::getInstanceS((char*)th->bytes,th->len);
}

bool ByteArray::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	return index<len;
}

ASObject* ByteArray::getVariableByMultiname(const multiname& name, bool skip_impl)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(skip_impl || !Array::isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,skip_impl);

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

void ByteArray::setVariableByMultiname(const multiname& name, ASObject* o)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o);

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

void ByteArray::writeU29(int32_t val)
{
	if(val>=0x40000000 || val<=(int32_t)0xbfffffff)
		throw AssertionException("Range exception in writeU29");

	for(uint32_t i=0;i<4;i++)
	{
		uint8_t b;
		if(i<3)
		{
			b=val&0x7f;
			val>>=7;
			if(val)
				b|=0x80;
		}
		else
			b=val&0xff;

		writeByte(b);
		if(val==0)
			break;
	}
}

void ByteArray::writeStringVR(map<tiny_string, uint32_t>& stringMap, const tiny_string& s)
{
	//TODO: use U29 format
	const uint32_t len=s.len();
	assert_and_throw(len<0x40);

	if(len!=0)
		assert_and_throw(stringMap.find(s)==stringMap.end());

	//A new string
	writeByte((len<<1)|1);
	
	getBuffer(position+len,true);
	memcpy(bytes+position,s.raw_buf(),len);
	position+=len;
}

void Timer::tick()
{
	//This will be executed once if repeatCount was originally 1
	//Otherwise it's executed until stopMe is set to true
	this->incRef();
	sys->currentVm->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS("timer")));

	if(repeatCount!=0)
	{
		currentCount++;
		if(repeatCount<currentCount)
		{
			this->incRef();
			sys->currentVm->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS("timerComplete")));
			stopMe=true;
			running=false;
		}
	}
}

void Timer::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<EventDispatcher>::getClass();
	c->max_level=c->super->max_level+1;
	c->setDeclaredMethodByQName("currentCount","",Class<IFunction>::getFunction(_getCurrentCount),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",Class<IFunction>::getFunction(_getRepeatCount),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",Class<IFunction>::getFunction(_setRepeatCount),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("running","",Class<IFunction>::getFunction(_getRunning),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",Class<IFunction>::getFunction(_getDelay),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",Class<IFunction>::getFunction(_setDelay),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("start","",Class<IFunction>::getFunction(start),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reset","",Class<IFunction>::getFunction(reset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(stop),NORMAL_METHOD,true);
}

ASFUNCTIONBODY(Timer,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	Timer* th=static_cast<Timer*>(obj);

	th->delay=args[0]->toInt();
	if(argslen>=2)
		th->repeatCount=args[1]->toInt();

	return NULL;
}

ASFUNCTIONBODY(Timer,_getCurrentCount)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_i(th->currentCount);
}

ASFUNCTIONBODY(Timer,_getRepeatCount)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_i(th->repeatCount);
}

ASFUNCTIONBODY(Timer,_setRepeatCount)
{
	assert_and_throw(argslen==1);
	int32_t count=args[0]->toInt();
	Timer* th=static_cast<Timer*>(obj);
	th->repeatCount=count;
	if(th->repeatCount>0 && th->repeatCount<=th->currentCount)
	{
		sys->removeJob(th);
		th->decRef();
		th->running=false;
	}
	return NULL;
}

ASFUNCTIONBODY(Timer,_getRunning)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_b(th->running);
}

ASFUNCTIONBODY(Timer,_getDelay)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_i(th->delay);
}

ASFUNCTIONBODY(Timer,_setDelay)
{
	assert_and_throw(argslen==1);
	int32_t newdelay = args[0]->toInt();
	if (newdelay<=0)
		throw Class<ASError>::getInstanceS("delay must be positive");

	Timer* th=static_cast<Timer*>(obj);
	th->delay=newdelay;

	return NULL;
}

ASFUNCTIONBODY(Timer,start)
{
	Timer* th=static_cast<Timer*>(obj);
	th->running=true;
	th->stopMe=false;
	th->incRef();
	if(th->repeatCount==1)
		sys->addWait(th->delay,th);
	else
		sys->addTick(th->delay,th);
	return NULL;
}

ASFUNCTIONBODY(Timer,reset)
{
	Timer* th=static_cast<Timer*>(obj);
	if(th->running)
	{
		//This spin waits if the timer is running right now
		sys->removeJob(th);
		//NOTE: although no new events will be sent now there might be old events in the queue.
		//Is this behaviour right?
		th->currentCount=0;
		//This is not anymore used by the timer, so it can die
		th->decRef();
		th->running=false;
	}
	return NULL;
}

ASFUNCTIONBODY(Timer,stop)
{
	Timer* th=static_cast<Timer*>(obj);
	if(th->running)
	{
		//This spin waits if the timer is running right now
		sys->removeJob(th);
		//NOTE: although no new events will be sent now there might be old events in the queue.
		//Is this behaviour right?

		//This is not anymore used by the timer, so it can die
		th->decRef();
		th->running=false;
	}
	return NULL;
}

ASFUNCTIONBODY(lightspark,getQualifiedClassName)
{
	//CHECK: what to do if ns is empty
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

ASFUNCTIONBODY(lightspark,describeType)
{
	assert_and_throw(argslen==1);

	if(args[0]->getObjectType()==T_FUNCTION)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Functions not supported in describeType");
		return new Undefined;
	}

	if(args[0]->getObjectType()==T_UNDEFINED)
		return new Undefined;

	bool isStatic=(args[0]->getObjectType()==T_CLASS);

	//TODO: add support in type for base, isDynamic, isFinal
	string ret = "<type name=\"";
	Class_base* type=NULL;
	if(isStatic)
		type=static_cast<Class_base*>(args[0]);
	else
		type=args[0]->getPrototype();

	assert_and_throw(type);
	ret+=type->class_name.getQualifiedName().raw_buf();
	ret+="\" isStatic=";
	ret+=(isStatic)?"\"true\"":"\"false\"";
	ret+=">";
	//TODO: add support for extendsClass and implementsInterface and factory
	auto it=args[0]->Variables.Variables.begin();
	for(;it!=args[0]->Variables.Variables.end();it++)
	{
		if(isStatic && it->second.kind==BORROWED_TRAIT)
			continue;

		//TODO: add support for constant, method, parameter
		if(it->second.var.getter)
		{
			//Output an accessor
			//TODO: add support in accessor for access,type,declaredBy
			ret+="<accessor name=\"";
			ret+=it->first.raw_buf();
			ret+="\"/>";
		}
		else if(it->second.var.var)
		{
			//Output a variable
			ret+="<variable name=\"";
			ret+=it->first.raw_buf();
			ret+="\"";
			if(it->second.var.type)
			{
				ret+=" type=\"";
				ret+=it->second.var.type->class_name.getQualifiedName().raw_buf();
				ret+="\"";
			}
			ret+="/>";
		}
	}
	ret+="</type>";

	return Class<XML>::getInstanceS(ret);;
}

ASFUNCTIONBODY(lightspark,getTimer)
{
	uint64_t ret=compat_msectiming() - sys->startTime;
	return abstract_i(ret);
}

void Dictionary::finalize()
{
	ASObject::finalize();
	data.clear();
}

void Dictionary::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
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

void Dictionary::setVariableByMultiname(const multiname& name, ASObject* o)
{
	assert_and_throw(implEnable);
	if(name.name_type==multiname::NAME_OBJECT)
	{
		_R<ASObject> name_o(name.name_o);
		//This is ugly, but at least we are sure that we own name_o
		multiname* tmp=const_cast<multiname*>(&name);
		tmp->name_o=NULL;

		map<_R<ASObject>, _R<ASObject> >::iterator it=data.find(name_o);
		if(it!=data.end())
			it->second=_MR(o);
		else
			data.insert(make_pair(name_o,_MR(o)));
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_NUMBER);
		ASObject::setVariableByMultiname(name, o);
	}
}

void Dictionary::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(implEnable);
	
	if(name.name_type==multiname::NAME_OBJECT)
	{
		_R<ASObject> name_o(name.name_o);
		//This is ugly, but at least we are sure that we own name_o
		multiname* tmp=const_cast<multiname*>(&name);
		tmp->name_o=NULL;

		map<_R<ASObject>, _R<ASObject> >::iterator it=data.find(name_o);
		if(it != data.end())
			data.erase(it);
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_NUMBER);
		return ASObject::deleteVariableByMultiname(name);
	}
}

ASObject* Dictionary::getVariableByMultiname(const multiname& name, bool skip_impl)
{
	if(!skip_impl && implEnable)
	{
		if(name.name_type==multiname::NAME_OBJECT)
		{
			_R<ASObject> name_o(name.name_o);

			map<_R<ASObject>, _R<ASObject> >::iterator it=data.find(name_o);
			if(it != data.end())
			{
				//This is ugly, but at least we are sure that we own name_o
				multiname* tmp=const_cast<multiname*>(&name);
				tmp->name_o=NULL;
				it->second->incRef();
				return it->second.getPtr();
			}
			else
			{
				//Make sure name_o gets not destroyed, it's still owned by name
				name_o->incRef();
				return NULL;
			}
		}
		else
		{
			//Primitive types _must_ be handled by the normal ASObject path
			//REFERENCE: Dictionary Object on AS3 reference
			assert(name.name_type==multiname::NAME_STRING ||
				name.name_type==multiname::NAME_INT ||
				name.name_type==multiname::NAME_NUMBER);
			return ASObject::getVariableByMultiname(name, skip_impl);
		}
	}
	//Try with the base implementation
	return ASObject::getVariableByMultiname(name, skip_impl);
}

bool Dictionary::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	if(name.name_type==multiname::NAME_OBJECT)
	{
		_R<ASObject> name_o(name.name_o);

		map<_R<ASObject>, _R<ASObject> >::iterator it=data.find(name_o);
		return it != data.end();
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_NUMBER);
		return ASObject::hasPropertyByMultiname(name, considerDynamic);
	}
}

uint32_t Dictionary::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<data.size())
		return cur_index+1;
	else
	{
		//Fall back on object properties
		uint32_t ret=ASObject::nextNameIndex(cur_index-data.size());
		if(ret==0)
			return 0;
		else
			return ret+data.size();

	}
}

_R<ASObject> Dictionary::nextName(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		map<_R<ASObject>,_R<ASObject> >::iterator it=data.begin();
		for(unsigned int i=1;i<index;i++)
			++it;

		return it->first;
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextName(index-data.size());
	}
}

_R<ASObject> Dictionary::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		map<_R<ASObject>,_R<ASObject> >::iterator it=data.begin();
		for(unsigned int i=1;i<index;i++)
			++it;

		return it->second;
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextValue(index-data.size());
	}
}

tiny_string Dictionary::toString(bool debugMsg)
{
	if(debugMsg)
		return ASObject::toString(debugMsg);
		
	std::stringstream retstr;
	retstr << "{";
	map<_R<ASObject>,_R<ASObject> >::iterator it=data.begin();
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

void Proxy::buildTraits(ASObject* o)
{
}

void Proxy::setVariableByMultiname(const multiname& name, ASObject* o)
{
	//If a variable named like this already exist, use that
	if(ASObject::hasPropertyByMultiname(name, true) || !implEnable)
	{
		ASObject::setVariableByMultiname(name,o);
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

ASObject* Proxy::getVariableByMultiname(const multiname& name, bool skip_impl)
{
	//It seems that various kind of implementation works only with the empty namespace
	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="" || ASObject::hasPropertyByMultiname(name, true) || !implEnable || skip_impl)
		return ASObject::getVariableByMultiname(name,skip_impl);

	//Check if there is a custom getter defined, skipping implementation to avoid recursive calls
	multiname getPropertyName;
	getPropertyName.name_type=multiname::NAME_STRING;
	getPropertyName.name_s="getProperty";
	getPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	ASObject* o=getVariableByMultiname(getPropertyName,true);

	if(o==NULL)
		return ASObject::getVariableByMultiname(name,skip_impl);

	assert_and_throw(o->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(o);

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* arg=Class<ASString>::getInstanceS(name.name_s);
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,"Proxy::getProperty");
	incRef();
	ASObject* ret=f->call(this,&arg,1);
	implEnable=true;
	return ret;
}

bool Proxy::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	throw UnsupportedException("Proxy::hasProperty");
}

uint32_t Proxy::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	LOG(LOG_CALLS,"Proxy::nextNameIndex");
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameIndexName;
	nextNameIndexName.name_type=multiname::NAME_STRING;
	nextNameIndexName.name_s="nextNameIndex";
	nextNameIndexName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	ASObject* o=getVariableByMultiname(nextNameIndexName,true);
	assert_and_throw(o && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o);
	ASObject* arg=abstract_i(cur_index);
	this->incRef();
	ASObject* ret=f->call(this,&arg,1);
	uint32_t newIndex=ret->toInt();
	ret->decRef();
	return newIndex;
}

_R<ASObject> Proxy::nextName(uint32_t index)
{
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
	return _MR(f->call(this,&arg,1));
}

_R<ASObject> Proxy::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	LOG(LOG_CALLS, _("Proxy::nextValue"));
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextValueName;
	nextValueName.name_type=multiname::NAME_STRING;
	nextValueName.name_s="nextValue";
	nextValueName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	ASObject* o=getVariableByMultiname(nextValueName,true);
	assert_and_throw(o && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o);
	ASObject* arg=abstract_i(index);
	incRef();
	return _MR(f->call(this,&arg,1));
}

ASFUNCTIONBODY(lightspark,setInterval)
{
	assert_and_throw(argslen >= 2 && args[0]->getObjectType()==T_FUNCTION);

	//Build arguments array
	ASObject* callbackArgs[argslen-2];
	uint32_t i;
	for(i=0; i<argslen-2; i++)
	{
		callbackArgs[i] = args[i+2];
		//incRef all passed arguments
		args[i+2]->incRef();
	}

	//incRef the function
	args[0]->incRef();
	IFunction* callback=static_cast<IFunction*>(args[0]);
	//Add interval through manager
	uint32_t id = sys->intervalManager->setInterval(_MR(callback), callbackArgs, argslen-2, _MR(new Null), args[1]->toInt());
	return abstract_i(id);
}

ASFUNCTIONBODY(lightspark,setTimeout)
{
	assert_and_throw(argslen >= 2);

	//Build arguments array
	ASObject* callbackArgs[argslen-2];
	uint32_t i;
	for(i=0; i<argslen-2; i++)
	{
		callbackArgs[i] = args[i+2];
		//incRef all passed arguments
		args[i+2]->incRef();
	}

	//incRef the function
	args[0]->incRef();
	IFunction* callback=static_cast<IFunction*>(args[0]);
	//Add timeout through manager
	uint32_t id = sys->intervalManager->setTimeout(_MR(callback), callbackArgs, argslen-2, _MR(new Null), args[1]->toInt());
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

IntervalRunner::IntervalRunner(IntervalRunner::INTERVALTYPE _type, uint32_t _id, _R<IFunction> _callback, ASObject** _args,
		const unsigned int _argslen, _R<ASObject> _obj, uint32_t _interval):
	type(_type), id(_id), callback(_callback),argslen(_argslen),obj(_obj),interval(_interval)
{
	args = new ASObject*[argslen];
	for(uint32_t i=0; i<argslen; i++)
		args[i] = _args[i];
}

IntervalRunner::~IntervalRunner()
{
	for(uint32_t i=0; i<argslen; i++)
		args[i]->decRef();
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
	_R<FunctionEvent> event(new FunctionEvent(callback, obj, args, argslen));
	getVm()->addEvent(NullRef,event);
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
		runners.erase(it++);
	}
	sem_destroy(&mutex);
}

uint32_t IntervalManager::setInterval(_R<IFunction> callback, ASObject** args, const unsigned int argslen, _R<ASObject> obj, uint32_t interval)
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
uint32_t IntervalManager::setTimeout(_R<IFunction> callback, ASObject** args, const unsigned int argslen, _R<ASObject> obj, uint32_t interval)
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
