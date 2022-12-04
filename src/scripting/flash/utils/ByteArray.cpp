/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/abc.h"
#include "scripting/flash/utils/ByteArray.h"
#include "asobject.h"
#include "scripting/class.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/errors/flasherrors.h"
#include <sstream>
#include <zlib.h>
#include <glib.h>

using namespace std;
using namespace lightspark;

#define BA_CHUNK_SIZE 4096
// the flash documentation doesn't tell how large ByteArrays are allowed to be
// so we simply don't allow bytearrays larger than 1GiB
// maybe we should set this smaller
#define BA_MAX_SIZE 0x40000000

ByteArray::ByteArray(ASWorker* wrk, Class_base* c, uint8_t* b, uint32_t l):ASObject(wrk,c,T_OBJECT,SUBTYPE_BYTEARRAY),littleEndian(false),objectEncoding(OBJECT_ENCODING::AMF3),currentObjectEncoding(OBJECT_ENCODING::AMF3),
	position(0),bytes(b),real_len(l),len(l),shareable(false)
{
#ifdef MEMORY_USAGE_PROFILING
	c->memoryAccount->addBytes(l);
#endif
}

ByteArray::~ByteArray()
{
	if(bytes)
	{
#ifdef MEMORY_USAGE_PROFILING
		getClass()->memoryAccount->removeBytes(real_len);
#endif
		delete[] bytes;
		bytes = nullptr;
	}
}

bool ByteArray::destruct()
{
	if(bytes)
	{
#ifdef MEMORY_USAGE_PROFILING
		getClass()->memoryAccount->removeBytes(real_len);
#endif
		delete[] bytes;
		bytes = nullptr;
	}
	currentObjectEncoding = OBJECT_ENCODING::AMF3;
	position = 0;
	real_len = 0;
	len = 0;
	shareable = false;
	littleEndian = false;
	return ASObject::destruct();
}

void ByteArray::finalize()
{
	if(bytes)
	{
#ifdef MEMORY_USAGE_PROFILING
		getClass()->memoryAccount->removeBytes(real_len);
#endif
		delete[] bytes;
		bytes = nullptr;
	}
}

void ByteArray::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_getLength,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_setLength),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesAvailable","",Class<IFunction>::getFunction(c->getSystemState(),_getBytesAvailable,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(c->getSystemState(),_getPosition,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(c->getSystemState(),_setPosition),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(c->getSystemState(),_getEndian,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(c->getSystemState(),_setEndian),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_getObjectEncoding,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_setObjectEncoding),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_getDefaultObjectEncoding,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(c->getSystemState(),_setDefaultObjectEncoding),SETTER_METHOD,false);

	c->getSystemState()->staticByteArrayDefaultObjectEncoding = OBJECT_ENCODING::DEFAULT;
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(c->getSystemState(),clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("compress","",Class<IFunction>::getFunction(c->getSystemState(),_compress),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uncompress","",Class<IFunction>::getFunction(c->getSystemState(),_uncompress),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("deflate","",Class<IFunction>::getFunction(c->getSystemState(),_deflate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflate","",Class<IFunction>::getFunction(c->getSystemState(),_inflate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readBoolean","",Class<IFunction>::getFunction(c->getSystemState(),readBoolean,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readBytes","",Class<IFunction>::getFunction(c->getSystemState(),readBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readByte","",Class<IFunction>::getFunction(c->getSystemState(),readByte,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readDouble","",Class<IFunction>::getFunction(c->getSystemState(),readDouble,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readFloat","",Class<IFunction>::getFunction(c->getSystemState(),readFloat,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readInt","",Class<IFunction>::getFunction(c->getSystemState(),readInt,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readMultiByte","",Class<IFunction>::getFunction(c->getSystemState(),readMultiByte,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readShort","",Class<IFunction>::getFunction(c->getSystemState(),readShort,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedByte","",Class<IFunction>::getFunction(c->getSystemState(),readUnsignedByte,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedInt","",Class<IFunction>::getFunction(c->getSystemState(),readUnsignedInt,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedShort","",Class<IFunction>::getFunction(c->getSystemState(),readUnsignedShort,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readObject","",Class<IFunction>::getFunction(c->getSystemState(),readObject,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTF","",Class<IFunction>::getFunction(c->getSystemState(),readUTF,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTFBytes","",Class<IFunction>::getFunction(c->getSystemState(),readUTFBytes,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeBoolean","",Class<IFunction>::getFunction(c->getSystemState(),writeBoolean),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUTF","",Class<IFunction>::getFunction(c->getSystemState(),writeUTF),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUTFBytes","",Class<IFunction>::getFunction(c->getSystemState(),writeUTFBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeBytes","",Class<IFunction>::getFunction(c->getSystemState(),writeBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeByte","",Class<IFunction>::getFunction(c->getSystemState(),writeByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeDouble","",Class<IFunction>::getFunction(c->getSystemState(),writeDouble),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeFloat","",Class<IFunction>::getFunction(c->getSystemState(),writeFloat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeInt","",Class<IFunction>::getFunction(c->getSystemState(),writeInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeMultiByte","",Class<IFunction>::getFunction(c->getSystemState(),writeMultiByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUnsignedInt","",Class<IFunction>::getFunction(c->getSystemState(),writeUnsignedInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeObject","",Class<IFunction>::getFunction(c->getSystemState(),writeObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeShort","",Class<IFunction>::getFunction(c->getSystemState(),writeShort),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),ByteArray::_toString),DYNAMIC_TRAIT);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,shareable,Boolean);
	c->setDeclaredMethodByQName("atomicCompareAndSwapIntAt","",Class<IFunction>::getFunction(c->getSystemState(),atomicCompareAndSwapIntAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("atomicCompareAndSwapLength","",Class<IFunction>::getFunction(c->getSystemState(),atomicCompareAndSwapLength),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toJSON",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toJSON),DYNAMIC_TRAIT);

	c->addImplementedInterface(InterfaceClass<IDataInput>::getClass(c->getSystemState()));
	IDataInput::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IDataOutput>::getClass(c->getSystemState()));
	IDataOutput::linkTraits(c);
}

uint8_t* ByteArray::getBufferIntern(unsigned int size, bool enableResize)
{
	if (size > BA_MAX_SIZE) 
	{
		createError<ASError>(getInstanceWorker(), kOutOfMemoryError);
		return nullptr;
	}
	// The first allocation is exactly the size we need,
	// the subsequent reallocations happen in increments of BA_CHUNK_SIZE bytes
	uint32_t prevLen = len;
	if(bytes==nullptr)
	{
		len=size;
		real_len=len;
		bytes = new uint8_t[len];
		memset(bytes,0,len);
#ifdef MEMORY_USAGE_PROFILING
		getClass()->memoryAccount->addBytes(len);
#endif
	}
	else if(enableResize==false)
	{
		assert_and_throw(size<=len);
	}
	else if(real_len<size) // && enableResize==true
	{
#ifdef MEMORY_USAGE_PROFILING
		uint32_t prev_real_len = real_len;
#endif
		while(real_len < size)
			real_len += BA_CHUNK_SIZE;
		// Reallocate the buffer, in chunks of BA_CHUNK_SIZE bytes
		uint8_t* bytes2 = new uint8_t[real_len];
		assert_and_throw(bytes2);
		memcpy(bytes2,bytes,prevLen);
		delete[] bytes;
#ifdef MEMORY_USAGE_PROFILING
		getClass()->memoryAccount->addBytes(real_len-prev_real_len);
#endif
		bytes = bytes2;
		if(prevLen<size)
			memset(bytes+prevLen,0,real_len-prevLen);
		len=size;
		bytes=bytes2;
	}
	else if(len<size)
	{
		len=size;
	}
	return bytes;
}

ASFUNCTIONBODY_ATOM(ByteArray,_constructor)
{
}

ASFUNCTIONBODY_ATOM(ByteArray,_getPosition)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	asAtomHandler::setUInt(ret,wrk,th->getPosition());
}

ASFUNCTIONBODY_ATOM(ByteArray,_setPosition)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	uint32_t pos=asAtomHandler::toUInt(args[0]);
	th->setPosition(pos);
}

ASFUNCTIONBODY_ATOM(ByteArray,_getEndian)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	if(th->littleEndian)
		ret = asAtomHandler::fromString(wrk->getSystemState(),Endian::littleEndian);
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),Endian::bigEndian);
}

ASFUNCTIONBODY_ATOM(ByteArray,_setEndian)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	if(asAtomHandler::toString(args[0],wrk) == Endian::littleEndian)
		th->littleEndian = true;
	else if(asAtomHandler::toString(args[0],wrk) == Endian::bigEndian)
		th->littleEndian = false;
	else
		createError<ArgumentError>(wrk,kInvalidEnumError, "endian");
}

ASFUNCTIONBODY_ATOM(ByteArray,_getObjectEncoding)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	asAtomHandler::setUInt(ret,wrk,th->objectEncoding);
}

ASFUNCTIONBODY_ATOM(ByteArray,_setObjectEncoding)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	uint32_t value;
	ARG_CHECK(ARG_UNPACK(value));
	if(value!=OBJECT_ENCODING::AMF0 && value!=OBJECT_ENCODING::AMF3)
	{
		createError<ArgumentError>(wrk,kInvalidEnumError, "objectEncoding");
		return;
	}

	th->objectEncoding=value;
	th->currentObjectEncoding=value;
}

ASFUNCTIONBODY_ATOM(ByteArray,_getDefaultObjectEncoding)
{
	asAtomHandler::setUInt(ret,wrk,wrk->getSystemState()->staticNetConnectionDefaultObjectEncoding);
}

ASFUNCTIONBODY_ATOM(ByteArray,_setDefaultObjectEncoding)
{
	assert_and_throw(argslen == 1);
	int32_t value = asAtomHandler::toInt(args[0]);
	if(value == 0)
		wrk->getSystemState()->staticByteArrayDefaultObjectEncoding = OBJECT_ENCODING::AMF0;
	else if(value == 3)
		wrk->getSystemState()->staticByteArrayDefaultObjectEncoding = OBJECT_ENCODING::AMF3;
	else
		throw RunTimeException("Invalid object encoding");
}

ASFUNCTIONBODY_ATOM(ByteArray,_setLength)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==1);

	uint32_t newLen=asAtomHandler::toInt(args[0]);
	th->lock();
	if(newLen==th->len) //Nothing to do
		return;
	th->setLength(newLen);
	th->unlock();
}
void ByteArray::setLength(uint32_t newLen)
{
	if (newLen > 0)
	{
		getBuffer(newLen,true);
	}
	else
	{
		if (bytes)
		{
#ifdef MEMORY_USAGE_PROFILING
			getClass()->memoryAccount->removeBytes(real_len);
#endif
			delete[] bytes;
		}
		bytes = nullptr;
		real_len = newLen;
	}
	len = newLen;
	if (position > len)
		position = (len > 0 ? len-1 : 0);
}
ASFUNCTIONBODY_ATOM(ByteArray,_getLength)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	asAtomHandler::setUInt(ret,wrk,th->len);
}

ASFUNCTIONBODY_ATOM(ByteArray,_getBytesAvailable)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	asAtomHandler::setUInt(ret,wrk,th->len-th->position);
}

ASFUNCTIONBODY_ATOM(ByteArray,readBoolean)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);

	th->lock();
	uint8_t res;
	if(!th->readByte(res))
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}

	th->unlock();
	asAtomHandler::setBool(ret,res!=0);
}

ASFUNCTIONBODY_ATOM(ByteArray,readBytes)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	_NR<ByteArray> out;
	uint32_t offset;
	uint32_t length;
	ARG_CHECK(ARG_UNPACK(out)(offset, 0)(length, 0));
	
	th->lock();
	if(length == 0)
	{
		assert(th->len >= th->position);
		length = th->len - th->position;
	}

	//Error checks
	if(th->position+length > th->len)
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	if((uint64_t)length+offset > 0xFFFFFFFF)
	{
		th->unlock();
		createError<RangeError>(wrk,0,"length+offset");
		return;
	}
	
	uint8_t* buf=out->getBuffer(length+offset,true);
	memcpy(buf+offset,th->bytes+th->position,length);
	th->position+=length;
	th->unlock();
}

bool ByteArray::readUTF(tiny_string& ret)
{
	uint16_t stringLen;
	if(!readShort(stringLen))
		return false;
	if(len < (position+stringLen))
		return false;
	// check for BOM
	if (len > position+3)
	{
		if (bytes[position] == 0xef &&
			bytes[position+1] == 0xbb &&
			bytes[position+2] == 0xbf)
		{
			position += 3;
			stringLen -= stringLen > 3 ? 3 : 0;
		}
	}
	char buf[stringLen+1];
	buf[stringLen]=0;
	strncpy(buf,(char*)bytes+position,(size_t)stringLen);
	ret=buf;
	position+=stringLen;
	return true;
}

ASFUNCTIONBODY_ATOM(ByteArray,readUTF)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);

	tiny_string res;
	th->lock();
	if (!th->readUTF(res))
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	th->unlock();
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}

ASFUNCTIONBODY_ATOM(ByteArray,readUTFBytes)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	uint32_t length;

	ARG_CHECK(ARG_UNPACK (length));
	th->lock();
	if(th->position+length > th->len)
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	tiny_string res;
	th->readUTFBytes(length,res);
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}

bool ByteArray::readUTFBytes(uint32_t length,tiny_string& ret)
{
	// check for BOM
	if (this->len > position+3)
	{
		if (bytes[position] == 0xef &&
			bytes[position+1] == 0xbb &&
			bytes[position+2] == 0xbf)
		{
			position += 3;
			length -=  length > 3 ? 3 : 0;
		}
	}
	uint8_t *bufStart=bytes+position;
	char* buf = new char[length+1];
	buf[length]=0;
	strncpy(buf,(char*)bufStart,(size_t)length);
	position+=length;
	ret = buf;
	ret.checkValidUTF();
	delete[] buf;
	return true;
}
bool ByteArray::readBytes(uint32_t offset, uint32_t length,uint8_t* ret)
{
	assert_and_throw(offset+length <= this->len);
	uint8_t *bufStart=bytes+offset;
	memcpy(ret,bufStart,(size_t)length);
	return true;
}

asAtom ByteArray::readObject()
{
	asAtom ret = asAtomHandler::nullAtom;
	Amf3Deserializer d(this);
	try
	{
		ret=d.readObject();
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Exception caught while parsing AMF3: " << e.cause);
		//TODO: throw AS exception
	}
	return ret;
}

ASObject* ByteArray::readSharedObject()
{
	ASObject* ret = Class<ASObject>::getInstanceS(getInstanceWorker());
	if (getLength()<=16)
		return ret;
	Amf3Deserializer d(this);
	try
	{
		d.readSharedObject(ret);
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Exception caught while parsing shared object: " << e.cause);
	}
	return ret;
}

void ByteArray::writeUTF(const tiny_string& str)
{
	getBuffer(position+str.numBytes()+2,true);
	if(str.numBytes() > 65535)
	{
		createError<RangeError>(getInstanceWorker(),kParamRangeError);
		return;
	}
	uint16_t numBytes=endianIn((uint16_t)str.numBytes());
	memcpy(bytes+position,&numBytes,2);
	memcpy(bytes+position+2,str.raw_buf(),str.numBytes());
	position+=str.numBytes()+2;
}

ASFUNCTIONBODY_ATOM(ByteArray,writeUTF)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	//Validate parameters
	assert_and_throw(argslen==1);
	assert_and_throw(asAtomHandler::isString(args[0]));
	th->lock();
	th->writeUTF(asAtomHandler::toString(args[0],wrk));
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,writeUTFBytes)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	//Validate parameters
	assert_and_throw(argslen==1);
	assert_and_throw(asAtomHandler::isString(args[0]));
	tiny_string str=asAtomHandler::toString(args[0],wrk);
	th->lock();
	th->getBuffer(th->position+str.numBytes(),true);
	memcpy(th->bytes+th->position,str.raw_buf(),str.numBytes());
	th->position+=str.numBytes();
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,writeMultiByte)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	tiny_string value;
	tiny_string charset;
	ARG_CHECK(ARG_UNPACK(value)(charset));

	// TODO: should convert from UTF-8 to charset
	LOG(LOG_NOT_IMPLEMENTED, "ByteArray.writeMultiByte doesn't convert charset");

	th->lock();
	th->getBuffer(th->position+value.numBytes(),true);
	memcpy(th->bytes+th->position,value.raw_buf(),value.numBytes());
	th->position+=value.numBytes();
	th->unlock();
}

uint32_t ByteArray::writeObject(ASObject* obj, ASWorker* wrk)
{
	//Return the length of the serialized object

	//TODO: support custom serialization
	map<tiny_string, uint32_t> stringMap;
	map<const ASObject*, uint32_t> objMap;
	map<const Class_base*, uint32_t> traitsMap;
	uint32_t oldPosition=position;
	obj->serialize(this, stringMap, objMap,traitsMap,wrk);
	return position-oldPosition;
}
uint32_t ByteArray::writeAtomObject(asAtom obj, ASWorker* wrk)
{
	//Return the length of the serialized object

	//TODO: support custom serialization
	map<tiny_string, uint32_t> stringMap;
	map<const ASObject*, uint32_t> objMap;
	map<const Class_base*, uint32_t> traitsMap;
	uint32_t oldPosition=position;
	asAtomHandler::serialize(this,stringMap,objMap,traitsMap,wrk,obj);
	return position-oldPosition;
}
void ByteArray::writeSharedObject(ASObject* obj, const tiny_string& name, ASWorker* wrk)
{
	// write amf header
	writeByte(0x00);
	writeByte(0xbf);
	uint32_t sizepos = getPosition();
	writeUnsignedInt(0); // size of file, will be filled at end of method
	writeByte('T');
	writeByte('C');
	writeByte('S');
	writeByte('O');
	writeByte(0x00);
	writeByte(0x04);
	writeByte(0x00);
	writeByte(0x00);
	writeByte(0x00);
	writeByte(0x00);
	writeUTF(name);
	writeByte(0x00);
	writeByte(0x00);
	writeByte(0x00);
	writeByte(0x03);// always store as AMF3

	map<tiny_string, uint32_t> stringMap;
	map<const ASObject*, uint32_t> objMap;
	map<const Class_base*, uint32_t> traitsMap;
	obj->serializeDynamicProperties(this, stringMap, objMap,traitsMap,wrk,true,true);
	setPosition(sizepos);
	writeUnsignedInt(GUINT32_TO_BE(getLength()-6));
	setPosition(0);
}

ASFUNCTIONBODY_ATOM(ByteArray,writeObject)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	//Validate parameters
	assert_and_throw(argslen==1);
	th->lock();
	th->writeAtomObject(args[0],wrk);
	th->unlock();
}

void ByteArray::writeShort(uint16_t val)
{
	int16_t value2 = endianIn(val);
	getBuffer(position+2,true);
	memcpy(bytes+position,&value2,2);
	position+=2;
}

ASFUNCTIONBODY_ATOM(ByteArray,writeShort)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	int32_t value;
	ARG_CHECK(ARG_UNPACK(value));

	th->lock();
	th->writeShort((static_cast<uint16_t>(value & 0xffff)));
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,writeBytes)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	//Validate parameters
	assert_and_throw(argslen>=1 && argslen<=3);
	assert_and_throw(asAtomHandler::is<ByteArray>(args[0]));
	ByteArray* out=asAtomHandler::as<ByteArray>(args[0]);
	uint32_t offset=0;
	uint32_t length=0;
	if(argslen>=2)
		offset=asAtomHandler::toUInt(args[1]);
	if(argslen==3)
		length=asAtomHandler::toUInt(args[2]);

	// We need to clamp offset to the beginning of the bytes array
	if(offset > out->getLength()-1)
		offset = 0;
	// We need to clamp length to the end of the bytes array
	if(length > out->getLength()-offset)
		length = 0;

	//If the length is 0 the whole buffer must be copied
	if(length == 0)
		length=(out->getLength()-offset);
	uint8_t* buf=out->getBuffer(offset+length,false);
	th->lock();
	th->getBuffer(th->position+length,true);
	memcpy(th->bytes+th->position,buf+offset,length);
	th->position+=length;
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,writeByte)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==1);

	int32_t value=asAtomHandler::toInt(args[0]);

	th->lock();
	th->writeByte(value&0xff);
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,writeBoolean)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	bool b;
	ARG_CHECK(ARG_UNPACK (b));

	th->lock();
	if (b)
		th->writeByte(1);
	else
		th->writeByte(0);
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,writeDouble)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==1);

	double value = asAtomHandler::toNumber(args[0]);
	uint64_t *intptr=reinterpret_cast<uint64_t*>(&value);
	uint64_t value2=th->endianIn(*intptr);

	th->lock();
	th->getBuffer(th->position+8,true);
	memcpy(th->bytes+th->position,&value2,8);
	th->position+=8;
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,writeFloat)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==1);

	float value = asAtomHandler::toNumber(args[0]);
	uint32_t *intptr=reinterpret_cast<uint32_t*>(&value);
	uint32_t value2=th->endianIn(*intptr);

	th->lock();
	th->getBuffer(th->position+4,true);
	memcpy(th->bytes+th->position,&value2,4);
	th->position+=4;
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,writeInt)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==1);

	uint32_t value=th->endianIn(static_cast<uint32_t>(asAtomHandler::toInt(args[0])));

	th->lock();
	th->getBuffer(th->position+4,true);
	memcpy(th->bytes+th->position,&value,4);
	th->position+=4;
	th->unlock();
}

void ByteArray::writeUnsignedInt(uint32_t val)
{
	getBuffer(position+4,true);
	memcpy(bytes+position,&val,4);
	position+=4;
}

ASFUNCTIONBODY_ATOM(ByteArray,writeUnsignedInt)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==1);

	th->lock();
	uint32_t value=th->endianIn(asAtomHandler::toUInt(args[0]));
	th->writeUnsignedInt(value);
	th->unlock();
}

bool ByteArray::peekByte(uint8_t& b)
{
	if (len <= position+1)
		return false;

	b=bytes[position+1];
	return true;
}


bool ByteArray::readU29(uint32_t& ret)
{
	//Be careful! This is different from u32 parsing.
	//Here the most significant bits appears before in the stream!
	ret=0;
	for(uint32_t i=0;i<4;i++)
	{
		if (len <= position)
			return false;

		uint8_t tmp=bytes[position++];
		ret <<= 7;
		if(i<3)
		{
			ret |= tmp&0x7f;
			if((tmp&0x80)==0)
				break;
		}
		else
		{
			ret <<=1;
			ret |= tmp;
//			//Sign extend
//			if(tmp&0x80)
//				ret|=0xe0000000;
		}
	}
	return true;
}

ASFUNCTIONBODY_ATOM(ByteArray, readByte)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==0);

	th->lock();
	uint8_t res;
	if(!th->readByte(res))
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	th->unlock();
	asAtomHandler::setInt(ret,wrk,(int8_t)res);
}

ASFUNCTIONBODY_ATOM(ByteArray,readDouble)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==0);

	th->lock();
	number_t res;
	if(!th->readDouble(res,th->position))
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	th->position+=8;

	th->unlock();
	asAtomHandler::setNumber(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(ByteArray,readFloat)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==0);

	th->lock();
	float res;
	if(!th->readFloat(res,th->position))
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	th->position+=4;

	th->unlock();
	asAtomHandler::setNumber(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(ByteArray,readInt)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==0);

	th->lock();
	if(th->len < th->position+4)
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}

	uint32_t res;
	memcpy(&res,th->bytes+th->position,4);
	th->position+=4;
	th->unlock();
	asAtomHandler::setInt(ret,wrk,(int32_t)th->endianOut(res));
}

bool ByteArray::readShort(uint16_t& ret)
{
	if (len < position+2)
		return false;

	uint16_t tmp;
	memcpy(&tmp,bytes+position,2);
	ret=endianOut(tmp);
	position+=2;
	return true;
}

ASFUNCTIONBODY_ATOM(ByteArray,readShort)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==0);

	uint16_t res;
	th->lock();
	if(!th->readShort(res))
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}

	th->unlock();
	asAtomHandler::setInt(ret,wrk,(int16_t)res);
}
ASFUNCTIONBODY_ATOM(ByteArray,readUnsignedByte)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==0);
	uint8_t res;
	th->lock();
	if (!th->readByte(res))
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	th->unlock();
	asAtomHandler::setUInt(ret,wrk,(uint32_t)res);
}

bool ByteArray::readUnsignedInt(uint32_t& ret)
{
	if(len < position+4)
		return false;

	uint32_t tmp;
	memcpy(&tmp,bytes+position,4);
	ret=endianOut(tmp);
	position+=4;
	return true;
}

ASFUNCTIONBODY_ATOM(ByteArray,readUnsignedInt)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==0);

	uint32_t res;
	th->lock();
	if(!th->readUnsignedInt(res))
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	th->unlock();
	asAtomHandler::setUInt(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(ByteArray,readUnsignedShort)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==0);

	uint16_t res;
	th->lock();
	if(!th->readShort(res))
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}

	th->unlock();
	asAtomHandler::setUInt(ret,wrk,(uint32_t)res);
}

ASFUNCTIONBODY_ATOM(ByteArray,readMultiByte)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	uint32_t strlen;
	tiny_string charset;
	ARG_CHECK(ARG_UNPACK(strlen)(charset));

	th->lock();
	if(th->len < th->position+strlen)
	{
		th->unlock();
		createError<EOFError>(wrk,kEOFError);
		return;
	}

	if (charset != "us-ascii" && charset != "utf-8")
		LOG(LOG_NOT_IMPLEMENTED, "ByteArray.readMultiByte doesn't convert charset "<<charset);
	char* s = g_newa(char,strlen+1);
	// ensure that the resulting string cuts off any zeros at the end
	strncpy(s,(const char*)th->bytes+th->position,strlen);
	s[strlen] = 0x0;
	tiny_string res(s,true);
	th->unlock();
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}

ASFUNCTIONBODY_ATOM(ByteArray,readObject)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	assert_and_throw(argslen==0);
	th->lock();
	if(th->bytes==nullptr)
	{
		th->unlock();
		// it seems that contrary to the specs Adobe returns Undefined when reading from an empty ByteArray
		asAtomHandler::setUndefined(ret);
		return;
		//createError<EOFError>(wrk,kEOFError);
	}
	ret = th->readObject();
	th->unlock();

	if(asAtomHandler::isInvalid(ret))
	{
		LOG(LOG_ERROR,"No objects in the AMF3 data. Returning Undefined");
		asAtomHandler::setUndefined(ret);
		return;
	}
}
// replicate adobe behaviour where invalid UTF8 chars are treated as single chars
// this is very expensive for big strings, so always do validateUtf8 first to check if parseUtf8 is neccessary
tiny_string parseUtf8(uint8_t* bytes, int32_t len)
{
	tiny_string res;
	uint8_t* p = bytes;
	while (len > 0)
	{
		switch ((*p)&0xc0)
		{
			case 0x00: // ascii
			case 0x40: // ascii
			case 0x80: // invalid, is added as byte
				res += (uint32_t)(uint8_t)(*p++);
				--len;
				break;
			case 0xc0: // > 1 byte UTF8
			{
				switch ((*p)&0x30)
				{
					case 0x00:// 2 byte UTF8
					case 0x10:// 2 byte UTF8
						if (len < 2 // truncated
							|| (((*(p+1))&0xC0) != 0x80))  // invalid 2nd char
						{
							res += (uint32_t)(uint8_t)(*p++);
							--len;
						}
						else
						{
							uint32_t c = (uint32_t)(uint8_t)(*p);
							res += (((c<<6) & 0x7C0) | ((*(p+1)) & 0x3F));
							p+=2;
							len-=2;
						}
						break;
					case 0x20: // 3 byte UTF8
						if (len < 3  // truncated
							|| (((*(p+1))&0xC0) != 0x80)  // invalid 2nd char
							|| (((*(p+2))&0xC0) != 0x80)) // invalid 3rd char
						{
							res += (uint32_t)(uint8_t)(*p++);
							--len;
						}
						else
						{
							uint32_t c = (uint32_t)(uint8_t)(*p);
							res += (((c<<12) & 0xF000) 
									| ((((uint32_t)(uint8_t)(*(p+1)))<<6) & 0xFC0) 
									| ((*(p+2)) & 0x3F));
							p+=3;
							len-=3;
						}
						break;
					case 0x30: // 4 byte UTF8
					{
						if (len < 4  // truncated
							|| (((*(p+1))&0xC0) != 0x80)  // invalid 2nd char
							|| (((*(p+2))&0xC0) != 0x80)  // invalid 3rd char
							|| (((*(p+3))&0xC0) != 0x80)) // invalid 4th char
						{
							res += (uint32_t)(uint8_t)(*p++);
							--len;
						}
						else
						{
							uint32_t c = (uint32_t)(uint8_t)(*p);
							res += (((c<<18) & 0x1C0000) 
									| ((((uint32_t)(uint8_t)(*(p+1)))<<12) & 0x3F000)
									| ((((uint32_t)(uint8_t)(*(p+2)))<<6) & 0xFC0)
									| ((*(p+3)) & 0x3F));
							p+=4;
							len-=4;
						}
						break;
					}
				}
				break;
			}
		}
	}
	return res;
}
// check if string is valid UTF8
bool validateUtf8(uint8_t* bytes, int32_t len)
{
	uint8_t* p = bytes;
	while (len > 0)
	{
		switch ((*p)&0xc0)
		{
			case 0x80: // invalid
				return false;
			case 0xc0: // > 1 byte UTF8
			{
				switch ((*p)&0x30)
				{
					case 0x00:// 2 byte UTF8
					case 0x10:// 2 byte UTF8
						if (len < 2 // truncated
							|| (((*(p+1))&0xC0) != 0x80))  // invalid 2nd char
							return false;
						p+=2;
						len-=2;
						break;
					case 0x20: // 3 byte UTF8
						if (len < 3  // truncated
							|| (((*(p+1))&0xC0) != 0x80)  // invalid 2nd char
							|| (((*(p+2))&0xC0) != 0x80)) // invalid 3rd char
							return false;
						p+=3;
						len-=3;
						break;
					case 0x30: // 4 byte UTF8
						if (len < 4  // truncated
							|| (((*(p+1))&0xC0) != 0x80)  // invalid 2nd char
							|| (((*(p+2))&0xC0) != 0x80)  // invalid 3rd char
							|| (((*(p+3))&0xC0) != 0x80)) // invalid 4th char
							return false;
						p+=4;
						len-=4;
						break;
				}
				break;
			}
			default:
				--len;
		}
	}
	return true;
}

ASFUNCTIONBODY_ATOM(ByteArray,_toString)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	//check for Byte Order Mark
	int start = 0;
	if (th->len > 3)
	{
		if (th->bytes[0] == 0xef &&
			th->bytes[1] == 0xbb &&
			th->bytes[2] == 0xbf)
			start = 3;
	}
	if (validateUtf8(th->bytes+start,th->len-start))
		ret = asAtomHandler::fromObject(abstract_s(wrk,(const char*)th->bytes+start,th->len-start));
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,parseUtf8(th->bytes+start,th->len-start)));
}

bool ByteArray::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
	if (!isConstructed())
		return false;
	unsigned int index=0;
	if(!Array::isValidMultiname(getSystemState(),name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);

	return index<len;
}

GET_VARIABLE_RESULT ByteArray::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	unsigned int index=0;
	if((opt & GET_VARIABLE_OPTION::SKIP_IMPL)!=0  || !implEnable || !Array::isValidMultiname(getSystemState(),name,index))
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	}

	if(index<len)
	{
		lock();
		uint8_t value = bytes[index];
		unlock();
		asAtomHandler::setUInt(ret,this->getInstanceWorker(),static_cast<uint32_t>(value));
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	asAtomHandler::setUndefined(ret);
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}
GET_VARIABLE_RESULT ByteArray::getVariableByInteger(asAtom &ret, int index, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	if (index < 0)
		return getVariableByIntegerIntern(ret,index,opt,wrk);
	if (index >=0 && uint32_t(index) < len)
	{
		lock();
		uint8_t value = bytes[index];
		unlock();
		asAtomHandler::setUInt(ret,this->getInstanceWorker(),static_cast<uint32_t>(value));
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	asAtomHandler::setUndefined(ret);
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}
int32_t ByteArray::getVariableByMultiname_i(const multiname& name, ASWorker* wrk)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(getSystemState(),name,index))
		return ASObject::getVariableByMultiname_i(name,wrk);

	if(index<len)
	{
		lock();
		uint8_t value = bytes[index];
		unlock();
		return static_cast<uint32_t>(value);
	}
	else
		return _MNR(getSystemState()->getUndefinedRef());
}

multiname *ByteArray::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(getSystemState(),name,index))
		return ASObject::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	if (index > BA_MAX_SIZE) 
	{
		createError<ASError>(wrk,kOutOfMemoryError);
		return nullptr;
	}

	lock();
	if(index>=len)
	{
		uint32_t prevLen = len;
		getBuffer(index+1, true);
		// Fill the gap between the end of the current data and the index with zeros
		memset(bytes+prevLen, 0, index-prevLen);
	}
	// Fill the byte pointed to by index with the truncated uint value of the object.
	uint8_t value = static_cast<uint8_t>(asAtomHandler::toUInt(o) & 0xff);
	bytes[index] = value;
	unlock();

	ASATOM_DECREF(o);
	return nullptr;
}
void ByteArray::setVariableByInteger(int index, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	if (index < 0)
	{
		setVariableByInteger_intern(index,o,allowConst,alreadyset,wrk);
		return;
	}
	lock();
	if(uint32_t(index)>=len)
	{
		uint32_t prevLen = len;
		getBuffer(index+1, true);
		// Fill the gap between the end of the current data and the index with zeros
		memset(bytes+prevLen, 0, index-prevLen);
	}

	// Fill the byte pointed to by index with the truncated uint value of the object.
	uint8_t value = static_cast<uint8_t>(asAtomHandler::toUInt(o) & 0xff);
	bytes[index] = value;
	unlock();

	ASATOM_DECREF(o);
}
void ByteArray::setVariableByMultiname_i(multiname& name, int32_t value, ASWorker* wrk)
{
	asAtom v = asAtomHandler::fromInt(value);
	setVariableByMultiname(name, v,ASObject::CONST_NOT_ALLOWED,nullptr,wrk);
}

void ByteArray::acquireBuffer(uint8_t* buf, int bufLen)
{
	if(bytes)
	{
#ifdef MEMORY_USAGE_PROFILING
		getClass()->memoryAccount->removeBytes(real_len);
#endif
		delete[] bytes;
	}
	bytes=buf;
	real_len=bufLen;
	len=bufLen;
#ifdef MEMORY_USAGE_PROFILING
	getClass()->memoryAccount->addBytes(real_len);
#endif
	position=0;
}

void ByteArray::writeU29(uint32_t val)
{
	for(uint32_t i=0;i<4;i++)
	{
		uint8_t b;
		if(i<3)
		{
			uint32_t tmp=(val >> ((3-i)*7));
			if(tmp==0)
				continue;

			b=(tmp&0x7f)|0x80;
		}
		else
			b=val&0x7f;

		writeByte(b);
	}
}

void ByteArray::serializeDouble(number_t val)
{
	//We have to write the double in network byte order (big endian)
	const uint64_t* tmpPtr=reinterpret_cast<const uint64_t*>(&val);
	uint64_t bigEndianVal=GINT64_FROM_BE(*tmpPtr);
	uint8_t* bigEndianPtr=reinterpret_cast<uint8_t*>(&bigEndianVal);
		
	for(uint32_t i=0;i<8;i++)
		writeByte(bigEndianPtr[i]);
	
}

void ByteArray::writeStringVR(map<tiny_string, uint32_t>& stringMap, const tiny_string& s)
{
	const uint32_t len=s.numBytes();
	if(len >= 1<<28)
	{
		createError<RangeError>(getInstanceWorker(),kParamRangeError);
		return;
	}

	//Check if the string is already in the map
	auto it=stringMap.find(s);
	if(it!=stringMap.end())
	{
		//The first bit must be 0, the next 29 bits
		//store the index of the string in the map
		writeU29(it->second << 1);
	}
	else
	{
		//The AMF3 spec says that the empty string is never sent by reference
		//So add the string to the map only if it's not the empty string
		if(len)
			stringMap.insert(make_pair(s, stringMap.size()));

		//The first bit must be 1, the next 29 bits
		//store the number of bytes of the string
		writeU29((len<<1) | 1);

		getBuffer(position+len,true);
		memcpy(bytes+position,s.raw_buf(),len);
		position+=len;
	}
}

void ByteArray::writeStringAMF0(const tiny_string& s)
{
	const uint32_t len=s.numBytes();
	if(len <= 0xffff)
	{
		writeUTF(s);
	}
	else
	{
		getBuffer(position+len+4,true);
		uint32_t numBytes=endianIn((uint32_t)len);
		memcpy(bytes+position,&numBytes,4);
		memcpy(bytes+position+4,s.raw_buf(),len);
		position+=len+4;
	}
}

void ByteArray::writeXMLString(std::map<const ASObject*, uint32_t>& objMap,
			       ASObject *xml,
			       const tiny_string& xmlstr)
{
	if(xmlstr.numBytes() >= 1<<28)
	{
		createError<RangeError>(getInstanceWorker(),kParamRangeError);
		return;
	}

	//Check if the XML object has been already serialized
	auto it=objMap.find(xml);
	if(it!=objMap.end())
	{
		//The least significant bit is 0 to signal a reference
		writeU29(it->second << 1);
	}
	else
	{
		//Add the XML object to the map
		objMap.insert(make_pair(xml, objMap.size()));

		//The first bit must be 1, the next 29 bits
		//store the number of bytes of the string
		writeU29((xmlstr.numBytes()<<1) | 1);

		getBuffer(position+xmlstr.numBytes(),true);
		memcpy(bytes+position,xmlstr.raw_buf(),xmlstr.numBytes());
		position+=xmlstr.numBytes();
	}
}
void ByteArray::append(streambuf *data, int length)
{
	lock();
	int oldlen = len;
	getBuffer(len+length,true);
	istream s(data);
	s.read((char*)bytes+oldlen,length);
	unlock();
}
void ByteArray::removeFrontBytes(int count)
{
	memmove(bytes,bytes+count,count);
	position -= count;
	len -= count;
}



void ByteArray::compress_zlib(bool raw)
{
	z_stream strm;
	int status;

	if(len==0)
		return;

	strm.zalloc=Z_NULL;
	strm.zfree=Z_NULL;
	strm.opaque=Z_NULL;
	strm.avail_in=len;
	strm.next_in=bytes;
	strm.avail_out=0;
	status=deflateInit2 (&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
						 raw ? -15 : 15, 
						 8,
						 Z_DEFAULT_STRATEGY);
	if (status != Z_OK)
		throw RunTimeException("zlib compress failed");
	unsigned long buflen=compressBound(len);
	position=0;
	unsigned char* compressed = new unsigned char[buflen];
	strm.avail_out = buflen;
	strm.next_out = compressed;
	status = deflate (&strm, Z_FINISH);
	if (status == Z_STREAM_ERROR || strm.avail_in != 0)
	{
		delete[] compressed;
		throw RunTimeException("zlib compress failed");
	}
	deflateEnd(&strm);

	acquireBuffer(compressed, buflen);
	position=buflen;
}

void ByteArray::uncompress_zlib(bool raw)
{
	z_stream strm;
	int status;

	if(len==0)
		return;

	strm.zalloc=Z_NULL;
	strm.zfree=Z_NULL;
	strm.opaque=Z_NULL;
	strm.avail_in=len;
	strm.next_in=bytes;
	strm.total_out=0;
	status=inflateInit2(&strm,raw ? -15 : 15);
	if(status==Z_VERSION_ERROR)
	{
		createError<IOError>(getInstanceWorker(),0,"not valid compressed data");
		return;
	}
	else if(status!=Z_OK)
		throw RunTimeException("zlib uncompress failed");

	vector<uint8_t> buf(3*len);
	do
	{
		strm.next_out=&buf[strm.total_out];
		strm.avail_out=buf.size()-strm.total_out;
		status=inflate(&strm, Z_NO_FLUSH);

		if(status!=Z_OK && status!=Z_STREAM_END)
		{
			inflateEnd(&strm);
			createError<IOError>(getInstanceWorker(),0,"not valid compressed data");
			return;
		}

		if(strm.avail_out==0)
			buf.resize(buf.size()+len);
	} while(status!=Z_STREAM_END);

	inflateEnd(&strm);

	len=strm.total_out;
#ifdef MEMORY_USAGE_PROFILING
	getClass()->memoryAccount->addBytes(len-real_len);
#endif
	real_len = len;
	uint8_t* bytes2 = new uint8_t[len];
	assert_and_throw(bytes2);
	delete[] bytes;
	bytes = bytes2;
	memcpy(bytes, &buf[0], len);
	position=0;
}

ASFUNCTIONBODY_ATOM(ByteArray,_compress)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	// flash throws an error if compress is called with a compression algorithm,
	// and always uses the zlib algorithm
	// but tamarin tests do not catch it, so we simply ignore any parameters provided
	th->lock();
	th->compress_zlib(false);
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,_uncompress)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	// flash throws an error if uncompress is called with a compression algorithm,
	// and always uses the zlib algorithm
	// but tamarin tests do not catch it, so we simply ignore any parameters provided
	th->lock();
	th->uncompress_zlib(false);
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,_deflate)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	th->lock();
	th->compress_zlib(true);
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,_inflate)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	th->lock();
	th->uncompress_zlib(true);
	th->unlock();
}

ASFUNCTIONBODY_ATOM(ByteArray,clear)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	th->lock();
	if(th->bytes)
	{
#ifdef MEMORY_USAGE_PROFILING
		th->getClass()->memoryAccount->removeBytes(th->real_len);
#endif
		delete[] th->bytes;
	}
	th->bytes = nullptr;
	th->len=0;
	th->real_len=0;
	th->position=0;
	th->unlock();
}

// this seems to be how AS3 handles generic pop calls in Array class
ASFUNCTIONBODY_ATOM(ByteArray,pop)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	uint8_t res = 0;
	th->lock();
	if (th->readByte(res))
	{
		memmove(th->bytes,(th->bytes+1),th->getLength()-1);
		th->len--;
	}
	th->unlock();
	asAtomHandler::setUInt(ret,wrk,(uint32_t)res);
	
}

// this seems to be how AS3 handles generic push calls in Array class
ASFUNCTIONBODY_ATOM(ByteArray,push)
{
	ByteArray* th=static_cast<ByteArray*>(asAtomHandler::getObject(obj));
	th->lock();
	th->getBuffer(th->len+argslen,true);
	for (unsigned int i = 0; i < argslen; i++)
	{
		th->bytes[th->len+i] = (uint8_t)asAtomHandler::toInt(args[i]);
	}
	uint32_t res = th->getLength();
	th->unlock();
	asAtomHandler::setUInt(ret,wrk,res);
}

// this seems to be how AS3 handles generic shift calls in Array class
ASFUNCTIONBODY_ATOM(ByteArray,shift)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	uint8_t res = 0;
	th->lock();
	if (th->readByte(res))
	{
		memmove(th->bytes,(th->bytes+1),th->getLength()-1);
		th->len--;
	}
	th->unlock();
	asAtomHandler::setUInt(ret,wrk,(uint32_t)res);
}

// this seems to be how AS3 handles generic unshift calls in Array class
ASFUNCTIONBODY_ATOM(ByteArray,unshift)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	th->lock();
	th->getBuffer(th->len+argslen,true);
	for (unsigned int i = 0; i < argslen; i++)
	{
		memmove((th->bytes+argslen),(th->bytes),th->len);
		th->bytes[i] = (uint8_t)asAtomHandler::toInt(args[i]);
	}
	uint32_t res = th->getLength();
	th->unlock();
	asAtomHandler::setUInt(ret,wrk,res);
}
ASFUNCTIONBODY_GETTER(ByteArray,shareable)
ASFUNCTIONBODY_ATOM(ByteArray,_setter_shareable)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	bool value;
	ARG_CHECK(ARG_UNPACK(value));
	if (!th->shareable)
		th->shareable=value;
}
ASFUNCTIONBODY_ATOM(ByteArray,atomicCompareAndSwapIntAt)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);

	int32_t byteindex,expectedValue,newvalue;
	ARG_CHECK(ARG_UNPACK(byteindex)(expectedValue)(newvalue));

	if ((byteindex < 0) || (byteindex%4))
	{
		createError<RangeError>(wrk,kInvalidRangeError, th->getClassName());
		return;
	}
	th->lock();
	if(byteindex >= (int32_t)th->len-4)
	{
		th->unlock();
		createError<RangeError>(wrk,kInvalidRangeError, th->getClassName());
		return;
	}
	int32_t res;
	memcpy(&res,th->bytes+byteindex,4);

	if (res == expectedValue)
	{
		memcpy(th->bytes+byteindex,&newvalue,4);
	}
	th->unlock();
	asAtomHandler::setInt(ret,wrk,res);
}
ASFUNCTIONBODY_ATOM(ByteArray,atomicCompareAndSwapLength)
{
	ByteArray* th=asAtomHandler::as<ByteArray>(obj);
	int32_t expectedLength,newLength;
	ARG_CHECK(ARG_UNPACK(expectedLength)(newLength));

	th->lock();
	int32_t res = th->len;
	if (res == expectedLength)
	{
		th->setLength(newLength);
	}
	th->unlock();
	asAtomHandler::setInt(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(ByteArray,_toJSON)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),"ByteArray");
}

void ByteArray::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing ByteArray in AMF0 not implemented");
		return;
	}
	assert_and_throw(objMap.find(this)==objMap.end());
	out->writeByte(byte_array_marker);
	//Check if the bytearray has been already serialized
	auto it=objMap.find(this);
	if(it!=objMap.end())
	{
		//The least significant bit is 0 to signal a reference
		out->writeU29(it->second << 1);
	}
	else
	{
		//Add the dictionary to the map
		objMap.insert(make_pair(this, objMap.size()));

		assert_and_throw(len<0x20000000);
		uint32_t value = (len << 1) | 1;
		out->writeU29(value);
		// TODO faster implementation
		for (unsigned int i = 0; i < len; i++)
			out->writeByte(this->bytes[i]);
	}
}
