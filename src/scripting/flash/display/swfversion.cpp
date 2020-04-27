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

#include "scripting/flash/display/swfversion.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void SWFVersion::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("FLASH1",nsNameAndKind(),asAtomHandler::fromUInt(1),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH10",nsNameAndKind(),asAtomHandler::fromUInt(10),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH11",nsNameAndKind(),asAtomHandler::fromUInt(11),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH12",nsNameAndKind(),asAtomHandler::fromUInt(12),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH2",nsNameAndKind(),asAtomHandler::fromUInt(2),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH3",nsNameAndKind(),asAtomHandler::fromUInt(3),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH4",nsNameAndKind(),asAtomHandler::fromUInt(4),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH5",nsNameAndKind(),asAtomHandler::fromUInt(5),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH6",nsNameAndKind(),asAtomHandler::fromUInt(6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH7",nsNameAndKind(),asAtomHandler::fromUInt(7),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH8",nsNameAndKind(),asAtomHandler::fromUInt(8),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLASH9",nsNameAndKind(),asAtomHandler::fromUInt(9),CONSTANT_TRAIT);
}