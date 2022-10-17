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
#include "ContextMenuBuiltInItems.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

ContextMenuBuiltInItems::ContextMenuBuiltInItems(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_CONTEXTMENUBUILTINITEMS),
	forwardAndBack(true),loop(true),play(true),print(true),quality(true),rewind(true),save(true),zoom(true)
{
}

void ContextMenuBuiltInItems::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	REGISTER_GETTER_SETTER(c,forwardAndBack);
	REGISTER_GETTER_SETTER(c,loop);
	REGISTER_GETTER_SETTER(c,play);
	REGISTER_GETTER_SETTER(c,print);
	REGISTER_GETTER_SETTER(c,quality);
	REGISTER_GETTER_SETTER(c,rewind);
	REGISTER_GETTER_SETTER(c,save);
	REGISTER_GETTER_SETTER(c,zoom);
}

ASFUNCTIONBODY_GETTER_SETTER(ContextMenuBuiltInItems,forwardAndBack)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuBuiltInItems,loop)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuBuiltInItems,play)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuBuiltInItems,print)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuBuiltInItems,quality)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuBuiltInItems,rewind)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuBuiltInItems,save)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuBuiltInItems,zoom)

ASFUNCTIONBODY_ATOM(ContextMenuBuiltInItems,_constructor)
{
}


