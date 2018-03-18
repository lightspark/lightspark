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

#include "ContextMenu.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

ContextMenu::ContextMenu(Class_base* c):EventDispatcher(c),customItems(Class<Array>::getInstanceSNoArgs(c->getSystemState())),builtInItems(Class<ContextMenuBuiltInItems>::getInstanceS(c->getSystemState()))
{
	subtype=SUBTYPE_CONTEXTMENU;
}

void ContextMenu::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_FINAL);
	c->setVariableByQName("isSupported","",abstract_b(c->getSystemState(),false),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("hideBuiltInItems","",Class<IFunction>::getFunction(c->getSystemState(),hideBuiltInItems),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,customItems);
	REGISTER_GETTER_SETTER(c,builtInItems);
}

ASFUNCTIONBODY_GETTER_SETTER(ContextMenu,customItems);
ASFUNCTIONBODY_GETTER_SETTER(ContextMenu,builtInItems);

ASFUNCTIONBODY_ATOM(ContextMenu,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);
	LOG(LOG_NOT_IMPLEMENTED,"ContextMenu constructor is a stub");
}

ASFUNCTIONBODY_ATOM(ContextMenu,hideBuiltInItems)
{
	LOG(LOG_NOT_IMPLEMENTED,"ContextMenu hideBuiltInItems is a stub");
}


