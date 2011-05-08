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

#ifndef _MXCORE_H
#define _MXCORE_H

#include "compat.h"
#include "asobject.h"
#include "toplevel.h"
#include "flashevents.h"

namespace lightspark
{

class IFlexDisplayObject: public IEventDispatcher /*,public IBitmapDrawable*/
{
public:
	static void linkTraits(Class_base* c);
};

}
#endif /* _MXCORE_H */
