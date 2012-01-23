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
CLASSBUILDABLE(Array);
protected:
	std::vector<data_slot> data;
	void outofbounds() const;
	Array();
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
	void finalize();
	//These utility methods are also used by ByteArray
	static bool isValidMultiname(const multiname& name, unsigned int& index);
	static bool isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index);

	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);

	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	ASFUNCTION(_push);
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
	ASFUNCTION(forEach);
	ASFUNCTION(_reverse);
	ASFUNCTION(lastIndexOf);
	ASFUNCTION(_map);
	ASFUNCTION(_toString);
	ASFUNCTION(slice);
	ASFUNCTION(every);
	ASFUNCTION(some);

	ASObject* at(unsigned int index) const;
	void set(unsigned int index, ASObject* o)
	{
		if(index<data.size())
		{
			assert_and_throw(data[index].data==NULL);
			data[index].data=o;
			data[index].type=DATA_OBJECT;
		}
		else
			outofbounds();
	}
	int size() const
	{
		return data.size();
	}
	void push(ASObject* o)
	{
		data.push_back(data_slot(o,DATA_OBJECT));
	}
	void resize(int n)
	{
		data.resize(n);
	}
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
			std::map<const ASObject*, uint32_t>& objMap) const;
};


}
#endif
