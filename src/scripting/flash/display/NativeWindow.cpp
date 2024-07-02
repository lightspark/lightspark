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

#include "NativeWindow.h"
#include "scripting/toplevel/Number.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "platforms/engineutils.h"
#include "abc.h"

using namespace lightspark;

NativeWindow::NativeWindow(ASWorker* wrk, Class_base* c):EventDispatcher(wrk,c),width(0),height(0),x(0),y(0)
{
	subtype = SUBTYPE_NATIVEWINDOW;
}

void NativeWindow::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,width,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,height,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,x,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,y,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,bounds,Rectangle);
	c->setDeclaredMethodByQName("startMove","",c->getSystemState()->getBuiltinFunction(startMove,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(NativeWindow,x)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(NativeWindow,y)

ASFUNCTIONBODY_ATOM(NativeWindow,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"class NativeWindow is a stub");
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
}

ASFUNCTIONBODY_ATOM(NativeWindow,_getter_bounds)
{
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	NativeWindow* th=asAtomHandler::as<NativeWindow>(obj);
	if (th == wrk->getSystemState()->stage->nativeWindow.getPtr())
	{
		Rectangle *bounds=Class<Rectangle>::getInstanceS(wrk);
		bounds->x = wrk->getSystemState()->getEngineData()->x;
		bounds->y = wrk->getSystemState()->getEngineData()->y;
		bounds->width = wrk->getSystemState()->getEngineData()->width;
		bounds->height = wrk->getSystemState()->getEngineData()->height;
		ret = asAtomHandler::fromObject(bounds);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"nativeWindow.bounds getter for non-main window");
		ret = asAtomHandler::nullAtom;
	}
}
ASFUNCTIONBODY_ATOM(NativeWindow,_setter_bounds)
{
	NativeWindow* th=asAtomHandler::as<NativeWindow>(obj);
	_NR<Rectangle> bounds;
	ARG_CHECK(ARG_UNPACK(bounds));
	
	if (th == wrk->getSystemState()->stage->nativeWindow.getPtr())
	{
		if (bounds)
		{
			if (bounds->x >= 0 
				&& bounds->y >= 0 
				&& bounds->width > 0 
				&& bounds->height > 0)
				wrk->getSystemState()->getEngineData()->setWindowPosition(bounds->x,bounds->y,bounds->width,bounds->height);
		}
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"nativeWindow.bounds setter for non-main window");
	}
}
ASFUNCTIONBODY_ATOM(NativeWindow,_getter_width)
{
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	NativeWindow* th=asAtomHandler::as<NativeWindow>(obj);
	if (th == wrk->getSystemState()->stage->nativeWindow.getPtr())
	{
		ret = asAtomHandler::fromUInt(wrk->getSystemState()->getEngineData()->width);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"nativeWindow.width getter for non-main window");
		ret = asAtomHandler::fromUInt(0);
	}
}
ASFUNCTIONBODY_ATOM(NativeWindow,_setter_width)
{
	NativeWindow* th=asAtomHandler::as<NativeWindow>(obj);
	number_t _width;
	ARG_CHECK(ARG_UNPACK(_width));
	
	if (th == wrk->getSystemState()->stage->nativeWindow.getPtr())
	{
		if (_width > 0)
			wrk->getSystemState()->getEngineData()->setWindowPosition(wrk->getSystemState()->getEngineData()->x,wrk->getSystemState()->getEngineData()->y,_width,wrk->getSystemState()->getEngineData()->height);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"nativeWindow.width setter for non-main window");
	}
}
ASFUNCTIONBODY_ATOM(NativeWindow,_getter_height)
{
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Arguments provided in getter");
		return;
	}
	NativeWindow* th=asAtomHandler::as<NativeWindow>(obj);
	if (th == wrk->getSystemState()->stage->nativeWindow.getPtr())
	{
		ret = asAtomHandler::fromUInt(wrk->getSystemState()->getEngineData()->height);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"nativeWindow.height getter for non-main window");
		ret = asAtomHandler::fromUInt(0);
	}
}
ASFUNCTIONBODY_ATOM(NativeWindow,_setter_height)
{
	NativeWindow* th=asAtomHandler::as<NativeWindow>(obj);
	number_t _height;
	ARG_CHECK(ARG_UNPACK(_height));
	
	if (th == wrk->getSystemState()->stage->nativeWindow.getPtr())
	{
		if (_height > 0)
			wrk->getSystemState()->getEngineData()->setWindowPosition(wrk->getSystemState()->getEngineData()->x,wrk->getSystemState()->getEngineData()->y,wrk->getSystemState()->getEngineData()->width,_height);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"nativeWindow.width setter for non-main window");
	}
}
ASFUNCTIONBODY_ATOM(NativeWindow,startMove)
{
	NativeWindow* th=asAtomHandler::as<NativeWindow>(obj);
	if (th != wrk->getSystemState()->stage->nativeWindow.getPtr())
	{
		LOG(LOG_NOT_IMPLEMENTED,"NativeWindow.startMove for non-main window");
		ret = asAtomHandler::falseAtom;
	}
	else if (wrk->getSystemState()->getEngineData()->inFullScreenMode())
	{
		ret = asAtomHandler::falseAtom;
	}
	else if (wrk->getSystemState()->getInMouseEvent())
	{
		wrk->getSystemState()->setWindowMoveMode(true);
		ret = asAtomHandler::trueAtom;
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"NativeWindow.startMove from outside MouseEvent");
		ret = asAtomHandler::falseAtom;
	}
}
