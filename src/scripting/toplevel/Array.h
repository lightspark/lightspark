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

enum DATA_TYPE {DATA_OBJECT=0,DATA_INT};

struct data_slot
{
	union
	{
		ASObject* data;
		int32_t data_i;
	};
	DATA_TYPE type;
	explicit data_slot(ASObject* o):data(o),type(DATA_OBJECT){}
	data_slot():data(NULL),type(DATA_OBJECT){}
	explicit data_slot(int32_t i):data_i(i),type(DATA_INT){}
	void clear() 
	{
		if (type == DATA_OBJECT && data)
			data->decRef();
	}
};
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
	typedef std::map<uint32_t,data_slot,std::less<uint32_t>,
		reporter_allocator<std::pair<const uint32_t, data_slot>>> arrayType;
	
	typedef std::map<uint32_t,data_slot>::iterator data_iterator;
	arrayType data;
	data_iterator currentpos;
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
		bool operator()(const data_slot& d1, const data_slot& d2);
	};
	class sortComparatorWrapper
	{
	private:
		IFunction* comparator;
	public:
		sortComparatorWrapper(IFunction* c):comparator(c){}
		bool operator()(const data_slot& d1, const data_slot& d2);
	};
	class sortOnComparator
	{
	private:
		std::vector<sorton_field> fields;
	public:
		sortOnComparator(const std::vector<sorton_field>& sf):fields(sf){}
		bool operator()(const data_slot& d1, const data_slot& d2);
	};
	void constructorImpl(ASObject* const* args, const unsigned int argslen);
	tiny_string toString_priv(bool localized=false) const;
	int capIndex(int i) const;
public:
	static bool isIntegerWithoutLeadingZeros(const tiny_string& value);
	enum SORTTYPE { CASEINSENSITIVE=1, DESCENDING=2, UNIQUESORT=4, RETURNINDEXEDARRAY=8, NUMERIC=16 };
	Array(Class_base* c);
	bool destruct()
	{
		for (auto it=data.begin() ; it != data.end(); ++it)
		{
			if(it->second.type==DATA_OBJECT && it->second.data)
				it->second.data->decRef();
		}
		data.clear();
		currentsize=0;
		currentpos = data.end();
		return ASObject::destruct();
	}
	
	//These utility methods are also used by ByteArray
	static bool isValidMultiname(SystemState* sys,const multiname& name, uint32_t& index);
	static bool isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index);

	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);

	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	ASFUNCTION(_push);
	ASFUNCTION(_push_as3);
	ASFUNCTION(_concat);
	ASFUNCTION(_pop);
	ASFUNCTION(join);
	ASFUNCTION(shift);
	ASFUNCTION(unshift);
	ASFUNCTION(splice);
	ASFUNCTION(_sort);
	ASFUNCTION(sortOn);
	ASFUNCTION(filter);
	ASFUNCTION(indexOf);
	ASFUNCTION(_getLength);
	ASFUNCTION(_setLength);
	ASFUNCTION(forEach);
	ASFUNCTION(_reverse);
	ASFUNCTION(lastIndexOf);
	ASFUNCTION(_map);
	ASFUNCTION(_toString);
	ASFUNCTION(_toLocaleString);
	ASFUNCTION(slice);
	ASFUNCTION(every);
	ASFUNCTION(some);

	_R<ASObject> at(unsigned int index) const;
	void set(unsigned int index, _R<ASObject> o);
	uint64_t size() const
	{
		return currentsize;
	}
	void push(_R<ASObject> o);
	void resize(uint64_t n);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	int32_t getVariableByMultiname_i(const multiname& name);
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst);
	bool deleteVariableByMultiname(const multiname& name);
	void setVariableByMultiname_i(const multiname& name, int32_t value);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	tiny_string toString();
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
	virtual tiny_string toJSON(std::vector<ASObject *> &path,IFunction* replacer, const tiny_string &spaces,const tiny_string& filter);
};


}
#endif /* SCRIPTING_TOPLEVEL_ARRAY_H */
