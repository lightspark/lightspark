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


namespace lightspark
{

class Dictionary: public ASObject
{
friend class ABCVm;
private:
	typedef std::map<_R<ASObject>,asAtom,std::less<_R<ASObject>>,
	       reporter_allocator<std::pair<const _R<ASObject>, asAtom>>> dictType;
	dictType data;
	dictType::iterator findKey(ASObject *);
	bool weakkeys;
public:
	Dictionary(Class_base* c);
	bool destruct()
	{
		Dictionary::dictType::iterator it = data.begin();
		while(it!=data.end())
		{
			ASATOM_DECREF(it->second);
			it++;
		}
		data.clear();
		return ASObject::destruct();
	}
	
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_toJSON);

	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name,GET_VARIABLE_OPTION opt=NONE);
	int32_t getVariableByMultiname_i(const multiname& name)
	{
		assert_and_throw(implEnable);
		throw UnsupportedException("getVariableByMultiName_i not supported for Dictionary");
	}
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst);
	void setVariableByMultiname_i(const multiname& name, int32_t value);
	bool deleteVariableByMultiname(const multiname& name);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	tiny_string toString();
	uint32_t nextNameIndex(uint32_t cur_index);
	void nextName(asAtom &ret, uint32_t index);
	void nextValue(asAtom &ret, uint32_t index);

	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

}

#endif /* SCRIPTING_FLASH_UTILS_DICTIONARY_H */
