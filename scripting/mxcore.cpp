/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "abc.h"
#include "mxcore.h"
#include "swf.h"
#include "compat.h"
#include "class.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("mx.core");

REGISTER_CLASS_NAME(IFlexDisplayObject);

void IFlexDisplayObject::linkTraits(Class_base* c)
{
	/* properties */
	//TODO

	/* methods */
	lookupAndLink(c,"getBounds","mx.core:IFlexDisplayObject");
	lookupAndLink(c,"getRect","mx.core:IFlexDisplayObject");
	lookupAndLink(c,"globalToLocal","mx.core:IFlexDisplayObject");
	lookupAndLink(c,"hitTestObject","mx.core:IFlexDisplayObject");
	lookupAndLink(c,"hitTestPoint","mx.core:IFlexDisplayObject");
	lookupAndLink(c,"localToGlobal","mx.core:IFlexDisplayObject");
	lookupAndLink(c,"move","mx.core:IFlexDisplayObject");
	lookupAndLink(c,"setActualSize","mx.core:IFlexDisplayObject");
}



