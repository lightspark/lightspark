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

#include "scripting/flash/globalization/nationaldigitstype.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void NationalDigitsType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("ARABIC_INDIC",nsNameAndKind(),asAtomHandler::fromUInt(0x0660),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BALINESE",nsNameAndKind(),asAtomHandler::fromUInt(0x1B50),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BENGALI",nsNameAndKind(),asAtomHandler::fromUInt(0x09E6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CHAM",nsNameAndKind(),asAtomHandler::fromUInt(0xAA50),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DEVANAGARI",nsNameAndKind(),asAtomHandler::fromUInt(0x0966),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EUROPEAN",nsNameAndKind(),asAtomHandler::fromUInt(0x0030),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EXTENDED_ARABIC_INDIC",nsNameAndKind(),asAtomHandler::fromUInt(0x06F0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FULL_WIDTH",nsNameAndKind(),asAtomHandler::fromUInt(0xFF10),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GUJARATI",nsNameAndKind(),asAtomHandler::fromUInt(0x0AE6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GURMUKHI",nsNameAndKind(),asAtomHandler::fromUInt(0x0A66),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KANNADA",nsNameAndKind(),asAtomHandler::fromUInt(0x0CE6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KAYAH_LI",nsNameAndKind(),asAtomHandler::fromUInt(0xA900),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KHMER",nsNameAndKind(),asAtomHandler::fromUInt(0x17E0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LAO",nsNameAndKind(),asAtomHandler::fromUInt(0x0ED0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEPCHA",nsNameAndKind(),asAtomHandler::fromUInt(0x1C40),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LIMBU",nsNameAndKind(),asAtomHandler::fromUInt(0x1946),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MALAYALAM",nsNameAndKind(),asAtomHandler::fromUInt(0x0D66),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MONGOLIAN",nsNameAndKind(),asAtomHandler::fromUInt(0x1810),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MYANMAR",nsNameAndKind(),asAtomHandler::fromUInt(0x1040),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MYANMAR_SHAN",nsNameAndKind(),asAtomHandler::fromUInt(0x1090),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NEW_TAI_LUE",nsNameAndKind(),asAtomHandler::fromUInt(0x19D0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NKO",nsNameAndKind(),asAtomHandler::fromUInt(0x07C0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OL_CHIKI",nsNameAndKind(),asAtomHandler::fromUInt(0x1C50),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ORIYA",nsNameAndKind(),asAtomHandler::fromUInt(0x0B66),CONSTANT_TRAIT);
	c->setVariableAtomByQName("OSMANYA",nsNameAndKind(),asAtomHandler::fromUInt(0x104A0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SAURASHTRA",nsNameAndKind(),asAtomHandler::fromUInt(0xA8D0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SUNDANESE",nsNameAndKind(),asAtomHandler::fromUInt(0x1BB0),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TAMIL",nsNameAndKind(),asAtomHandler::fromUInt(0x0BE6),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TELUGU",nsNameAndKind(),asAtomHandler::fromUInt(0x0C66),CONSTANT_TRAIT);
	c->setVariableAtomByQName("THAI",nsNameAndKind(),asAtomHandler::fromUInt(0x0E50),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TIBETAN",nsNameAndKind(),asAtomHandler::fromUInt(0x0F20),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VAI",nsNameAndKind(),asAtomHandler::fromUInt(0xA620),CONSTANT_TRAIT);
}

