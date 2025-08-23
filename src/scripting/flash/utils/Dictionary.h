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

#ifndef SCRIPTING_FLASH_UTILS_DICTIONARY_H
#define SCRIPTING_FLASH_UTILS_DICTIONARY_H 1

#include "compat.h"
#include "swftypes.h"
#include "asobject.h"

namespace lightspark
{
struct asAtomStrict {
	asAtom key;
	ASWorker* wrk;
	inline bool operator==(const asAtomStrict& r) const
	{
		asAtom a1 = key;
		asAtom a2 = r.key;
		bool res=asAtomHandler::isEqualStrict(a1,wrk,a2);
		return res;
	}
};
struct asAtomStrictHash
{
	std::size_t operator()(const asAtomStrict& s) const noexcept
	{
		return s.key.uintval>>3;
	}
};
class Dictionary: public ASObject
{
friend class ABCVm;
private:
	typedef std::unordered_map<asAtomStrict, asAtom,asAtomStrictHash> dictType;
	dictType data;
	dictType::iterator findKey(asAtom o);
	std::vector<asAtom> enumeratedKeys;
	bool weakkeys;
	void getKeyFromMultiname(const multiname& name, lightspark::asAtom& key);
public:
	Dictionary(ASWorker* wrk,Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;

	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_toJSON);

	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	int32_t getVariableByMultiname_i(const multiname& name, ASWorker* wrk) override
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByMultiName_i not supported for Dictionary");
	}
	asAtomWithNumber getAtomWithNumberByMultiname(const multiname& name, ASWorker* wrk, GET_VARIABLE_OPTION opt) override;
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk) override;
	void setVariableByMultiname_i(multiname& name, int32_t value, ASWorker* wrk) override;
	bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk) override;
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk) override;
	tiny_string toString();
	uint32_t nextNameIndex(uint32_t cur_index) override;
	void nextName(asAtom &ret, uint32_t index) override;
	void nextValue(asAtom &ret, uint32_t index) override;
	bool countCylicMemberReferences(lightspark::garbagecollectorstate& gcstate) override;

	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
};

}

#endif /* SCRIPTING_FLASH_UTILS_DICTIONARY_H */
