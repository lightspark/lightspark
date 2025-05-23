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
#include "ContextMenu.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Undefined.h"

using namespace std;
using namespace lightspark;

void ContextMenuItem::sinit(Class_base* c)
{
	CLASS_SETUP(c, NativeMenuItem, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("caption","",c->getSystemState()->getBuiltinFunction(NativeMenuItem::_getter_label),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("caption","",c->getSystemState()->getBuiltinFunction(NativeMenuItem::_setter_label),SETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c,separatorBefore);
	REGISTER_GETTER_SETTER(c,visible);
	REGISTER_GETTER_SETTER(c,enabled);
}

void ContextMenuItem::finalize()
{
	ASATOM_REMOVESTOREDMEMBER(callbackfunction);
	callbackfunction=asAtomHandler::invalidAtom;
	NativeMenuItem::finalize();
}

bool ContextMenuItem::destruct()
{
	ASATOM_REMOVESTOREDMEMBER(callbackfunction);
	callbackfunction=asAtomHandler::invalidAtom;
	return NativeMenuItem::destruct();
}

void ContextMenuItem::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	NativeMenuItem::prepareShutdown();
	ASObject* o = asAtomHandler::getObject(callbackfunction);
	if (o)
		o->prepareShutdown();
}

bool ContextMenuItem::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = NativeMenuItem::countCylicMemberReferences(gcstate);
	ASObject* o = asAtomHandler::getObject(callbackfunction);
	if (o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

ContextMenuItem::ContextMenuItem(ASWorker* wrk, Class_base* c):NativeMenuItem(wrk,c),callbackfunction(asAtomHandler::invalidAtom)
{
}

ContextMenuItem::~ContextMenuItem()
{
}

void ContextMenuItem::defaultEventBehavior(Ref<Event> e)
{
	if (e->type == "menuItemSelect" && asAtomHandler::isValid(callbackfunction))
	{
		asAtom obj = asAtomHandler::fromObjectNoPrimitive(this);
		this->incRef();
		if (asAtomHandler::is<AVM1Function>(callbackfunction))
		{
			asAtom args[2];
			if (this->menu && this->menu->owner)
			{
				this->menu->owner->incRef();
				args[0]=asAtomHandler::fromObjectNoPrimitive(this->menu->owner);
			}
			else
				args[0]=asAtomHandler::nullAtom;
			this->incRef();
			args[1]=asAtomHandler::fromObjectNoPrimitive(this);
			asAtomHandler::as<AVM1Function>(callbackfunction)->call(nullptr,&obj,args,2);
		}
		else
		{
			asAtom ret = asAtomHandler::invalidAtom;
			asAtomHandler::callFunction(callbackfunction,getInstanceWorker(),ret,obj,nullptr,0,false);
		}
	}
	
}

void ContextMenuItem::addToMenu(std::vector<_R<NativeMenuItem> > &items, ContextMenu* menu)
{
	if (this->visible)
	{
		if (this->separatorBefore)
		{
			NativeMenuItem* n = Class<NativeMenuItem>::getInstanceSNoArgs(getInstanceWorker());
			n->isSeparator = true;
			items.push_back(_MR(n));
		}
		this->incRef();
		items.push_back(_MR(this));
		this->menu = menu;
	}
}

ASFUNCTIONBODY_GETTER_SETTER(ContextMenuItem,separatorBefore)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuItem,visible)

ASFUNCTIONBODY_ATOM(ContextMenuItem,_constructor)
{
	ContextMenuItem* th=asAtomHandler::as<ContextMenuItem>(obj);
	if (wrk->getSystemState()->mainClip->needsActionScript3())
	{
		ARG_CHECK(ARG_UNPACK(th->label,"")(th->separatorBefore,false)(th->separatorBefore,false)(th->enabled,true)(th->visible,true));
	}
	else
	{
		// contrary to spec constructors without label and callbackfunction are allowed
		asAtom callback = asAtomHandler::invalidAtom;
		ARG_CHECK(ARG_UNPACK(th->label,"")(callback,asAtomHandler::undefinedAtom)(th->separatorBefore,false)(th->separatorBefore,false)(th->enabled,true)(th->visible,true));
		ASATOM_ADDSTOREDMEMBER(callback);
		th->callbackfunction=callback;
	}
	EventDispatcher::_constructor(ret,wrk,obj,nullptr,0);
}
