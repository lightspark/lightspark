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

#ifndef SCRIPTING_FLASH_UTILS_PROXY_H
#define SCRIPTING_FLASH_UTILS_PROXY_H 1

#include "compat.h"
#include "swftypes.h"

namespace lightspark
{

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
}

#endif /* SCRIPTING_FLASH_UTILS_PROXY_H */
