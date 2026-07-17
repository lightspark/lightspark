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

#define BULLET_INDENT 36.0

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

static constexpr bool isSwfNewline(uint32_t ch)
{
	return ch == '\n' || ch == '\b';
}

size_t TextField::getParagraphStart(size_t idx) const
{
	tiny_string text;
	{
		Locker l(lineMutex);
		text = getText();
	}

	// NOTE: The index can be equal to the text length.
	if (idx > text.numChars())
		return -1;

	for(; idx && isSwfNewline(text[idx]); --idx);
	return idx;
}

size_t TextField::getParagraphLength(size_t idx) const
{
	auto startIdx = getParagraphStart(idx);
	if (startIdx == -1)
		return -1;

	tiny_string text;
	{
		Locker l(lineMutex);
		text = getText();
	}

	size_t textLen = text.numChars();
	// NOTE: If the index is equal to the text length, Flash Player will
	// act as if a character is at that point, and return the length of
	// the last paragraph + 1.
	if (idx == textLen)
		return textLen - startIdx + 1;

	for(; idx < textLen && isSwfNewline(text[idx]); ++idx);
	// NOTE: The trailing newline also counts towards the length.
	idx += idx < textLen && isSwfNewline(text[idx]);
	return idx - startIdx;
}

void TextField::afterSetLegacyMatrix()
{
	setupOriginalPosition();
	textUpdated();
}
void TextField::setupOriginalPosition()
{
	origXPos = getMatrix().tx;
	origWidth = size.x;
}

void TextField::setScrollH(size_t _scrollH);
{
	bool isDifferent = scrollH != _scrollH;
	scrollH = _scrollH;
	setHasChanged(true);
	setNeedsTextureRecalculation();

	if (isOnStage() && isDifferent && isVisible())
		requestInvalidation(getSys());
}

void TextField::setScrollV(number_t _scrollV);
{
	// NOTE: While not exact, this overflow limit was experimentally
	// found by the Ruffle team, and was found to be the same in both
	// AVM1, and 2.
	constexpr number_t overflowLimit = 767100486418433.0;
	bool scrollOverflows =
	(
		std::isnan(_scrollV) ||
		_scrollV < 0 ||
		_scrollV >= overflowLimit
	);

	auto scrollLines = clampTmpl<size_t>
	(
		scrollOverflows ? 1 : _scrollV,
		1,
		getMaxScrollV()
	);

	if (scrollV == scrollLines)
		return;

	scrollV = scrollLines;
	setHasChanged(true);
	setNeedsTextureRecalculation();

	if (isOnStage() && isVisible())
		requestInvalidation(getSys());
}

size_t TextField::getMaxScrollH()
{
	return !wordWrap && textSize.x > size.x ? textSize.x : 0;
}

size_t TextField::getMaxScrollV()
{
	Locker l(lineMutex);
	if (getLineCount() <= 1)
		return 1;

	size_t yMax = 0;
	for (const auto& line : textlines)
		yMax += line.height;

	if (yMax <= size.y)
		return 1;

	// one full page from the bottom
	size_t pageSize = 0;
	for (size_t i = getLineCount() - 1; i; --i)
	{
		pageSize += textlines[i].height;
		if (yMax - pageSize > size.y)
			return std::min(i + 2, getLineCount());
	}

	return 1;
}

void TextField::updateSizes(bool updateFormat)
{
	constexpr Twips textPadding = Twips::fromPx(TEXTFIELD_PADDING * 2);

	Locker l(invalidateMutex);
	Vector2u _textSize;
	Vector2f _size;
	scaling = 1 / 1024.0;

	auto trySetTextWidth = [&]
	{
		if (_size.x > _textSize.x)
			_textSize.x = _size.x;
	};

	Locker l(lineMutex);
	for (auto it = textlines.begin() it != textlines.end(); ++it)
	{
		auto& line = *it;
		if (updateFormat)
			line.format = FormatText(*this);
		const auto& fmt = line.format;
		auto fontTag =
		(
			fmt.embeddedFontID != UINT32_MAX ?
			movie.getEmbeddedFontByID(fmt.embeddedFontID) :
			nullptr
		);

		line.size = getTextSizes
		(
			getSys(),
			fmt,
			fontTag,
			line.text
		);

		bool listChanged = false;
		auto realWidth = size.x;
		if (fmt.leftMargin.hasValue())
			realWidth -= *fmt.leftMargin;
		if (fmt.rightMargin.hasValue())
			realWidth -= *fmt.rightMargin;
		if (fmt.bullet)
			realWidth -= BULLET_INDENT;


		bool calcWordWrap =
		(
			wordWrap &&
			realwidth > textPadding &&
			_size.x > realwidth - textPadding
		);

		bool listChanged = false;
		if (calcWordWrap)
		{
			constexpr auto npos = tiny_string::npos;
			// calculate lines for wordwrap
			auto text = line.text;
			auto pos = text.rfind(' ');// TODO check for other whitespace characters

			auto _realWidth = realWidth - textPadding;
			for (; pos && pos != npos; pos = text.rfind(' ', pos - 1))
			{
				_size = getTextSizes
				(
					getSys(),
					fmt,
					fontTag,
					text.substr(0, pos)
				);

				if (_size.x > _realWidth)
					continue;

				trySetTextWidth();
				line.size.x = _size.x;
				line.text = text.substr(0, pos).removeWhitespace();

				text = text.substr(pos, npos);
				if (line.leftMargin.hasValue())
					text = text.removeWhitespace();

				_size = getTextSizes
				(
					getSys(),
					fmt,
					fontTag,
					text
				);

				listChanged = true;
				it = textlines.insert(++it, textline
				{
					.autoSizePos = 0,
					.text = text,
					.size = _size,
					.format = fmt,
				});

				if (_size.x <= _realWidth)
				{
					trySetTextWidth();
					break;
				}

				_textSize.y += _size.y;
				pos = text.numChars();
			}
		}
		else
			trySetTextWidth();

		_textSize.y += _size.y;
		if (it != textlines.end())
			_textSize.y += leading;
		if (listChanged)
			--it;
	}

	trySetTextWidth();
	textSize = _textSize;
}

tiny_string TextField::toHtmlText()
{
	auto isSameFont = []
	(
		const FormatText& a,
		const FormatText& b
	)
	{
		return
		(
			a.font == b.font &&
			a.fontColor == b.fontColor &&
			a.fontSize == b.fontSize
		);
	};

	pugi::xml_document doc;

	//Split text into paragraphs and wraps them into <p> tags
	size_t openParagraph = 0;
	pugi::xml_node node = doc;
	std::stack<FormatText> formatStack;
	FormatText prevFormat;
	FormatText lastFormat;
	bool firstline = true;
	size_t prevLineBreaks = 0;

	auto addFontAttr = [&]
	(
		bool flag,
		bool lastFlag,
		bool needsFlag,
		const char* tagStr
	)
	{
		if (flag || needsFlag)
		{
			node = node.append_child(tagStr);
			formatStack.push(format);
			return true;
		}
		else if (!firstLine || node.type() == pugi::node_null)
			return true;

		if (!needsFlag && lastFlag && !prevLineBreaks)
			node = node.parent();
		return false;
	};

	Locker l(lineMutex);
	for (auto it = textlines.begin(); it != textlines.end(); it++)
	{
		const auto& line = *it;
		bool mergeText = true;
		bool ignoreText = false;
		const auto& format = line.format;
		auto prevNode = node;

		while (!formatStack.empty())
		{
			prevFormat = formatstack.top();
			if (format.level >= prevFormat.level)
				break;

			formatStack.pop();
			prevNode = prevNode.parent();
		}

		pugi::xml_node startNode = doc;
		bool isLastLine = it == textlines.end() - 1;

		bool hasAttrs =
		(
			firstLine ||
			line.needsNewline ||
			line.linebreaks
		) &&
		(
			format.blockIndent.hasValue() ||
			format.indent.hasValue() ||
			format.leading.hasValue() ||
			format.leftMargin.hasValue() ||
			format.rightMargin.hasValue() ||
			format.tabStops.hasValue()
		)

		if (hasAttrs)
		{
			startNode = node = doc.append_child("TEXTFORMAT");
			formatStack.push(format);
			if (format.blockIndent.hasValue())
				node.append_attribute("BLOCKINDENT").set_value(*format.blockIndent);
			if (format.rightMargin.hasValue())
				node.append_attribute("RIGHTMARGIN").set_value(*format.rightMargin);
			if (format.indent.hasValue())
				node.append_attribute("INDENT").set_value(*format.indent);
			if (format.leftMargin.hasValue())
				node.append_attribute("LEFTMARGIN").set_value(*format.leftMargin);
			if (format.leading.hasValue())
				node.append_attribute("LEADING").set_value(*format.leading);
			if (format.tabStops.hasValue())
				node.append_attribute("TABSTOPS").set_value(*format.tabStops);
		}

		bool setFont = false;

		if (format.bullet)
		{
			setFont = true;
			node = doc.append_child("LI");
			formatStack.push(format);
		}
		else if
		(
			condenseWhite &&
			line.text.empty() &&
			(!multiline || !format.paragraph)
		)
			continue;
		else if
		(
			(!openParagraph && (!isLastLine || firstLine)) ||
			format.paragraph ||
			line.needsNewline ||
			(prevLineBreaks && !line.text.empty())
		)
		{
			openParagraph++;
			setFont = true;
			tiny_string parentName = node.parent().name();
			if (format.paragraph && parentName == "FONT")
			{
				// it seems adobe merges text when adding paragraph inside font tag, but still closes the current paragraph?!?
				tiny_string t = node.text().as_string();
				node.text().set((t + line.text).raw_buf());
				ignoreText = true;
				setFont = false;
				node = startNode;
			}
			else
			{
				node = startNode.append_child("P");
				formatStack.push(format);
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
		bool fontChanged = !format.font.empty() && !isSameFont
		(
			format,
			prevFormat
		);
		bool needsBold = false;
		bool needsItalic = false;
		bool needsUnderline = false;
		bool needsUrl = false;

		if (!setFont && !fontChanged)
			goto checkUrl;
		if
		(
			!setFont &&
			fontChanged &&
			format.level == prevformat.level &&
			formatStack.size() > 1
		)
		{
			formatStack.pop();
			auto tmpFmt = formatstack.top();
			_addFontTag = !isSameFont(format, tmpFmt);
			formatstack.push(tmpFmt);
			if (!_addFontTag)
				node = preNnode.parent().append_child(pugi::node_pcdata);
		}

		if (!_addFontTag)
			goto checkUrls;

		if
		(
			line.needsNewline ||
			format.paragraph ||
			format.bullet
		)
			goto addFontTag;
		if
		(
			!prevFormat.bold &&
			!prevFormat.italic &&
			!prevFormat.underline &&
			prevFormat.url.empty() &&
			prevFormat.target.empty()
		)
			goto addFontTag;

		if (format.level >= prevformat.level)
		{
			needsBold =
			(
				prevFormat.bold ==
				lastFormat.bold
			) && prevFormat.bold;

			needsItalic =
			(
				prevFormat.italic ==
				lastFormat.italic
			) && prevFormat.italic;

			needsUnderline =
			(
				prevFormat.underline ==
				lastFormat.underline
			) && prevFormat.underline;
			needsUrl =
			(
				prevFormat.url == lastFormat.url &&
				prevFormat.target == lastFormat.target
			) &&
			(
				!prevFormat.url.empty() ||
				!prevFormat.target.empty()
			);
		}

		if (!prevLineBreaks)
			node = node.parent();

	addFontTag:
		node = node.append_child("FONT");
		formatStack.push(format);
		std::stringstream s;
		s << format.fontSize;
		if (setFont || format.font != lastFormat.font)
			node.append_attribute("FACE").set_value(format.font.raw_buf());
		if (setFont || format.fontSize != lastFormat.fontSize)
			node.append_attribute("SIZE").set_value(ss.str().c_str());
		if (setFont || format.fontColor != lastFormat.fontColor)
		{
			auto str = format.fontColor.toString(false);
			node.append_attribute("COLOR").set_value(str.uppercase().raw_buf());
		}
		if (setFont || format.letterspacing != lastFormat.letterspacing)
			node.append_attribute("LETTERSPACING").set_value(format.letterspacing);
		if (setFont || format.kerning != lastFormat.kerning)
			node.append_attribute("KERNING").set_value(format.kerning);
	checkUrl:
		if
		(
			format.url == prevformat.url &&
			format.target == prevformat.target &&
			!needsUrl
		)
			goto checkBold;

		if (!needsUrl && !prevFormat.url.empty())
			node = node.parent();
		if (!format.url.empty() || needsUrl)
		{
			node = node.append_child("A");
			formatStack.push(format);
			node.append_attribute("HREF").set_value(format.url.raw_buf());
			node.append_attribute("TARGET").set_value(format.target.raw_buf());
		}
		else if (!firstLine && node.type() != pugi::node_null)
		{
			node = node.parent();
			mergeText = false;
		}

	checkBold:
		if
		(
			format.bold != prevformat.bold ||
			(line.needsNewline && !format.bold) ||
			needsBold
		)
		{
			mergeText = addFontAttr
			(
				format.bold,
				needsBold,
				lastFormat.bold,
				"B"
			);
		}

		if
		(
			format.italic != prevFormat.italic ||
			(line.needsNewline && format.italic) &&
			needsItalic
		)
		{
			mergeText = addFontAttr
			(
				format.italic,
				needsItalic,
				lastFormat.italic,
				"I"
			);
		}

		if
		(
			format.underline != prevFormat.underline ||
			(line.needsNewline && format.underline) ||
			needsUnderline
		)
		{
			mergeText = addFontAttr
			(
				format.underline,
				needsUnderline,
				lastFormat.underline,
				"U"
			);
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
		lastFormat = format;
		prevLineBreaks = line.linebreaks;
		firstLine = false;
	}

	std::stringstream s;
	doc.print(s, "\t", pugi::format_raw|pugi::format_no_escapes);
	return buf.str();
}

void TextField::setHtmlText(const tiny_string& html)
{
	std::vector<tiny_string> oldText;
	std::vector<FormatText> oldFormats;

	lineMutex.lock();
	if (isConstructed())
	{
		oldText.reserve(textlines.size());
		oldFormats.reserve(textlines.size());

		for (const auto& line : textlines)
		{
			oldText.emplace_back(line.text);
			oldFormats.emplace_back(line.format);
		}
	}

	auto swfVersion = getSwfVersion();
	HtmlTextParser parser
	(
		swfVersion,
		condenseWhite,
		multiline,
		movie
	);

	parser.parseTextAndFormating(html, *this);
	if
	(
		swfVersion >= 8 &&
		condenseWhite &&
		isWhitespaceOnly(multiline)
	)
		clear();
	if
	(
		swfVersion >= 7 &&
		!multiline &&
		getLineCount() > 1 &&
		textlines.back().text.empty()
	)
	{
		//more than one line and last line is empty => remove last line
		textlines.pop_back();
	}
	lineMutex.unlock();

	isHtml = true;
	if (isConstructed() && TextIsEqual(oldText, oldFormats))
	{
		setHasChanged(true);
		setNeedsTextureRecalculation();
		textUpdated();
	}
}

std::string TextField::toDebugString() const
{
	std::stringstream s;
	return
	(
		s << InteractiveObject::toDebugString() <<
		'"' << getText(0) << "\";" <<
		'(' << getLineCount() << ") " <<
		textSize.x << 'x' << textSize.y << ' ' <<
		autoSizePos << ' ' <<
		autoSize << '/' << align << ' ' <<
		fontName
	).str();
}

void TextField::updateText(const tiny_string& newText)
{
	if (!hasChanged() && getText() == newText)
		return;
	{
		Locker l(lineMutex);
		FormatText format(*this);
		setText(newText, false, &format);
	}
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

// Based on Ruffle's `EditText::try_bind_text_field_variable()`.
bool TextField::tryBindVar(AVM1Activation& act, bool setInitVal)
{
	// A `TextField` with no variable is treated as a success by default.
	if (tagVarName.empty())
		return true;

	// The previous binding (if any) should've been cleared at this point.
	assert(tagVarTarget == nullptr);

	auto parent = getAVM1Parent();
	for
	(
		;
		parent != nullptr && parent->is<Button>();
		parent = parent->getAVM1Parent()
	);

	if (parent == nullptr)
		return false;

	auto childFrameFunc = [&](AVM1Activation& _act) -> Any
	{
		auto pair = _act.resolveVariablePath(*parent, tagVarName);
		if (!pair.hasValue())
			return false;

		auto obj = pair->first;
		const auto& prop = pair->second;
		// If this `TextField` was just created, we propagate the text
		// to the variable (or vice versa).
		if (!setInitVal)
			goto checkObject;

		// If the prop exists on the object, we overwrite the text with
		// the prop's value.
		if (obj.hasProp(act, prop))
		{
			setHtmlText(obj->getProp
			(
				act,
				prop
			).tryToString(act).valueOr(""));
		}
		// Otherwise, we only initialize the prop with the `TextField`'s
		// text, if it's non empty.
		// NOTE: HTML `TextField`s are usually initialized with an empty
		// `<p>` tag, which isn't considered empty.
		else if (!getText().empty())
			obj->setProp(act, prop, getText());
	checkObject:
		auto dispObj = obj->as<DisplayObject>();
		if (dispObj == nullptr)
			return false;
		tagVarTarget = dispObj;
		AVM1VarBinding(*this, prop).registerBinding(dispObj);
		return true;
	};

	return act.runChildFrameForClip
	(
		"[TextField Binding]",
		*parent,
		getSwfVersion(),
		childFrameFunc
	).unsafeAs<bool>();
}

void TextField::avm1Unload()
{
	dropFocus();
	DisplayObject::avm1Unload();

	if (tagVarTarget != nullptr)
	{
		AVM1VarBinding::clearBinding(tagVarTarget, *this);
		tagVarTarget = nullptr;
	}

	AVM1VarBinding::unregisterBindings(*this);

	if (!tagVarName.empty())
		getSys()->removeUnboundTextField(*this);
	setRemovedByAVM1(true);
}

void TextField::afterTimelineInsertion()
{
	if (isAS3() || tryToAVM1Object().isNull())
		return;

	AVM1Context::runWithStackFrameForClip(*this, [&](auto& act)
	{
		if (!tryBindVar(act, true))
			getSys()->addUnboundTextField(*this);
		AVM1VarBinding::bindVars(act);
		initBroadcaster(act);
	});

	origWidth = size.x;
}

void TextField::afterTimelineDeletion(bool inskipping)
{
	if (inskipping)
		avm1Unload();
}

void TextField::lostFocus()
{
	SDL_StopTextInput();
	getSys()->removeJob(this);
	caretBlinkState = false;
	setHasChanged(true);
	setNeedsTextureRecalculation();
	if(isOnStage() && isVisible())
		requestInvalidation(getSys());
}

void TextField::gotFocus()
{
	if (editType != ET_EDITABLE)
		return;

	SDL_StartTextInput();
	getSys()->addTick(500, this);
}

void TextField::textInputChanged(const tiny_string& newText)
{
	if (editType != ET_EDITABLE)
		return;

	tiny_string tmp;
	{
		Locker l(lineMutex;
		tmpText = getText();
	}
	
	if (maxChars && tmp.numChars() + newText.numChars() > maxChars)
	{
		updateText(tmp);
		return;
	}

	caretIndex = std::max(caretIndex, 0);

	if (caretIndex < tmp.numChars())
		tmp.replace(caretIndex, 0, newText);
	else
		tmp += newText;

	caretIndex += newText.numChars();
	updateText(tmp);
}

void TextField::tick()
{
	if (editType != ET_EDITABLE)
		return;

	caretBlinkState ^= this == getSys()->stage->getFocusTarget();
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
	{
		Locker l(lineMutex;
		checkEmbeddedFont(this);
	}

	updateSizes();
	setSizeAndPositionFromAutoSize();
	setNeedsTextureRecalculation();
	setHasChanged(true);
	if(isOnStage() && isVisible())
		requestInvalidation(getSys());
	else
		requestInvalidationFilterParent(getSys());
}

void TextField::requestInvalidation
(
	InvalidateQueue* q,
	bool forceTextureRefresh)
{
	if (tokensEmpty())
	{
		requestInvalidationFilterParent(q);
		q->addToInvalidateQueue(this);
		return;
	}

	TokenContainer::requestInvalidation(q, forceTextureRefresh);
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
	Locker l(invalidateMutex);

	auto smoothMode =
	(
		smoothing ?
		SMOOTH_MODE::SMOOTH_SUBPIXEL :
		SMOOTH_MODE::SMOOTH_NONE
	);

	auto _rect = tryBoundsRect(false);
	if (!_bounds.hasValue())
	{
		//No contents, nothing to do
		return nullptr;
	}

	ColorTransform ct;
	ct.fillConcatenated(this);
	auto matrix = getMatrix();

	MATRIX m;
	m.scale(matrix.getScale());
	auto _bounds = computeBoundsForTransformedRect(_rect, m);

	tokens.clear();
	if (tokens.filltokens.isNull())
		tokens.filltokens = _MR(new tokenListRef());
	scaling = 1 / 1024.0;

	auto makeRefreshableDrawable = [&]
	{
		resetNeedsTextureRecalculation();
		return new RefreshableDrawable
		(
			_bounds.min,
			_bounds.max.ceil(),
			matrix.getScale(),
			isMask(),
			getCachedBitmapPreference(),
			getScaleFactor(),
			getConcatenatedAlpha()
			ct,
			smoothMode,
			getBlendMode(),
			matrix
		);
	};

	auto addRect = [&]
	(
		const GEOM_TOKEN_TYPE& _type,
		const GeomToken& arg
	)
	{
		auto& _fillTokens = tokens.filltokens->tokens;
		auto boundsSize = _bounds.size();
		_fillTokes.insert(_fillTokens.end(),
		{
			GeomToken(_type).uval,
			arg.uval,
			GeomToken(MOVE).uval,
			GeomToken(Vector2(_bounds.min) / scaling).uval,
			GeomToken(STRAIGHT).uval,
			GeomToken(Vector2
			(
				_bounds.min.x,
				boundsSize.y
			) / scaling).uval,
			GeomToken(STRAIGHT).uval,
			GeomToken(Vector2(boundsSize) / scaling).uval,
			GeomToken(STRAIGHT).uval,
			GeomToken(Vector2
			(
				boundsSize.x,
				_bounds.min.y
			) / scaling).uval,
			GeomToken(STRAIGHT).uval,
			GeomToken(Vector2(_bounds.min) / scaling).uval,
			GeomToken(CLEAR_FILL).uval
		});

	};

	if (background)
	{
		fillStyleBackgroundColor.FillStyleType = SOLID_FILL;
		fillStyleBackgroundColor.Color = backgroundColor;
		addRect(SET_FILL, fillStyleBackgroundColor);
	}

	if (border)
	{
		lineStyleBorder.Color = borderColor;
		lineStyleBorder.Width = 0; //hairline
		addRect(SET_STROKE, lineStyleBorder);
	}

	if (this->caretblinkstate)
	{
		uint32_t textWidth = !getText().empty() ? getTextSizes
		(
			getSys(),
			FormatText(),
			nullptr,
			getText().substr(0, caretIndex)
		).x + autoSizePos : autoSizePos;

		lineStyleCaret.Color = RGB();
		lineStyleCaret.Width = 40;
		Vector2 padding(0, _rect.size().y - 2);

		tokens.filltokens->tokens.push_back(GeomToken(SET_STROKE).uval);
		tokens.filltokens->tokens.push_back(GeomToken(lineStyleCaret).uval);
		tokens.filltokens->tokens.push_back(GeomToken(MOVE).uval);
		tokens.filltokens->tokens.push_back(GeomToken((Vector2
		(
			textWidth,
			_rect.min.y
		) + padding) / scaling).uval);
		tokens.filltokens->tokens.push_back(GeomToken(STRAIGHT).uval);
		tokens.filltokens->tokens.push_back(GeomToken((Vector2
		(
			textWidth,
			_rect.size().y
		) - padding) / scaling).uval);
		tokens.filltokens->tokens.push_back(GeomToken(CLEAR_STROKE).uval);
	}

	if (embeddedFont != nullptr)
	{
		Vector2Twips startPos(0,
		(
			TEXTFIELD_PADDING +
			_rect.min.y +
			embeddedFont->getAscent() *
			fontSize / 1024
		));
		RGBA color = textColor;
		auto tk = &tokens;
		bool first = tk->empty();

		lineMutex.lock();
		for (auto& line : textlines)
		{
			if (startPos.y > size.y)
				break;
			if (line.text.empty())
			{
				startPos.y +=
				(
					embeddedFont->getAscent() +
					embeddedFont->getDescent() +
					embeddedFont->getLeading()
				) * fontSize / 1024;
				continue;
			}

			if (!first)
				tk = tk->next = new tokensVector();

			first = false;
			startPos.x =
			(
				TEXTFIELD_PADDING +
				autoSizePos +
				line.autoSizePos
			);

			if (line.format.bullet)
			{
				auto bullet = tiny_string::fromChar(0x2022); // unicode bullet char
				tk = embeddedFont->fillTextTokens
				(
					*tk,
					// unicode bullet char,
					tiny_string::fromChar(0x2022),
					line.format,
					color,
					leading,
					startPos
				);
				startPos.x += BULLET_INDENT;
				tk = tk->next = new tokensVector();
			}

			auto _text = isPassword ? tiny_string(std::string
			(
				'*',
				line.text.numChars()
			) : line.text;

			tk = embeddedFont->fillTextTokens
			(
				*tk,
				_text,
				line.format,
				color,
				leading,
				startPos
			);

			startPos.y +=
			(
				embeddedFont->getAscent() +
				embeddedFont->getDescent() +
				embeddedFont->getLeading()
			) * fontSize / 1024;
		}
		lineMutex.unlock();

		if (tokens.empty())
			return makeRefreshableDrawable();
		{
			resetNeedsTextureRecalculation();
			return new RefreshableDrawable
			(
				_bounds.min,
				_bounds.max.ceil(),
				matrix.getScale(),
				isMask(),
				getCachedBitmapPreference(),
				getScaleFactor(),
				getConcatenatedAlpha()
				ct,
				smoothMode,
				getBlendMode(),
				matrix
			);
		}
		// it seems that textfields are always rendered with subpixel smoothing when rendering to bitmap
		return TokenContainer::invalidate(smoothMode, false, tokens);
	}
	if (editType != ET_EDITABLE)
	{
		Locker l(lineMutex);
		if (!getLineCount())
			return makeRefreshableDrawable();
	}

	if (!size.x || !size.y)
		return makeRefreshableDrawable();

	auto _scale = getConcatenatedMatrix().getScale();
	// use specialized Renderer from EngineData, if available, otherwise fallback to nanoVG
	auto ret = getSys()->getEngineData()->getTextRenderDrawable
	(
		*this,
		matrix,
		_bounds.min,
		_bounds.max.ceil(),
		getConcatenatedMatrix().getScale(),
		isMask(),
		getCachedBitmapPreference(),
		1.0f,
		getConcatenatedAlpha(),
		ColorTransformBase(),
		smoothMode,
		getBlendMode()
	);
	if (ret != nullptr)
		return ret;

	ret = makeRefreshableDrawable();
	ret->getState()->tokens.filltokens = tokens.filltokens;
	ret->getState()->tokens.stroketokens = tokens.stroketokens;
	ret->getState()->tokens.next = tokens.next;
	ret->getState()->tokens.color = tokens.color;
	ret->getState()->tokens.startMatrix = tokens.startMatrix;
	ret->getState()->textdata = *this;
	ret->getState()->renderWithNanoVG = true;
	return ret;

}
void TextField::refreshSurfaceState()
{
	Locker l(lineMutex)
	getCachedSurface()->getState()->textdata = *this;
}

void TextField::HtmlTextParser::parseTextAndFormating
(
	const tiny_string& html,
	TextData* dest
)
{
	auto swfVersion = getSwfVersion();

	textData = dest;
	if (textdata == nullptr)
		return;

	auto rooted = html;
	if (condenseWhite)
	{
		// according to ruffle swf >= 8 condenses whitespace across html tags, so we condense whitspace later
		rooted =
		(
			swfVersion < 8 ?
			html.compactHTMLWhiteSpace(false) :
			html.trimLeft()
		);
	}

	if (rooted.isWhiteSpaceOnly())
	 	return;
	size_t pos = rooted.rfind("<br>");
	// ensure <br> tags are properly parsed

	for (; pos != tiny_string::npos; pos = rooted.find("<br>", pos))
		rooted.replace_bytes(pos, 4, "<br />");

	pugi::xml_document doc;
	auto options =
	(
		pugi::parse_cdata |
		pugi::parse_escapes |
		pugi::parse_wconv_attribute |
		pugi::parse_eol |
		pugi::parse_fragment |
		pugi::parse_embed_pcdata
	);

	if (swfVersion >= 8)
		options |= pugi::parse_ws_pcdata;
	else if (dest->multiline && swfVersion > 6)
	{
		options |=
		(
			pugi::parse_ws_pcdata |
			pugi::parse_ws_pcdata_single
		);
	}

	pugi::xml_parse_result result = doc.load_buffer
	(
		rooted.raw_buf(),
		rooted.numBytes(),
		options
	);

	if (result.status != pugi::status_ok)
	{
		LOG(LOG_ERROR, "TextField HTML parser error:"<<rooted);
		LOG(LOG_ERROR, "Reason: " << result.description());
		LOG(LOG_ERROR, "Offset: " << result.offset);
		LOG(LOG_ERROR, "Text at offset: " << (rooted.raw_buf() + result.offset));
		return;
	}

	textdata->clear();
	doc.traverse(*this);

	bool _addFormatText =
	(
		!textData->getLineCount() &&
		!formatStack.empty() &&
		formatStack.back().paragraph
	);
	if (_addFormatText)
	{
		textdata->appendFormatText
		(
			"",
			formatStack.back(),
			swfVersion,
			condenseWhite
		);
	}

	formatStack.clear();
	textData->checklastline
	(
		rooted.endsWith("\n</p>") ||
		rooted.endsWith("\n</li>")
	);
}

bool TextField::HtmlTextParser::for_each(pugi::xml_node& node)
{
	if (textData == nullptr)
		return true;

	auto currentDepth = depth();
	auto name = tiny_string(node.name()).lowercase();
	tiny_string v = node.value();
	tiny_string newText;
	tiny_string parentName = node.parent().name();

	if (currentDepth < prevDepth)
	{
		for (size_t i = currentDepth; i < prevDepth; ++i)
			formatStack.pop_back();
	}

	FormatText format;
	if (formatStack.empty())
	{
		format = FormatText(*textdata);
		formatStack.push_back(format);
	}

	format = formatStack.back();
	format.level = currentDepth;

	size_t idx = v.find("&nbsp;");
	for (; idx != tiny_string::npos; index = v.find("&nbsp;", idx))
		v.replace(idx, 6, " ");

	newText += v;
	bool emptyContent =
	(
		node.children().begin() ==
		node.children().end()
	);

	if (name == "br" || name == "sbr") // adobe seems to interpret the unknown tag <sbr /> as <br> ?
	{
		if (parentName == "textformat" && !format.bullet)
			format.paragraph = ++textData->maxParagraphID;

		bool firstLineOnly =
		((
			parentName == "textformat" ||
			format.bullet
		) && !strlen(node.parent().value())) ||
		(
			node.previous_sibling().type() == pugi::node_null &&
			node.next_sibling().type() == pugi::node_null
		);
		textData->appendLineBreak
		(
			currentDepth == prevDepth,
			firstLineOnly,
			format
		);
	}
	else if (name == "p")
	{
		format.paragraph = ++textData->maxParagraphID;
		for (auto attr : node.attributes())
		{
			auto attrName = tiny_string(attr.name()).lowercase();
			if (attrName != "align")
			{
				LOG
				(
					LOG_NOT_IMPLEMENTED,
					"TextField html tag <" << name <<
					">: unsupported attribute:" <<
					attrName
				);
				continue;
			}

			tiny_string value = attr.value();
			if (value == "left")
			{
				textData->align = ALIGNMENT::AS_LEFT;
				format.align = ALIGNMENT::AS_LEFT;
			}
			else if (value == "center")
			{
				textData->align = ALIGNMENT::AS_CENTER;
				format.align = ALIGNMENT::AS_CENTER;
			}
			else if (value == "right")
			{
				textData->align = ALIGNMENT::AS_RIGHT;
				format.align = ALIGNMENT::AS_RIGHT;
			}
		}
	}
	else if (name == "font")
	{
		if (format.paragraph)
			emptyContent = false;
		else
			format.paragraph= ++textData->maxParagraphID;
		for (auto attr : node.attributes())
		{
			auto attrName = tiny_string(attr.name()).lowercase();
			auto value = attr.value();
			if (attrName == "face")
				format.font = value;
			else if (attrName == "size")
				format.fontSize = parseFontSize(value, format.fontSize);
			else if (attrName == "color")
				format.fontColor = RGB(tiny_string(value));
			else if (attrName == "kerning")
			{
				format.kerning = it->as_double();
				// it seems that adobe overwrites the kerning setting if it is an embedded font without kerning table
				auto font = movie.getEmbeddedFont(format.font);
				if (font != nullptr && !font->hasKerning())
					format.kerning = 0;
			}
			else if (attrName == "letterspacing")
				format.letterspacing = it->as_double();
			else
			{
				LOG
				(
					LOG_NOT_IMPLEMENTED,
					"TextField html tag <font>: "
					"unsupported attribute:" <<
					attrName << ' ' << value
				);
			}
		}
	}
	else if (name == "a")
	{
		for (auto attr : node.attributes())
		{
			auto attrName = tiny_string(attr.name()).lowercase();
			if (attrName == "href")
				format.url = attr.value();
			else if (attrName == "target")
				format.target = attr.value();
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
		if (!currentDepth || parentName == "textformat" || parentName == "font")
		{
			for (auto it : node.attributes())
			{
				tiny_string attrValue = it.value();
				if (attrValue == "0")
					continue;

				auto attrName = tiny_string(it.name()).lowercase();
				if (attrName == "blockindent")
					format.blockindent = attrValue;
				else if (attrName == "indent")
					format.indent = attrValue;
				else if (attrName == "leading")
					format.leading = attrValue;
				else if (attrName == "leftmargin")
					format.leftmargin = attrValue;
				else if (attrName == "rightmargin")
					format.rightmargin = attrValue;
				else if (attrName == "tabstops")
					format.tabstops = attrValue;
			}
		}
		else if (emptyContent && newtext.empty())
			textdata->appendLineBreak(false, true, format);
	}
	else if (name == "img" || name == "span" ||  name == "tab")
	{
		LOG
		(
			LOG_NOT_IMPLEMENTED,
			"Unsupported tag in TextField: " << name
		);
	}

	if (!emptyContent)
		formatStack.push_back(format);
	if (swfVersion < 8  && condenseWhite && newText.removeWhitespace().empty())
		newText = "";

	bool _addFormatText = !newText.empty() ||
	(
		swfversion < 7 &&
		(name == "p" || name == "li") &&
		emptyContent
	) ||
	(
		(
			swfVersion >= 8 ||
			(
				textData->multiline &&
				swfVersion == 7
			)
		) &&
		(
			emptycontent ||
			(
				name != "p" &&
				name != "li" &&
				name != "font"
			)
		)
	);

	if (_addFormatText)
	{
		textData->appendFormatText
		(
			newText,
			format,
			swfVersion,
			condenseWhite
		);
	}

	prevDepth = currentDepth;
	prevName = name;
	return true;
}

uint8_t TextField::HtmlTextParser::parseFontSize
(
	const tiny_string& str,
	size_t currentFontSize
)
{
	if (str.empty())
		return currentFontSize;

	auto it = str.begin();
	size_t baseSize = 0;
	ssize_t mult = 1;
	if (*it == '+' || *it == '-')
	{
		// relative size
		baseSize = currentFontSize;
		mult = *it++ != '-' ? 1 : -1;
	}

	if (*it < '0'|| *it > '9')
		return currentFontSize;

	return iclamp
	(
		baseSize + mult * str.substr
		(
			it,
			tiny_string::npos
		).tryToNumber<size_t>().valueOr(-1),
		1,
		127
	);
}
