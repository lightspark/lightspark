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

#ifndef _FLASH_TEXT_ENGINE_H
#define _FLASH_TEXT_ENGINE_H

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/display/flashdisplay.h"
#include "tiny_string.h"
#include "backends/graphics.h"

namespace lightspark
{

class ElementFormat;
class ContentElement: public ASObject
{
public:
	ContentElement(Class_base* c): ASObject(c),elementFormat(NULL) {}
	static void sinit(Class_base* c);
	ASPROPERTY_GETTER_SETTER(_NR<ElementFormat>,elementFormat);
};

class FontDescription;
class ElementFormat: public ASObject
{
public:
	ElementFormat(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER_SETTER(tiny_string,alignmentBaseline);
	ASPROPERTY_GETTER_SETTER(number_t,alpha);
	ASPROPERTY_GETTER_SETTER(number_t,baselineShift);
	ASPROPERTY_GETTER_SETTER(tiny_string,breakOpportunity);
	ASPROPERTY_GETTER_SETTER(uint,color);
	ASPROPERTY_GETTER_SETTER(tiny_string,digitCase);
	ASPROPERTY_GETTER_SETTER(tiny_string,digitWidth);
	ASPROPERTY_GETTER_SETTER(tiny_string,dominantBaseline);
	ASPROPERTY_GETTER_SETTER(_NR<FontDescription>,fontDescription);
	ASPROPERTY_GETTER_SETTER(number_t,fontSize);
	ASPROPERTY_GETTER_SETTER(tiny_string,kerning);
	ASPROPERTY_GETTER_SETTER(tiny_string,ligatureLevel);
	ASPROPERTY_GETTER_SETTER(tiny_string,locale);
	ASPROPERTY_GETTER_SETTER(bool,locked);
	ASPROPERTY_GETTER_SETTER(tiny_string,textRotation);
	ASPROPERTY_GETTER_SETTER(number_t,trackingLeft);
	ASPROPERTY_GETTER_SETTER(number_t,trackingRight);
	ASPROPERTY_GETTER_SETTER(tiny_string,typographicCase);
};

class FontLookup: public ASObject
{
public:
	FontLookup(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};

class FontDescription: public ASObject
{
public:
	FontDescription(Class_base* c): ASObject(c), 
		cffHinting("horizontalStem"), fontLookup("device"), fontName("_serif"), fontPosture("normal"), fontWeight("normal"),locked(false), renderingMode("cff") {}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER_SETTER(tiny_string,cffHinting);
	ASPROPERTY_GETTER_SETTER(tiny_string,fontLookup);
	ASPROPERTY_GETTER_SETTER(tiny_string,fontName);
	ASPROPERTY_GETTER_SETTER(tiny_string,fontPosture);
	ASPROPERTY_GETTER_SETTER(tiny_string,fontWeight);
	ASPROPERTY_GETTER_SETTER(bool,locked);
	ASPROPERTY_GETTER_SETTER(tiny_string,renderingMode);
};

class FontWeight: public ASObject
{
public:
	FontWeight(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};

class FontMetrics: public ASObject
{
public:
	FontMetrics(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class Kerning: public ASObject
{
public:
	Kerning(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class LineJustification: public ASObject
{
public:
	LineJustification(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};

class TextBaseline: public ASObject
{
public:
	TextBaseline(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};

class TextJustifier: public ASObject
{
public:
	TextJustifier(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class SpaceJustifier: public TextJustifier
{
public:
	SpaceJustifier(Class_base* c): TextJustifier(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class EastAsianJustifier: public TextJustifier
{
public:
	EastAsianJustifier(Class_base* c): TextJustifier(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
};

class TextBlock: public ASObject
{
public:
	TextBlock(Class_base* c): ASObject(c),bidiLevel(0) {}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(createTextLine);
	ASFUNCTION(recreateTextLine);
	ASPROPERTY_GETTER_SETTER(_NR<ContentElement>, content);
	ASPROPERTY_GETTER_SETTER(_NR<TextJustifier>, textJustifier);
	ASPROPERTY_GETTER_SETTER(int,bidiLevel);
};

class TextElement: public ContentElement
{
public:
	TextElement(Class_base* c): ContentElement(c) {}
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
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
public:
	TextLine(Class_base* c,tiny_string linetext = "", _NR<TextBlock> owner=NullRef);
	static void sinit(Class_base* c);
	void updateSizes();
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER(_NR<TextBlock>, textBlock);
	ASPROPERTY_GETTER(_NR<TextLine>, nextLine);
	ASPROPERTY_GETTER(_NR<TextLine>, previousLine);
	ASPROPERTY_GETTER_SETTER(tiny_string,validity);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,userData);
	ASFUNCTION(getDescent);
	ASFUNCTION(getAscent);
	ASFUNCTION(getTextWidth);
	ASFUNCTION(getTextHeight);

};

class TextLineValidity: public ASObject
{
public:
	TextLineValidity(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};

}

#endif
