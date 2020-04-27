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

#include "scripting/flash/media/h264level.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void H264Level::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("LEVEL_1",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"1"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_1_1",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"1.1"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_1_2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"1.2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_1_3",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"1.3"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_1B",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"1b"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_2_1",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"2.1"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_2_2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"2.2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_3",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"3"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_3_1",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"3.1"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_3_2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"3.2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_4",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"4"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_4_1",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"4.1"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_4_2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"4.2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_5",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"5"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEVEL_5_1",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"5.1"),CONSTANT_TRAIT);
}