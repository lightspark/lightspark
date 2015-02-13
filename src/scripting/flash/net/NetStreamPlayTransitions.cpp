/**************************************************************************
    Lightspark, a free flash player implementation

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
#include "scripting/flash/net/NetStreamPlayTransitions.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

NetStreamPlayTransitions::NetStreamPlayTransitions(Class_base* c):ASObject(c)
{
	
}

void NetStreamPlayTransitions::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, NULL, CLASS_FINAL);
	c->setVariableByQName("APPEND","",Class<ASString>::getInstanceS("append"),CONSTANT_TRAIT);
	c->setVariableByQName("APPEND_AND_WAIT","",Class<ASString>::getInstanceS("appendAndWait"),CONSTANT_TRAIT);
	c->setVariableByQName("RESET","",Class<ASString>::getInstanceS("reset"),CONSTANT_TRAIT);
	c->setVariableByQName("RESUME","",Class<ASString>::getInstanceS("resume"),CONSTANT_TRAIT);
	c->setVariableByQName("STOP","",Class<ASString>::getInstanceS("stop"),CONSTANT_TRAIT);
	c->setVariableByQName("SWAP","",Class<ASString>::getInstanceS("swap"),CONSTANT_TRAIT);
	c->setVariableByQName("SWITCH","",Class<ASString>::getInstanceS("switch"),CONSTANT_TRAIT);
}

