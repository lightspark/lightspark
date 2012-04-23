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

#ifndef _FLASH_FILTERS_H
#define _FLASH_FILTERS_H

#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class BitmapFilter: public ASObject
{
private:
	virtual BitmapFilter* cloneImpl() const;
public:
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
	ASFUNCTION(clone);
};

class GlowFilter: public BitmapFilter
{
private:
	virtual GlowFilter* cloneImpl() const;
public:
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
};

class DropShadowFilter: public BitmapFilter
{
private:
	virtual DropShadowFilter* cloneImpl() const;
public:
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
};

};

#endif
