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
#include "backends/graphics.h"

namespace lightspark
{

class ElementFormat;
class GroupElement;
class ContentElement: public ASObject
{
public:
	ContentElement(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_CONTENTELEMENT),textBlockBeginIndex(-1) {}
	static void sinit(Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	ASPROPERTY_GETTER(tiny_string,rawText);
	ASPROPERTY_GETTER_SETTER(_NR<ElementFormat>,elementFormat);
	ASPROPERTY_GETTER(_NR<GroupElement>,groupElement);
	ASPROPERTY_GETTER(int,textBlockBeginIndex);
};

class FontDescription;
class ElementFormat: public ASObject
{
public:
	ElementFormat(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
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
	FontLookup(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};

class FontDescription: public ASObject
{
private:
	tiny_string cffHinting;
	tiny_string fontLookup;

	tiny_string fontPosture;
	tiny_string fontWeight;
	bool locked;
	tiny_string renderingMode;

public:
	tiny_string fontName;
	FontDescription(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_FONTDESCRIPTION),
		cffHinting("horizontalStem"), fontLookup("device"), fontPosture("normal"), fontWeight("normal"),locked(false), renderingMode("cff"), fontName("_serif") {}
	static void sinit(Class_base* c);
	bool destruct() override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_clone);
	ASFUNCTION_ATOM(isFontCompatible);
	ASFUNCTION_ATOM(isDeviceFontCompatible);

	ASFUNCTION_ATOM(_getCffHinting);
	ASFUNCTION_ATOM(_setCffHinting);

	ASFUNCTION_ATOM(_getFontLookup);
	ASFUNCTION_ATOM(_setFontLookup);

	ASFUNCTION_ATOM(_getFontName);
	ASFUNCTION_ATOM(_setFontName);

	ASFUNCTION_ATOM(_getFontPosture);
	ASFUNCTION_ATOM(_setFontPosture);

	ASFUNCTION_ATOM(_getFontWeight);
	ASFUNCTION_ATOM(_setFontWeight);

	ASFUNCTION_ATOM(_getLocked);
	ASFUNCTION_ATOM(_setLocked);

	ASFUNCTION_ATOM(_getRenderingMode);
	ASFUNCTION_ATOM(_setRenderingMode);
};

class FontPosture: public ASObject
{
public:
	FontPosture(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class FontWeight: public ASObject
{
public:
	FontWeight(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};

class FontMetrics: public ASObject
{
public:
	FontMetrics(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class Kerning: public ASObject
{
public:
	Kerning(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class LineJustification: public ASObject
{
public:
	LineJustification(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};

class TextBaseline: public ASObject
{
public:
	TextBaseline(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};

class TextJustifier: public ASObject
{
public:
	TextJustifier(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_TEXTJUSTIFIER) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class SpaceJustifier: public TextJustifier
{
public:
	SpaceJustifier(ASWorker* wrk,Class_base* c): TextJustifier(wrk,c) { subtype = SUBTYPE_SPACEJUSTIFIER; }
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class EastAsianJustifier: public TextJustifier
{
public:
	EastAsianJustifier(ASWorker* wrk,Class_base* c): TextJustifier(wrk,c) { subtype = SUBTYPE_EASTASIANJUSTIFIER; }
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class TextLine;
class TextBlock: public ASObject
{
private:
	bool fillTextLine(NullableRef<TextLine> textLine, bool fitSomething, _NR<TextLine> previousLine);
public:
	TextBlock(ASWorker* wrk,Class_base* c);
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
	void settext_cb(asAtom oldValue);
public:
	TextElement(ASWorker* wrk,Class_base* c): ContentElement(wrk,c) { subtype = SUBTYPE_TEXTELEMENT; }
	static void sinit(Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(replaceText);
	ASPROPERTY_GETTER_SETTER(asAtom,text);
	
};
class GroupElement: public ContentElement
{
public:
	GroupElement(ASWorker* wrk,Class_base* c): ContentElement(wrk,c) {}
	static void sinit(Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	ASFUNCTION_ATOM(_constructor);
};
class TextLine: public DisplayObjectContainer, public TextData, public TokenContainer
{
private:
	// these are only used when drawing to DisplayObject, so they are guarranteed not to be destroyed during rendering
	list<FILLSTYLE> fillstyleTextColor;
	// Much of the rendering/bounds checking/hit testing code is
	// similar to TextField and should be shared
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	IDrawable* invalidate(bool smoothing) override;
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
public:
	TextLine(ASWorker* wrk,Class_base* c, _NR<TextBlock> owner=NullRef);
	static void sinit(Class_base* c);
	void finalize() override;
	float getScaleFactor() const override { return this->scaling; }
	void updateSizes();
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
	ASFUNCTION_ATOM(flushAtomData);
	ASPROPERTY_GETTER(bool,hasGraphicElement);
	ASPROPERTY_GETTER(bool,hasTabs);
	ASPROPERTY_GETTER(int,rawTextLength);
	ASPROPERTY_GETTER(number_t,specifiedWidth);
	ASPROPERTY_GETTER(int,textBlockBeginIndex);
	std::string toDebugString() const override;
};
class TabStop: public ASObject
{
public:
	TabStop(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class BreakOpportunity: public ASObject
{
public:
	BreakOpportunity(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class CFFHinting: public ASObject
{
public:
	CFFHinting(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class DigitCase: public ASObject
{
public:
	DigitCase(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class DigitWidth: public ASObject
{
public:
	DigitWidth(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class JustificationStyle: public ASObject
{
public:
	JustificationStyle(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class LigatureLevel: public ASObject
{
public:
	LigatureLevel(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class RenderingMode: public ASObject
{
public:
	RenderingMode(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class TabAlignment: public ASObject
{
public:
	TabAlignment(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class TextLineValidity: public ASObject
{
public:
	TextLineValidity(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};
class TextRotation: public ASObject
{
public:
	TextRotation(ASWorker* wrk,Class_base* c):ASObject(wrk,c) {}
	static void sinit(Class_base* c);
};

}

#endif
