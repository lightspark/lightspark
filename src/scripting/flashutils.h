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

#ifndef _FLASH_UTILS_H
#define _FLASH_UTILS_H

#include "compat.h"
#include "swftypes.h"
#include "flashevents.h"
#include "thread_pool.h"
#include "timer.h"

#include <map>

namespace lightspark
{
const tiny_string flash_proxy="http://www.adobe.com/2006/actionscript/flash/proxy";

class ByteArray: public ASObject
{
friend class Loader;
friend class URLLoader;
protected:
	uint8_t* bytes;
	uint32_t len;
	uint32_t position;
	ByteArray(const ByteArray& b);
public:
	ByteArray(uint8_t* b = NULL, uint32_t l = 0);
	~ByteArray();
	ASFUNCTION(_getBytesAvailable);
	ASFUNCTION(_getLength);
	ASFUNCTION(_setLength);
	ASFUNCTION(_getPosition);
	ASFUNCTION(_setPosition);
	ASFUNCTION(_getDefaultObjectEncoding);
	ASFUNCTION(_setDefaultObjectEncoding);
	ASFUNCTION(readBytes);
	ASFUNCTION(readObject);
	ASFUNCTION(writeByte);
	ASFUNCTION(readByte);
	ASFUNCTION(writeBytes);
	ASFUNCTION(writeUTFBytes);
	ASFUNCTION(_toString);

	/**
		Get ownership over the passed buffer
		@param buf Pointer to the buffer to acquire, ownership and delete authority is acquired
		@param bufLen Lenght of the buffer
		@pre buf must be allocated using new[]
	*/
	void acquireBuffer(uint8_t* buf, int bufLen);
	uint8_t* getBuffer(unsigned int size, bool enableResize);
	uint32_t getLength() const { return len; }

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl=false, ASObject* base=NULL);
	intptr_t getVariableByMultiname_i(const multiname& name);
	void setVariableByMultiname(const multiname& name, ASObject* o, ASObject* base=NULL);
	void setVariableByMultiname_i(const multiname& name, intptr_t value);
	bool isEqual(ASObject* r);
};

class Timer: public EventDispatcher, public ITickJob
{
private:
	void tick();
protected:
	uint32_t delay;
	uint32_t repeatCount;
	uint32_t currentCount;
	bool running;
public:
	Timer():delay(0),repeatCount(0),currentCount(0),running(false){};
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
	std::map<_R<ASObject>,_R<ASObject> > data;
public:
	Dictionary(){}
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl=false, ASObject* base=NULL);
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByMultiName_i not supported for Dictionary");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o, ASObject* base=NULL);
	void setVariableByMultiname_i(const multiname& name, intptr_t value);
	void deleteVariableByMultiname(const multiname& name);
	tiny_string toString(bool debugMsg=false);
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
};

class Proxy: public ASObject
{
friend class ABCVm;
public:
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
//	ASFUNCTION(_constructor);
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl=false, ASObject* base=NULL);
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByMultiName_i not supported for Proxy");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o, ASObject* base=NULL);
	void setVariableByMultiname_i(const multiname& name, intptr_t value)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("setVariableByMultiName_i not supported for Proxy");
	}
	void deleteVariableByMultiname(const multiname& name)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("deleteVariableByMultiName not supported for Proxy");
	}
	tiny_string toString(bool debugMsg=false)
	{
		if(debugMsg)
			return ASObject::toString(debugMsg);
		else
			throw UnsupportedException("Proxy is missing some stuff");
	}
	bool isEqual(ASObject* r)
	{
		assert_and_throw(implEnable);
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

class IntervalRunner : public ITickJob, public EventDispatcher
{
public:
	enum INTERVALTYPE { INTERVAL, TIMEOUT };
private:
	INTERVALTYPE type;
	uint32_t id;
	_R<IFunction> callback;
	ASObject** args;
	const unsigned int argslen;
	_R<ASObject> obj;
	uint32_t interval;
public:
	IntervalRunner(INTERVALTYPE _type, uint32_t _id, _R<IFunction> _callback, ASObject** _args,
			const unsigned int _argslen, _R<ASObject> _obj, const uint32_t _interval);
	~IntervalRunner();
	void tick();
	INTERVALTYPE getType() { return type; }
};

class IntervalManager
{
private:
	sem_t mutex;
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

#endif
