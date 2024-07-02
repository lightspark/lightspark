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
#include "scripting/toplevel/Array.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void TextRenderer::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);

	c->setDeclaredMethodByQName("setAdvancedAntiAliasingTable","",c->getSystemState()->getBuiltinFunction(_setAdvancedAntiAliasingTable),NORMAL_METHOD,false);

    c->setDeclaredMethodByQName("displayMode","",c->getSystemState()->getBuiltinFunction(_getDisplayMode),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("displayMode","",c->getSystemState()->getBuiltinFunction(_setDisplayMode),SETTER_METHOD,false);

    c->setDeclaredMethodByQName("maxLevel","",c->getSystemState()->getBuiltinFunction(_getMaxLevel),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("maxLevel","",c->getSystemState()->getBuiltinFunction(_setMaxLevel),SETTER_METHOD,false);
}

ASFUNCTIONBODY_ATOM(TextRenderer,_setAdvancedAntiAliasingTable)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextRenderer.setAdvancedAntiAliasingTable is not implemented");
	tiny_string fontName, fontStyle, colorType;
	_NR<Array> advancedAntiAliasingTable;
	ARG_CHECK(ARG_UNPACK(fontName)(fontStyle)(colorType)(advancedAntiAliasingTable));
}

ASFUNCTIONBODY_ATOM(TextRenderer,_getDisplayMode)
{
    LOG(LOG_NOT_IMPLEMENTED,"TextRenderer.displayMode is not fully implemented and always returns \"default\"");
	ret = asAtomHandler::fromString(wrk->getSystemState(),"default"); // "default" is default value
}

ASFUNCTIONBODY_ATOM(TextRenderer,_setDisplayMode)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextRenderer.displayMode is not implemented");
	tiny_string displayMode;
	ARG_CHECK(ARG_UNPACK(displayMode));
}

ASFUNCTIONBODY_ATOM(TextRenderer,_getMaxLevel)
{
    LOG(LOG_NOT_IMPLEMENTED,"TextRenderer.maxLevel is not fully implemented and always returns 4");
	uint32_t maxLevel = 4;	// 4 is default value
	ret = asAtomHandler::fromUInt(maxLevel);
}

ASFUNCTIONBODY_ATOM(TextRenderer,_setMaxLevel)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextRenderer.maxLevel is not implemented");
	uint32_t maxLevel;
    ARG_CHECK(ARG_UNPACK(maxLevel));
}
