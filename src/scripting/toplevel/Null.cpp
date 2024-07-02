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


#include "scripting/class.h"
#include "scripting/toplevel/Null.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Error.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/utils/ByteArray.h"
#include "parsing/amf3_generator.h"

using namespace std;
using namespace lightspark;

Null::Null():ASObject(nullptr,nullptr,T_NULL)
{
	traitsInitialized = true;
	constructIndicator = true;
	constructorCallComplete = true;
}


bool Null::isEqual(ASObject* r)
{
	switch(r->getObjectType())
	{
		case T_NULL:
		case T_UNDEFINED:
			return true;
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_BOOLEAN:
			return false;
		case T_FUNCTION:
		case T_STRING:
			if (!r->isConstructed())
				return true;
			return false;
		default:
			return r->isEqual(this);
	}
}

TRISTATE Null::isLess(ASObject* r)
{
	if(r->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(r);
		return (0<i->toInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_UINTEGER)
	{
		UInteger* i=static_cast<UInteger*>(r);
		return (0<i->toUInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NUMBER)
	{
		Number* i=static_cast<Number*>(r);
		if(std::isnan(i->toNumber())) return TUNDEFINED;
		return (0<i->toNumber())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_BOOLEAN)
	{
		Boolean* i=static_cast<Boolean*>(r);
		return (0<i->val)?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NULL)
	{
		return TFALSE;
	}
	else if(r->getObjectType()==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(r->getObjectType()==T_STRING)
	{
		double val2=r->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
	else
	{
		asAtom val2p=asAtomHandler::invalidAtom;
		bool isrefcounted;
		r->toPrimitive(val2p,isrefcounted,NUMBER_HINT);
		double val2=asAtomHandler::toNumber(val2p);
		if (isrefcounted)
			ASATOM_DECREF(val2p);
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
}

TRISTATE Null::isLessAtom(asAtom& r)
{
	if(asAtomHandler::getObjectType(r)==T_INTEGER)
	{
		return (0<asAtomHandler::toInt(r))?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_UINTEGER)
	{
		return (0<asAtomHandler::toUInt(r))?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_NUMBER)
	{
		if(std::isnan(asAtomHandler::toNumber(r))) return TUNDEFINED;
		return (0<asAtomHandler::toNumber(r))?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_BOOLEAN)
	{
		return (0<asAtomHandler::toInt(r))?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_NULL)
	{
		return TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(asAtomHandler::getObjectType(r)==T_STRING)
	{
		double val2=asAtomHandler::toNumber(r);
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
	else
	{
		asAtom val2p=asAtomHandler::invalidAtom;
		bool isrefcounted;
		asAtomHandler::getObject(r)->toPrimitive(val2p,isrefcounted,NUMBER_HINT);
		double val2=asAtomHandler::toNumber(val2p);
		if (isrefcounted)
			ASATOM_DECREF(val2p);
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
}

int32_t Null::getVariableByMultiname_i(const multiname& name, ASWorker* wrk)
{
	LOG(LOG_ERROR,"trying to get variable on null:"<<name);
	createError<TypeError>(wrk,kConvertNullToObjectError);
	return 0;
}

GET_VARIABLE_RESULT Null::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	LOG(LOG_ERROR,"trying to get variable on null:"<<name);
	createError<TypeError>(wrk,kConvertNullToObjectError);
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}

int32_t Null::toInt()
{
	return 0;
}
int64_t Null::toInt64()
{
	return 0;
}

void Null::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
		out->writeByte(amf0_null_marker);
	else
		out->writeByte(null_marker);
}

multiname *Null::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	LOG(LOG_ERROR,"trying to set variable on null:"<<name<<" value:"<<asAtomHandler::toDebugString(o));
	ASATOM_DECREF(o);
	createError<TypeError>(wrk,kConvertNullToObjectError);
	return nullptr;
}
