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

#ifndef TOPLEVEL_ARRAY_H
#define TOPLEVEL_ARRAY_H

#include "asobject.h"

namespace lightspark
{

enum DATA_TYPE {DATA_OBJECT=0,DATA_INT};

struct data_slot
{
	DATA_TYPE type;
	union
	{
		ASObject* data;
		int32_t data_i;
	};
	explicit data_slot(ASObject* o,DATA_TYPE t=DATA_OBJECT):type(t),data(o){}
	data_slot():type(DATA_OBJECT),data(NULL){}
	explicit data_slot(int32_t i):type(DATA_INT),data_i(i){}
};

class Array: public ASObject
{
friend class ABCVm;
protected:
	uint64_t currentsize;
	std::map<uint32_t,data_slot> data;
	void outofbounds() const;
	~Array();
private:
	enum SORTTYPE { CASEINSENSITIVE=1, DESCENDING=2, UNIQUESORT=4, RETURNINDEXEDARRAY=8, NUMERIC=16 };
	class sortComparatorDefault
	{
	private:
		bool isNumeric;
		bool isCaseInsensitive;
	public:
		sortComparatorDefault(bool n, bool ci):isNumeric(n),isCaseInsensitive(ci){}
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
	tiny_string toString_priv() const;
	int capIndex(int i) const;
public:
	Array(Class_base* c);
	void finalize();
	//These utility methods are also used by ByteArray
	static bool isValidMultiname(const multiname& name, uint32_t& index);
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
	ASFUNCTION(slice);
	ASFUNCTION(every);
	ASFUNCTION(some);

	_R<ASObject> at(unsigned int index) const;
	void set(unsigned int index, _R<ASObject> o)
	{
		if(index<currentsize)
		{
			if(!data.count(index))
				data[index]=data_slot();
			o->incRef();
			data[index].data=o.getPtr();
			data[index].type=DATA_OBJECT;
		}
		else
			outofbounds();
	}
	uint64_t size() const
	{
		return currentsize;
	}
	void push(_R<ASObject> o)
	{
		o->incRef();
		data[currentsize] = data_slot(o.getPtr(),DATA_OBJECT);
		currentsize++;
	}
	void resize(uint64_t n);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	int32_t getVariableByMultiname_i(const multiname& name);
	void setVariableByMultiname(const multiname& name, ASObject* o);
	bool deleteVariableByMultiname(const multiname& name);
	void setVariableByMultiname_i(const multiname& name, int32_t value);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic);
	tiny_string toString();
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};


}
#endif
