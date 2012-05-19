/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "class.h"

#include "flashdesktop.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.desktop");

REGISTER_CLASS_NAME(NativeApplication);

void NativeApplication::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
}

void NativeApplication::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(NativeApplication,_constructor)
{
	return NULL;
}
