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

#include "ContextMenuItem.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

void ContextMenuItem::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_FINAL);
	REGISTER_GETTER_SETTER(c, caption);
}

ASFUNCTIONBODY(ContextMenuItem,_constructor)
{
	EventDispatcher::_constructor(obj, NULL, 0);
	LOG(LOG_NOT_IMPLEMENTED,"ContextMenuItem constructor is a stub");
	return NULL;
}

ASFUNCTIONBODY_GETTER_SETTER(ContextMenuItem,caption);

