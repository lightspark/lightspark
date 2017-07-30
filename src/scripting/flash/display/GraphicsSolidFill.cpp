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

#include "scripting/flash/display/GraphicsSolidFill.h"
#include "scripting/flash/display/Graphics.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace lightspark;

GraphicsSolidFill::GraphicsSolidFill(Class_base* c):
	ASObject(c), alpha(1.0), color(0)
{
}

void GraphicsSolidFill::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, alpha);
	REGISTER_GETTER_SETTER(c, color);

	c->addImplementedInterface(InterfaceClass<IGraphicsFill>::getClass(c->getSystemState()));
	IGraphicsFill::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IGraphicsData>::getClass(c->getSystemState()));
	IGraphicsData::linkTraits(c);
}

ASFUNCTIONBODY_ATOM(GraphicsSolidFill, _constructor)
{
	GraphicsSolidFill* th = obj.as<GraphicsSolidFill>();
	ARG_UNPACK_ATOM (th->color, 0) (th->alpha, 1.0);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_GETTER_SETTER(GraphicsSolidFill, alpha);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsSolidFill, color);

FILLSTYLE GraphicsSolidFill::toFillStyle()
{
	return Graphics::createSolidFill(color, static_cast<uint8_t>(255*alpha));
}

void GraphicsSolidFill::appendToTokens(tokensVector& tokens)
{
	tokens.emplace_back(GeomToken(SET_FILL, toFillStyle()));
}
