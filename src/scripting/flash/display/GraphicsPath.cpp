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

#include "scripting/toplevel/Vector.h"
#include "scripting/flash/display/GraphicsPath.h"
#include "scripting/flash/display/Graphics.h"
#include "scripting/argconv.h"
#include "scripting/class.h"

using namespace lightspark;

GraphicsPath::GraphicsPath(Class_base* c):
	ASObject(c), winding("evenOdd")
{
}

void GraphicsPath::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());

	REGISTER_GETTER_SETTER(c, commands);
	REGISTER_GETTER_SETTER(c, data);
	REGISTER_GETTER_SETTER(c, winding);
	c->setDeclaredMethodByQName("curveTo","",Class<IFunction>::getFunction(curveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lineTo","",Class<IFunction>::getFunction(lineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("moveTo","",Class<IFunction>::getFunction(moveTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("wideLineTo","",Class<IFunction>::getFunction(wideLineTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("wideMoveTo","",Class<IFunction>::getFunction(wideMoveTo),NORMAL_METHOD,true);

	c->addImplementedInterface(InterfaceClass<IGraphicsPath>::getClass());
	IGraphicsPath::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IGraphicsData>::getClass());
	IGraphicsData::linkTraits(c);
}

ASFUNCTIONBODY_GETTER_SETTER(GraphicsPath, commands);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsPath, data);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsPath, winding);

ASFUNCTIONBODY(GraphicsPath, _constructor)
{
	_NR<Vector> commands;
	_NR<Vector> data;
	GraphicsPath* th = obj->as<GraphicsPath>();
	ARG_UNPACK(commands, NullRef)(data, NullRef)(th->winding, "evenOdd");

	ASObject::_constructor(obj,NULL,0);

	if (!commands.isNull())
		th->commands = commands;
	if (!data.isNull())
		th->data = data;

	return NULL;
}

void GraphicsPath::finalize()
{
	ASObject::finalize();
	commands.reset();
	data.reset();
}

void GraphicsPath::ensureValid()
{
	if (commands.isNull())
		commands = _MNR(Class<Vector>::getInstanceS(Class<Integer>::getClass()));
	if (data.isNull())
		data = _MNR(Class<Vector>::getInstanceS(Class<Number>::getClass()));
}

ASFUNCTIONBODY(GraphicsPath, curveTo)
{
	GraphicsPath* th=obj->as<GraphicsPath>();
	number_t cx;
	number_t cy;
	number_t ax;
	number_t ay;
	ARG_UNPACK (cx) (cy) (ax) (ay);

	th->ensureValid();
	th->commands->append(abstract_i(GraphicsPathCommand::CURVE_TO));
	th->data->append(abstract_d(ax));
	th->data->append(abstract_d(ay));
	th->data->append(abstract_d(cx));
	th->data->append(abstract_d(cy));

	return NULL;
}

ASFUNCTIONBODY(GraphicsPath, lineTo)
{
	GraphicsPath* th=obj->as<GraphicsPath>();
	number_t x;
	number_t y;
	ARG_UNPACK (x) (y);

	th->ensureValid();
	th->commands->append(abstract_i(GraphicsPathCommand::LINE_TO));
	th->data->append(abstract_d(x));
	th->data->append(abstract_d(y));

	return NULL;
}

ASFUNCTIONBODY(GraphicsPath, moveTo)
{
	GraphicsPath* th=obj->as<GraphicsPath>();
	number_t x;
	number_t y;
	ARG_UNPACK (x) (y);

	th->ensureValid();
	th->commands->append(abstract_i(GraphicsPathCommand::MOVE_TO));
	th->data->append(abstract_d(x));
	th->data->append(abstract_d(y));

	return NULL;
}

ASFUNCTIONBODY(GraphicsPath, wideLineTo)
{
	GraphicsPath* th=obj->as<GraphicsPath>();
	number_t x;
	number_t y;
	ARG_UNPACK (x) (y);

	th->ensureValid();
	th->commands->append(abstract_i(GraphicsPathCommand::LINE_TO));
	th->data->append(abstract_d(0));
	th->data->append(abstract_d(0));
	th->data->append(abstract_d(x));
	th->data->append(abstract_d(y));

	return NULL;
}

ASFUNCTIONBODY(GraphicsPath, wideMoveTo)
{
	GraphicsPath* th=obj->as<GraphicsPath>();
	number_t x;
	number_t y;
	ARG_UNPACK (x) (y);

	th->ensureValid();
	th->commands->append(abstract_i(GraphicsPathCommand::MOVE_TO));
	th->data->append(abstract_d(0));
	th->data->append(abstract_d(0));
	th->data->append(abstract_d(x));
	th->data->append(abstract_d(y));

	return NULL;
}

void GraphicsPath::appendToTokens(std::vector<GeomToken>& tokens)
{
	Graphics::pathToTokens(commands, data, winding, tokens);
}
