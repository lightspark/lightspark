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

#include <cassert>

#include "swf.h"
#include "abc.h"
#include "backends/textdata.h"
#include "logger.h"
#include "compat.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/system/ApplicationDomain.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/toplevel.h"
#include "parsing/tags.h"

using namespace lightspark;

extern void nanoVGgetTextBounds(SystemState* sys, TextData& tData, const FormatText& format, const tiny_string& text, number_t& tw, number_t& th);

tiny_string TextData::getText(uint32_t line, bool convertNewlineToSpace) const
{
	tiny_string text;
	if (line == UINT32_MAX)
	{
		for (auto it = textlines.begin(); it != textlines.end(); it++)
		{
			text += (*it).text;
			if (it->linebreaks)
			{
				for (uint32_t i = 0; i < it->linebreaks; i++)
					text += convertNewlineToSpace ? " " : "\r";
			}
			else if ((this->swfversion < 7 && it->needsnewline)
				|| (this->swfversion >= 7 && this->multiline && it->needsnewline))
				text += convertNewlineToSpace ? " " : "\r";
		}
	}
	else if (textlines.size() > line)
		text = textlines[line].text;
	return text;
}

void TextData::setText(const char* text, bool firstlineonly,FormatText* format)
{
	textlines.clear();
	appendText(text,firstlineonly,format);
}
void TextData::appendText(const char *text, bool firstlineonly, const FormatText* format, uint32_t swfversion, bool condenseWhite)
{
	if (!this->multiline && (swfversion >= 7) && *text == 0x00)
		return;
	tiny_string t = text;
	FormatText newformat;
	if (format)
		newformat=*format;
	bool mergeline = false;
	if (getLineCount())
	{
		mergeline=true;
		if (format)
		{
			if (swfversion < 8)
			{
				mergeline = condenseWhite && !(
								(this->multiline && (format->paragraph || format->bullet))
								|| textlines.back().format.paramsChanged(format));
			}
			else
			{
				mergeline = (textlines.back().text.endsWithHTMLWhitespace() && t.isWhiteSpaceOnly())
							|| !(this->multiline && (textlines.back().format.paragraph || textlines.back().format.bullet));
				if ((!t.isWhiteSpaceOnly() || t.endsWith("\n")) && !(textlines.back().format.paragraph || textlines.back().format.bullet))
					mergeline &= !textlines.back().format.paramsChanged(format);
			}
		}
		if (mergeline)
		{
			t = textlines.back().text + t;
			newformat = textlines.back().format;
			textlines.pop_back();
		}
		else if (textlines.back().format.paragraph || textlines.back().format.bullet)
			textlines.back().needsnewline = true;
	}
	bool hasnewline=false;
	if (swfversion >= 8 && condenseWhite)
	{
		tiny_string s = t.compactHTMLWhiteSpace(true,this->multiline ? &hasnewline : nullptr);
		if (!t.isWhiteSpaceOnly())
			t = s;
		else if (this->multiline)
		{
			if (mergeline || newformat.bullet)
				t = s;
			else if (!hasnewline && !(newformat.paragraph || newformat.bullet))
				return;
		}
	}
	
	uint32_t index=0;
	do
	{
		textline line;
		line.format = newformat;
		line.autosizeposition=0;
		line.textwidth=UINT32_MAX;
		line.needsnewline = t.getLine(index,line.text) || newformat.paragraph || newformat.bullet;
		textlines.push_back(line);
	}
	while (!condenseWhite && index != tiny_string::npos && !firstlineonly);
}

void TextData::appendFormatText(const char *text, const FormatText& format, uint32_t swfversion, bool condensewhite)
{
	appendText(text, false, &format, swfversion, condensewhite);
}

void TextData::appendLineBreak(bool needsadditionalbreak, bool emptyline,FormatText format)
{
	if (!emptyline)
	{
		if (!textlines.empty())
		{
			textlines.back().linebreaks++;
			needsadditionalbreak &= (textlines.back().format.paragraph);
			format = textlines.back().format;
		}
		else
			needsadditionalbreak=true;
	}
	if (needsadditionalbreak || emptyline)
	{
		if (format.bullet && !textlines.empty() && !textlines.back().format.paragraph && !textlines.back().format.bullet)
			textlines.back().needsnewline=true;
		textline line;
		line.format = format;
		line.autosizeposition=0;
		line.textwidth=UINT32_MAX;
		line.linebreaks=1;
		textlines.push_back(line);
	}
}

void TextData::appendTextToLastLine(const tiny_string& text)
{
	assert(textlines.size());
	textlines.back().text += text;
}

void TextData::setNeedsNewLineToLastLine()
{
	if (!textlines.empty())
		textlines.back().needsnewline=true;
}

void TextData::clear()
{
	textlines.clear();
	maxParagraphID=0;
}

bool TextData::isWhitespaceOnly(bool multiline) const
{
	for (auto it = textlines.begin(); it != textlines.end(); it++)
	{
		if (!it->text.isWhiteSpaceOnly())
			return false;
		if (multiline && (it->format.paragraph || it->format.bullet))
		  	return false;
	}
	return true;
}

// sizes are returned in twips
void TextData::getTextSizes(SystemState* sys, const FormatText& format,FontTag* ef, const tiny_string& text, number_t& tw, number_t& th)
{
	if (!ef)
		ef = embeddedFont;
	if (ef)
	{
		if (!useOutlines)
		{
			if (nanoVGFontID != -1)
				nanoVGgetTextBounds(sys,*this,format,text, tw, th);
			if (nanoVGFontID > 0) // system font found
				return;
			// no system font found, fallback to embedded font
		}
		ef->getTextBounds(text,format,tw,th);
	}
	else
		nanoVGgetTextBounds(sys,*this,format,text, tw, th);
	number_t l= parseNumber(format.leading,sys->getSwfVersion()<11)*TWIPS_FACTOR;
	if (!std::isnan(l))
		th += l;
}

bool TextData::TextIsEqual(const std::vector<tiny_string>& lines, const vector<FormatText>& oldformats) const
{
	if (this->textlines.size() != lines.size() || oldformats.size() != lines.size())
		return false;
	for (uint32_t i = 0; i < this->textlines.size(); i++)
	{
		if (this->textlines[i].text != lines[i]
			|| this->textlines[i].format.paramsChanged(&oldformats[i]))
			return false;
	}
	return true;
}
extern int nanoVGdefaultFontID;
extern void nanoVGSetupFont(SystemState* sys, TextData& tData);
FontTag* TextData::checkEmbeddedFont(DisplayObject* d)
{
	ApplicationDomain* currentDomain=d->loadedFrom;
	if (!currentDomain) currentDomain = d->getSystemState()->mainClip->applicationDomain.getPtr();
	FontTag* embeddedfont = (fontID != UINT32_MAX ? currentDomain->getEmbeddedFontByID(fontID) : currentDomain->getEmbeddedFont(d->getSystemState()->getStringFromUniqueId(fontname)));
	if (embeddedfont)
	{
		this->isBold = embeddedfont->isBold();
		this->isItalic = embeddedfont->isItalic();
		this->fontname = d->getSystemState()->getUniqueStringId(embeddedfont->getFontname());
		this->fontID = embeddedfont->getId();
		if (!useOutlines)
			nanoVGSetupFont(d->getSystemState(),*this);
		if (!useOutlines && nanoVGFontID >=0)
			embeddedfont=nullptr; // use system font
		else
		{
			for (auto it = textlines.begin(); it != textlines.end(); it++)
			{
				if (!embeddedfont->hasGlyphs((*it).text))
				{
					embeddedfont=nullptr;
					break;
				}
			}
		}
	}
	else if (nanoVGFontID==-1) // no system font found, fallback to default font
		nanoVGFontID = nanoVGdefaultFontID;
	this->embeddedFont=embeddedfont;
	return embeddedfont;
}

void TextData::checklastline(bool needsadditionalline)
{
	if (!textlines.empty()
		&& (textlines.back().format.paragraph
			|| textlines.back().format.bullet))
		textlines.back().needsnewline=true;
	if (needsadditionalline && swfversion < 8)
	{
		if (textlines.empty() || !textlines.back().text.empty())
		{
			textline line;
			line.autosizeposition=0;
			line.textwidth=UINT32_MAX;
			line.needsnewline=true;
			if (textlines.empty())
				line.format.paragraph=++maxParagraphID;
			else
			{
				line.format.bullet = textlines.back().format.bullet;
				line.format.paragraph = textlines.back().format.paragraph;
				line.format.font = textlines.back().format.font;
				line.format.fontColor = textlines.back().format.fontColor;
				line.format.fontSize = textlines.back().format.fontSize;
				line.format.align = textlines.back().format.align;
			}
			textlines.push_back(line);
		}
	}
	if (!textlines.empty())
	{
		if (!textlines.back().text.isWhiteSpaceOnly())
		{
			bool canhaveparagraph = !textlines.back().needsnewline;
			if (textlines.size() > 1 && !((textlines.end()-2)->needsnewline))
			 	canhaveparagraph =textlines.back().format.paramsChanged(&(textlines.end()-1)->format);
			if (canhaveparagraph)
				textlines.back().format.paragraph=maxParagraphID;
		}
	}
}

bool FormatText::paramsChanged(const FormatText* f) const
{
	return
		bold!=f->bold ||
		italic!=f->italic ||
		underline!=f->underline ||
		!(fontColor==f->fontColor) ||
		fontSize!=f->fontSize ||
		font!=f->font ||
		embeddedfontID!=f->embeddedfontID ||
		kerning!=f->kerning ||
		letterspacing!=f->letterspacing ||
		url!=f->url ||
		target!=f->target;
}

FormatText::FormatText(const TextData& tdata)
{
	align = tdata.align;
	fontColor = tdata.textColor;
	fontSize = tdata.fontSize;
	font = tdata.fontname;
	if (tdata.embeddedFont)
		embeddedfontID = tdata.embeddedFont->getId();
	else if (tdata.fontID != UINT32_MAX)
		embeddedfontID = tdata.fontID;
	if (tdata.leading)
		leading = Integer::toString(tdata.leading/TWIPS_FACTOR);
	if (tdata.leftMargin)
		leftmargin = Integer::toString(tdata.leftMargin/TWIPS_FACTOR);
	if (tdata.rightMargin)
		rightmargin = Integer::toString(tdata.rightMargin/TWIPS_FACTOR);
	if (tdata.indent)
		indent = Integer::toString(tdata.indent/TWIPS_FACTOR);

}
