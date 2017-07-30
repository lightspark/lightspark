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

#include "scripting/flash/display/GraphicsTrianglePath.h"
#include "scripting/flash/display/Graphics.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace lightspark;

GraphicsTrianglePath::GraphicsTrianglePath(Class_base* c):
	ASObject(c), culling("none")
{
}

void GraphicsTrianglePath::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, culling);
	REGISTER_GETTER_SETTER(c, indices);
	REGISTER_GETTER_SETTER(c, uvtData);
	REGISTER_GETTER_SETTER(c, vertices);

	c->addImplementedInterface(InterfaceClass<IGraphicsPath>::getClass(c->getSystemState()));
	IGraphicsPath::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IGraphicsData>::getClass(c->getSystemState()));
	IGraphicsData::linkTraits(c);
}

void GraphicsTrianglePath::finalize()
{
	ASObject::finalize();
	indices.reset();
	uvtData.reset();
	vertices.reset();
}

ASFUNCTIONBODY_ATOM(GraphicsTrianglePath, _constructor)
{
	GraphicsTrianglePath* th = obj.as<GraphicsTrianglePath>();
	ARG_UNPACK_ATOM (th->vertices, NullRef)
		(th->indices, NullRef)
		(th->uvtData, NullRef)
		(th->culling, "none");
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_GETTER_SETTER(GraphicsTrianglePath, culling);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsTrianglePath, indices);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsTrianglePath, uvtData);
ASFUNCTIONBODY_GETTER_SETTER(GraphicsTrianglePath, vertices);

void GraphicsTrianglePath::appendToTokens(tokensVector& tokens)
{
	Graphics::drawTrianglesToTokens(vertices, indices, uvtData, culling, tokens);
}
