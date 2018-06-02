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
private:
	bool proxyconstructionCompleted;
public:
	Proxy(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_PROXY),proxyconstructionCompleted(false){}
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
//	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_isAttribute);
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name,GET_VARIABLE_OPTION opt=NONE);
	int32_t getVariableByMultiname_i(const multiname& name)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByMultiName_i not supported for Proxy");
	}
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst);
	void setVariableByMultiname_i(const multiname& name, int32_t value)
	{
		asAtom v(value);
		setVariableByMultiname(name,v,CONST_NOT_ALLOWED);
	}
	
	bool deleteVariableByMultiname(const multiname& name);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	tiny_string toString()
	{
		throw UnsupportedException("Proxy is missing some stuff");
	}
	uint32_t nextNameIndex(uint32_t cur_index);
	void nextName(asAtom &ret, uint32_t index);
	void nextValue(asAtom &ret, uint32_t index);
	bool isConstructed() const;
};
}

#endif /* SCRIPTING_FLASH_UTILS_PROXY_H */
