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
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/toplevel/Array.h"

using namespace std;
using namespace lightspark;

ContextMenu::ContextMenu(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),isSupported(true),customItems(Class<Array>::getInstanceSNoArgs(wrk)),builtInItems(Class<ContextMenuBuiltInItems>::getInstanceS(wrk)),owner(nullptr)
{
	subtype=SUBTYPE_CONTEXTMENU;
}

void ContextMenu::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_FINAL);
	REGISTER_GETTER_RESULTTYPE(c,isSupported,Boolean);
	c->setDeclaredMethodByQName("hideBuiltInItems","",c->getSystemState()->getBuiltinFunction(hideBuiltInItems),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,customItems,Array);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,builtInItems,ContextMenuBuiltInItems);
}

void ContextMenu::finalize()
{
	customItems.reset();
	builtInItems.reset();
	EventDispatcher::finalize();
}

bool ContextMenu::destruct()
{
	customItems.reset();
	builtInItems.reset();
	return EventDispatcher::destruct();
}

void ContextMenu::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	if (customItems)
		customItems->prepareShutdown();
	if(builtInItems)
		builtInItems->prepareShutdown();
	
}

bool ContextMenu::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	if (customItems)
		ret = customItems->countAllCylicMemberReferences(gcstate) || ret;
	if (builtInItems)
		ret = builtInItems->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void ContextMenu::getCurrentContextMenuItems(std::vector<_R<NativeMenuItem> >& items)
{
	if (!customItems.isNull() && customItems->size())
	{
		bool checkReservedLabels = getSystemState()->mainClip != nullptr && getSystemState()->mainClip->needsActionScript3();
		for (uint32_t i = 0; i < customItems->size() && i < 15; i++)
		{
			asAtom item = asAtomHandler::invalidAtom;;
			customItems->at_nocheck(item,i);
			if (asAtomHandler::is<NativeMenuItem>(item))
			{
				if (!checkReservedLabels || !asAtomHandler::as<NativeMenuItem>(item)->isReservedLabel())
						asAtomHandler::as<NativeMenuItem>(item)->addToMenu(items,this);
			}
		}
		NativeMenuItem* n = Class<NativeMenuItem>::getInstanceSNoArgs(getInstanceWorker());
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	getVisibleBuiltinContextMenuItems(this,items,getInstanceWorker());
}

bool ContextMenu::builtInItemEnabled(const tiny_string& name)
{
	if (name == "save")
		return builtInItems->save;
	else if (name == "zoom")
		return builtInItems->zoom;
	else if (name == "quality")
		return builtInItems->quality;
	else if (name == "play")
		return builtInItems->play;
	else if (name == "forwardAndBack")
		return builtInItems->forwardAndBack;
	else if (name == "print")
		return builtInItems->print;
	return false;
}

void ContextMenu::getVisibleBuiltinContextMenuItems(ContextMenu *m, std::vector<Ref<NativeMenuItem> >& items, ASWorker* worker)
{
	NativeMenuItem* n;
	if (!m || m->builtInItemEnabled("save"))
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Save";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItemEnabled("zoom"))
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Zoom In";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Zoom Out";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="100%";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Show all";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItemEnabled("quality"))
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Quality";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItemEnabled("play"))
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Play";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Loop";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItemEnabled("forwardAndBack"))
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Rewind";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Forward";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Back";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	if (!m || m->builtInItemEnabled("print"))
	{
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->label="Print";
		items.push_back(_MR(n));
		n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
		n->isSeparator = true;
		items.push_back(_MR(n));
	}
	n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
	n->label="Settings";
	items.push_back(_MR(n));
	n = Class<NativeMenuItem>::getInstanceSNoArgs(worker);
	n->label="About";
	items.push_back(_MR(n));
}

ASFUNCTIONBODY_GETTER(ContextMenu,isSupported)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenu,customItems)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenu,builtInItems)

ASFUNCTIONBODY_ATOM(ContextMenu,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
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
	ContextMenu* res = Class<ContextMenu>::getInstanceSNoArgs(wrk);
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
			ASATOM_INCREF(a);
			res->customItems->push(a);
		}
	}
	ret = asAtomHandler::fromObjectNoPrimitive(res);
}

