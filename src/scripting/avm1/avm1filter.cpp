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

#include "avm1filter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Integer.h"

using namespace std;
using namespace lightspark;

AVM1BitmapFilter::AVM1BitmapFilter(ASWorker* wrk, Class_base* c, CLASS_SUBTYPE st):BitmapFilter(wrk,c,st)
{
}

void AVM1BitmapFilter::sinit(Class_base* c)
{
	BitmapFilter::sinit(c);
}


