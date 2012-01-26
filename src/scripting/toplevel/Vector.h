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

#ifndef TOPLEVEL_VECTOR_H
#define TOPLEVEL_VECTOR_H

#include "asobject.h"

namespace lightspark
{

template<class T> class TemplatedClass;
class Vector: public ASObject
{
	Type* vec_type;
	bool fixed;
	std::vector<ASObject*> vec;
public:
	Vector() : vec_type(NULL) {}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o) {};
	static ASObject* generator(TemplatedClass<Vector>* o_class, ASObject* const* args, const unsigned int argslen);

	void setTypes(const std::vector<Type*>& types);

	//Overloads
	tiny_string toString(bool debugMsg=false);
	void setVariableByMultiname(const multiname& name, ASObject* o);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);

	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);

	//TODO: do we need to implement generator?
	ASFUNCTION(_constructor);
	ASFUNCTION(_applytype);

	ASFUNCTION(push);
	ASFUNCTION(getLength);
	ASFUNCTION(setLength);
	ASFUNCTION(_toString);
};



}
#endif
