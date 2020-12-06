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
	CLASS_SETUP(c, NativeMenuItem, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("caption","",Class<IFunction>::getFunction(c->getSystemState(),NativeMenuItem::_getter_label),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("caption","",Class<IFunction>::getFunction(c->getSystemState(),NativeMenuItem::_setter_label),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,separatorBefore);
	REGISTER_GETTER_SETTER(c,visible);
	REGISTER_GETTER_SETTER(c,enabled);
}

void ContextMenuItem::defaultEventBehavior(Ref<Event> e)
{
	if (e->type == "menuItemSelect" && !callbackfunction.isNull())
	{
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		this->incRef();
		// TODO it is not known what arguments are needed to add to function call
		if (callbackfunction->is<AVM1Function>())
			callbackfunction->as<AVM1Function>()->call(nullptr,&obj,nullptr,0);
		else
		{
			asAtom caller = asAtomHandler::fromObjectNoPrimitive(callbackfunction.getPtr());
			asAtom ret = asAtomHandler::invalidAtom;
			asAtomHandler::callFunction(caller,ret,obj,nullptr,0,false);
		}
	}
	
}

void ContextMenuItem::addToMenu(std::vector<_R<NativeMenuItem> > &items)
{
	if (this->visible)
	{
		if (this->separatorBefore)
		{
			NativeMenuItem* n = Class<NativeMenuItem>::getInstanceSNoArgs(getSystemState());
			n->isSeparator = true;
			items.push_back(_MR(n));
		}
		this->incRef();
		items.push_back(_MR(this));
	}
}

ASFUNCTIONBODY_GETTER_SETTER(ContextMenuItem,separatorBefore);
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuItem,visible);

ASFUNCTIONBODY_ATOM(ContextMenuItem,_constructor)
{
	ContextMenuItem* th=asAtomHandler::as<ContextMenuItem>(obj);
	if (sys->mainClip->usesActionScript3)
		ARG_UNPACK_ATOM(th->label,"")(th->separatorBefore,false)(th->separatorBefore,false)(th->enabled,true)(th->visible,true);
	else
	{
		// contrary to spec constructors without label and callbackfunction are allowed
		ARG_UNPACK_ATOM(th->label,"")(th->callbackfunction,NullRef)(th->separatorBefore,false)(th->separatorBefore,false)(th->enabled,true)(th->visible,true);
	}
	EventDispatcher::_constructor(ret,sys,obj,nullptr,0);
}
