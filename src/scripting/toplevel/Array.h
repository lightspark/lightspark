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

#ifndef SCRIPTING_TOPLEVEL_ARRAY_H
#define SCRIPTING_TOPLEVEL_ARRAY_H 1

#include "asobject.h"

namespace lightspark
{

struct sorton_field
{
	bool isNumeric;
	bool isCaseInsensitive;
	bool isDescending;
	multiname fieldname;
	sorton_field(const multiname& sortfieldname):isNumeric(false),isCaseInsensitive(false),isDescending(false),fieldname(sortfieldname){}
};

class Array: public ASObject
{
friend class ABCVm;
protected:
	uint64_t currentsize;
	typedef boost::container::flat_map<uint32_t,asAtom,std::less<uint32_t>,
		reporter_allocator<std::pair<uint32_t, asAtom>>> arrayType;
	
	typedef boost::container::flat_map<uint32_t,asAtom>::iterator data_iterator;
	arrayType data;
	uint32_t currentpos;
	void outofbounds(unsigned int index) const;
	~Array();
private:
	class sortComparatorDefault
	{
	private:
		bool isNumeric;
		bool isCaseInsensitive;
		bool isDescending;
	public:
		sortComparatorDefault(bool n, bool ci, bool d):isNumeric(n),isCaseInsensitive(ci),isDescending(d){}
		bool operator()(asAtom& d1, asAtom& d2);
	};
	class sortComparatorWrapper
	{
	private:
		IFunction* comparator;
	public:
		sortComparatorWrapper(IFunction* c):comparator(c){}
		bool operator()(asAtom& d1, asAtom& d2);
	};
	class sortOnComparator
	{
	private:
		std::vector<sorton_field> fields;
	public:
		sortOnComparator(const std::vector<sorton_field>& sf):fields(sf){}
		bool operator()(asAtom& d1, asAtom& d2);
	};
	void constructorImpl(ASObject* const* args, const unsigned int argslen);
	tiny_string toString_priv(bool localized=false);
	int capIndex(int i);
public:
	static bool isIntegerWithoutLeadingZeros(const tiny_string& value);
	enum SORTTYPE { CASEINSENSITIVE=1, DESCENDING=2, UNIQUESORT=4, RETURNINDEXEDARRAY=8, NUMERIC=16 };
	Array(Class_base* c);
	bool destruct()
	{
		for (auto it=data.begin() ; it != data.end(); ++it)
		{
			ASATOM_DECREF(it->second);
		}
		data.clear();
		currentsize=0;
		currentpos = 0;
		return ASObject::destruct();
	}
	
	//These utility methods are also used by ByteArray
	static bool isValidMultiname(SystemState* sys,const multiname& name, uint32_t& index);
	static bool isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index);

	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);

	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	ASFUNCTION_ATOM(_push);
	ASFUNCTION_ATOM(_push_as3);
	ASFUNCTION(_concat);
	ASFUNCTION(_pop);
	ASFUNCTION(join);
	ASFUNCTION(shift);
	ASFUNCTION(unshift);
	ASFUNCTION(splice);
	ASFUNCTION(_sort);
	ASFUNCTION_ATOM(sortOn);
	ASFUNCTION(filter);
	ASFUNCTION(indexOf);
	ASFUNCTION_ATOM(_getLength);
	ASFUNCTION_ATOM(_setLength);
	ASFUNCTION(forEach);
	ASFUNCTION(_reverse);
	ASFUNCTION(lastIndexOf);
	ASFUNCTION(_map);
	ASFUNCTION(_toString);
	ASFUNCTION(_toLocaleString);
	ASFUNCTION(slice);
	ASFUNCTION(every);
	ASFUNCTION(some);
	ASFUNCTION(insertAt);
	ASFUNCTION(removeAt);

	_R<ASObject> at(unsigned int index);
	void set(unsigned int index, asAtom o);
	uint64_t size();
	void push(asAtom o);
	void resize(uint64_t n);
	asAtom getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	int32_t getVariableByMultiname_i(const multiname& name);
	void setVariableByMultiname(const multiname& name, asAtom o, CONST_ALLOWED_FLAG allowConst);
	bool deleteVariableByMultiname(const multiname& name);
	void setVariableByMultiname_i(const multiname& name, int32_t value);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	tiny_string toString();
	uint32_t nextNameIndex(uint32_t cur_index);
	asAtom nextName(uint32_t index);
	asAtom nextValue(uint32_t index);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
	virtual tiny_string toJSON(std::vector<ASObject *> &path,IFunction* replacer, const tiny_string &spaces,const tiny_string& filter);
};


}
#endif /* SCRIPTING_TOPLEVEL_ARRAY_H */
