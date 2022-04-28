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
#include "scripting/class.h"
#include "scripting/argconv.h"
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
}
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(NativeWindow,width)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(NativeWindow,height)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(NativeWindow,x)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(NativeWindow,y)

ASFUNCTIONBODY_ATOM(NativeWindow,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"class NativeWindow is a stub");
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
}

