/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_UTILS_FLASHUTILS_H
#define SCRIPTING_FLASH_UTILS_FLASHUTILS_H 1

#include "compat.h"
#include "swftypes.h"
#include "scripting/flash/events/flashevents.h"
#include "thread_pool.h"
#include "timer.h"

#include <map>

namespace lightspark
{
const tiny_string flash_proxy="http://www.adobe.com/2006/actionscript/flash/proxy";

class Endian : public ASObject
{
public:
	static const char* bigEndian;
	static const char* littleEndian;
	Endian(Class_base* c):ASObject(c){};
	static void sinit(Class_base* c);
};

class IExternalizable
{
public:
	static void linkTraits(Class_base* c);
};

class IDataInput
{
public:
	static void linkTraits(Class_base* c);
};

class IDataOutput
{
public:
	static void linkTraits(Class_base* c);
};

class ByteArray: public ASObject, public IDataInput, public IDataOutput
{
friend class LoaderThread;
friend class URLLoader;
protected:
	bool littleEndian;
	uint8_t objectEncoding;
	uint32_t position;
	uint8_t* bytes;
	uint32_t real_len;
	uint32_t len;
	void compress_zlib();
	void uncompress_zlib();
public:
	ByteArray(Class_base* c, uint8_t* b = NULL, uint32_t l = 0);
	~ByteArray();
	//Helper interface for serialization
	bool readByte(uint8_t& b);
	bool readShort(uint16_t& ret);
	bool readUnsignedInt(uint32_t& ret);
	bool readU29(uint32_t& ret);
	bool readUTF(tiny_string& ret);
	void writeByte(uint8_t b);
	void writeShort(uint16_t val);
	void writeUnsignedInt(uint32_t val);
	void writeUTF(const tiny_string& str);
	uint32_t writeObject(ASObject* obj);
	void writeStringVR(std::map<tiny_string, uint32_t>& stringMap, const tiny_string& s);
	void writeXMLString(std::map<const ASObject*, uint32_t>& objMap, ASObject *xml, const tiny_string& s);
	void writeU29(uint32_t val);
	uint32_t getPosition() const;
	void setPosition(uint32_t p);

	ASFUNCTION(_getBytesAvailable);
	ASFUNCTION(_getLength);
	ASFUNCTION(_setLength);
	ASFUNCTION(_getPosition);
	ASFUNCTION(_setPosition);
	ASFUNCTION(_getEndian);
	ASFUNCTION(_setEndian);
	ASFUNCTION(_getObjectEncoding);
	ASFUNCTION(_setObjectEncoding);
	ASFUNCTION(_getDefaultObjectEncoding);
	ASFUNCTION(_setDefaultObjectEncoding);
	ASFUNCTION(_compress);
	ASFUNCTION(_uncompress);
	ASFUNCTION(_deflate);
	ASFUNCTION(_inflate);
	ASFUNCTION(clear);
	ASFUNCTION(readBoolean);
	ASFUNCTION(readByte);
	ASFUNCTION(readBytes);
	ASFUNCTION(readDouble);
	ASFUNCTION(readFloat);
	ASFUNCTION(readInt);
	ASFUNCTION(readMultiByte);
	ASFUNCTION(readObject);
	ASFUNCTION(readShort);
	ASFUNCTION(readUnsignedByte);
	ASFUNCTION(readUnsignedInt);
	ASFUNCTION(readUnsignedShort);
	ASFUNCTION(readUTF);
	ASFUNCTION(readUTFBytes);
	ASFUNCTION(writeBoolean);
	ASFUNCTION(writeByte);
	ASFUNCTION(writeBytes);
	ASFUNCTION(writeDouble);
	ASFUNCTION(writeFloat);
	ASFUNCTION(writeInt);
	ASFUNCTION(writeUnsignedInt);
	ASFUNCTION(writeMultiByte);
	ASFUNCTION(writeObject);
	ASFUNCTION(writeShort);
	ASFUNCTION(writeUTF);
	ASFUNCTION(writeUTFBytes);
	ASFUNCTION(_toString);

	// these are internal methods used if the generic Array-Methods are called on a ByteArray
	ASFUNCTION(pop);
	ASFUNCTION(push);
	ASFUNCTION(shift);
	ASFUNCTION(unshift);
	/**
		Get ownership over the passed buffer
		@param buf Pointer to the buffer to acquire, ownership and delete authority is acquired
		@param bufLen Lenght of the buffer
		@pre buf must be allocated using new[]
	*/
	void acquireBuffer(uint8_t* buf, int bufLen);
	uint8_t* getBuffer(unsigned int size, bool enableResize);
	uint32_t getLength() const { return len; }

	uint16_t endianIn(uint16_t value);
	uint32_t endianIn(uint32_t value);
	uint64_t endianIn(uint64_t value);

	uint16_t endianOut(uint16_t value);
	uint32_t endianOut(uint32_t value);
	uint64_t endianOut(uint64_t value);

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	int32_t getVariableByMultiname_i(const multiname& name);
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst);
	void setVariableByMultiname_i(const multiname& name, int32_t value);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
};

class Timer: public EventDispatcher, public ITickJob
{
private:
	void tick();
	void tickFence();
protected:
	bool running;
	uint32_t delay;
	uint32_t repeatCount;
	uint32_t currentCount;
public:
	Timer(Class_base* c):EventDispatcher(c),running(false),delay(0),repeatCount(0),currentCount(0){};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getCurrentCount);
	ASFUNCTION(_getRepeatCount);
	ASFUNCTION(_setRepeatCount);
	ASFUNCTION(_getRunning);
	ASFUNCTION(_getDelay);
	ASFUNCTION(_setDelay);
	ASFUNCTION(start);
	ASFUNCTION(reset);
	ASFUNCTION(stop);
};

class Dictionary: public ASObject
{
friend class ABCVm;
private:
	typedef std::map<_R<ASObject>,_R<ASObject>,std::less<_R<ASObject>>,
	       reporter_allocator<std::pair<const _R<ASObject>, _R<ASObject>>>> dictType;
	dictType data;
	dictType::iterator findKey(ASObject *);
public:
	Dictionary(Class_base* c);
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	int32_t getVariableByMultiname_i(const multiname& name)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByMultiName_i not supported for Dictionary");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst);
	void setVariableByMultiname_i(const multiname& name, int32_t value);
	bool deleteVariableByMultiname(const multiname& name);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	tiny_string toString();
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
};

class Proxy: public ASObject
{
friend class ABCVm;
public:
	Proxy(Class_base* c):ASObject(c){}
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
//	ASFUNCTION(_constructor);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	int32_t getVariableByMultiname_i(const multiname& name)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByMultiName_i not supported for Proxy");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst);
	void setVariableByMultiname_i(const multiname& name, int32_t value)
	{
		setVariableByMultiname(name,abstract_i(value),CONST_NOT_ALLOWED);
	}
	
	bool deleteVariableByMultiname(const multiname& name);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	tiny_string toString()
	{
		throw UnsupportedException("Proxy is missing some stuff");
	}
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
};

ASObject* getQualifiedClassName(ASObject*, ASObject* const* args, const unsigned int len);
ASObject* getQualifiedSuperclassName(ASObject*, ASObject* const* args, const unsigned int len);
ASObject* getDefinitionByName(ASObject*, ASObject* const* args, const unsigned int len);
ASObject* getTimer(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* setInterval(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* setTimeout(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* clearInterval(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* clearTimeout(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* describeType(ASObject* obj,ASObject* const* args, const unsigned int argslen);

class IntervalRunner : public ITickJob, public EventDispatcher
{
public:
	enum INTERVALTYPE { INTERVAL, TIMEOUT };
private:
	// IntervalRunner will delete itself in tickFence, others
	// should not call the destructor.
	~IntervalRunner();
	INTERVALTYPE type;
	uint32_t id;
	_R<IFunction> callback;
	ASObject** args;
	_R<ASObject> obj;
	const unsigned int argslen;
	uint32_t interval;
public:
	IntervalRunner(INTERVALTYPE _type, uint32_t _id, _R<IFunction> _callback, ASObject** _args,
			const unsigned int _argslen, _R<ASObject> _obj, const uint32_t _interval);
	void tick();
	void tickFence();
	INTERVALTYPE getType() { return type; }
};

class IntervalManager
{
private:
	Mutex mutex;
	std::map<uint32_t,IntervalRunner*> runners;
	uint32_t currentID;
public:
	IntervalManager();
	~IntervalManager();
	uint32_t setInterval(_R<IFunction> callback, ASObject** args, const unsigned int argslen, _R<ASObject> obj, const uint32_t interval);
	uint32_t setTimeout(_R<IFunction> callback, ASObject** args, const unsigned int argslen, _R<ASObject> obj, const uint32_t interval);
	uint32_t getFreeID();
	void clearInterval(uint32_t id, IntervalRunner::INTERVALTYPE type, bool removeJob);
};

};

#endif /* SCRIPTING_FLASH_UTILS_FLASHUTILS_H */
