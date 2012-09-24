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

#ifndef SCRIPTING_FLASH_TEXT_FLASHTEXT_H
#define SCRIPTING_FLASH_TEXT_FLASHTEXT_H 1

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/display/flashdisplay.h"

namespace lightspark
{

class AntiAliasType : public ASObject
{
public:
	AntiAliasType(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class ASFont: public ASObject
{
private:
	static std::vector<ASObject*>* getFontList();
public:
	ASFont(Class_base* c):ASObject(c),fontType("device"){}
	void SetFont(tiny_string& fontname,bool is_bold,bool is_italic, bool is_Embedded, bool is_EmbeddedCFF);
	static void sinit(Class_base* c);
//	static void buildTraits(ASObject* o);
	ASFUNCTION(enumerateFonts);
	ASFUNCTION(registerFont);
	ASPROPERTY_GETTER(tiny_string, fontName);
	ASPROPERTY_GETTER(tiny_string, fontStyle);
	ASPROPERTY_GETTER(tiny_string, fontType);
};

class TextField: public InteractiveObject, public TextData
{
private:
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type);
	void renderImpl(RenderContext& ctxt) const;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix);
	void requestInvalidation(InvalidateQueue* q);
	void updateText(const tiny_string& new_text);
	//Computes and changes (text)width and (text)height using Pango
	void updateSizes();
public:
	TextField(Class_base* c):InteractiveObject(c) {};
	TextField(Class_base* c,const TextData& textData):InteractiveObject(c),TextData(textData) {};
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(appendText);
	ASFUNCTION(_getWidth);
	ASFUNCTION(_setWidth);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_setHeight);
	ASFUNCTION(_getText);
	ASFUNCTION(_setText);
	ASFUNCTION(_setAutoSize);
	ASFUNCTION(_getAutoSize);
	ASFUNCTION(_setWordWrap);
	ASFUNCTION(_getWordWrap);
	ASFUNCTION(_getTextWidth);
	ASFUNCTION(_getTextHeight);
	ASFUNCTION(_getTextFormat);
	ASFUNCTION(_setTextFormat);
	ASFUNCTION(_getDefaultTextFormat);
	ASFUNCTION(_setDefaultTextFormat);
	
	ASFUNCTION_GETTER_SETTER(textColor);
};

class TextFormat: public ASObject
{
public:
	TextFormat(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,color);
	ASPROPERTY_GETTER_SETTER(tiny_string,font);
	ASPROPERTY_GETTER_SETTER(int32_t,size);
};

class TextFieldType: public ASObject
{
public:
	TextFieldType(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class TextFormatAlign: public ASObject
{
public:
	TextFormatAlign(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class TextFieldAutoSize: public ASObject
{
public:
	TextFieldAutoSize(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class StyleSheet: public EventDispatcher
{
private:
	std::map<tiny_string, _R<ASObject> > styles;
public:
	StyleSheet(Class_base* c):EventDispatcher(c){}
	void finalize();
	ASFUNCTION(getStyle);
	ASFUNCTION(setStyle);
	ASFUNCTION(_getStyleNames);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class StaticText: public DisplayObject, public TokenContainer
{
private:
	ASFUNCTION(_getText);
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
		{ return TokenContainer::boundsRect(xmin,xmax,ymin,ymax); }
	void renderImpl(RenderContext& ctxt) const
		{ TokenContainer::renderImpl(ctxt); }
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type)
		{ return TokenContainer::hitTestImpl(last, x, y, type); }
public:
	StaticText(Class_base* c) : DisplayObject(c),TokenContainer(this) {};
	StaticText(Class_base* c, const tokensVector& tokens):
		DisplayObject(c),TokenContainer(this, tokens, 1.0f/1024.0f/20.0f/20.0f) {};
	static void sinit(Class_base* c);
	void requestInvalidation(InvalidateQueue* q) { TokenContainer::requestInvalidation(q); }
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix)
	{ return TokenContainer::invalidate(target, initialMatrix); }
};

class FontStyle: public ASObject
{
public:
	FontStyle(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class FontType: public ASObject
{
public:
	FontType(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class TextDisplayMode: public ASObject
{
public:
	TextDisplayMode(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class TextColorType: public ASObject
{
public:
	TextColorType(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class GridFitType: public ASObject
{
public:
	GridFitType(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};
};

#endif /* SCRIPTING_FLASH_TEXT_FLASHTEXT_H */
