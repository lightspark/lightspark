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

#include "scripting/flash/desktop/clipboardformats.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void ClipboardFormats::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("BITMAP_FORMAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"air:bitmap"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FILE_LIST_FORMAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"air:file list"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FILE_PROMISE_LIST_FORMAT ",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"air:file promise list"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HTML_FORMAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"air:html"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RICH_TEXT_FORMAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"air:rtf"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TEXT_FORMAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"air:text"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("URL_FORMAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"air:url"),CONSTANT_TRAIT);
}