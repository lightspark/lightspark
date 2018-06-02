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
	std::vector<asAtom, reporter_allocator<asAtom>> vec;
	int capIndex(int i) const;
	class sortComparatorDefault
	{
	private:
		bool isNumeric;
		bool isCaseInsensitive;
		bool isDescending;
	public:
		sortComparatorDefault(bool n, bool ci, bool d):isNumeric(n),isCaseInsensitive(ci),isDescending(d){}
		bool operator()(const asAtom& d1, const asAtom& d2);
	};
public:
	class sortComparatorWrapper
	{
	private:
		asAtom comparator;
	public:
		sortComparatorWrapper(asAtom c):comparator(c){}
		number_t compare(const asAtom& d1, const asAtom& d2);
	};
	Vector(Class_base* c, const Type *vtype=NULL);
	~Vector();
	bool destruct()
	{
		for(unsigned int i=0;i<size();i++)
		{
			ASATOM_DECREF(vec[i]);
		}
		vec.clear();
		return ASObject::destruct();
	}
	
	
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o) {}
	static void generator(asAtom& ret, SystemState* sys, asAtom& o_class, asAtom* args, const unsigned int argslen);

	void setTypes(const std::vector<const Type*>& types);
	bool sameType(const Class_base* cls) const;

	//Overloads
	tiny_string toString();
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt);
	static bool isValidMultiname(SystemState* sys, const multiname& name, uint32_t& index, bool *isNumber = NULL);

	tiny_string toJSON(std::vector<ASObject *> &path, asAtom replacer, const tiny_string &spaces,const tiny_string& filter);

	uint32_t nextNameIndex(uint32_t cur_index);
	void nextName(asAtom &ret, uint32_t index);
	void nextValue(asAtom &ret, uint32_t index);

	uint32_t size() const
	{
		return vec.size();
	}
	asAtom at(unsigned int index) const
	{
		return vec.at(index);
	}
	void set(uint32_t index, asAtom v)
	{
		if (index < size())
			vec[index] = v;
	}
	//Get value at index, or return defaultValue (a borrowed
	//reference) if index is out-of-range
	asAtom at(unsigned int index, asAtom defaultValue) const;

	//Appends an object to the Vector. o is coerced to vec_type.
	//Takes ownership of o.
	void append(asAtom& o);
	void setFixed(bool v) { fixed = v; }

	//TODO: do we need to implement generator?
	ASFUNCTION_ATOM(_constructor);

	ASFUNCTION_ATOM(push);
	ASFUNCTION_ATOM(_concat);
	ASFUNCTION_ATOM(_pop);
	ASFUNCTION_ATOM(join);
	ASFUNCTION_ATOM(shift);
	ASFUNCTION_ATOM(unshift);
	ASFUNCTION_ATOM(splice);
	ASFUNCTION_ATOM(_sort);
	ASFUNCTION_ATOM(getLength);
	ASFUNCTION_ATOM(setLength);
	ASFUNCTION_ATOM(filter);
	ASFUNCTION_ATOM(indexOf);
	ASFUNCTION_ATOM(_getLength);
	ASFUNCTION_ATOM(_setLength);
	ASFUNCTION_ATOM(getFixed);
	ASFUNCTION_ATOM(setFixed);
	ASFUNCTION_ATOM(forEach);
	ASFUNCTION_ATOM(_reverse);
	ASFUNCTION_ATOM(lastIndexOf);
	ASFUNCTION_ATOM(_map);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(slice);
	ASFUNCTION_ATOM(every);
	ASFUNCTION_ATOM(some);
	ASFUNCTION_ATOM(insertAt);
	ASFUNCTION_ATOM(removeAt);

	ASObject* describeType() const;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

}
#endif /* SCRIPTING_TOPLEVEL_VECTOR_H */
