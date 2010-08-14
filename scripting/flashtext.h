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

#ifndef _FLASH_TEXT_H
#define _FLASH_TEXT_H

#include "compat.h"
#include "asobject.h"
#include "flashdisplay.h"

namespace lightspark
{

class Font: public ASObject
{
public:
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
	ASFUNCTION(enumerateFonts);
};

class TextField: public DisplayObject
{
private:
	intptr_t width;
	intptr_t height;
public:
	TextField():width(0),height(0){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void Render();
	ASFUNCTION(_getWidth);
	ASFUNCTION(_setWidth);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_setHeight);
};

class StyleSheet: public EventDispatcher
{
private:
	std::map<tiny_string, ASObject*> styles;
public:
	StyleSheet(){}
	ASFUNCTION(setStyle);
	ASFUNCTION(_getStyleNames);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

};

#endif
