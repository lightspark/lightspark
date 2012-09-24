/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _FLASH_TEXT_ENGINE_H
#define _FLASH_TEXT_ENGINE_H

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/display/flashdisplay.h"
#include "tiny_string.h"
#include "backends/graphics.h"

namespace lightspark
{

class ContentElement: public ASObject
{
public:
	ContentElement(Class_base* c): ASObject(c) {};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class ElementFormat: public ASObject
{
public:
	ElementFormat(Class_base* c): ASObject(c) {};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class FontDescription: public ASObject
{
public:
	FontDescription(Class_base* c): ASObject(c) {};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class FontWeight: public ASObject
{
public:
	FontWeight(Class_base* c): ASObject(c) {};
	static void sinit(Class_base* c);
};

class TextBlock: public ASObject
{
public:
	TextBlock(Class_base* c): ASObject(c) {};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(createTextLine);
	ASPROPERTY_GETTER_SETTER(_NR<ContentElement>, content);
};

class TextElement: public ContentElement
{
public:
	TextElement(Class_base* c): ContentElement(c) {};
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER_SETTER(tiny_string,text);
};

class TextLine: public DisplayObjectContainer, public TextData
{
private:
	// Much of the rendering/bounds checking/hit testing code is
	// similar to TextField and should be shared
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void requestInvalidation(InvalidateQueue* q);
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix);
	void renderImpl(RenderContext& ctxt) const;
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
public:
	TextLine(Class_base* c, _NR<ContentElement> content=NullRef, _NR<TextBlock> owner=NullRef);
	static void sinit(Class_base* c);
	void updateSizes();
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER(_NR<TextBlock>, textBlock);
};
};

#endif
