/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _FLASH_UTILS_H
#define _FLASH_UTILS_H

#include "swftypes.h"
#include "flashevents.h"
#include "thread_pool.h"

#include <map>

namespace lightspark
{

class ByteArray: public IInterface
{
friend class Loader;
protected:
	uint8_t* bytes;
	int len;
	int position;
public:
	ByteArray();
	~ByteArray();
	ASFUNCTION(_getBytesAvailable);
	ASFUNCTION(_getLength);
	ASFUNCTION(_getPosition);
	ASFUNCTION(_setPosition);
	ASFUNCTION(readBytes);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	uint8_t* getBuffer(unsigned int size);
	bool getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& out)
	{
		abort();
	}
	bool getVariableByMultiname(const multiname& name, ASObject*& out);
	bool getVariableByMultiname_i(const multiname& name, intptr_t& out);
	bool setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o);
	bool setVariableByMultiname(const multiname& name, ASObject* o);
	bool setVariableByMultiname_i(const multiname& name, intptr_t value);
	bool isEqual(bool& ret, ASObject* r);
};

class Timer: public EventDispatcher, public IThreadJob
{
private:
	void execute();
protected:
	uint32_t delay;
	bool running;
public:
	Timer():delay(0),running(false){};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(start);
	ASFUNCTION(reset);
};

class Dictionary: public IInterface
{
friend class ABCVm;
/*private:
	bool isValidMultiname(const multiname& name, int& index) const;
	bool isValidQName(const tiny_string& name, const tiny_string& ns, int& index);*/
private:
	std::map<ASObject*,ASObject*> data;
public:
	Dictionary(){};
	virtual ~Dictionary(){};
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	bool getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& out)
	{
		abort();
	}
	bool getVariableByMultiname(const multiname& name, ASObject*& out);
	bool getVariableByMultiname_i(const multiname& name, intptr_t& out)
	{
		abort();
	}
	bool setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o)
	{
		abort();
	}
	bool setVariableByMultiname(const multiname& name, ASObject* o);
	bool setVariableByMultiname_i(const multiname& name, intptr_t value);
	bool deleteVariableByMultiname(const multiname& name);
	bool toString(tiny_string& ret)
	{
		return false;
	}
	bool isEqual(bool& ret, ASObject* r)
	{
		abort();
	}
	tiny_string toString() const
	{
		abort();
	}
	bool hasNext(int& index, bool& out);
	bool nextName(int index, ASObject*& out);
	bool nextValue(int index, ASObject*& out);
};
ASObject* getQualifiedClassName(ASObject* , arguments* args);
ASObject* getQualifiedSuperclassName(ASObject* , arguments* args);
ASObject* getDefinitionByName(ASObject* , arguments* args);
ASObject* getTimer(ASObject* , arguments* args);

};

#endif
