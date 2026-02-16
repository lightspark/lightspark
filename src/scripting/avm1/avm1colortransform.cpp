/**************************************************************************
    Lightspark, a free flash player implementation

	Copyright (C) 2026  Ludger Krämer <dbluelle@onlinehome.de>

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

#include "avm1colortransform.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/class.h"

using namespace std;
using namespace lightspark;

void AVM1ColorTransform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject,_constructor, CLASS_DYNAMIC_NOT_FINAL);

	c->prototype->setDeclaredMethodByQName("rgb","",c->getSystemState()->getBuiltinFunction(getColor,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("rgb","",c->getSystemState()->getBuiltinFunction(setColor),SETTER_METHOD,false,false);

	c->prototype->setDeclaredMethodByQName("redMultiplier","",c->getSystemState()->getBuiltinFunction(getRedMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("redMultiplier","",c->getSystemState()->getBuiltinFunction(setRedMultiplier),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("greenMultiplier","",c->getSystemState()->getBuiltinFunction(getGreenMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("greenMultiplier","",c->getSystemState()->getBuiltinFunction(setGreenMultiplier),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("blueMultiplier","",c->getSystemState()->getBuiltinFunction(getBlueMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("blueMultiplier","",c->getSystemState()->getBuiltinFunction(setBlueMultiplier),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("alphaMultiplier","",c->getSystemState()->getBuiltinFunction(getAlphaMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("alphaMultiplier","",c->getSystemState()->getBuiltinFunction(setAlphaMultiplier),SETTER_METHOD,false);

	c->prototype->setDeclaredMethodByQName("redOffset","",c->getSystemState()->getBuiltinFunction(getRedOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("redOffset","",c->getSystemState()->getBuiltinFunction(setRedOffset),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("greenOffset","",c->getSystemState()->getBuiltinFunction(getGreenOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("greenOffset","",c->getSystemState()->getBuiltinFunction(setGreenOffset),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("blueOffset","",c->getSystemState()->getBuiltinFunction(getBlueOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("blueOffset","",c->getSystemState()->getBuiltinFunction(setBlueOffset),SETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("alphaOffset","",c->getSystemState()->getBuiltinFunction(getAlphaOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("alphaOffset","",c->getSystemState()->getBuiltinFunction(setAlphaOffset),SETTER_METHOD,false);

	c->prototype->setDeclaredMethodByQName("concat","",c->getSystemState()->getBuiltinFunction(concat),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
}
