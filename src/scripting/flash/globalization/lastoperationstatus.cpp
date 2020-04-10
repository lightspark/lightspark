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

#include "scripting/flash/globalization/lastoperationstatus.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void LastOperationStatus::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("BUFFER_OVERFLOW_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bufferOverflowError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ERROR_CODE_UNKNOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"errorCodeUnknown"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ILLEGAL_ARGUMENT_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"illegalArgumentError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INDEX_OUT_OF_BOUNDS_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"indexOutOfBoundsError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVALID_ATTR_VALUE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invalidAttrValue"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVALID_CHAR_FOUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invalidCharFound"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MEMORY_ALLOCATION_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"memoryAllocationError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"noError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_OVERFLOW_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"numberOverflowError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PARSE_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"parseError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PATTERN_SYNTAX_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"patternSyntaxError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PLATFORM_API_FAILED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"platformAPIFailed"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TRUNCATED_CHAR_FOUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"truncatedCharFound"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNEXPECTED_TOKEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"unexpectedToken"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNSUPPORTED_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"unsupportedError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("USING_DEFAULT_WARNING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"usingDefaultWarning"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("USING_FALLBACK_WARNING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"usingFallbackWarning"),CONSTANT_TRAIT);
}