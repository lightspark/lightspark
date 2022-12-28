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
#include "swf.h"
#include "asobject.h"
#include "scripting/flash/display/flashdisplay.h"
#include "3rdparty/pugixml/src/pugixml.hpp"

// according to TextLineMetrics specs, there are always 2 pixels added to each side of a textfield
#define TEXTFIELD_PADDING 2

namespace lightspark
{
class Array;
class DefineEditTextTag;

class AntiAliasType : public ASObject
{
public:
	AntiAliasType(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class ASFont: public ASObject
{
public:
	static std::vector<asAtom>* getFontList();
	ASFont(ASWorker* wrk,Class_base* c):ASObject(wrk,c),fontType("device"){}
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
	StyleSheet(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c){}
	void finalize() override;
	ASFUNCTION_ATOM(getStyle);
	ASFUNCTION_ATOM(setStyle);
	ASFUNCTION_ATOM(_getStyleNames);
	ASFUNCTION_ATOM(parseCSS);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class TextField: public InteractiveObject, public TextData, public TokenContainer, public ITickJob
{
private:
	/*
	 * A parser for the HTML subset supported by TextField.
	 */
	class HtmlTextParser : public pugi::xml_tree_walker {
	protected:
		TextData *textdata;

		uint32_t parseFontSize(const char *s, uint32_t currentFontSize);
		bool for_each(pugi::xml_node& node);
	public:
		HtmlTextParser() : textdata(NULL) {}
		//Stores the text and formating into a TextData object
		void parseTextAndFormating(const tiny_string& html, TextData *dest);
	};

public:
	enum EDIT_TYPE { ET_READ_ONLY, ET_EDITABLE };
	enum ANTI_ALIAS_TYPE { AA_NORMAL, AA_ADVANCED };
	enum GRID_FIT_TYPE { GF_NONE, GF_PIXEL, GF_SUBPIXEL };
	enum TEXT_INTERACTION_MODE { TI_NORMAL, TI_SELECTION };
private:
	_NR<DisplayObject> hitTestImpl(number_t x, number_t y, HIT_TYPE type,bool interactiveObjectsOnly) override;
	bool renderImpl(RenderContext& ctxt) override;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) override;
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap) override;
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	void defaultEventBehavior(_R<Event> e) override;
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
	void setSizeAndPositionFromAutoSize(bool updatewidth=true);
	void replaceText(unsigned int begin, unsigned int end, const tiny_string& newText);
	EDIT_TYPE type;
	ANTI_ALIAS_TYPE antiAliasType;
	GRID_FIT_TYPE gridFitType;
	TEXT_INTERACTION_MODE textInteractionMode;
	_NR<ASString> restrictChars;
	number_t autosizeposition;
	tiny_string tagvarname;
	DisplayObject* tagvartarget;
	Mutex invalidatemutex;
	DefineEditTextTag* tag;
	int32_t originalXPosition;
	int32_t originalWidth;

	// these are only used when drawing to DisplayObject, so they are guarranteed not to be destroyed during rendering
	list<FILLSTYLE> fillstyleTextColor;
	FILLSTYLE fillstyleBackgroundColor;
	LINESTYLE2 lineStyleBorder;
	LINESTYLE2 lineStyleCaret;
	Mutex* linemutex;
	void getTextBounds(const tiny_string &txt, number_t &xmin, number_t &xmax, number_t &ymin, number_t &ymax);
protected:
	void afterSetLegacyMatrix() override;
public:
	TextField(ASWorker* wrk,Class_base* c, const TextData& textData=TextData(), bool _selectable=true, bool readOnly=true, const char* varname="", DefineEditTextTag* _tag=nullptr);
	~TextField();
	void finalize() override;
	void prepareShutdown() override;
	static void sinit(Class_base* c);
	void setHtmlText(const tiny_string& html);
	void avm1SyncTagVar();
	void UpdateVariableBinding(asAtom v) override;
	void afterLegacyInsert() override;
	void afterLegacyDelete(DisplayObjectContainer* parent, bool inskipping) override;
	void lostFocus() override;
	void gotFocus() override;
	void textInputChanged(const tiny_string& newtext) override;
	void tick() override;
	void tickFence() override;
	uint32_t getTagID() const override;
	
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
	ASFUNCTION_ATOM(_getCharBoundaries);
	ASFUNCTION_ATOM(_getdisplayAsPassword);
	ASFUNCTION_ATOM(_setdisplayAsPassword);
	ASPROPERTY_GETTER_SETTER(bool, alwaysShowSelection);
	ASFUNCTION_GETTER_SETTER(background);
	ASFUNCTION_GETTER_SETTER(backgroundColor);
	ASFUNCTION_GETTER_SETTER(border);
	ASFUNCTION_GETTER_SETTER(borderColor);
	ASPROPERTY_GETTER(uint32_t, caretIndex);
	ASPROPERTY_GETTER_SETTER(bool, condenseWhite);
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
	ASFUNCTION_ATOM(_getTextFieldX);
	ASFUNCTION_ATOM(_setTextFieldX);

	std::string toDebugString() const override;
};

class TextFormat: public ASObject
{
private:
	void onAlign(const asAtom& old);
public:
	TextFormat(ASWorker* wrk,Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER_ATOM(align);
	ASPROPERTY_GETTER_SETTER_ATOM(blockIndent);
	ASPROPERTY_GETTER_SETTER_ATOM(bold);
	ASPROPERTY_GETTER_SETTER_ATOM(bullet);
	ASPROPERTY_GETTER_SETTER_ATOM(color);
	ASPROPERTY_GETTER_SETTER_ATOM(font);
	ASPROPERTY_GETTER_SETTER_ATOM(indent);
	ASPROPERTY_GETTER_SETTER_ATOM(italic);
	ASPROPERTY_GETTER_SETTER_ATOM(kerning);
	ASPROPERTY_GETTER_SETTER_ATOM(leading);
	ASPROPERTY_GETTER_SETTER_ATOM(leftMargin);
	ASPROPERTY_GETTER_SETTER_ATOM(letterSpacing);
	ASPROPERTY_GETTER_SETTER_ATOM(rightMargin);
	ASPROPERTY_GETTER_SETTER_ATOM(size);
	ASPROPERTY_GETTER_SETTER(_NR<Array>,tabStops);
	ASPROPERTY_GETTER_SETTER_ATOM(target);
	ASPROPERTY_GETTER_SETTER_ATOM(underline);
	ASPROPERTY_GETTER_SETTER_ATOM(url);
	ASPROPERTY_GETTER_SETTER_ATOM(display);
};

class TextFieldType: public ASObject
{
public:
	TextFieldType(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class TextFormatAlign: public ASObject
{
public:
	TextFormatAlign(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class TextFieldAutoSize: public ASObject
{
public:
	TextFieldAutoSize(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class StaticText: public DisplayObject, public TokenContainer
{
private:
	ASFUNCTION_ATOM(_getText);
	RECT bounds;
	uint32_t tagID;
protected:
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) override;
	bool renderImpl(RenderContext& ctxt) override;
	_NR<DisplayObject> hitTestImpl(number_t x, number_t y, HIT_TYPE type,bool interactiveObjectsOnly) override;
public:
	StaticText(ASWorker* wrk,Class_base* c):DisplayObject(wrk,c),TokenContainer(this),tagID(UINT32_MAX) {}
	StaticText(ASWorker* wrk,Class_base* c, const tokensVector& tokens,const RECT& b,uint32_t _tagID):
		DisplayObject(wrk,c),TokenContainer(this, tokens, 1.0f/1024.0f/20.0f/20.0f),bounds(b),tagID(_tagID) {}
	static void sinit(Class_base* c);
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override { TokenContainer::requestInvalidation(q,forceTextureRefresh); }
	IDrawable* invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap) override;
	uint32_t getTagID() const override { return tagID; }
};

class FontStyle: public ASObject
{
public:
	FontStyle(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class FontType: public ASObject
{
public:
	FontType(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class TextDisplayMode: public ASObject
{
public:
	TextDisplayMode(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class TextColorType: public ASObject
{
public:
	TextColorType(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class GridFitType: public ASObject
{
public:
	GridFitType(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class TextInteractionMode: public ASObject
{
public:
	TextInteractionMode(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
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
	TextLineMetrics(ASWorker* wrk,Class_base* c, number_t _x=0, number_t _width=0, number_t _height=0,
			number_t _ascent=0, number_t _descent=0, number_t _leading=0)
		: ASObject(wrk,c,T_OBJECT,SUBTYPE_TEXTLINEMETRICS), ascent(_ascent), descent(_descent),
		  height(_height), leading(_leading), width(_width), x(_x) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

}

#endif /* SCRIPTING_FLASH_TEXT_FLASHTEXT_H */
