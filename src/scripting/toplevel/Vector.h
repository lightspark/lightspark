/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
	const Type* vec_type;
	bool fixed;
	std::vector<ASObject*, reporter_allocator<ASObject*>> vec;
	int capIndex(int i) const;
	class sortComparatorDefault
	{
	private:
		bool isNumeric;
		bool isCaseInsensitive;
		bool isDescending;
	public:
		sortComparatorDefault(bool n, bool ci, bool d):isNumeric(n),isCaseInsensitive(ci),isDescending(d){}
		bool operator()(ASObject* d1, ASObject* d2);
	};
	class sortComparatorWrapper
	{
	private:
		asAtom comparator;
		const Type* vec_type;
	public:
		sortComparatorWrapper(asAtom c, const Type* v):comparator(c),vec_type(v){}
		bool operator()(ASObject* d1, ASObject* d2);
	};
public:
	Vector(Class_base* c, const Type *vtype=NULL);
	~Vector();
	bool destruct()
	{
		for(unsigned int i=0;i<size();i++)
		{
			if(vec[i])
				vec[i]->decRef();
		}
		vec.clear();
		return ASObject::destruct();
	}
	
	
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o) {}
	static asAtom generator(asAtom& o_class, asAtom* args, const unsigned int argslen);

	void setTypes(const std::vector<const Type*>& types);
	bool sameType(const Class_base* cls) const;

	//Overloads
	tiny_string toString();
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	asAtom getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	static bool isValidMultiname(SystemState* sys,const multiname& name, uint32_t& index);

	tiny_string toJSON(std::vector<ASObject *> &path, asAtom replacer, const tiny_string &spaces,const tiny_string& filter);

	uint32_t nextNameIndex(uint32_t cur_index);
	asAtom nextName(uint32_t index);
	asAtom nextValue(uint32_t index);

	uint32_t size() const
	{
		return vec.size();
	}
	ASObject* at(unsigned int index) const
	{
		return vec.at(index);
	}
	//Get value at index, or return defaultValue (a borrowed
	//reference) if index is out-of-range
	ASObject* at(unsigned int index, ASObject *defaultValue) const;

	//Appends an object to the Vector. o is coerced to vec_type.
	//Takes ownership of o.
	void append(ASObject *o);
	void setFixed(bool v) { fixed = v; }

	//TODO: do we need to implement generator?
	ASFUNCTION(_constructor);

	ASFUNCTION_ATOM(push);
	ASFUNCTION_ATOM(_concat);
	ASFUNCTION_ATOM(_pop);
	ASFUNCTION(join);
	ASFUNCTION_ATOM(shift);
	ASFUNCTION_ATOM(unshift);
	ASFUNCTION(splice);
	ASFUNCTION_ATOM(_sort);
	ASFUNCTION(getLength);
	ASFUNCTION(setLength);
	ASFUNCTION_ATOM(filter);
	ASFUNCTION(indexOf);
	ASFUNCTION(_getLength);
	ASFUNCTION(_setLength);
	ASFUNCTION(getFixed);
	ASFUNCTION(setFixed);
	ASFUNCTION_ATOM(forEach);
	ASFUNCTION(_reverse);
	ASFUNCTION(lastIndexOf);
	ASFUNCTION_ATOM(_map);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(slice);
	ASFUNCTION_ATOM(every);
	ASFUNCTION_ATOM(some);
	ASFUNCTION(insertAt);
	ASFUNCTION(removeAt);

	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

}
#endif /* SCRIPTING_TOPLEVEL_VECTOR_H */
