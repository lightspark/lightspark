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

#include "scripting/flash/text/textrenderer.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void TextRenderer::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);

    c->setDeclaredMethodByQName("displayMode","",Class<IFunction>::getFunction(c->getSystemState(),_getDisplayMode),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("displayMode","",Class<IFunction>::getFunction(c->getSystemState(),_setDisplayMode),SETTER_METHOD,false);

    c->setDeclaredMethodByQName("maxLevel","",Class<IFunction>::getFunction(c->getSystemState(),_getMaxLevel),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("maxLevel","",Class<IFunction>::getFunction(c->getSystemState(),_setMaxLevel),SETTER_METHOD,false);
}

ASFUNCTIONBODY_ATOM(TextRenderer,_getDisplayMode)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextRenderer.displayMode does not change rendering of anti-aliasing");
    TextRenderer* th = asAtomHandler::as<TextRenderer>(obj);
    ret = asAtomHandler::fromString(sys,th->displayMode);
}

ASFUNCTIONBODY_ATOM(TextRenderer,_setDisplayMode)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextRenderer.displayMode does not change rendering of anti-aliasing");
    TextRenderer* th = asAtomHandler::as<TextRenderer>(obj);
    ARG_UNPACK_ATOM(th->displayMode);
}

ASFUNCTIONBODY_ATOM(TextRenderer,_getMaxLevel)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextRenderer.displayMode does not change quality level of anti-aliasing");
    TextRenderer* th = asAtomHandler::as<TextRenderer>(obj);
    //ret = asAtomHandler::fromInt(sys,th->maxLevel);
}

ASFUNCTIONBODY_ATOM(TextRenderer,_setMaxLevel)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextRenderer.displayMode does not change quality level of anti-aliasing");
    TextRenderer* th = asAtomHandler::as<TextRenderer>(obj);
    ARG_UNPACK_ATOM(th->maxLevel);
}