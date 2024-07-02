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

#ifndef SCRIPTING_TOPLEVEL_NULL_H
#define SCRIPTING_TOPLEVEL_NULL_H 1

#include "asobject.h"

namespace lightspark
{

class ASWorker;

class Null: public ASObject
{
public:
	Null();
	bool isEqual(ASObject* r) override;
	TRISTATE isLess(ASObject* r) override;
	TRISTATE isLessAtom(asAtom& r) override;
	int32_t toInt() override;
	int64_t toInt64() override;
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	int32_t getVariableByMultiname_i(const multiname& name, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;

	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
};

}

#endif /* SCRIPTING_TOPLEVEL_NULL_H */
