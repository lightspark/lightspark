/**************************************************************************
    Lightspark, a free flash player implementation

	Copyright (C) 2026  Ludger Krämer <dbluelle@onlinehome.de>

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

#include "avm1IME.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/Stage.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/avm1/avm1rectangle.h"
#include "scripting/avm1/avm1display.h"

using namespace std;
using namespace lightspark;

AVM1IME::AVM1IME(ASWorker* wrk, Class_base* c):ASObject(wrk,c)
{
}

void AVM1IME::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isReusable = true;

	c->setVariableAtomByQName("ALPHANUMERIC_FULL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ALPHANUMERIC_FULL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALPHANUMERIC_HALF",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ALPHANUMERIC_HALF"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CHINESE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"CHINESE"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JAPANESE_HIRAGANA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"JAPANESE_HIRAGANA"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JAPANESE_KATAKANA_FULL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"JAPANESE_KATAKANA_FULL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JAPANESE_KATAKANA_HALF",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"JAPANESE_KATAKANA_HALF"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KOREAN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"KOREAN"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNKNOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"UNKNOWN"),CONSTANT_TRAIT);

	c->prototype->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(AVM1Broadcaster::addListener),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(AVM1Broadcaster::removeListener),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("getEnabled","",c->getSystemState()->getBuiltinFunction(getEnabled),NORMAL_METHOD,false,false);

}
ASFUNCTIONBODY_ATOM(AVM1IME,getEnabled)
{
	LOG(LOG_NOT_IMPLEMENTED,"AVM1 IME not yet supported");
	asAtomHandler::setBool(ret,false);
}
