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
#include "scripting/toplevel/Undefined.h"
#include "parsing/amf3_generator.h"
#include "scripting/flash/utils/ByteArray.h"

using namespace std;
using namespace lightspark;

Undefined::Undefined():ASObject(nullptr,nullptr,T_UNDEFINED)
{
	traitsInitialized = true;
	constructIndicator = true;
	constructorCallComplete = true;
}

ASFUNCTIONBODY_ATOM(Undefined,call)
{
	LOG_CALL("Undefined function");
}

TRISTATE Undefined::isLess(ASObject* r)
{
	//ECMA-262 complaiant
	//As undefined became NaN when converted to number the operation is undefined
	return TUNDEFINED;
}
TRISTATE Undefined::isLessAtom(asAtom& r)
{
	//ECMA-262 complaiant
	//As undefined became NaN when converted to number the operation is undefined
	return TUNDEFINED;
}

bool Undefined::isEqual(ASObject* r)
{
	switch(r->getObjectType())
	{
		case T_UNDEFINED:
		case T_NULL:
			return true;
		case T_NUMBER:
		case T_INTEGER:
		case T_UINTEGER:
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

int32_t Undefined::toInt()
{
	return 0;
}
int64_t Undefined::toInt64()
{
	return 0;
}

ASObject *Undefined::describeType(ASWorker* wrk) const
{
	return ASObject::describeType(wrk);
}

void Undefined::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
		out->writeByte(amf0_undefined_marker);
	else
		out->writeByte(undefined_marker);
}

multiname *Undefined::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	LOG(LOG_ERROR,"trying to set variable on undefined:"<<name <<" "<<asAtomHandler::toDebugString(o));
	createError<TypeError>(getInstanceWorker(),kConvertUndefinedToObjectError);
	return nullptr;
}
