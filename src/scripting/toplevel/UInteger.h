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

#ifndef SCRIPTING_TOPLEVEL_UINTEGER_H
#define SCRIPTING_TOPLEVEL_UINTEGER_H 1
#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class UInteger: public ASObject
{
friend ASObject* abstract_ui(SystemState* sys, uint32_t i);
public:
	uint32_t val;
	UInteger(ASWorker* wrk,Class_base* c,uint32_t v=0):ASObject(wrk,c,T_UINTEGER),val(v){}

	static void sinit(Class_base* c);
	tiny_string toString() const;
	static tiny_string toString(uint32_t val);
	inline number_t toNumber() override { return val; }
	inline bool destruct() override { val=0; return destructIntern();}
	inline int32_t toInt() override { return val; }
	inline int64_t toInt64() override { return val; }
	inline uint32_t toUInt() override { return val; }
	TRISTATE isLess(ASObject* r) override;
	TRISTATE isLessAtom(asAtom& r) override;
	bool isEqual(ASObject* o) override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(_valueOf);
	ASFUNCTION_ATOM(_toExponential);
	ASFUNCTION_ATOM(_toFixed);
	ASFUNCTION_ATOM(_toPrecision);
	std::string toDebugString() const override;
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
	static void serializeValue(ByteArray* out,uint32_t val);
};

}
#endif /* SCRIPTING_TOPLEVEL_UINTEGER_H */
