/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_AVM1_AVM1FILTER_H
#define SCRIPTING_AVM1_AVM1FILTER_H


#include "scripting/flash/filters/flashfilters.h"

namespace lightspark
{

class AVM1BitmapFilter: public BitmapFilter
{
public:
	AVM1BitmapFilter(ASWorker* wrk,Class_base* c, CLASS_SUBTYPE st=SUBTYPE_BITMAPFILTER);
	static void sinit(Class_base* c);
};

}
#endif // SCRIPTING_AVM1_AVM1FILTER_H
