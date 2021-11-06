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
	ContentElement(Class_base* c): ASObject(c,T_OBJECT,SUBTYPE_CONTENTELEMENT),elementFormat(NULL) {}
	static void sinit(Class_base* c);
	ASPROPERTY_GETTER(tiny_string,rawText);
	ASPROPERTY_GETTER_SETTER(_NR<ElementFormat>,elementFormat);
};

class FontDescription;
class ElementFormat: public ASObject
{
public:
	ElementFormat(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_clone);
	ASPROPERTY_GETTER_SETTER(tiny_string,alignmentBaseline);
	ASPROPERTY_GETTER_SETTER(number_t,alpha);
	ASPROPERTY_GETTER_SETTER(number_t,baselineShift);
	ASPROPERTY_GETTER_SETTER(tiny_string,breakOpportunity);
	ASPROPERTY_GETTER_SETTER(unsigned int,color);
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
	FontDescription(Class_base* c): ASObject(c,T_OBJECT,SUBTYPE_FONTDESCRIPTION), 
		cffHinting("horizontalStem"), fontLookup("device"), fontName("_serif"), fontPosture("normal"), fontWeight("normal"),locked(false), renderingMode("cff") {}
	static void sinit(Class_base* c);
	bool destruct();
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_clone);
	ASFUNCTION_ATOM(isFontCompatible);
	ASPROPERTY_GETTER_SETTER(tiny_string,cffHinting);
	ASPROPERTY_GETTER_SETTER(tiny_string,fontLookup);
	ASPROPERTY_GETTER_SETTER(tiny_string,fontName);
	ASPROPERTY_GETTER_SETTER(tiny_string,fontPosture);
	ASPROPERTY_GETTER_SETTER(tiny_string,fontWeight);
	ASPROPERTY_GETTER_SETTER(bool,locked);
	ASPROPERTY_GETTER_SETTER(tiny_string,renderingMode);
};

class FontPosture: public ASObject
{
public:
	FontPosture(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
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
	ASFUNCTION_ATOM(_constructor);
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
	ASFUNCTION_ATOM(_constructor);
};

class SpaceJustifier: public TextJustifier
{
public:
	SpaceJustifier(Class_base* c): TextJustifier(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class EastAsianJustifier: public TextJustifier
{
public:
	EastAsianJustifier(Class_base* c): TextJustifier(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class TextLine;
class TextBlock: public ASObject
{
public:
	TextBlock(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(createTextLine);
	ASFUNCTION_ATOM(recreateTextLine);
	ASFUNCTION_ATOM(releaseLines);
	ASPROPERTY_GETTER_SETTER(bool,applyNonLinearFontScaling);
	ASPROPERTY_GETTER_SETTER(_NR<FontDescription>, baselineFontDescription);
	ASPROPERTY_GETTER_SETTER(number_t,baselineFontSize);
	ASPROPERTY_GETTER_SETTER(tiny_string,baselineZero);
	ASPROPERTY_GETTER_SETTER(int,bidiLevel);
	ASPROPERTY_GETTER_SETTER(_NR<ContentElement>, content);
	ASPROPERTY_GETTER(_NR<TextLine>, firstInvalidLine );
	ASPROPERTY_GETTER(_NR<TextLine>, firstLine);
	ASPROPERTY_GETTER(_NR<TextLine>, lastLine);
	ASPROPERTY_GETTER_SETTER(tiny_string,lineRotation);
	ASPROPERTY_GETTER_SETTER(_NR<TextJustifier>, textJustifier);
	ASPROPERTY_GETTER_SETTER(_NR<Vector>, tabStops);
	ASPROPERTY_GETTER(tiny_string, textLineCreationResult);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>, userData);
};

class TextElement: public ContentElement
{
private:
	void settext_cb(tiny_string oldValue);
public:
	TextElement(Class_base* c): ContentElement(c) { subtype = SUBTYPE_TEXTELEMENT; }
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(replaceText);
	ASPROPERTY_GETTER_SETTER(tiny_string,text);
	
};
class GroupElement: public ContentElement
{
public:
	GroupElement(Class_base* c): ContentElement(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};
class TextLine: public DisplayObjectContainer, public TextData
{
private:
	// Much of the rendering/bounds checking/hit testing code is
	// similar to TextField and should be shared
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const override;
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap) override;
	bool renderImpl(RenderContext& ctxt) const override;
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
public:
	TextLine(Class_base* c,tiny_string linetext = "", _NR<TextBlock> owner=NullRef);
	static void sinit(Class_base* c);
	void updateSizes();
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER(_NR<TextBlock>, textBlock);
	ASPROPERTY_GETTER(_NR<TextLine>, nextLine);
	ASPROPERTY_GETTER(_NR<TextLine>, previousLine);
	ASPROPERTY_GETTER_SETTER(tiny_string,validity);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,userData);
	ASFUNCTION_ATOM(getDescent);
	ASFUNCTION_ATOM(getAscent);
	ASFUNCTION_ATOM(getTextWidth);
	ASFUNCTION_ATOM(getTextHeight);
	ASFUNCTION_ATOM(getBaselinePosition);
	ASFUNCTION_ATOM(getUnjustifiedTextWidth);
	ASPROPERTY_GETTER(bool,hasGraphicElement);
	ASPROPERTY_GETTER(bool,hasTabs);
	ASPROPERTY_GETTER(int,rawTextLength);
	ASPROPERTY_GETTER(number_t,specifiedWidth);
	ASPROPERTY_GETTER(int,textBlockBeginIndex);

};
class TabStop: public ASObject
{
public:
	TabStop(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class BreakOpportunity: public ASObject
{
public:
	BreakOpportunity(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class CFFHinting: public ASObject
{
public:
	CFFHinting(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class DigitCase: public ASObject
{
public:
	DigitCase(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class DigitWidth: public ASObject
{
public:
	DigitWidth(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class JustificationStyle: public ASObject
{
public:
	JustificationStyle(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class LigatureLevel: public ASObject
{
public:
	LigatureLevel(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class RenderingMode: public ASObject
{
public:
	RenderingMode(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class TabAlignment: public ASObject
{
public:
	TabAlignment(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class TextLineValidity: public ASObject
{
public:
	TextLineValidity(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};
class TextRotation: public ASObject
{
public:
	TextRotation(Class_base* c): ASObject(c) {}
	static void sinit(Class_base* c);
};

}

#endif
