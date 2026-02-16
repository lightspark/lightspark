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

#include "avm1transform.h"
#include "scripting/avm1/avm1colortransform.h"
#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/geom/Matrix3D.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/toplevel/Error.h"
#include "scripting/class.h"

using namespace std;
using namespace lightspark;


AVM1Transform::AVM1Transform(ASWorker* wrk, Class_base* c):Transform(wrk,c)
{
	subtype=SUBTYPE_AVM1TRANSFORM;
}

AVM1Transform::AVM1Transform(ASWorker* wrk, Class_base* c, DisplayObject* o):Transform(wrk,c,o)
{
	subtype=SUBTYPE_AVM1TRANSFORM;
}

void AVM1Transform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject,_constructor, CLASS_DYNAMIC_NOT_FINAL);

	c->prototype->setDeclaredMethodByQName("colorTransform","",c->getSystemState()->getBuiltinFunction(AVM1_getColorTransform,0,Class<AVM1ColorTransform>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("colorTransform","",c->getSystemState()->getBuiltinFunction(_setColorTransform),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("matrix","",c->getSystemState()->getBuiltinFunction(_setMatrix),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("matrix","",c->getSystemState()->getBuiltinFunction(_getMatrix,0,Class<Matrix>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("concatenatedMatrix","",c->getSystemState()->getBuiltinFunction(_getConcatenatedMatrix,0,Class<Matrix>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("pixelBounds","",c->getSystemState()->getBuiltinFunction(_getPixelBounds,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("concatenatedColorTransform","",c->getSystemState()->getBuiltinFunction(_getConcatenatedColorTransform,0,Class<ColorTransform>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
}

ASFUNCTIONBODY_ATOM(AVM1Transform,_constructor)
{
	AVM1Transform* th=asAtomHandler::as<AVM1Transform>(obj);
	if (!argslen)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "mc", "1", "0");
		return;
	}
	if (asAtomHandler::is<DisplayObject>(args[0]))
	{
		th->owner = asAtomHandler::as<DisplayObject>(args[0]);
		th->owner->addOwnedObject(th);
	}
}
ASFUNCTIONBODY_ATOM(AVM1Transform,AVM1_getColorTransform)
{
	AVM1Transform* th=asAtomHandler::as<AVM1Transform>(obj);
	AVM1ColorTransform* ct = Class<AVM1ColorTransform>::getInstanceS(wrk,th->owner->colorTransform);

	ret = asAtomHandler::fromObject(ct);
}

