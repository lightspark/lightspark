/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2013  Antti Ajanki (antti.ajanki@iki.fi)

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

#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/display/GraphicsPath.h"
#include "scripting/flash/display/Graphics.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/argconv.h"
#include "scripting/class.h"

using namespace lightspark;

GraphicsPath::GraphicsPath(ASWorker* wrk, Class_base* c):
	ASObject(wrk,c,T_OBJECT,SUBTYPE_GRAPHICSPATH), winding("evenOdd")
{
}

void GraphicsPath::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, commands, Vector);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, data, Vector);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, winding, ASString);
	c->setDeclaredMethodByQName("curveTo","",c->getSystemState()->getBuiltinFunction(curveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",c->getSystemState()->getBuiltinFunction(lineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",c->getSystemState()->getBuiltinFunction(moveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("wideLineTo","",c->getSystemState()->getBuiltinFunction(wideLineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("wideMoveTo","",c->getSystemState()->getBuiltinFunction(wideMoveTo),NORMAL_METHOD,true);

	c->addImplementedInterface(InterfaceClass<IGraphicsPath>::getClass(c->getSystemState()));
	IGraphicsPath::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IGraphicsData>::getClass(c->getSystemState()));
	IGraphicsData::linkTraits(c);
}

ASFUNCTIONBODY_GETTER_SETTER(GraphicsPath, commands)
ASFUNCTIONBODY_GETTER_SETTER(GraphicsPath, data)
ASFUNCTIONBODY_GETTER_SETTER(GraphicsPath, winding)

ASFUNCTIONBODY_ATOM(GraphicsPath,_constructor)
{
	_NR<Vector> commands;
	_NR<Vector> data;
	GraphicsPath* th = asAtomHandler::as<GraphicsPath>(obj);
	ARG_CHECK(ARG_UNPACK(commands, NullRef)(data, NullRef)(th->winding, "evenOdd"));

	if (!commands.isNull())
		th->commands = commands;
	if (!data.isNull())
		th->data = data;
}

void GraphicsPath::finalize()
{
	ASObject::finalize();
	commands.reset();
	data.reset();
}

void GraphicsPath::ensureValid()
{
	RootMovieClip* root = getInstanceWorker()->rootClip.getPtr();
	if (commands.isNull())
	{
		asAtom v=asAtomHandler::invalidAtom;
		Template<Vector>::getInstanceS(getInstanceWorker(),v,root,Class<Integer>::getClass(getSystemState()),NullRef);
		commands = _MNR(asAtomHandler::as<Vector>(v));
	}
	if (data.isNull())
	{
		asAtom v=asAtomHandler::invalidAtom;
		Template<Vector>::getInstanceS(getInstanceWorker(),v,root,Class<Number>::getClass(getSystemState()),NullRef);
		data = _MNR(asAtomHandler::as<Vector>(v));
	}
}

ASFUNCTIONBODY_ATOM(GraphicsPath, curveTo)
{
	GraphicsPath* th=asAtomHandler::as<GraphicsPath>(obj);
	asAtom cx = asAtomHandler::fromType(T_NUMBER);
	asAtom cy = asAtomHandler::fromType(T_NUMBER);
	asAtom ax = asAtomHandler::fromType(T_NUMBER);
	asAtom ay = asAtomHandler::fromType(T_NUMBER);
	ARG_CHECK(ARG_UNPACK (cx) (cy) (ax) (ay));

	th->ensureValid();
	asAtom v = asAtomHandler::fromInt((int32_t)GRAPHICSPATH_COMMANDTYPE::CURVE_TO);
	th->commands->append(v);
	ASATOM_INCREF(ax);
	th->data->append(ax);
	ASATOM_INCREF(ay);
	th->data->append(ay);
	ASATOM_INCREF(cx);
	th->data->append(cx);
	ASATOM_INCREF(cy);
	th->data->append(cy);
}

ASFUNCTIONBODY_ATOM(GraphicsPath, lineTo)
{
	GraphicsPath* th=asAtomHandler::as<GraphicsPath>(obj);
	asAtom x = asAtomHandler::fromType(T_NUMBER);
	asAtom y = asAtomHandler::fromType(T_NUMBER);
	ARG_CHECK(ARG_UNPACK (x) (y));

	th->ensureValid();
	asAtom v = asAtomHandler::fromInt((int32_t)(GRAPHICSPATH_COMMANDTYPE::LINE_TO));
	th->commands->append(v);
	ASATOM_INCREF(x);
	th->data->append(x);
	ASATOM_INCREF(y);
	th->data->append(y);
}

ASFUNCTIONBODY_ATOM(GraphicsPath, moveTo)
{
	GraphicsPath* th=asAtomHandler::as<GraphicsPath>(obj);
	asAtom x = asAtomHandler::fromType(T_NUMBER);
	asAtom y = asAtomHandler::fromType(T_NUMBER);
	ARG_CHECK(ARG_UNPACK (x) (y));

	th->ensureValid();
	asAtom v = asAtomHandler::fromInt((int32_t)(GRAPHICSPATH_COMMANDTYPE::MOVE_TO));
	th->commands->append(v);
	ASATOM_INCREF(x);
	th->data->append(x);
	ASATOM_INCREF(y);
	th->data->append(y);
}

ASFUNCTIONBODY_ATOM(GraphicsPath, wideLineTo)
{
	GraphicsPath* th=asAtomHandler::as<GraphicsPath>(obj);
	asAtom x = asAtomHandler::fromType(T_NUMBER);
	asAtom y = asAtomHandler::fromType(T_NUMBER);
	ARG_CHECK(ARG_UNPACK (x) (y));

	th->ensureValid();
	asAtom v = asAtomHandler::fromInt((int32_t)(GRAPHICSPATH_COMMANDTYPE::LINE_TO));
	th->commands->append(v);
	asAtom n = asAtomHandler::fromNumber(wrk,0.0,false);
	ASATOM_INCREF(n);
	th->data->append(n);
	ASATOM_INCREF(n);
	th->data->append(n);
	ASATOM_INCREF(x);
	th->data->append(x);
	ASATOM_INCREF(y);
	th->data->append(y);
}

ASFUNCTIONBODY_ATOM(GraphicsPath, wideMoveTo)
{
	GraphicsPath* th=asAtomHandler::as<GraphicsPath>(obj);
	asAtom x = asAtomHandler::fromType(T_NUMBER);
	asAtom y = asAtomHandler::fromType(T_NUMBER);
	ARG_CHECK(ARG_UNPACK (x) (y));

	th->ensureValid();
	asAtom v = asAtomHandler::fromInt((int32_t)(GRAPHICSPATH_COMMANDTYPE::MOVE_TO));
	th->commands->append(v);
	asAtom n = asAtomHandler::fromNumber(wrk,0.0,false);
	ASATOM_INCREF(n);
	th->data->append(n);
	ASATOM_INCREF(n);
	th->data->append(n);
	ASATOM_INCREF(x);
	th->data->append(x);
	ASATOM_INCREF(y);
	th->data->append(y);
}

void GraphicsPath::appendToTokens(tokensVector& tokens,Graphics* graphics)
{
	graphics->pathToTokens(commands, data, winding, tokens);
}
