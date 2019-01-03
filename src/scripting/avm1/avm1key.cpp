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

#include "scripting/avm1/avm1key.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "backends/input.h"

using namespace std;
using namespace lightspark;

void AVM1Key::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("SPACE",nsNameAndKind(),asAtom(32),CONSTANT_TRAIT);

	c->setDeclaredMethodByQName("isDown","",Class<IFunction>::getFunction(c->getSystemState(),isDown),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(AVM1Key,isDown)
{
	int key;
	ARG_UNPACK_ATOM (key);
	AS3KeyCode c = sys->getInputThread()->getLastKeyDown();
	bool b=false;
	switch (key)
	{
		case 32: //SPACE
			b = c == AS3KEYCODE_SPACE;
			break;
		default:	
			LOG(LOG_NOT_IMPLEMENTED,"AVM1: Key.isDown handling of key "<<key);
	}
	ret.setBool(b);
}

