/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_FILTERS_FLASHFILTERS_H
#define SCRIPTING_FLASH_FILTERS_FLASHFILTERS_H 1

#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class BitmapFilter: public ASObject
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	BitmapFilter(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
	ASFUNCTION(clone);
};

class GlowFilter: public BitmapFilter
{
private:
	virtual GlowFilter* cloneImpl() const;
public:
	GlowFilter(Class_base* c):BitmapFilter(c){}
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
};

class DropShadowFilter: public BitmapFilter
{
private:
	virtual DropShadowFilter* cloneImpl() const;
public:
	DropShadowFilter(Class_base* c):BitmapFilter(c){}
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
};

};

#endif /* SCRIPTING_FLASH_FILTERS_FLASHFILTERS_H */
