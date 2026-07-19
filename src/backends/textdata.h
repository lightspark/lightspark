/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef BACKENDS_TEXTDATA_H
#define BACKENDS_TEXTDATA_H 1


#include "swftypes.h"

namespace lightspark
{
enum ALIGNMENT {AS_NONE = 0, AS_LEFT, AS_RIGHT, AS_CENTER, AS_JUSTIFY };
class TextData;

struct FormatText
{
	bool bullet {false};
	bool bold {false};
	bool italic {false};
	bool underline {false};
	uint32_t paragraph {0};
	ALIGNMENT align {AS_NONE};
	RGBA fontColor {0x000000,0}; // use alpha 0 as "not set" indicator
	uint32_t fontSize {0};
	uint32_t font {BUILTIN_STRINGS::EMPTY};
	uint32_t embeddedfontID {UINT32_MAX};
	tiny_string url;
	tiny_string target;
	number_t kerning {0};
	number_t letterspacing {0};
	tiny_string blockindent;
	tiny_string indent;
	tiny_string leading;
	tiny_string leftmargin;
	tiny_string rightmargin;
	tiny_string tabstops;
	uint32_t level {0};
	bool paramsChanged(const FormatText* f) const;
	FormatText() {}
	FormatText(const TextData& tdata);
};

struct textline
{
	tiny_string text;
	number_t autosizeposition {0};
	uint32_t textwidth {0};
	uint32_t height {0};
	FormatText format;
	bool needsnewline {false};
	uint32_t linebreaks {0};
};

class FontTag;
class DLL_PUBLIC TextData
{
friend class CachedSurface;
protected:
	std::vector<textline> textlines;
public:
	/* the default values are from the spec for flash.text.TextField and flash.text.TextFormat */
	TextData()
	:swfversion(0)
	,width(100*TWIPS_FACTOR)
	,height(100*TWIPS_FACTOR)
	,leftMargin(0)
	,rightMargin(0)
	,indent(0)
	,blockIndent(0)
	,leading(0)
	,textWidth(0)
	,textHeight(0)
	,fontname(BUILTIN_STRINGS::STRING_TIMES_NEW_ROMAN)
	,fontID(UINT32_MAX)
	,scrollH(0)
	,scrollV(1)
	,maxParagraphID(0)
	,backgroundColor(0xFFFFFF)
	,borderColor(0x000000)
	,background(false)
	,border(false)
	,multiline(false)
	,isBold(false)
	,isItalic(false)
	,wordWrap(false)
	,caretblinkstate(false)
	,isPassword(false)
	,useOutlines(false)
	,autoSize(AS_NONE)
	,align(AS_NONE)
	,fontSize(12)
	,embeddedFont(nullptr)
	,nanoVGFontID(-2) // -2 means no check for system font executed
	{}

	uint32_t swfversion;
	uint32_t width;
	uint32_t height;
	uint32_t leftMargin;
	uint32_t rightMargin;
	uint32_t indent;
	uint32_t blockIndent;
	int32_t leading;
	uint32_t textWidth;
	uint32_t textHeight;
	uint32_t fontname;
	uint32_t fontID;
	int32_t scrollH; // pixels, 0-based
	int32_t scrollV; // lines, 1-based
	uint32_t maxParagraphID;
	RGB backgroundColor;
	RGB borderColor;
	bool background:1;
	bool border:1;
	bool multiline:1;
	bool isBold:1;
	bool isItalic:1;
	bool wordWrap:1;
	bool caretblinkstate:1;
	bool isPassword:1;
	bool useOutlines;
	RGB textColor;
	ALIGNMENT autoSize;
	ALIGNMENT align;
	uint32_t fontSize;
	FontTag* embeddedFont;
	int nanoVGFontID;
	tiny_string getText(uint32_t line=UINT32_MAX, bool convertNewlineToSpace = false) const;
	void setText(const char* text, bool firstlineonly=false, FormatText* format=nullptr);
	void appendText(const char* text, bool firstlineonly=false, const FormatText* format = nullptr, uint32_t swfversion=UINT32_MAX, bool condensewhite=false);
	void appendFormatText(const char* text, const FormatText& format, uint32_t swfversion, bool condensewhite);
	void appendLineBreak(bool needsadditionalbreak, bool emptyline, FormatText format);
	void appendTextToLastLine(const tiny_string& text);
	void setNeedsNewLineToLastLine();
	void clear();
	bool isWhitespaceOnly(bool multiline) const;
	void getTextSizes(SystemState* sys, const FormatText& format, FontTag* ef, const tiny_string& text, number_t& tw, number_t& th);
	bool TextIsEqual(const std::vector<tiny_string>& lines, const std::vector<FormatText>& oldformats) const;
	uint32_t getLineCount() const { return textlines.size(); }
	FontTag* checkEmbeddedFont(DisplayObject* d);
	void checklastline(bool needsadditionalline);
};

class LineData {
public:
	LineData(int32_t x, int32_t y, int32_t _width,
		 int32_t _height, int32_t _firstCharOffset, int32_t _length,
		 number_t _ascent, number_t _descent, number_t _leading,
		 number_t _indent):
		extents(x, x+_width, y, y+_height), 
		firstCharOffset(_firstCharOffset), length(_length),
		ascent(_ascent), descent(_descent), leading(_leading),
		indent(_indent) {}
	// position and size
	RECT extents;
	// Offset of the first character on this line
	int32_t firstCharOffset;
	// length of the line in characters
	int32_t length;
	number_t ascent;
	number_t descent;
	number_t leading;
	number_t indent;
};
}
#endif /* BACKENDS_TEXTDATA_H */
