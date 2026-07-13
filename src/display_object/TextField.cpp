/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2023-2024, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include "backends/geometry.h"
#include "backends/rendering_context.h"
#include "backends/cachedsurface.h"
#include "display_object/TextField.h"
#include "display_object/RootMovieClip.h"
#include "display_object/Stage.h"
#include "platforms/engineutils.h"
#include "parsing/tags.h"
#include "scripting/flash/ui/keycodes.h"

#define BULLER_INDENT 36.0

using namespace lightspark;

TextField::TextField
(
	SystemState* sys,
	SWFMovie& _movie,
	Optional<const tiny_string&> name,
	const TextData& _textData,
	bool _selectable,
	bool readOnly,
	const tiny_string& varName,
	DefineEditTextTag* _tag
) :
InteractiveObject(Type::TextField, sys, name),
TextData(_textData),
TokenContainer(this),
movie(_movie),
editType(readOnly ? ET_READ_ONLY : ET_EDITABLE),
antiAliasType
((
	_tag != nullptr &&
	!_tag->UseFlashType
) ? AA_NORMAL : AA_ADVANCE),
gridFitType(GF_PIXEL),
textInteractionMode(TI_NORMAL),
autoSizePos(0),
tagVarName(varName),
tagvartarget(nullptr),
tag(_tag),
originalXPosition(0),
originalWidth(textData.width),
fillStyleTextColor({ 0xff }),
fillStyleBackgroundColor(0xff),
lineStyleBorder(0xff),
lineStyleCaret(0xff),
inAVM1syncVar(false),
inUpdateVarBinding(false),
isHtml(false),
alwaysShowSelection(false),
caretIndex(-1),
condenseWhite(false),
maxChars(_tag != nullptr ? _tag->MaxLength : 0),
mouseWheelEnabled(true),
selectable(_selectable),
selectionBeginIndex(-1),
selectionEndIndex(-1),
sharpness(_tag != nullptr ? _tag->Sharpness : 0),
thickness(_tag != nullptr ? _tag->Thickness : 0),
useRichTextClipboard(false)
{
	if (!readOnly)
		tabEnabled = true;
	if (_tag != nullptr)
		align = ALIGNMENT(_tag->Align + 1);
}

Optional<Rect<Twips>> TextField::tryBoundsRect(bool visibleOnly)
{
	if (visibleOnly && !isVisible())
		return {};

	if (type == ET_EDITABLE && tag != nullptr)
		return tag->Bounds;

	if (isCreatedByTimeline() && tag != nullptr && autoSize == AS_NONE)
	{
		auto _size = size;
		if (!wordWrap)
			_size.x = textSize.x + 2 * TEXTFIELD_PADDING;

		return Rect<Twips>
		{
			tag->Bounds.min,
			std::max(Vector2Twips(), _size + tag->Bounds.min)
		};
	}

	auto minVec = tag != nullptr ? tag->Bounds.min : Vector2Twips();
	if (wordWrap || autoSize == AS_NONE)
	{
		return Rect<Twips>
		{
			minVec,
			std::max(Vector2Twips(), size + minVec)
		};
	}

	Vector2Twips _minVec(autoSizePos, minVec.y);
	Vector2Twips _size(textSize.x, size.y);
	return Rect<Twips>
	{
		minVec,
		std::max
		(
			Vector2Twips(),
			_size + _minVec
		) + Vector2Twips(2 * TEXFIELD_PADDING + minVec.x, 0)
	};
}

ASFUNCTIONBODY_ATOM(TextField,_constructor)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	InteractiveObject::_constructor(ret,wrk,obj,nullptr,0);
	th->restrictChars=asAtomHandler::nullAtom;
}

void TextField::setDisplayAsPassword(bool _isPassword)
{
	isPassword = _isPassword;
	setSizeAndPositionFromAutoSize();
	setHasChanged(true);
	setNeedsTextureRecalculation();
	if(isOnStage() && isVisible())
		requestInvalidation(getSys());
}

ASFUNCTIONBODY_ATOM(TextField,_getWordWrap)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	asAtomHandler::setBool(ret,th->wordWrap);
}

void TextField::setWordWrap(bool _wordWrap)
{
	wordWrap = _wordWrap;
	setSizeAndPositionFromAutoSize();
	setHasChanged(true);
	setNeedsTextureRecalculation();
	if(isOnStage() && isVisible())
		requestInvalidation(getSys());
}

ASFUNCTIONBODY_ATOM(TextField,_getAutoSize)
{
	TextField* th=asAtomHandler::as<TextField>(obj);
	switch(th->autoSize)
	{
		case AS_NONE:
		case AS_JUSTIFY:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"none");
			return;
		case AS_LEFT:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"left");
			return;
		case AS_RIGHT:
			ret =asAtomHandler::fromString(wrk->getSystemState(),"right");
			return;
		case AS_CENTER:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"center");
			return;
	}
}

ASFUNCTIONBODY_ATOM(TextField,_setAutoSize)
void TextField::setAutoSize(const ALIGNMENT& _type)
{
	if (autoSize == _type)
		return;

	autoSize = _type;
	setSizeAndPositionFromAutoSize();
	setHasChanged(true);
	setNeedsTextureRecalculation();
	if(isOnStage() && isVisible())
		requestInvalidation(getSys());
}

void TextField::setSizeAndPositionFromAutoSize(bool updatWwidth)
{
	constexpr Twips textPadding(TEXTFIELD_PADDING * 2);

	auto tryCentering = [](const ALIGN& align, number_t pos)
	{
		return align == AS_CENTER ? pos / 2 : pos;
	};

	auto _forEachLine = [&](const Twips& pos, const ALIGN& _align)
	{
		Locker l(lineMutex);
		for (auto& line : textlines)
		{
			bool isSmaller = line.textSize.x < textSize.x;
			line.autoSizePos = !isSmaller ? 0 : tryCentering
			(
				_align,
				pos - line.textSize.x
			);
		}
	};

	if (autoSize == AS_NONE && align != AS_RIGHT && align != AS_CENTER)
	{
		autoSizePos = 0;
		size.x =
		(
			updateWidth &&
			!wordWrap
		) ? originalWidth : size.x;
		return;
	}
	else if (autoSize == AS_NONE)
	{

		autoSizePos = tryCentering
		(
			align,
			size.x - textSize.x - textPadding
		);

		_forEachLine(textSize.x - textPadding, align);
		size.x =
		(
			updateWidth &&
			!wordWrap
		) ? originalWidth : size.x;
		return;
	}

	bool updatePos = autoSize == AS_RIGHT || autoSize == AS_CENTER;
	autoSizePos = 0;

	if (!updatePos && autoSize != AS_LEFT)
		return;

	size.x =
	(
		updateWidth &&
		!wordWrap
	) ? textSize.x + textPadding : size.x;

	if (autoSize == AS_LEFT)
		return;


	if (wordWrap)
	{
		auto newPos = size.x - textPadding - textSize.x;
		autoSizePos =
		(
			autoSize == AS_CENTER ? newPos / 2 :
			autoSize == AS_RIGHT ? std::max(0, newPos) :
			autoSizePos
		);
	}
	else
	{
		setX(tryCentering(autoSize, origX +
		(
			origWidth -
			textPadding -
			textSize.x
		)) * getScaleX());
	}

	_forEachLine(textSize.x, autoSize);
	size.y = textSize.y + textPadding;
	geometryChanged();
}

void setSize(const Vector2Twips& _size)
{
	if (size == _size || _size < Vector2Twips())
		return;

	size = _size;
	origWidth = size.x;
	setHasChanged(true);
	setNeedsTextureRecalculation();
	updateSizes()
	setSizeAndPositionFromAutoSize(false);
	size.x -= TEXTFIELD_PADDING * 2;

	if(isOnStage() && isVisible())
		requestInvalidation(getSys());
}

FormatText TextField::getTextFormat(size_t from, size_t to) const
{
	return *this;
}

void TextField::setTextFormat
(
	const FormatText& fmt,
	size_t from,
	size_t to
)
{
	if (fmt.fontColor.Alpha)
		textColor = fmt.fontColor;

	bool updateSizes = false;

	if (fmt.align != AS_NONE && align != fmt.align)
	{
		align = fmt.align;
		updateSizes = true;
	}

	if (!fmt.font.empty() && fontName != fmt.font)
	{
		fontName = fmt.font;
		fontID = UINT32_MAX;
		updateSizes = true;
	}

	if (fmt.fontSize && fontSize != fmt.fontSize)
	{
		fontSize = fmt.fontSize;
		updateSizes = true;
	}

	if (fmt.leftMargin.hasValue() && leftMargin != *fmt.leftMargin)
	{
		leftMargin = *fmt.leftMargin;
		updateSizes = true;
	}

	if (fmt.rightMargin.hasValue() && rightMargin != *fmt.rightMargin)
	{
		rightMargin = *fmt.rightMargin;
		updateSizes = true;
	}

	if (fmt.indent.hasValue() && indent != *fmt.indent)
	{
		indent = *fmt.indent;
		updateSizes = true;
	}

	if (fmt.blockIndent.hasValue() && blockIndent != *fmt.blockIndent)
	{
		blockIndent = *fmt.blockIndent;
		updateSizes = true;
	}

	if (!updateSizes)
	{
		textUpdated();
		return;
	}

	{
		Locker l(lineMutex);
		checkEmbeddedFont(*this);
		if (wordWrap)
		{
			FormatText format(*this);
			// reset text to single line
			setText
			(
				getText(UINT32_MAX, true),
				false,
				&format
			);
		}
	}

	updateSizes(true);
	setSizeAndPositionFromAutoSize(false);
	setHasChanged(true);
	setNeedsTextureRecalculation();
	textUpdated();
}

FormatText TextField::getDefaultTextFormat() const
{
	FormatText ret;

	ret.font = fontName;
	ret.bold = isBold;
	ret.italic = isItalic;
	return ret;
}

void TextField::setDefaultTextFormat(const FormatText& fmt)
{
	if (fmt.fontColor.Alpha)
		textColor = fmt.fontColor;

	if (!fmt.font.empty() && fontName != fmt.font)
	{
		fontName = fmt.font;
		fontID = UINT32_MAX;
	}

	if (fmt.fontSize && fontSize != fmt.fontSize)
		fontSize = fmt.fontSize;

	isBold = fmt.bold;
	isItalic = fmt.italic;

	if (fmt.align != AS_NONE && align != fmt.align)
	{
		align = fmt.align;
		updateSizes();
		setSizeAndPositionFromAutoSize();
		setHasChanged(true);
		setNeedsTextureRecalculation();
	}
}

size_t TextField::getLineIndexAtPoint(const Vector2Twips& pos) const
{
	Twips yMin;
	Twips yMax;

	Locker l(lineMutex);
	for (size_t i = 0; i < getLineCount(); ++i)
	{
		auto _autoSizePos = textlines[i].autoSizePos;
		auto textWidth = textlines[i].textSize.x;
		yMax += textlines[i].size.y;

		bool inBounds = Rect<Twips>
		{
			Vector2Twips(_autoSizePos, yMin),
			Vector2Twips(_autoSizePos + textWidth, yMin)

		}.intersects(pos);

		if (inBounds)
			return i;

		yMin += textlines[i].size.y;
	}
	return -1;
}

size_t TextField::getLineIndexOfChar(size_t idx) const
{
	size_t firstOffset = 0;
	Locker l(lineMutex);
	for (size_t i = 0; i < getLineCount(); ++i)
	{
		auto _textSize = textlines[i].text.numChars();
		if (idx >= firstOffset && idx < firstOffset + _textSize)
			return i;

		// Add 1 for the newline.
		firstOffset += _textSize + 1;
	}

	return -1;
}

size_t TextField::getLineLength(size_t lineIdx) const
{

	Locker l(lineMutex);
	if (lineIdx >= getLineCount())
		return -1;
	return textlines[lineIdx].text.numChars();
}

Optional<LineMetrics> TextField::getLineMetrics(size_t lineIdx) const
{

	Locker l(lineMutex);
	if (lineIdx >= getLineCount())
		return {};
	const auto& line = textlines[lineIdx];
	return LineMetrics
	(
		line.autoSizePos,
		line.textWidth,
		line.height,
		line.ascent,
		line.descent,
		line.format.leading
	);
}

size_t TextField::getLineOffset(size_t lineIdx) const
{
	Locker l(lineMutex);
	if (lineIdx >= getLineCount())
		return -1;

	auto lines = makeSpan(textlines).getFirst(lineIdx);
	size_t ret = 0;
	for (const auto& line : lines)
	{
		// Add 1 for the newline.
		ret += line.text.numChars() + 1;
	}
	return ret;
}

Optional<tiny_string> getLineText(size_t lineIdx) const
{
	Locker l(lineMutex);
	if (lineIdx >= getLineCount())
		return {};
	return textlines[lineIdx].text;
}

size_t TextField::getTextLength() const
{
	Locker l(lineMutex);
	return getText().numChars();
}

size_t TextField::getLineCountWithLock() const
{
	Locker l(lineMutex);
	return getLineCount();
}

size_t TextField::getBottomScrollV() const
{
	Twips yMin;

	Locker l(lineMutex);
	for (size_t i = 1; i < getLineCount(); ++i)
	{
		if (yMin >= size.y)
			return i;
		yMin += textlines[k-1].height;
	}
	return getLineCount() + 1;
}

void TextField::setSelection(size_t from, size_t to)
{
	Locker l(lineMutex);
	auto text = th->getText();

	from = std::min(from, text.numChars());
	to = std::min(to, text.numChars());

	caretIndex = to;
	if (from > to)
		std::swap(from, to);

	selectionBeginIndex = from
	selectionEndIndex = to;
}

void TextField::replaceText(size_t begin, size_t end, const tiny_string& newText)
{
	if (!styleSheet.isNull())
		return;

	tiny_string text;
	{
		Locker l(lineMutex);
		text = getText();
	}

	if (begin > end)
		return;

	if (begin >= text.numChars())
		text += newText;
	else if (end >= text.numChars())
		text = text.substr(0, begin) + newText;
	else
	{
		text = text.substr(0, begin) + newText + text.substr
		(
			end,
			tiny_string::npos
		);
	}

	{
		Locker l(lineMutex);
		setText(text);
	}
	textUpdated();
}

Rect<Twips> TextField::getTextBounds(const tiny_string& txt)
{
	if (embeddedFont != nullptr)
		scaling = 1 / 1024.0;

	Vector2Twips min(autoSizePos, 0);
	return Rect<Twips> { min, getTextSizes(getSys()) + min };
}

Optional<Rect<Twips>> TextField::getCharBounds(size_t charIdx) const
{
	tiny_string text;
	{
		Locker l(lineMutex);
		text = getText();
	}

	if (charIndex >= text.numChars())
		return {}
	Rect<Twips> rectA;
	number_t xmin=0,xmax=0,ymin=0,ymax=0;
	if (charIndex > 0)
		rectA = getTextBounds(text.substr(0, charIdx - 1));
	auto rectB = getTextBounds(text.substr(0, charIdx));

	Vector2Twips min(rectA.min.x, rectB.min.y),
	return Rect<Twips> { min, min + rectB.max };
}

ASFUNCTIONBODY_ATOM(TextField,getFirstCharInParagraph)
{
	TextField* th=asAtomHandler::as<TextField>(obj);

	int32_t charIndex;
	ARG_CHECK(ARG_UNPACK(charIndex));

	LOG(LOG_NOT_IMPLEMENTED,"TextField.getFirstCharInParagraph always returns 0");
	ret = asAtomHandler::fromInt(0);
}
ASFUNCTIONBODY_ATOM(TextField,getParagraphLength)
{
	TextField* th=asAtomHandler::as<TextField>(obj);

	int32_t charIndex;
	ARG_CHECK(ARG_UNPACK(charIndex));

	LOG(LOG_NOT_IMPLEMENTED,"TextField.getParagraphLength always returns 0");
	ret = asAtomHandler::fromInt(0);
}

void TextField::afterSetLegacyMatrix()
{
	setupOriginalPosition();
	textUpdated();
}
void TextField::setupOriginalPosition()
{
	originalXPosition = getMatrix().getTranslateX();
	originalWidth = width;
}

void TextField::validateThickness(number_t /*oldValue*/)
{
	if (needsActionScript3())
		thickness = dmin(dmax(thickness, -200.), 200.);
}

void TextField::validateSharpness(number_t /*oldValue*/)
{
	if (needsActionScript3())
		sharpness = dmin(dmax(sharpness, -400.), 400.);
}

void TextField::validateScrollH(int32_t oldValue)
{
	int32_t maxScrollH = getMaxScrollH();
	if (scrollH > maxScrollH)
		scrollH = maxScrollH;
	hasChanged=true;
	setNeedsTextureRecalculation();

	if (onStage && (scrollH != oldValue) && isVisible())
		requestInvalidation(this->getSystemState());
}

void TextField::validateScrollV(int32_t oldValue)
{
	int32_t maxScrollV = getMaxScrollV();
	if (scrollV < 1)
		scrollV = 1;
	else if (scrollV > maxScrollV)
		scrollV = maxScrollV;
	hasChanged=true;
	setNeedsTextureRecalculation();

	if (onStage && (scrollV != oldValue) && isVisible())
		requestInvalidation(this->getSystemState());
}

int32_t TextField::getMaxScrollH()
{
	if (wordWrap || (textWidth <= width))
		return 0;
	else
		return textWidth;
}

int32_t TextField::getMaxScrollV()
{
	Locker l(*linemutex);
	if (getLineCount() <= 1)
		return 1;
	int32_t Ymax = 0;
	for (uint32_t i = 0; i < getLineCount(); i++)
	{
		Ymax+=textlines[i].height;
	}
	if (Ymax <= (int32_t)height)
		return 1;

	// one full page from the bottom
	int32_t pagesize=0;
	for (int k=(int)getLineCount()-1; k>=0; k--)
	{
		pagesize+=textlines[k].height;
		if (Ymax - pagesize > (int32_t)height)
		{
			return imin(k+1+1, getLineCount());
		}
	}
	return 1;
}

void TextField::updateSizes(bool updateformat)
{
	Locker l(invalidatemutex);
	uint32_t tw,th;
	tw = 0;
	th = 0;
	
	scaling = 1.0f/1024.0f;
	th=0;
	number_t w=0;
	number_t h=0;

	linemutex->lock();
	auto it = textlines.begin();
	while (it != textlines.end())
	{
		if (updateformat)
			(*it).format = FormatText(*this);
		FontTag* ef = nullptr;
		if ((*it).format.embeddedfontID != UINT32_MAX)
			ef = this->loadedFrom->getEmbeddedFontByID((*it).format.embeddedfontID);
		getTextSizes(getSystemState(),(*it).format,ef,(*it).text,w,h);
		(*it).textwidth=w;
		(*it).height=h;
		bool listchanged=false;
		uint32_t realwidth = width;
		if (!it->format.leftmargin.empty())
			realwidth -= parseNumber(it->format.leftmargin)*TWIPS_FACTOR;
		if (!it->format.rightmargin.empty())
			realwidth -= parseNumber(it->format.rightmargin)*TWIPS_FACTOR;
		if (it->format.bullet)
			realwidth -= BULLER_INDENT*TWIPS_FACTOR;
		if (wordWrap
			&& realwidth > TEXTFIELD_PADDING*2
			&& uint32_t(w) > realwidth-TEXTFIELD_PADDING*2)
		{
			// calculate lines for wordwrap
			tiny_string text =(*it).text;
			uint32_t c= text.rfind(" ");// TODO check for other whitespace characters
			while (c != tiny_string::npos && c != 0)
			{
				getTextSizes(getSystemState(),(*it).format,ef,text.substr(0,c),w,h);
				if (w <= realwidth-TEXTFIELD_PADDING*2)
				{
					if(w>tw)
						tw = w;
					(*it).textwidth=w;
					(*it).text = text.substr(0,c).removeWhitespace();
					textline t;
					t.autosizeposition=0;
					text = text.substr(c,UINT32_MAX);
					if (!it->format.leftmargin.empty())
						text = text.removeWhitespace();
					getTextSizes(getSystemState(),(*it).format,ef,text,w,h);
					t.text = text;
					t.textwidth=w;
					t.height=h;
					t.format= (*it).format;
					it = textlines.insert(++it,t);
					listchanged=true;
					if (uint32_t(w) <= realwidth-TEXTFIELD_PADDING*2)
					{
						if(w>tw)
							tw = w;
						break;
					}
					th+=h;
					c=text.numChars();
				}
				c= text.rfind(" ",c-1);// TODO check for other whitespace characters
			}
		}
		else if (w>tw)
			tw = w;
		if (!listchanged)
			it++;
		th+=h;
		if (it != textlines.end())
			th+=this->leading/TWIPS_FACTOR;
	}
	linemutex->unlock();
	if(w>tw)
		tw = w;
	textWidth=tw;
	textHeight=th;
}

tiny_string TextField::toHtmlText()
{
	pugi::xml_document doc;

	Locker l(*linemutex);
	//Split text into paragraphs and wraps them into <p> tags
	int openParagraph=0;
	pugi::xml_node node=doc;
	std::stack<FormatText> formatstack;
	FormatText prevformat;
	FormatText lastformat;
	bool firstline=true;
	uint32_t prevlinebreaks=0;
	for (auto it = textlines.begin(); it != textlines.end(); it++)
	{
		bool mergetext=true;
		bool ignoretext=false;
		FormatText& format = (*it).format;
		pugi::xml_node prevnode = node;
		while (!formatstack.empty())
		{
			prevformat = formatstack.top();
			if (format.level < prevformat.level)
			{
				formatstack.pop();
				prevnode = prevnode.parent();
			}
			else
				break;
		}
		pugi::xml_node startnode = doc;
		bool islastline = it == textlines.end()-1;
		if ((firstline || it->needsnewline || it->linebreaks) && (
			!format.blockindent.empty()
			|| !format.blockindent.empty()
			|| !format.indent.empty()
			|| !format.leading.empty()
			|| !format.leftmargin.empty()
			|| !format.rightmargin.empty()
			|| !format.tabstops.empty()
																  ))
		{
			startnode = node = doc.append_child("TEXTFORMAT");
			formatstack.push(format);
			if (!format.blockindent.empty())
				node.append_attribute("BLOCKINDENT").set_value(format.blockindent.raw_buf());
			if (!format.rightmargin.empty())
				node.append_attribute("RIGHTMARGIN").set_value(format.rightmargin.raw_buf());
			if (!format.indent.empty())
				node.append_attribute("INDENT").set_value(format.indent.raw_buf());
			if (!format.leftmargin.empty())
				node.append_attribute("LEFTMARGIN").set_value(format.leftmargin.raw_buf());
			if (!format.leading.empty())
				node.append_attribute("LEADING").set_value(format.leading.raw_buf());
			if (!format.tabstops.empty())
				node.append_attribute("TABSTOPS").set_value(format.tabstops.raw_buf());
		}
		bool setfont = false;
		if (format.bullet)
		{
			setfont=true;
			node = doc.append_child("LI");
			formatstack.push(format);
		}
		else
		{
			if ((!this->multiline || !format.paragraph) && condenseWhite && (*it).text.empty())
				continue;
			if (openParagraph==0 || it->needsnewline || format.paragraph || (prevlinebreaks && !it->text.empty()))// || (swfversion < 7 && !format.bullet))
			{
				if (((!islastline || firstline) && openParagraph==0)
					|| (format.paragraph)
					|| (it->needsnewline)
					|| (prevlinebreaks && !it->text.empty())
					)
				{
					openParagraph++;
					setfont=true;
					tiny_string parentname = node.parent().name();
					if (format.paragraph && parentname=="FONT")
					{
						// it seems adobe merges text when adding paragraph inside font tag, but still closes the current paragraph?!?
						tiny_string t = node.text().as_string();
						t += (*it).text;
						node.text().set(t.raw_buf());
						ignoretext=true;
						setfont=false;
						node=startnode;
					}
					else
					{
						node = startnode.append_child("P");
						formatstack.push(format);
						switch (format.align)
						{
							case ALIGNMENT::AS_NONE:
							case ALIGNMENT::AS_LEFT:
								node.append_attribute("ALIGN").set_value("LEFT");
								break;
							case ALIGNMENT::AS_CENTER:
								node.append_attribute("ALIGN").set_value("CENTER");
								break;
							case ALIGNMENT::AS_RIGHT:
								node.append_attribute("ALIGN").set_value("RIGHT");
								break;
							default:
								break;
						}
					}
				}
			}
		}
		bool fontchanged = format.font!=BUILTIN_STRINGS::EMPTY && (format.font != prevformat.font || !(format.fontColor == prevformat.fontColor) || format.fontSize != prevformat.fontSize);
		bool needsbold=false;
		bool needsitalic=false;
		bool needsunderline=false;
		bool needsurl=false;
		if (setfont || fontchanged)
		{
			bool addFontTag = true;
			if (!setfont && fontchanged)
			{
				if (format.level==prevformat.level)
				{
					if (formatstack.size() > 1)
					{
						formatstack.pop();
						FormatText tmpformat = formatstack.top();
						addFontTag = format.font != tmpformat.font || !(format.fontColor == tmpformat.fontColor) || format.fontSize != tmpformat.fontSize;
						formatstack.push(tmpformat);
						if (!addFontTag)
							node = prevnode.parent().append_child(pugi::node_pcdata);
					}
				}
			}
			if (addFontTag)
			{
				if (!it->needsnewline && !format.paragraph && !format.bullet &&
					(prevformat.bold || prevformat.italic || prevformat.underline || !prevformat.url.empty() || !prevformat.target.empty()))
				{
					if (format.level>=prevformat.level)
					{
						if (prevformat.bold == lastformat.bold)
							needsbold=prevformat.bold;
						if (prevformat.italic == lastformat.italic)
							needsitalic=prevformat.italic;
						if (prevformat.underline == lastformat.underline)
							needsunderline=prevformat.underline;
						if (prevformat.url == lastformat.url && prevformat.target == lastformat.target)
							needsurl=!prevformat.url.empty() || !prevformat.target.empty();
					}
					if (!prevlinebreaks)
						node = node.parent();
				}
				node = node.append_child("FONT");
				formatstack.push(format);
				ostringstream ss;
				ss << format.fontSize;
				if (setfont || format.font != lastformat.font)
					node.append_attribute("FACE").set_value(getSystemState()->getStringFromUniqueId(format.font).raw_buf());
				if (setfont || format.fontSize != lastformat.fontSize)
					node.append_attribute("SIZE").set_value(ss.str().c_str());
				if (setfont || !(format.fontColor == lastformat.fontColor))
					node.append_attribute("COLOR").set_value(format.fontColor.toString(false).uppercase().raw_buf());
				if (setfont || format.letterspacing != lastformat.letterspacing)
					node.append_attribute("LETTERSPACING").set_value(format.letterspacing);
				if (setfont || format.kerning != lastformat.kerning)
					node.append_attribute("KERNING").set_value(format.kerning);
			}
		}
		if (format.url != prevformat.url || format.target != prevformat.target || needsurl)
		{
			if (!needsurl && !prevformat.url.empty())
				node = node.parent();
			if (!format.url.empty() || needsurl)
			{
				node = node.append_child("A");
				formatstack.push(format);
				node.append_attribute("HREF").set_value(format.url.raw_buf());
				node.append_attribute("TARGET").set_value(format.target.raw_buf());
			}
			else if (!firstline && node.type() != pugi::node_null)
			{
				node = node.parent();
				mergetext=false;
			}
		}
		if (format.bold != prevformat.bold || (it->needsnewline && format.bold) || needsbold)
		{
			if (format.bold || needsbold)
			{
				node = node.append_child("B");
				formatstack.push(format);
			}
			else if (!firstline && node.type() != pugi::node_null)
			{
				if (!needsbold && lastformat.bold && !prevlinebreaks)
					node = node.parent();
				mergetext=false;
			}
		}
		if (format.italic != prevformat.italic || (it->needsnewline && format.italic) || needsitalic)
		{
			if (format.italic || needsitalic)
			{
				node = node.append_child("I");
				formatstack.push(format);
			}
			else if (!firstline && node.type() != pugi::node_null)
			{
				if (!needsitalic && lastformat.italic && !prevlinebreaks)
					node = node.parent();
				mergetext=false;
			}
		}
		if (format.underline != prevformat.underline || (it->needsnewline && format.underline) || needsunderline)
		{
			if (format.underline || needsunderline)
			{
				node = node.append_child("U");
				formatstack.push(format);
			}
			else if (!firstline && node.type() != pugi::node_null)
			{
				if (!needsunderline && lastformat.underline && !prevlinebreaks)
					node = node.parent();
				mergetext=false;
			}
		}
		if (!ignoretext)
		{
			tiny_string t = node.text().as_string();
			if (mergetext)
				t += (*it).text;
			else
			{
				node = node.append_child(pugi::node_pcdata);
				t = (*it).text;
			}
			node.text().set(t.raw_buf());
		}
		lastformat=format;
		prevlinebreaks=it->linebreaks;
		firstline=false;
	}

	ostringstream buf;
	doc.print(buf,"\t",pugi::format_raw|pugi::format_no_escapes);
	tiny_string ret = tiny_string(buf.str());
	return ret;
}

void TextField::setHtmlText(const tiny_string& html)
{
	linemutex->lock();
	vector<tiny_string> oldtext;
	vector<FormatText> oldformats;
	if (this->isConstructed())
	{
		oldtext.reserve(textlines.size());
		oldformats.reserve(textlines.size());
		for (uint32_t i =0; i < textlines.size(); i++)
		{
			oldtext.push_back(textlines[i].text);
			oldformats.push_back(textlines[i].format);
		}
	}
	uint32_t swfversion = this->loadedFrom->version;
	HtmlTextParser parser(swfversion,condenseWhite,multiline,this->loadedFrom);
	parser.parseTextAndFormating(html, this);
	if(swfversion >= 8 && condenseWhite && isWhitespaceOnly(multiline))
		clear();
	if (swfversion >= 7 && !multiline && getLineCount()>1 && this->textlines.back().text.empty())
	{
		//more than one line and last line is empty => remove last line
		this->textlines.pop_back();
	}
	linemutex->unlock();

	isHtml=true;
	if (this->isConstructed() && !this->TextIsEqual(oldtext,oldformats))
	{
		hasChanged=true;
		setNeedsTextureRecalculation();
		textUpdated();
	}
}

std::string TextField::toDebugString() const
{
	std::string res = InteractiveObject::toDebugString();
	res += " \"";
	res += this->getText(0);
	res += "\";";
	char buf[100];
	sprintf(buf,"(%i) %5.2fx%5.2f %5.2f %d/%d %s",this->getLineCount(),textWidth/TWIPS_FACTOR,textHeight/TWIPS_FACTOR,autosizeposition,autoSize,align,getSystemState()->getStringFromUniqueId(fontname).raw_buf());
	res += buf;
	return res;
}

void TextField::updateText(const tiny_string& new_text)
{
	if (!hasChanged && getText() == new_text)
		return;
	linemutex->lock();
	FormatText format(*this);
	setText(new_text.raw_buf(),false,&format);
	linemutex->unlock();
	textUpdated();
}

void TextField::avm1SyncTagVar()
{
	if (!tagvarname.empty()
		&& tagvarname != "_url") // "_url" is readonly and always read from root movie, no need to update
	{
		if (tagvartarget && !inAVM1syncVar)
		{
			inAVM1syncVar=true;
			asAtom value=asAtomHandler::invalidAtom;
			number_t n;
			linemutex->lock();
			if (Integer::fromStringFlashCompatible(getText().raw_buf(),n,10,true))
				value = asAtomHandler::fromNumber(n);
			else
				value = asAtomHandler::fromString(getSystemState(),getText());
			linemutex->unlock();
			ASATOM_INCREF(value); // ensure that value is not destructed during AVM1SetVariable
			tagvartarget->as<MovieClip>()->AVM1SetVariable(tagvarname,value);
			ASATOM_DECREF(value);
			inAVM1syncVar=false;
		}
	}
}

void TextField::UpdateVariableBinding(asAtom v)
{
	inUpdateVarBinding = true;
	tiny_string s = asAtomHandler::toString(v,getInstanceWorker());
	if (!s.empty() && tag->isHTML())
		setHtmlText(s);
	else
		updateText(s);
	inUpdateVarBinding = false;
}

void TextField::afterLegacyInsert()
{
	if (!tagvarname.empty() && !getConstructIndicator())
	{
		tagvartarget = getParent();
		uint32_t finddot = tagvarname.rfind(".");
		if (finddot != tiny_string::npos)
		{
			tiny_string path = tagvarname.substr(0,finddot);
			tagvartarget = tagvartarget->AVM1GetClipFromPath(path);
			tagvarname = tagvarname.substr(finddot+1,tagvarname.numChars()-(finddot+1));
		}
		while (tagvartarget)
		{
			if (tagvartarget->is<MovieClip>())
			{
				tagvartarget->as<MovieClip>()->setVariableBinding(tagvarname,this);
				asAtom value = tagvartarget->as<MovieClip>()->getVariableBindingValue(tagvarname);
				if (asAtomHandler::isValid(value) && !asAtomHandler::isUndefined(value))
				{
					UpdateVariableBinding(value);
				}
				ASATOM_DECREF(value);
				break;
			}
			tagvartarget = tagvartarget->getParent();
		}
		if (tagvartarget)
		{
			tagvartarget->incRef();
			tagvartarget->addStoredMember();
		}
	}
	if (!loadedFrom->usesActionScript3 && !getConstructIndicator())
	{
		setConstructIndicator();
		constructionComplete();
		afterConstruction();
	}
	originalWidth=width;
	avm1SyncTagVar();
	InteractiveObject::afterLegacyInsert();
}

void TextField::afterLegacyDelete(bool inskipping)
{
	if (!tagvarname.empty() && !inskipping)
	{
		if (tagvartarget)
		{
			tagvartarget->as<MovieClip>()->setVariableBinding(tagvarname,nullptr);
			tagvartarget->removeStoredMember();
			tagvartarget=nullptr;
		}
	}
}

void TextField::lostFocus()
{
	SDL_StopTextInput();
	getSystemState()->removeJob(this);
	caretblinkstate = false;
	hasChanged=true;
	setNeedsTextureRecalculation();
	if(onStage && isVisible())
		requestInvalidation(this->getSystemState());
}

void TextField::gotFocus()
{
	if (this->type != ET_EDITABLE)
		return;
	SDL_StartTextInput();
	getSystemState()->addTick(500,this);
}

void TextField::textInputChanged(const tiny_string &newtext)
{
	if (this->type != ET_EDITABLE)
		return;
	linemutex->lock();
	tiny_string tmptext = getText();
	linemutex->unlock();
	
	if (maxChars == 0 || tmptext.numChars()+newtext.numChars() <= uint32_t(maxChars))
	{
		if (caretIndex< 0)
			caretIndex=0;
		if (caretIndex < int(tmptext.numChars()))
			tmptext.replace(caretIndex,0,newtext);
		else
			tmptext+=newtext;
		caretIndex+= newtext.numChars();
	}
	this->updateText(tmptext);
}

void TextField::tick()
{
	if (this->type != ET_EDITABLE)
		return;
	if (this == getSystemState()->stage->getFocusTarget())
		caretblinkstate = !caretblinkstate;
	else
		caretblinkstate = false;
	hasChanged=true;
	setNeedsTextureRecalculation();
	
	if(onStage && isVisible())
		requestInvalidation(this->getSystemState());
}

void TextField::tickFence()
{
}

uint32_t TextField::getTagID() const
{
	return tag ? tag->getId() : UINT32_MAX;
}

bool TextField::isFocusable(bool fromMouse)
{
	return selectable || !fromMouse;
}

int TextField::getTextCharCount()
{
	Locker l(*linemutex);
	return getText().numChars();
}

void TextField::textUpdated()
{
	// Don't sync the bound variable if we're updating the binding.
	if (!inUpdateVarBinding)
		avm1SyncTagVar();
	scrollH = 0;
	scrollV = 1;
	linemutex->lock();
	checkEmbeddedFont(this);
	linemutex->unlock();
	updateSizes();
	setSizeAndPositionFromAutoSize();
	setNeedsTextureRecalculation();
	hasChanged=true;

	if(onStage && isVisible())
		requestInvalidation(this->getSystemState());
	else
		requestInvalidationFilterParent(this->getSystemState());
}

void TextField::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (!tokensEmpty())
		TokenContainer::requestInvalidation(q,forceTextureRefresh);
	else
	{
		requestInvalidationFilterParent(q);
		q->addToInvalidateQueue(this);
	}
}

void TextField::defaultEventBehavior(_R<Event> e)
{
	if (this->type != ET_EDITABLE && e->type == "keyDown")
	{
		KeyboardEvent* ev = e->as<KeyboardEvent>();
		const LSModifier& modifiers = ev->getModifiers();
		if (modifiers == LSModifier::None)
		{
			switch (ev->getKeyCode())
			{
				case AS3KEYCODE_BACKSPACE:
					linemutex->lock();
					if (!this->getText().empty() && caretIndex > 0)
					{
						caretIndex--;
						tiny_string tmptext = getText();
						if (caretIndex < int(tmptext.numChars()))
							tmptext.replace(caretIndex,1,"");
						else
							tmptext = tmptext.substr(0,tmptext.numChars()-1);
						setText(tmptext.raw_buf());
						linemutex->unlock();
						textUpdated();
					}
					else
						linemutex->unlock();
					break;
				case AS3KEYCODE_LEFT:
					if (this->caretIndex > 0)
						this->caretIndex--;
					break;
				case AS3KEYCODE_RIGHT:
					linemutex->lock();
					if (this->caretIndex < int(this->getText().numChars()))
						this->caretIndex++;
					linemutex->unlock();
					break;
				default:
					break;
			}
		}
		else
		{
			bool handled = false;
			switch (ev->getKeyCode())
			{
				case AS3KEYCODE_V:
					if (modifiers & LSModifier::Ctrl)
					{
						textInputChanged(tiny_string(SDL_GetClipboardText()));
						handled = true;
					}
					break;
				default:
					break;
			}
			if (!handled)
				LOG(LOG_NOT_IMPLEMENTED,"TextField keyDown event handling for modifier "<<modifiers<<" and char code "<<hex<<ev->getCharCode());
		}
	}
	else if (e->type =="mouseDown")
	{
		if (this->isHtml && !this->textlines.empty())
		{
			MouseEvent* ev = e->as<MouseEvent>();
			int xpos=0,ypos=0;
			tiny_string url;
			auto it = textlines.begin();
			while (it != textlines.end())
			{
				if (!it->format.url.empty()
					&& ypos <= ev->localY*TWIPS_FACTOR
					&& ypos+it->height > ev->localY*TWIPS_FACTOR
					&& xpos <= ev->localX*TWIPS_FACTOR
					&& xpos+it->textwidth > ev->localX*TWIPS_FACTOR  )
				{
					url = it->format.url;
					break;
				}
				xpos += it->textwidth;
				uint32_t curparagraph = it->format.paragraph;
				int curheight = it->height * (it->linebreaks+1);
				++it;
				if (it == textlines.end())
					break;
				if (it->format.paragraph != curparagraph)
				{
					xpos = 0;
					ypos += curheight;
					if (ypos > ev->localY*TWIPS_FACTOR)
						break;
				}
			}
			if (!url.empty())
			{
				if (this->needsActionScript3())
				{
					LOG(LOG_NOT_IMPLEMENTED,"[TextField] click on URL:"<<url);
				}
				else
				{
					if (url.startsWith("asfunction:"))
					{
						tiny_string funcname = url.substr_bytes(11,url.numBytes()-11);
						tiny_string arg;
						uint32_t split = funcname.findFirst(",");
						if (split != tiny_string::npos)
						{
							arg = funcname.substr(split+1,tiny_string::npos);
							funcname = funcname.substr(0,split);
						}
						if (!funcname.empty())
						{
							asAtom func = asAtomHandler::invalidAtom;
							multiname m(nullptr);
							m.name_type = multiname::NAME_STRING;
							m.name_s_id = getSystemState()->getUniqueStringId(funcname);
							m.isAttribute = false;
							asAtom obj = asAtomHandler::fromObject(this);
							if (getParent())
							{
								DisplayObject* clip = getParent();
								uint32_t pathindex = funcname.findLast(".");
								if (pathindex != tiny_string::npos)
								{
									tiny_string path = funcname.substr(0,pathindex);
									clip = getParent()->AVM1GetClipFromPath(path);
									m.name_s_id = getSystemState()->getUniqueStringId(funcname.substr(pathindex+1,tiny_string::npos));
								}
								if (clip)
								{
									clip->AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker(),false);
									obj = asAtomHandler::fromObject(clip);
								}
							}
							if (!asAtomHandler::isValid(func))
							{
								AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
								obj = asAtomHandler::fromObject(this);
							}
							if (!asAtomHandler::isValid(func))
							{
								this->getSystemState()->avm1global->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
								obj = asAtomHandler::fromObject(this->getSystemState()->avm1global);
							}

							if (asAtomHandler::is<IFunction>(func))
							{
								asAtom ret = asAtomHandler::invalidAtom;
								asAtom args = arg.empty() ? asAtomHandler::undefinedAtom : asAtomHandler::fromString(getSystemState(),arg);
								asAtomHandler::callFunction(func,getInstanceWorker(),ret,obj,&args,1,false);
								ASATOM_DECREF(ret);
							}
							ASATOM_DECREF(func);
						}
					}
					else
						LOG(LOG_ERROR,"[TextField] click on invalid URL:"<<url);
				}
			}
		}
	}
}

IDrawable* TextField::invalidate(bool smoothing)
{
	Locker l(invalidatemutex);
	number_t x,y;
	number_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax,false)==false)
	{
		//No contents, nothing to do
		return nullptr;
	}

	ColorTransformBase ct;
	ct.fillConcatenated(this);
	MATRIX matrix = getMatrix();
	bool isMask=this->isMask();
	MATRIX m;
	m.scale(matrix.getScaleX(),matrix.getScaleY());
	computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,m);
	tokens.clear();
	if (!tokens.filltokens)
		tokens.filltokens = _MR(new tokenListRef());
	scaling = 1.0f/1024.0f/20.0f;
	if ( this->background)
	{
		fillstyleBackgroundColor.FillStyleType=SOLID_FILL;
		fillstyleBackgroundColor.Color=this->backgroundColor;
		tokens.filltokens->tokens.push_back(GeomToken(SET_FILL).uval);
		tokens.filltokens->tokens.push_back(GeomToken(fillstyleBackgroundColor).uval);
		tokens.filltokens->tokens.push_back(GeomToken(MOVE).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2(bxmin/scaling, bymin/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2(bxmin/scaling, (bymax-bymin)/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2((bxmax-bxmin)/scaling, (bymax-bymin)/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2((bxmax-bxmin)/scaling, bymin/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2(bxmin/scaling, bymin/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(CLEAR_FILL).uval);
	}
	if (this->border)
	{
		lineStyleBorder.Color=this->borderColor;
		lineStyleBorder.Width=0;//hairline
		tokens.filltokens->tokens.push_back(GeomToken(SET_STROKE).uval);
		tokens.filltokens->tokens.push_back(GeomToken(lineStyleBorder).uval);
		tokens.filltokens->tokens.push_back(GeomToken(MOVE).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2(bxmin/scaling, bymin/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2(bxmin/scaling, (bymax-bymin)/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2((bxmax-bxmin)/scaling, (bymax-bymin)/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2((bxmax-bxmin)/scaling, bymin/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2(bxmin/scaling, bymin/scaling)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(CLEAR_STROKE).uval);
	}
	if (this->caretblinkstate)
	{
		uint32_t tw=0;
		if (!getText().empty())
		{
			tiny_string tmptxt = getText().substr(0,caretIndex);
			number_t w,h;
			getTextSizes(getSystemState(),FormatText(),nullptr,tmptxt,w,h);
			tw = w;
			tw += autosizeposition/scaling;
		}
		else
		{
			tw += autosizeposition;
			tw /=scaling;
		}
		lineStyleCaret.Color=RGB(0,0,0);
		lineStyleCaret.Width=40;
		int ypadding = (bymax-bymin-2)/scaling;
		tokens.filltokens->tokens.push_back(GeomToken(SET_STROKE).uval);
		tokens.filltokens->tokens.push_back(GeomToken(lineStyleCaret).uval);
		tokens.filltokens->tokens.push_back(GeomToken(MOVE).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2(tw, bymin/scaling+ypadding)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken(Vector2(tw, (bymax-bymin)/scaling-ypadding)).uval);
		tokens.filltokens->tokens.push_back(GeomToken(CLEAR_STROKE).uval);
	}
	if (embeddedFont)
	{
		int32_t startposy = TEXTFIELD_PADDING+bymin+(embeddedFont->getAscent())*fontSize/1024*TWIPS_FACTOR;

		linemutex->lock();
		RGBA color(textColor.Red,textColor.Green,textColor.Blue,0xff);
		tokensVector* tk = &tokens;
		bool first = tk->empty();
		for (auto it = textlines.begin(); it != textlines.end(); it++)
		{
			if (startposy > (int32_t)this->height)
				break;
			if ((*it).text.empty())
			{
				startposy += (embeddedFont->getAscent()+embeddedFont->getDescent()+embeddedFont->getLeading())*fontSize/1024*TWIPS_FACTOR;
				continue;
			}
			if (!first)
				tk = tk->next = new tokensVector();

			first = false;
			int startposx = (TEXTFIELD_PADDING+autosizeposition+(*it).autosizeposition);
			if (it->format.bullet)
			{
				tiny_string bullet = tiny_string::fromChar(0x2022); // unicode bullet char
				tk = embeddedFont->fillTextTokens(*tk,bullet,(*it).format,color,leading,startposx,startposy);
				startposx += BULLER_INDENT*TWIPS_FACTOR;
				tk = tk->next = new tokensVector();
			}
			if (isPassword)
			{
				tiny_string pwtxt;
				for (uint32_t i = 0; i < (*it).text.numChars(); i++)
					pwtxt+="*";
				tk = embeddedFont->fillTextTokens(*tk,pwtxt,(*it).format,color,leading,startposx,startposy);
			}
			else
				tk = embeddedFont->fillTextTokens(*tk,(*it).text,(*it).format,color,leading,startposx,startposy);
			startposy += (embeddedFont->getAscent()+embeddedFont->getDescent()+embeddedFont->getLeading())*fontSize/1024*TWIPS_FACTOR;
		}
		linemutex->unlock();
		if (tokens.empty())
		{
			this->resetNeedsTextureRecalculation();
			return new RefreshableDrawable(x, y, ceil(width), ceil(height)
										   , matrix.getScaleX(), matrix.getScaleY()
										   , isMask, cacheAsBitmap
										   , getScaleFactor(), getConcatenatedAlpha()
										   , ct, smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
		}
		// it seems that textfields are always rendered with subpixel smoothing when rendering to bitmap
		return TokenContainer::invalidate(smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,false,tokens);
	}
	if (this->type != ET_EDITABLE)
	{
		Locker l(*linemutex);
		if (getLineCount()==0)
		{
			this->resetNeedsTextureRecalculation();
			return new RefreshableDrawable(x, y, ceil(width), ceil(height)
										   , matrix.getScaleX(), matrix.getScaleY()
										   , isMask, cacheAsBitmap
										   , getScaleFactor(), getConcatenatedAlpha()
										   , ct, smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
		}
	}
	if(width==0 || height==0)
	{
		this->resetNeedsTextureRecalculation();
		return new RefreshableDrawable(x, y, ceil(width), ceil(height)
									   , matrix.getScaleX(), matrix.getScaleY()
									   , isMask, cacheAsBitmap
									   , getScaleFactor(), getConcatenatedAlpha()
									   , ct, smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
	}

	float xscale = getConcatenatedMatrix().getScaleX();
	float yscale = getConcatenatedMatrix().getScaleY();
	// use specialized Renderer from EngineData, if available, otherwise fallback to nanoVG
	IDrawable* res = this->getSystemState()->getEngineData()->getTextRenderDrawable(*this,matrix, x, y, ceil(width), ceil(height),
																					xscale,yscale,isMask,cacheAsBitmap, 1.0f,getConcatenatedAlpha(),
																					ColorTransformBase(),
																					smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode());
	if (res != nullptr)
		return res;
	res = new RefreshableDrawable(x,y, ceil(width), ceil(height)
								   , matrix.getScaleX(), matrix.getScaleY()
								   , isMask, cacheAsBitmap
								   , TWIPS_FACTOR, getConcatenatedAlpha()
								   , ct, smoothing ? SMOOTH_MODE::SMOOTH_SUBPIXEL : SMOOTH_MODE::SMOOTH_NONE,this->getBlendMode(),matrix);
	res->getState()->tokens.filltokens = tokens.filltokens;
	res->getState()->tokens.stroketokens = tokens.stroketokens;
	res->getState()->tokens.next = tokens.next;
	res->getState()->tokens.color = tokens.color;
	res->getState()->tokens.startMatrix = tokens.startMatrix;
	res->getState()->textdata = *this;
	res->getState()->renderWithNanoVG = true;
	this->resetNeedsTextureRecalculation();
	return res;

}
void TextField::refreshSurfaceState()
{
	linemutex->lock();
	getCachedSurface()->getState()->textdata = *this;
	linemutex->unlock();
}

void TextField::HtmlTextParser::parseTextAndFormating(const tiny_string& html,
						      TextData *dest)
{
	textdata = dest;
	if (!textdata)
		return;

	tiny_string rooted = html;
	if (condenseWhite)
	{
		// according to ruffle swf >= 8 condenses whitespace across html tags, so we condense whitspace later
		rooted = swfversion < 8 ? html.compactHTMLWhiteSpace(false) : html.trimLeft();
	}
	if (rooted.isWhiteSpaceOnly())
	 	return;
	uint32_t pos=0;
	// ensure <br> tags are properly parsed
	while ((pos = rooted.find("<br>",pos)) != tiny_string::npos)
		rooted.replace_bytes(pos,4,"<br />");
	pugi::xml_document doc;
	unsigned int options = pugi::parse_cdata | pugi::parse_escapes | pugi::parse_wconv_attribute | pugi::parse_eol | pugi::parse_fragment | pugi::parse_embed_pcdata;
	if ((dest->multiline && swfversion > 6) || swfversion>=8)
	{
		options |= pugi::parse_ws_pcdata;
		if (swfversion<8)
			options |= pugi::parse_ws_pcdata_single;
	}
	pugi::xml_parse_result result = doc.load_buffer(rooted.raw_buf(),rooted.numBytes(), options);
	if (result.status == pugi::status_ok)
	{
		textdata->clear();
		doc.traverse(*this);
		if (textdata->getLineCount() == 0 && !formatStack.empty() && formatStack.back().paragraph)
			textdata->appendFormatText("",formatStack.back(),swfversion,condenseWhite);
		formatStack.erase(formatStack.begin(), formatStack.end());
		textdata->checklastline(rooted.endsWith("\n</p>") || rooted.endsWith("\n</li>"));
	}
	else
	{
		LOG(LOG_ERROR, "TextField HTML parser error:"<<rooted);
		LOG(LOG_ERROR, "Reason: " << result.description());
		LOG(LOG_ERROR, "Offset: " << result.offset);
		LOG(LOG_ERROR, "Text at offset: " << (rooted.raw_buf() + result.offset));
		return;
	}
}

bool TextField::HtmlTextParser::for_each(pugi::xml_node &node)
{
	if (!textdata)
		return true;

	int currentDepth = depth();
	tiny_string name = node.name();
	name = name.lowercase();
	tiny_string v = node.value();
	tiny_string newtext;
	tiny_string parentname = node.parent().name();
	if (currentDepth < prevDepth)
	{
		for (int i = currentDepth; i < prevDepth; ++i)
			formatStack.pop_back();
	}
	FormatText format;
	if (formatStack.empty())
	{
		format = FormatText(*textdata);
		formatStack.push_back(format);
	}
	format = formatStack.back();
	format.level=currentDepth;
	uint32_t index =v.find("&nbsp;");
	while (index != tiny_string::npos)
	{
		v.replace(index,6," ");
		index =v.find("&nbsp;",index);
	}
	newtext += v;
	bool emptycontent= node.children().begin() == node.children().end();
	if (name == "br" || name == "sbr") // adobe seems to interpret the unknown tag <sbr /> as <br> ?
	{
		if (parentname=="textformat" && !format.bullet)
			format.paragraph = ++textdata->maxParagraphID;
		textdata->appendLineBreak(currentDepth == prevDepth
								  ,((parentname=="textformat" || format.bullet) && strlen(node.parent().value())==0) || (node.previous_sibling().type() == pugi::node_null && node.next_sibling().type() == pugi::node_null)
								  ,format);
	}
	if (name == "p")
	{
		format.paragraph= ++textdata->maxParagraphID;
		for (auto it=node.attributes_begin(); it!=node.attributes_end(); ++it)
		{
			tiny_string attrname = it->name();
			attrname = attrname.lowercase();
			tiny_string value = it->value();
			if (attrname == "align")
			{
				if (value == "left")
				{
					textdata->align = ALIGNMENT::AS_LEFT;
					format.align = ALIGNMENT::AS_LEFT;
				}
				if (value == "center")
				{
					textdata->align = ALIGNMENT::AS_CENTER;
					format.align = ALIGNMENT::AS_CENTER;
				}
				if (value == "right")
				{
					textdata->align = ALIGNMENT::AS_RIGHT;
					format.align = ALIGNMENT::AS_RIGHT;
				}
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,"TextField html tag <"<<name<<">: unsupported attribute:"<<attrname);
			}
		}
	}
	else if (name == "font")
	{
		if (format.paragraph)
			emptycontent=false;
		else
			format.paragraph= ++textdata->maxParagraphID;
		for (auto it=node.attributes_begin(); it!=node.attributes_end(); ++it)
		{
			tiny_string attrname = it->name();
			attrname = attrname.lowercase();
			if (attrname == "face")
				format.font = appdomain->getSystemState()->getUniqueStringId(it->value());
			else if (attrname == "size")
				format.fontSize = parseFontSize(it->value(), format.fontSize);
			else if (attrname == "color")
				format.fontColor = RGB(tiny_string(it->value()));
			else if (attrname == "kerning")
			{
				format.kerning = it->as_double();
				if (appdomain)
				{
					// it seems that adobe overwrites the kerning setting if it is an embedded font without kerning table
					FontTag* font = appdomain->getEmbeddedFont(appdomain->getSystemState()->getStringFromUniqueId(format.font));
					if (font && !font->hasKerning())
						format.kerning = 0;
				}
			}
			else if (attrname == "letterspacing")
			{
				format.letterspacing = it->as_double();
			}
			else
				LOG(LOG_NOT_IMPLEMENTED,"TextField html tag <font>: unsupported attribute:"<<attrname<<" "<<it->value());
		}
	}
	else if (name == "a")
	{
		for (auto it : node.attributes())
		{
			tiny_string attrname = it.name();
			attrname = attrname.lowercase();
			if (attrname == "href")
				format.url = it.value();
			else if (attrname == "target")
				format.target = it.value();
		}
	}
	else if (name == "b")
		format.bold = true;
	else if (name == "i")
		format.italic = true;
	else if (name == "u")
		format.underline = true;
	else if (name == "li")
		format.bullet = true;
	else if (name == "textformat")
	{
		// Adobe seems to ignore textformat tags not on root level except if it is the child of a textformat or font tag
		if (currentDepth==0 || parentname=="textformat" || parentname=="font")
		{
			for (auto it : node.attributes())
			{
				tiny_string attrname = it.name();
				tiny_string attrvalue = it.value();
				if (attrvalue=="0")
					continue;
				attrname = attrname.lowercase();
				if (attrname == "blockindent")
					format.blockindent = attrvalue;
				else if (attrname == "indent")
					format.indent = attrvalue;
				else if (attrname == "leading")
					format.leading = attrvalue;
				else if (attrname == "leftmargin")
					format.leftmargin = attrvalue;
				else if (attrname == "rightmargin")
					format.rightmargin = attrvalue;
				else if (attrname == "tabstops")
					format.tabstops = attrvalue;
			}
		}
		else
		{
			if (emptycontent && newtext.empty())
				textdata->appendLineBreak(false,true,format);
		}
	}
	else if (name == "img" || name == "span" ||  name == "tab")
	{
		LOG(LOG_NOT_IMPLEMENTED, "Unsupported tag in TextField: " << name);
	}
	if (!emptycontent)
		formatStack.push_back(format);
	if (swfversion < 8  && condenseWhite && newtext.removeWhitespace().empty())
		newtext="";
	if (!newtext.empty()
		|| (swfversion < 7 && (name == "p" || name == "li") && emptycontent)
		|| (((textdata->multiline && swfversion>=7)|| swfversion>=8)
			&& (emptycontent
				|| (name != "p" && name != "li" && name != "font"))))
	{
		textdata->appendFormatText(newtext.raw_buf(), format,swfversion,condenseWhite);
	}
	prevDepth = currentDepth;
	prevName = name;
	return true;
}

uint32_t TextField::HtmlTextParser::parseFontSize(const char* s,
						  uint32_t currentFontSize)
{
	if (!s)
		return currentFontSize;

	uint32_t basesize = 0;
	int multiplier = 1;
	if (s[0] == '+' || s[0] == '-')
	{
		// relative size
		basesize = currentFontSize;
		if (s[0] == '-')
			multiplier = -1;
		s++;
	}
	if (s[0]<'0'||s[0]>'9')
		return currentFontSize;

	int64_t size = basesize + multiplier*strtoll(s, nullptr, 10);
	if (size < 1)
		size = 1;
	if (size > 127)
		size = 127;
	
	return (uint32_t)size;
}
