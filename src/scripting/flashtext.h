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

class TextField: public InteractiveObject, public TextData
{
private:
        int textWidth, textHeight;

	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type);
	void renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void invalidate();
	void requestInvalidation();
	void updateText(const tiny_string& new_text);
public:
	TextField() : textWidth(0),textHeight(0) {};
	TextField(const TextData& textData) : TextData(textData),textWidth(0),textHeight(0) {};
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	void setTextSize(int textwidth, int textheight);
	ASFUNCTION(appendText);
	ASFUNCTION(_getWidth);
	ASFUNCTION(_setWidth);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_setHeight);
	ASFUNCTION(_getText);
	ASFUNCTION(_setText);
	ASFUNCTION(_getTextWidth);
	ASFUNCTION(_getTextHeight);
};

class TextFormat: public ASObject, public TextFormat_data
{
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class TextFieldType: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class TextFormatAlign: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class TextFieldAutoSize: public ASObject
{
public:
	static void sinit(Class_base* c);
};

class StyleSheet: public EventDispatcher
{
private:
	std::map<tiny_string, _R<ASObject> > styles;
public:
	StyleSheet(){}
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
	void renderImpl(bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
		{ TokenContainer::renderImpl(maskEnabled,t1,t2,t3,t4); }
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, HIT_TYPE type)
		{ return TokenContainer::hitTestImpl(last, x, y, type); }
public:
	StaticText() : TokenContainer(this) {};
	StaticText(const std::vector<GeomToken>& tokens) : TokenContainer(this, tokens, 1.0f/1024.0f/20.0f/20.0f) {};
	static void sinit(Class_base* c);
	void requestInvalidation() { TokenContainer::requestInvalidation(); }
	void invalidate() { TokenContainer::invalidate(); }
};
};

#endif
