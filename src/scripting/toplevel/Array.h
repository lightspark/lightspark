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
#include <unordered_map>

namespace lightspark
{
// maximum index stored in vector
#define ARRAY_SIZE_THRESHOLD 65536


struct sorton_field
{
	bool isNumeric;
	bool isCaseInsensitive;
	bool isDescending;
	multiname fieldname;
	sorton_field(const multiname& sortfieldname):isNumeric(false),isCaseInsensitive(false),isDescending(false),fieldname(sortfieldname){}
};
struct sorton_value
{
	std::vector<asAtom> sortvalues;
	asAtom dataAtom;
	sorton_value(asAtom _dataAtom):dataAtom(_dataAtom) {}
};

class Array: public ASObject
{
friend class ABCVm;
protected:
	uint64_t currentsize;
	// data is split into a vector for the first ARRAY_SIZE_THRESHOLD indexes, and a map for bigger indexes
	std::vector<asAtom> data_first;
	std::unordered_map<uint32_t,asAtom> data_second;
	
	void outofbounds(unsigned int index) const;
	~Array();
private:
	class sortComparatorDefault
	{
	private:
		bool isNumeric;
		bool isCaseInsensitive;
		bool isDescending;
		bool useoldversion;
	public:
		sortComparatorDefault(bool oldversion, bool n, bool ci, bool d):isNumeric(n),isCaseInsensitive(ci),isDescending(d),useoldversion(oldversion){}
		bool operator()(const asAtom& d1, const asAtom& d2);
	};
	class sortOnComparator
	{
	private:
		std::vector<sorton_field> fields;
	public:
		sortOnComparator(const std::vector<sorton_field>& sf):fields(sf){}
		bool operator()(const sorton_value& d1, const sorton_value& d2);
	};
	void constructorImpl(asAtom *args, const unsigned int argslen);
	tiny_string toString_priv(bool localized=false);
	int capIndex(int i);
public:
	class sortComparatorWrapper
	{
	private:
		asAtom comparator;
	public:
		sortComparatorWrapper(asAtom c):comparator(c){}
		number_t compare(const asAtom& d1, const asAtom& d2);
	};
	static bool isIntegerWithoutLeadingZeros(const tiny_string& value);
	enum SORTTYPE { CASEINSENSITIVE=1, DESCENDING=2, UNIQUESORT=4, RETURNINDEXEDARRAY=8, NUMERIC=16 };
	Array(ASWorker *w,Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	
	//These utility methods are also used by ByteArray
	static bool isValidMultiname(SystemState* sys,const multiname& name, uint32_t& index);
	static bool isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index);

	static void sinit(Class_base*);

	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_push);
	ASFUNCTION_ATOM(_push_as3);
	ASFUNCTION_ATOM(_concat);
	ASFUNCTION_ATOM(_pop);
	ASFUNCTION_ATOM(join);
	ASFUNCTION_ATOM(shift);
	ASFUNCTION_ATOM(unshift);
	ASFUNCTION_ATOM(splice);
	ASFUNCTION_ATOM(_sort);
	ASFUNCTION_ATOM(sortOn);
	ASFUNCTION_ATOM(filter);
	ASFUNCTION_ATOM(indexOf);
	ASFUNCTION_ATOM(_getLength);
	ASFUNCTION_ATOM(_setLength);
	ASFUNCTION_ATOM(forEach);
	ASFUNCTION_ATOM(_reverse);
	ASFUNCTION_ATOM(lastIndexOf);
	ASFUNCTION_ATOM(_map);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(_toLocaleString);
	ASFUNCTION_ATOM(slice);
	ASFUNCTION_ATOM(every);
	ASFUNCTION_ATOM(some);
	ASFUNCTION_ATOM(insertAt);
	ASFUNCTION_ATOM(removeAt);

	asAtom at(unsigned int index);
	FORCE_INLINE void at_nocheck(asAtom& ret,unsigned int index)
	{
		if (index < ARRAY_SIZE_THRESHOLD)
		{
			if (index < data_first.size())
				asAtomHandler::set(ret,data_first.at(index));
		}
		else
		{
			auto it = data_second.find(index);
			if (it != data_second.end())
				asAtomHandler::set(ret,it->second);
		}
		if (asAtomHandler::isInvalid(ret))
			asAtomHandler::setUndefined(ret);
	}
	
	bool set(unsigned int index, asAtom &o, bool checkbounds = true, bool addref = true, bool addmember=true);
	uint64_t size();
	void push(asAtom o);// push doesn't increment the refcount, so the caller has to take care of that
	void resize(uint64_t n, bool removemember=true);
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	GET_VARIABLE_RESULT getVariableByInteger(asAtom& ret, int index, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	
	int32_t getVariableByMultiname_i(const multiname& name, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
	void setVariableByInteger(int index, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk) override;
	bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk) override;
	void setVariableByMultiname_i(multiname& name, int32_t value,ASWorker* wrk) override;
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk) override;
	tiny_string toString();
	uint32_t nextNameIndex(uint32_t cur_index) override;
	void nextName(asAtom &ret, uint32_t index) override;
	void nextValue(asAtom &ret, uint32_t index) override;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
	virtual tiny_string toJSON(std::vector<ASObject *> &path,asAtom replacer, const tiny_string &spaces,const tiny_string& filter) override;
};


}
#endif /* SCRIPTING_TOPLEVEL_ARRAY_H */
