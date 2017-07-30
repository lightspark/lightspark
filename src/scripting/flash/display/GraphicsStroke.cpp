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

#include <limits>
#include "scripting/flash/display/GraphicsStroke.h"
#include "scripting/flash/display/IGraphicsFill.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "swftypes.h"

using namespace lightspark;

GraphicsStroke::GraphicsStroke(Class_base* c):
	ASObject(c), caps("none"), joints("round"), miterLimit(3.0),
	pixelHinting(false), scaleMode("normal"),
	thickness(std::numeric_limits<double>::quiet_NaN())
{
}

void GraphicsStroke::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, caps);
	REGISTER_GETTER_SETTER(c, fill);
	REGISTER_GETTER_SETTER(c, joints);
	REGISTER_GETTER_SETTER(c, miterLimit);
	REGISTER_GETTER_SETTER(c, pixelHinting);
	REGISTER_GETTER_SETTER(c, scaleMode);
	REGISTER_GETTER_SETTER(c, thickness);

	c->addImplementedInterface(InterfaceClass<IGraphicsStroke>::getClass(c->getSystemState()));
	IGraphicsStroke::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IGraphicsData>::getClass(c->getSystemState()));
	IGraphicsData::linkTraits(c);
}

void GraphicsStroke::finalize()
{
	ASObject::finalize();
	fill.reset();
}

ASFUNCTIONBODY_ATOM(GraphicsStroke, _constructor)
{
	GraphicsStroke* th = obj.as<GraphicsStroke>();
	_NR<ASObject> fill;
	ARG_UNPACK_ATOM (th->thickness, std::numeric_limits<double>::quiet_NaN())
		(th->pixelHinting, false)
		(th->scaleMode, "normal")
		(th->caps, "none")
		(th->joints, "rounds")
		(th->miterLimit, 3.0)
		(th->fill, NullRef);
	th->validateFill(NullRef);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_GETTER_SETTER(GraphicsStroke, caps);
ASFUNCTIONBODY_GETTER_SETTER_CB(GraphicsStroke, fill, validateFill);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsStroke, joints);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsStroke, miterLimit);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsStroke, pixelHinting);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsStroke, scaleMode);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsStroke, thickness);

void GraphicsStroke::validateFill(_NR<ASObject> oldValue)
{
	if (!fill.isNull() && !fill->is<IGraphicsFill>())
	{
		tiny_string wrongClass = fill->getClassName();
		fill = oldValue;
		throwError<TypeError>(kCheckTypeFailedError, wrongClass, "IGraphicsFill");
	}
}

void GraphicsStroke::appendToTokens(tokensVector& tokens)
{
	LINESTYLE2 style(0xff);
	style.Width = thickness;

	// TODO: pixel hinting, scaling, caps, miter, joints

	if (!fill.isNull())
	{
		IGraphicsFill *gfill = dynamic_cast<IGraphicsFill*>(fill.getPtr());
		assert(gfill);
		style.HasFillFlag = true;
		style.FillType = gfill->toFillStyle();
	}

	tokens.emplace_back(GeomToken(SET_STROKE, style));
}
