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

#include "scripting/flash/display/GraphicsShaderFill.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace lightspark;

GraphicsShaderFill::GraphicsShaderFill(Class_base* c):
	ASObject(c)
{
}

void GraphicsShaderFill::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, matrix);
	REGISTER_GETTER_SETTER(c, shader);

	c->addImplementedInterface(InterfaceClass<IGraphicsFill>::getClass(c->getSystemState()));
	IGraphicsFill::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IGraphicsData>::getClass(c->getSystemState()));
	IGraphicsData::linkTraits(c);
}

void GraphicsShaderFill::finalize()
{
	ASObject::finalize();
	matrix.reset();
	shader.reset();
}

ASFUNCTIONBODY_ATOM(GraphicsShaderFill, _constructor)
{
	GraphicsShaderFill* th = obj.as<GraphicsShaderFill>();
	ARG_UNPACK_ATOM (th->shader, NullRef) (th->matrix, NullRef);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_GETTER_SETTER(GraphicsShaderFill, matrix);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsShaderFill, shader);

FILLSTYLE GraphicsShaderFill::toFillStyle()
{
	LOG(LOG_NOT_IMPLEMENTED, "GraphicsShaderFill::toFillStyle()");
	return FILLSTYLE(0xff);
}

void GraphicsShaderFill::appendToTokens(tokensVector& tokens)
{
	LOG(LOG_NOT_IMPLEMENTED, "GraphicsShaderFill::appendToTokens()");
	return;
}
