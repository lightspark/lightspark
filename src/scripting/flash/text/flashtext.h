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

#ifndef SCRIPTING_FLASH_TEXT_FLASHTEXT_H
#define SCRIPTING_FLASH_TEXT_FLASHTEXT_H 1

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/display/flashdisplay.h"
#include "3rdparty/pugixml/src/pugixml.hpp"

namespace lightspark
{
class Array;

class AntiAliasType : public ASObject
{
public:
	AntiAliasType(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class ASFont: public ASObject
{
private:
	static std::vector<asAtom>* getFontList();
public:
	ASFont(Class_base* c):ASObject(c),fontType("device"){}
	void SetFont(tiny_string& fontname,bool is_bold,bool is_italic, bool is_Embedded, bool is_EmbeddedCFF);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(enumerateFonts);
	ASFUNCTION_ATOM(registerFont);
	ASFUNCTION_ATOM(hasGlyphs);
	ASPROPERTY_GETTER(tiny_string, fontName);
	ASPROPERTY_GETTER(tiny_string, fontStyle);
	ASPROPERTY_GETTER(tiny_string, fontType);
};

class StyleSheet: public EventDispatcher
{
private:
	std::map<tiny_string, asAtom > styles;
public:
	StyleSheet(Class_base* c):EventDispatcher(c){}
	void finalize();
	ASFUNCTION_ATOM(getStyle);
	ASFUNCTION_ATOM(setStyle);
	ASFUNCTION_ATOM(_getStyleNames);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class TextField: public InteractiveObject, public TextData, public TokenContainer
{
private:
	/*
	 * A parser for the HTML subset supported by TextField.
	 */
	class HtmlTextParser : public pugi::xml_tree_walker {
	protected:
		TextData *textdata;

		uint32_t parseFontSize(const Glib::ustring& s, uint32_t currentFontSize);
		bool for_each(pugi::xml_node& node);
	public:
		HtmlTextParser() : textdata(NULL) {};
		//Stores the text and formating into a TextData object
		void parseTextAndFormating(const tiny_string& html, TextData *dest);
	};

public:
	enum EDIT_TYPE { ET_READ_ONLY, ET_EDITABLE };
	enum ANTI_ALIAS_TYPE { AA_NORMAL, AA_ADVANCED };
	enum GRID_FIT_TYPE { GF_NONE, GF_PIXEL, GF_SUBPIXEL };
	enum TEXT_INTERACTION_MODE { TI_NORMAL, TI_SELECTION };
private:
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, HIT_TYPE type);
	void renderImpl(RenderContext& ctxt) const;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix);
	void requestInvalidation(InvalidateQueue* q);
	void updateText(const tiny_string& new_text);
	//Computes and changes (text)width and (text)height using Pango
	void updateSizes();
	tiny_string toHtmlText();
	tiny_string compactHTMLWhiteSpace(const tiny_string&);
	void validateThickness(number_t oldValue);
	void validateSharpness(number_t oldValue);
	void validateScrollH(int32_t oldValue);
	void validateScrollV(int32_t oldValue);
	int32_t getMaxScrollH();
	int32_t getMaxScrollV();
	void textUpdated();
	void setSizeAndPositionFromAutoSize();
	void replaceText(unsigned int begin, unsigned int end, const tiny_string& newText);
	EDIT_TYPE type;
	ANTI_ALIAS_TYPE antiAliasType;
	GRID_FIT_TYPE gridFitType;
	TEXT_INTERACTION_MODE textInteractionMode;
	_NR<ASString> restrictChars;
public:
	TextField(Class_base* c, const TextData& textData=TextData(), bool _selectable=true, bool readOnly=true);
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	void setHtmlText(const tiny_string& html);
	ASFUNCTION_ATOM(appendText);
	ASFUNCTION_ATOM(_getAntiAliasType);
	ASFUNCTION_ATOM(_setAntiAliasType);
	ASFUNCTION_ATOM(_getGridFitType);
	ASFUNCTION_ATOM(_setGridFitType);
	ASFUNCTION_ATOM(_getLength);
	ASFUNCTION_ATOM(_getWidth);
	ASFUNCTION_ATOM(_setWidth);
	ASFUNCTION_ATOM(_getHeight);
	ASFUNCTION_ATOM(_setHeight);
	ASFUNCTION_ATOM(_getHtmlText);
	ASFUNCTION_ATOM(_setHtmlText);
	ASFUNCTION_ATOM(_getText);
	ASFUNCTION_ATOM(_setText);
	ASFUNCTION_ATOM(_setAutoSize);
	ASFUNCTION_ATOM(_getAutoSize);
	ASFUNCTION_ATOM(_setWordWrap);
	ASFUNCTION_ATOM(_getWordWrap);
	ASFUNCTION_ATOM(_getTextWidth);
	ASFUNCTION_ATOM(_getTextHeight);
	ASFUNCTION_ATOM(_getTextFormat);
	ASFUNCTION_ATOM(_setTextFormat);
	ASFUNCTION_ATOM(_getDefaultTextFormat);
	ASFUNCTION_ATOM(_setDefaultTextFormat);
	ASFUNCTION_ATOM(_getLineIndexAtPoint);
	ASFUNCTION_ATOM(_getLineIndexOfChar);
	ASFUNCTION_ATOM(_getLineLength);
	ASFUNCTION_ATOM(_getLineMetrics);
	ASFUNCTION_ATOM(_getLineOffset);
	ASFUNCTION_ATOM(_getLineText);
	ASFUNCTION_ATOM(_getNumLines);
	ASFUNCTION_ATOM(_getMaxScrollH);
	ASFUNCTION_ATOM(_getMaxScrollV);
	ASFUNCTION_ATOM(_getBottomScrollV);
	ASFUNCTION_ATOM(_getRestrict);
	ASFUNCTION_ATOM(_setRestrict);
	ASFUNCTION_ATOM(_getTextInteractionMode);
	ASFUNCTION_ATOM(_setSelection);
	ASFUNCTION_ATOM(_replaceText);
	ASFUNCTION_ATOM(_replaceSelectedText);
	ASPROPERTY_GETTER_SETTER(bool, alwaysShowSelection);
	ASFUNCTION_GETTER_SETTER(background);
	ASFUNCTION_GETTER_SETTER(backgroundColor);
	ASFUNCTION_GETTER_SETTER(border);
	ASFUNCTION_GETTER_SETTER(borderColor);
	ASPROPERTY_GETTER(int32_t, caretIndex);
	ASPROPERTY_GETTER_SETTER(bool, condenseWhite);
	ASPROPERTY_GETTER_SETTER(bool, displayAsPassword);
	ASPROPERTY_GETTER_SETTER(bool, embedFonts);
	ASPROPERTY_GETTER_SETTER(int32_t, maxChars);
	ASFUNCTION_GETTER_SETTER(multiline);
	ASPROPERTY_GETTER_SETTER(bool, mouseWheelEnabled);
	ASFUNCTION_GETTER_SETTER(scrollH);
	ASFUNCTION_GETTER_SETTER(scrollV);
	ASPROPERTY_GETTER_SETTER(bool, selectable);
	ASPROPERTY_GETTER(int32_t, selectionBeginIndex);
	ASPROPERTY_GETTER(int32_t, selectionEndIndex);
	ASPROPERTY_GETTER_SETTER(number_t, sharpness);
	ASPROPERTY_GETTER_SETTER(_NR<StyleSheet>, styleSheet);
	ASFUNCTION_GETTER_SETTER(textColor);
	ASPROPERTY_GETTER_SETTER(number_t, thickness);
	ASFUNCTION_GETTER_SETTER(type);
	ASPROPERTY_GETTER_SETTER(bool, useRichTextClipboard);
};

class TextFormat: public ASObject
{
private:
	void onAlign(const tiny_string& old);
public:
	TextFormat(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_TEXTFORMAT){}
	void finalize();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(tiny_string,align);
	ASPROPERTY_GETTER_SETTER(asAtom,blockIndent);
	ASPROPERTY_GETTER_SETTER(asAtom,bold);
	ASPROPERTY_GETTER_SETTER(asAtom,bullet);
	ASPROPERTY_GETTER_SETTER(asAtom,color);
	ASPROPERTY_GETTER_SETTER(tiny_string,font);
	ASPROPERTY_GETTER_SETTER(asAtom,indent);
	ASPROPERTY_GETTER_SETTER(asAtom,italic);
	ASPROPERTY_GETTER_SETTER(asAtom,kerning);
	ASPROPERTY_GETTER_SETTER(asAtom,leading);
	ASPROPERTY_GETTER_SETTER(asAtom,leftMargin);
	ASPROPERTY_GETTER_SETTER(asAtom,letterSpacing);
	ASPROPERTY_GETTER_SETTER(asAtom,rightMargin);
	ASPROPERTY_GETTER_SETTER(int32_t,size);
	ASPROPERTY_GETTER_SETTER(_NR<Array>,tabStops);
	ASPROPERTY_GETTER_SETTER(tiny_string,target);
	ASPROPERTY_GETTER_SETTER(asAtom,underline);
	ASPROPERTY_GETTER_SETTER(tiny_string,url);
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

class StaticText: public DisplayObject, public TokenContainer
{
private:
	ASFUNCTION_ATOM(_getText);
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
		{ return TokenContainer::boundsRect(xmin,xmax,ymin,ymax); }
	void renderImpl(RenderContext& ctxt) const
		{ TokenContainer::renderImpl(ctxt); }
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, HIT_TYPE type)
		{ return TokenContainer::hitTestImpl(last, x, y, type); }
public:
	StaticText(Class_base* c) : DisplayObject(c),TokenContainer(this) {}
	StaticText(Class_base* c, const tokensVector& tokens):
		DisplayObject(c),TokenContainer(this, tokens, 1.0f/1024.0f/20.0f/20.0f) {}
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

class TextInteractionMode: public ASObject
{
public:
	TextInteractionMode(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class TextLineMetrics : public ASObject
{
protected:
	ASPROPERTY_GETTER_SETTER(number_t, ascent);
	ASPROPERTY_GETTER_SETTER(number_t, descent);
	ASPROPERTY_GETTER_SETTER(number_t, height);
	ASPROPERTY_GETTER_SETTER(number_t, leading);
	ASPROPERTY_GETTER_SETTER(number_t, width);
	ASPROPERTY_GETTER_SETTER(number_t, x);
public:
	TextLineMetrics(Class_base* c, number_t _x=0, number_t _width=0, number_t _height=0,
			number_t _ascent=0, number_t _descent=0, number_t _leading=0)
		: ASObject(c), ascent(_ascent), descent(_descent),
		  height(_height), leading(_leading), width(_width), x(_x) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

};

#endif /* SCRIPTING_FLASH_TEXT_FLASHTEXT_H */
