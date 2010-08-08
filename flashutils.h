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
	unsigned int len;
	unsigned int position;
	ByteArray(const ByteArray& b);
public:
	ByteArray();
	~ByteArray();
	ASFUNCTION(_getBytesAvailable);
	ASFUNCTION(_getLength);
	ASFUNCTION(_getPosition);
	ASFUNCTION(_setPosition);
	ASFUNCTION(readBytes);

	void acquireBuffer(uint8_t* buf, int bufLen);
	uint8_t* getBuffer(unsigned int size);

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl=false)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByName not supported for ByteArray");
		return NULL;
	}
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl=false, bool enableOverride=true, ASObject* base=NULL);
	intptr_t getVariableByMultiname_i(const multiname& name);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool skip_impl=false);
	void setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride=true, ASObject* base=NULL);
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
	bool running;
public:
	Timer():delay(0),repeatCount(0),running(false){};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(start);
	ASFUNCTION(reset);
};

class Dictionary: public ASObject
{
friend class ABCVm;
private:
	std::map<ASObject*,ASObject*> data;
public:
	Dictionary(){}
	virtual ~Dictionary();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl=false)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByQName not supported for Dictionary");
		return NULL;
	}
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl=false, bool enableOverride=true, ASObject* base=NULL);
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByMultiName_i not supported for Dictionary");
	}
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool skip_impl=false)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("setVariableByQName not supported for Dictionary");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride=true, ASObject* base=NULL);
	void setVariableByMultiname_i(const multiname& name, intptr_t value);
	void deleteVariableByMultiname(const multiname& name);
	tiny_string toString(bool debugMsg=false)
	{
		if(debugMsg)
			return ASObject::toString(debugMsg);
		else
			throw UnsupportedException("Dictionary::toString");
		return "";
	}
	bool isEqual(ASObject* r)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("isEqual not supported for Dictionary");
	}
	bool hasNext(unsigned int& index, bool& out);
	bool nextName(unsigned int index, ASObject*& out);
	bool nextValue(unsigned int index, ASObject*& out);
};

class Proxy: public ASObject
{
friend class ABCVm;
public:
	static void sinit(Class_base*);
//	static void buildTraits(ASObject* o);
//	ASFUNCTION(_constructor);
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl=false)
	{
		if(!implEnable || skip_impl)
			return ASObject::getVariableByQName(name,ns,skip_impl);
		throw RunTimeException("Proxy::getVariableByQName");
		return NULL;
	}
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl=false, bool enableOverride=true, ASObject* base=NULL);
	intptr_t getVariableByMultiname_i(const multiname& name)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByMultiName_i not supported for Proxy");
	}
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool skip_impl=false)
	{
		if(!implEnable || skip_impl)
			ASObject::setVariableByQName(name,ns,o,skip_impl);
		else
			throw RunTimeException("Proxy::setVariableByQName");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride=true, ASObject* base=NULL);
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
	bool hasNext(unsigned int& index, bool& out)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("Proxy is missing some stuff");
	}
	bool nextName(unsigned int index, ASObject*& out)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("Proxy is missing some stuff");
	}
	bool nextValue(unsigned int index, ASObject*& out)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("Proxy is missing some stuff");
	}
};

ASObject* getQualifiedClassName(ASObject*, ASObject* const* args, const unsigned int len);
ASObject* getQualifiedSuperclassName(ASObject*, ASObject* const* args, const unsigned int len);
ASObject* getDefinitionByName(ASObject*, ASObject* const* args, const unsigned int len);
ASObject* getTimer(ASObject* obj,ASObject* const* args, const unsigned int argslen);

};

#endif
