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

ContextMenu::ContextMenu(Class_base* c):EventDispatcher(c),isSupported(true),customItems(Class<Array>::getInstanceSNoArgs(c->getSystemState())),builtInItems(Class<ContextMenuBuiltInItems>::getInstanceS(c->getSystemState()))
{
	subtype=SUBTYPE_CONTEXTMENU;
}

void ContextMenu::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_FINAL);
	REGISTER_GETTER_RESULTTYPE(c,isSupported,Boolean);
	c->setDeclaredMethodByQName("hideBuiltInItems","",Class<IFunction>::getFunction(c->getSystemState(),hideBuiltInItems),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,customItems);
	REGISTER_GETTER_SETTER(c,builtInItems);
}

void ContextMenu::getCurrentContextMenuItems(std::vector<_R<NativeMenuItem> >& items)
{
	if (!customItems.isNull() && customItems->size())
	{
		for (uint32_t i = 0; i < customItems->size() && i < 15; i++)
		{
			// TODO check for reserved labels?
			asAtom item = asAtomHandler::invalidAtom;;
			customItems->at_nocheck(item,i);
			if (asAtomHandler::is<NativeMenuItem>(item))
				asAtomHandler::as<NativeMenuItem>(item)->addToMenu(items);
		}
		NativeMenuItem* n = Class<NativeMenuItem>::getInstanceSNoArgs(getSystemState());
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	getVisibleBuiltinContextMenuItems(this,items,getSystemState());
}

void ContextMenu::getVisibleBuiltinContextMenuItems(ContextMenu *m, std::vector<Ref<NativeMenuItem> >& items, SystemState* sys)
{
	NativeMenuItem* n;
	if (!m || m->builtInItems->save)
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Save";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItems->zoom)
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Zoom In";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Zoom Out";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="100%";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Show all";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItems->quality)
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Quality";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItems->play)
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Play";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Loop";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItems->forwardAndBack)
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Rewind";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Forward";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Back";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItems->print)
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->label="Print";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
	n->label="Settings";
	items.push_back(_MR(n));
	n = Class<NativeMenuItem>::getInstanceSNoArgs(sys);
	n->label="About";
	items.push_back(_MR(n));
}

ASFUNCTIONBODY_GETTER(ContextMenu,isSupported);
ASFUNCTIONBODY_GETTER_SETTER(ContextMenu,customItems);
ASFUNCTIONBODY_GETTER_SETTER(ContextMenu,builtInItems);

ASFUNCTIONBODY_ATOM(ContextMenu,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);
}

ASFUNCTIONBODY_ATOM(ContextMenu,hideBuiltInItems)
{
	ContextMenu* th=asAtomHandler::as<ContextMenu>(obj);
	th->builtInItems->forwardAndBack=false;
	th->builtInItems->loop=false;
	th->builtInItems->play=false;
	th->builtInItems->print=false;
	th->builtInItems->quality=false;
	th->builtInItems->rewind=false;
	th->builtInItems->save=false;
	th->builtInItems->zoom=false;
}
ASFUNCTIONBODY_ATOM(ContextMenu,clone)
{
	ContextMenu* th=asAtomHandler::as<ContextMenu>(obj);
	ContextMenu* res = Class<ContextMenu>::getInstanceSNoArgs(sys);
	if (th->builtInItems)
	{
		res->builtInItems->forwardAndBack=th->builtInItems->forwardAndBack;
		res->builtInItems->loop=th->builtInItems->loop;
		res->builtInItems->play=th->builtInItems->play;
		res->builtInItems->print=th->builtInItems->print;
		res->builtInItems->quality=th->builtInItems->quality;
		res->builtInItems->rewind=th->builtInItems->rewind;
		res->builtInItems->save=th->builtInItems->save;
		res->builtInItems->zoom=th->builtInItems->zoom;
	}
	if (th->customItems)
	{
		for (uint32_t i=0; i < th->customItems->size(); i++)
		{
			asAtom a=asAtomHandler::invalidAtom;
			th->customItems->at_nocheck(a,i);
			res->customItems->push(a);
		}
	}
	ret = asAtomHandler::fromObjectNoPrimitive(res);
}

