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

#ifndef SCRIPTING_AVM1_AVM1ARRAY_H
#define SCRIPTING_AVM1_AVM1ARRAY_H


#include "scripting/toplevel/Array.h"

namespace lightspark
{

class AVM1Array: public Array
{
private:
	int64_t avm1_currentsize;
	// AVM1 array properties are enumerated by order of insertion
	std::list<std::pair<uint32_t,bool>> name_enumeration;
	uint32_t currentEnumerationIndex;
	std::list<std::pair<uint32_t,bool>>::const_iterator currentEnumerationIterator;
	void resetEnumerator();
	void shrinkEnumeration(uint32_t oldsize);
public:
	bool isAVM1Array() const override { return true; }
	Array* createInstance() override;
	AVM1Array(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	bool destruct() override;
	ASFUNCTION_ATOM(AVM1_getLength);
	ASFUNCTION_ATOM(AVM1_setLength);
	ASFUNCTION_ATOM(AVM1_call);
	ASFUNCTION_ATOM(AVM1_apply);
	ASFUNCTION_ATOM(AVM1_pop);
	ASFUNCTION_ATOM(AVM1_push);
	ASFUNCTION_ATOM(AVM1_shift);
	ASFUNCTION_ATOM(AVM1_unshift);
	ASFUNCTION_ATOM(AVM1_splice);
	void setCurrentSize(int64_t size);
	void addEnumerationValue(uint32_t name, bool isIndex)
	{
		name_enumeration.push_back(make_pair(name,isIndex));
	}
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
	bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk) override;

	uint32_t nextNameIndex(uint32_t cur_index) override;
	void nextName(asAtom &ret, uint32_t index) override;
	void nextValue(asAtom &ret, uint32_t index) override;
};

}
#endif // SCRIPTING_AVM1_AVM1ARRAY_H
