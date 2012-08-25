/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_TOPLEVEL_VECTOR_H
#define SCRIPTING_TOPLEVEL_VECTOR_H 1

#include "asobject.h"

namespace lightspark
{

template<class T> class TemplatedClass;
class Vector: public ASObject
{
	Type* vec_type;
	bool fixed;
	std::vector<ASObject*, reporter_allocator<ASObject*>> vec;
	int capIndex(int i) const;
	class sortComparatorWrapper
	{
	private:
		IFunction* comparator;
		Type* vec_type;
	public:
		sortComparatorWrapper(IFunction* c, Type* v):comparator(c),vec_type(v){}
		bool operator()(ASObject* d1, ASObject* d2);
	};
public:
	Vector(Class_base* c, Type *vtype=NULL);
	~Vector();
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o) {};
	static ASObject* generator(TemplatedClass<Vector>* o_class, ASObject* const* args, const unsigned int argslen);

	void setTypes(const std::vector<Type*>& types);

	//Overloads
	tiny_string toString(bool debugMsg=false);
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	static bool isValidMultiname(const multiname& name, uint32_t& index);

	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);

	uint32_t size() const
	{
		return vec.size();
	}
        ASObject* at(unsigned int index) const
        {
                return vec.at(index);
        }

	//TODO: do we need to implement generator?
	ASFUNCTION(_constructor);
	ASFUNCTION(_applytype);

	ASFUNCTION(push);
	ASFUNCTION(_concat);
	ASFUNCTION(_pop);
	ASFUNCTION(join);
	ASFUNCTION(shift);
	ASFUNCTION(unshift);
	ASFUNCTION(splice);
	ASFUNCTION(_sort);
	ASFUNCTION(getLength);
	ASFUNCTION(setLength);
	ASFUNCTION(filter);
	ASFUNCTION(indexOf);
	ASFUNCTION(_getLength);
	ASFUNCTION(_setLength);
	ASFUNCTION(getFixed);
	ASFUNCTION(setFixed);
	ASFUNCTION(forEach);
	ASFUNCTION(_reverse);
	ASFUNCTION(lastIndexOf);
	ASFUNCTION(_map);
	ASFUNCTION(_toString);
	ASFUNCTION(slice);
	ASFUNCTION(every);
	ASFUNCTION(some);
};



}
#endif /* SCRIPTING_TOPLEVEL_VECTOR_H */
