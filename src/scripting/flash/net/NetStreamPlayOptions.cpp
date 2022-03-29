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

#include "scripting/flash/net/NetStreamPlayOptions.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

NetStreamPlayOptions::NetStreamPlayOptions(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),len(-1),offset(-1),start(-2)
{
	
}
void NetStreamPlayOptions::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	REGISTER_GETTER_SETTER(c,len);
	REGISTER_GETTER_SETTER(c,offset);
	REGISTER_GETTER_SETTER(c,oldStreamName);
	REGISTER_GETTER_SETTER(c,start);
	REGISTER_GETTER_SETTER(c,streamName);
	REGISTER_GETTER_SETTER(c,transition);
}

ASFUNCTIONBODY_GETTER_SETTER(NetStreamPlayOptions,len);
ASFUNCTIONBODY_GETTER_SETTER(NetStreamPlayOptions,offset);
ASFUNCTIONBODY_GETTER_SETTER(NetStreamPlayOptions,oldStreamName);
ASFUNCTIONBODY_GETTER_SETTER(NetStreamPlayOptions,start);
ASFUNCTIONBODY_GETTER_SETTER(NetStreamPlayOptions,streamName);
ASFUNCTIONBODY_GETTER_SETTER(NetStreamPlayOptions,transition);

ASFUNCTIONBODY_ATOM(NetStreamPlayOptions,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, NULL, 0);
}

