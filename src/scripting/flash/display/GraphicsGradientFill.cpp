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

#include "scripting/flash/display/GraphicsGradientFill.h"
#include "scripting/flash/display/Graphics.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Array.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace lightspark;

GraphicsGradientFill::GraphicsGradientFill(Class_base* c):
	ASObject(c), focalPointRatio(0), interpolationMethod("rgb"),
	spreadMethod("pad"), type("linear")
{
}

void GraphicsGradientFill::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, alphas);
	REGISTER_GETTER_SETTER(c, colors);
	REGISTER_GETTER_SETTER(c, focalPointRatio);
	REGISTER_GETTER_SETTER(c, interpolationMethod);
	REGISTER_GETTER_SETTER(c, matrix);
	REGISTER_GETTER_SETTER(c, ratios);
	REGISTER_GETTER_SETTER(c, spreadMethod);
	REGISTER_GETTER_SETTER(c, type);

	c->addImplementedInterface(InterfaceClass<IGraphicsFill>::getClass(c->getSystemState()));
	IGraphicsFill::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IGraphicsData>::getClass(c->getSystemState()));
	IGraphicsData::linkTraits(c);
}

void GraphicsGradientFill::finalize()
{
	ASObject::finalize();
	alphas.reset();
	colors.reset();
	matrix.reset();
	ratios.reset();
}

ASFUNCTIONBODY_ATOM(GraphicsGradientFill, _constructor)
{
	GraphicsGradientFill* th = obj.as<GraphicsGradientFill>();
	ARG_UNPACK_ATOM (th->type, "linear")
		(th->colors, NullRef)
		(th->alphas, NullRef)
		(th->ratios, NullRef)
		(th->matrix, NullRef)
		(th->spreadMethod, "pad")
		(th->interpolationMethod, "rgb")
		(th->focalPointRatio, 0);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_GETTER_SETTER(GraphicsGradientFill, alphas);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsGradientFill, colors);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsGradientFill, focalPointRatio);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsGradientFill, interpolationMethod);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsGradientFill, matrix);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsGradientFill, ratios);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsGradientFill, spreadMethod);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsGradientFill, type);

FILLSTYLE GraphicsGradientFill::toFillStyle()
{
	return Graphics::createGradientFill(type, colors, alphas, ratios,
		matrix, spreadMethod, interpolationMethod, focalPointRatio);
}

void GraphicsGradientFill::appendToTokens(tokensVector& tokens)
{
	tokens.emplace_back(GeomToken(SET_FILL, toFillStyle()));
}
