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

#include "argconv.h"
#include "UInteger.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME2(UInteger,"uint","");

tiny_string UInteger::toString()
{
	return UInteger::toString(val);
}

/* static helper function */
tiny_string UInteger::toString(uint32_t val)
{
	char buf[20];
	snprintf(buf,sizeof(buf),"%u",val);
	return tiny_string(buf,true);
}

TRISTATE UInteger::isLess(ASObject* o)
{
	if(o->getObjectType()==T_UINTEGER)
	{
		uint32_t val1=val;
		uint32_t val2=o->toUInt();
		return (val1<val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_INTEGER ||
	   o->getObjectType()==T_BOOLEAN)
	{
		uint32_t val1=val;
		int32_t val2=o->toInt();
		if(val2<0)
			return TFALSE;
		else
			return (val1<(uint32_t)val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		number_t val2=o->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (number_t(val) < val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NULL)
	{
		// UInteger is never less than int(null) == 0
		return TFALSE;
	}
	else if(o->getObjectType()==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(o->getObjectType()==T_STRING)
	{
		double val2=o->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
	else
	{
		double val2=o->toPrimitive()->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
}

ASFUNCTIONBODY(UInteger,generator)
{
	return abstract_ui(args[0]->toUInt());
}
