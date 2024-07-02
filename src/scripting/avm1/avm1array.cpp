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

#include "avm1array.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Integer.h"

using namespace std;
using namespace lightspark;

void AVM1Array::sinit(Class_base* c)
{
	Array::sinit(c);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(AVM1_getLength,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(AVM1_setLength),SETTER_METHOD,true);
}

bool AVM1Array::destruct()
{
	avm1_currentsize=0;
	return Array::destruct();
}

ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_getLength)
{
	AVM1Array* th=asAtomHandler::as<AVM1Array>(obj);
	if (th->avm1_currentsize<0 && th->currentsize==0) // array is empty and length was set to value < 0
		asAtomHandler::setInt(ret,wrk,th->avm1_currentsize);
	else
		asAtomHandler::setInt(ret,wrk,th->currentsize);
}

ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_setLength)
{
	int64_t newLen;
	ARG_CHECK(ARG_UNPACK(newLen));
	AVM1Array* th=asAtomHandler::as<AVM1Array>(obj);
	if (newLen < 0)
	{
		th->avm1_currentsize=newLen;
		th->resize(0);
	}
	else
		Array::_setLength(ret,wrk,obj,args,argslen);
}

