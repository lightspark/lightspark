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

#ifndef DISPLAY_OBJECT_TEXTFIELD_H
#define DISPLAY_OBJECT_TEXTFIELD_H 1

#include "3rdparty/pugixml/src/pugixml.hpp"
#include "backends/textdata.h"
#include "compat.h"
#include "display_object/InteractiveObject.h"
#include "graphics/TokenContainer.h"
#include "interfaces/timer.h"
#include "swf.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/optional.h"

namespace lightspark
{

class DefineEditTextTag;
class SWFMovie;

class TextField :
public InteractiveObject,
public TextData,
public TokenContainer,
public ITickJob
{
private:
	/*
	 * A parser for the HTML subset supported by TextField.
	 */
	class HtmlTextParser : public pugi::xml_tree_walker
	{
		friend class TextField;
	protected:
		TextData* textData;
		std::vector<FormatText> formatStack;
		tiny_string prevName;
		int32_t prevDepth;
		uint32_t swfVersion;
		bool condenseWhite;
		bool multiline;

		uint32_t parseFontSize(const tiny_string& str, uint32_t currentFontSize);
		bool for_each(pugi::xml_node& node);
	public:
		HtmlTextParser
		(
			uint32_t _swfversion,
			bool _condenseWhite,
			bool _multiline
		) :
		textData(nullptr),
		prevDepth(-1),
		swfversion(_swfversion),
		condenseWhite(_condenseWhite),
		multiline(_multiline) {}

		//Stores the text and formating into a TextData object
		void parseTextAndFormating(const tiny_string& html, TextData& dest);
	};

	SWFMovie& movie;
	tokensVector tokens;
public:
	enum EDIT_TYPE { ET_READ_ONLY, ET_EDITABLE };
	enum ANTI_ALIAS_TYPE { AA_NORMAL, AA_ADVANCED };
	enum GRID_FIT_TYPE { GF_NONE, GF_PIXEL, GF_SUBPIXEL };
	enum TEXT_INTERACTION_MODE { TI_NORMAL, TI_SELECTION };
private:
	Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly) override;

	IDrawable* invalidate(bool smoothing) override;
	void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	) override;

	void defaultEventBehavior(_R<Event> e) override;
	void updateText(const tiny_string& new_text);
	//Computes and changes (text)width and (text)height
	void updateSizes(bool updateFormat = false);
	tiny_string toHtmlText();
	tiny_string compactHTMLWhiteSpace(const tiny_string& str);
	void validateScrollH(int32_t oldValue);
	void validateScrollV(int32_t oldValue);
	int32_t getMaxScrollH();
	int32_t getMaxScrollV();
	void textUpdated();
	void setSizeAndPositionFromAutoSize(bool updateWidth = true);
	void replaceText
	(
		size_t begin,
		size_t end,
		const tiny_string& newText
	);

	EDIT_TYPE editType;
	ANTI_ALIAS_TYPE antiAliasType;
	GRID_FIT_TYPE gridFitType;
	TEXT_INTERACTION_MODE textInteractionMode;
	Optional<tiny_string> restrictChars;
	number_t autoSizePos;
	tiny_string tagVarName;
	DisplayObject* tagVarTarget;
	Mutex invalidatemutex;
	DefineEditTextTag* tag;
	Twips origX;
	Twips origWidth;

	// these are only used when drawing to DisplayObject, so they are guarranteed not to be destroyed during rendering
	std::list<FILLSTYLE> fillStyleTextColor;
	FILLSTYLE fillStyleBackgroundColor;
	LINESTYLE2 lineStyleBorder;
	LINESTYLE2 lineStyleCaret;
	Mutex lineMutex;
	bool inAVM1syncVar;
	bool inUpdateVarBinding;
	bool isHtml;

	bool alwaysShowSelection;
	size_t caretIndex;
	bool condenseWhite;
	size_t maxChars;
	bool mouseWheelEnabled;
	bool selectable;
	size_t selectionBeginIndex;
	size_t selectionEndIndex;
	number_t sharpness;
	StyleSheet* styleSheet;
	number_t thickness;
	bool useRichTextClipboard;

	Rect<Twips> getTextBounds(const tiny_string& txt);
protected:
	void afterSetLegacyMatrix() override;
public:
	TextField
	(
		SystemState* sys,
		SWFMovie& _movie,
		DefineEditTextTag& _tag,
		Optional<const tiny_string&> name = {},
		const TextData& _textData = TextData(),
		bool _selectable = true,
		bool readOnly = true,
		const tiny_string& varName = ""

	) : TextField
	(
		sys,
		_movie,
		name,
		_textData,
		_selectable,
		readOnly,
		varName,
		&_tag
	) {}

	TextField
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {},
		const TextData& _textData = TextData(),
		bool _selectable = true,
		bool readOnly = true,
		const tiny_string& varName = "",
		DefineEditTextTag* _tag = nullptr
	);

	void setHtmlText(const tiny_string& html);
	void avm1SyncTagVar();
	void UpdateVariableBinding(asAtom v) override;
	void afterTimelineCreation() override;
	void afterTimelineDeletion(bool inskipping) override;
	void lostFocus() override;
	void gotFocus() override;
	void textInputChanged(const tiny_string& newText) override;
	void tick() override;
	void tickFence() override;
	uint32_t getTagID() const override;
	number_t getScaleFactor() const override { return scaling; }
	bool isInUpdateVarBinding() const { return inUpdateVarBinding; }
	bool isFocusable(bool fromMouse) override;
	size_t getTextCharCount();
	void refreshSurfaceState() override;
	void setupOriginalPosition();

	const ANTI_ALIAS_TYPE& getAntiAliasType() const
	{
		return antiAliasType;
	}

	void setAntiAliasType(const ANTI_ALIAS_TYPE& _type)
	{
		antiAliasType = _type;
	}

	const GRID_FIT_TYPE& getGridFitType() const
	{
		return gridFitType;
	}

	void setGridFitType(const GRID_FIT_TYPE& _type)
	{
		gridFitType = _type;
	}

	const Vector2Twips& getSize() const { return size; }
	void setSize(const Vector2Twips& _size);
	const Twips& getWidth() const { return size.x; }
	void setWidth(const Twips& _width);
	{
		setSize(Vector2Twips(_width, size.y));
	}

	const Twips& getHeight() const { return size.y; }
	void setHeight(const Twips& _height)
	{
		setSize(Vector2Twips(size.x, _height));
	}

	const ALIGNMENT& getAutoSize() const { return autoSize; }
	void setAutoSize(const ALIGNMENT& _type);
	bool getWordWrap() const { return wordWrap; }
	void setWordWrap(bool _wordWrap);
	const Vector2Twips& getTextSize() const { return textSize; }
	const Twips& getTextWidth() const { return textSize.x; }
	const Twips& getTextHeight() const { return textSize.y; }
	const FormatText& getTextFormat(size_t from, size_t to) const;
	void setTextFormat(const FormatText& fmt, size_t from, size_t to);
	const FormatText& getDefaultTextFormat() const;
	void setDefaultTextFormat(const FormatText& fmt);
	// TODO: Implement this later.
	//size_t getCharIndexAtPoint(const Vector2Twips& pos) const;
	size_t getLineIndexAtPoint(const Vector2Twips& pos) const;
	size_t getLineIndexOfChar(size_t idx) const;
	size_t getLineLength(size_t lineIdx) const;
	Optional<LineMetrics> getLineMetrics(size_t lineIdx) const;
	size_t getLineOffset(size_t lineIdx) const;
	Optional<tiny_string> getLineText(size_t lineIdx) const;
	size_t getTextLength() const;
	size_t getLineCountWithLock() const;
	size_t getBottomScrollV() const;
	Optional<const tiny_string&> getRestrict() const
	{
		return restrictChars.asRef();
	}

	void setRestrict(Optional<const tiny_string&> str)
	{
		restrictChars = str;
	}

	const TEXT_INTERACTION_MODE& getTextInteractionMode() const
	{
		return textInteractionMode;
	}

	Vector2u getSelection() const
	{
		return Vector2u
		(
			selectionBeginIndex,
			selectionEndIndex
		);
	}

	void setSelection(size_t from, size_t to);
	Optional<Rect<Twips>> getCharBounds(size_t charIdx) const;
	bool getDisplayAsPassword() const { return isPassword; }
	void setDisplayAsPassword(bool _isPassword);
	size_t getParagraphStart(size_t idx) const;
	size_t getParagraphLength(size_t idx) const;

	bool getAlwaysShowSelection() const { return alwaysShowSelection; }
	void setAlwaysShowSelection(bool flag)
	{
		alwaysShowSelection = flag;
	}

	bool hasBackground() const { return background; }
	void setBackground(bool flag) { background = flag; }
	const RGB& getBackgroundColor() const { return backgroundColor; }
	void setBackgroundColor(const RGB& color)
	{
		backgroundColor = color;
	}

	bool hasBorder() const { return border; }
	void setBorder(bool flag) { border = flag; }
	const RGB& getBorderColor() const { return borderColor; }
	void setBorderColor(const RGB& color) { borderColor = color; }
	size_t getCaretIndex() const { return caretIndex; }
	bool getCondenseWhite const { return condenseWhite; }
	void setCondenseWhite(bool flag) { condenseWhite = flag; }
	size_t getMaxChars() const { return maxChars; }
	void setMaxChars(size_t _maxChars) { maxChars = _maxChars; }
	bool getMultiline() const { return multiline; }
	void setMultiline(bool flag) { multiline = flag; }
	bool isMouseWheelEnabled() const { return mouseWheelEnabled; }
	void setMouseWheelEnabled(bool flag) { mouseWheelEnabled = flag; }
	size_t getScrollH() const { return scrollH; }
	void setScrollH(size_t _scrollH);
	size_t getScrollV() { return scrollV; }
	void setScrollV(size_t _scrollV);
	bool isSelectable() const { return selectable; }
	void setSelectable(bool flag) { selectable = flag; }
	number_t getSharpness() const { return sharpness; }
	void setSharpness(number_t val);
	StyleSheet* getStyleSheet() const { return styleSheet; }
	void setStyleSheet(StyleSheet* _styleSheet)
	{
		styleSheet = _styleSheet;
	}

	const RGB& getTextColor() const { return textColor; }
	void setTextColor(const RGB& color) { textColor = color; }
	number_t getThickness() const { return thickness; }
	const EDIT_TYPE& getEditType() const { return editType; }
	void setEditType(const EDIT_TYPE& _type) { editType = _type; }
	void setThickness(number_t val);
	bool getUseRichTextClipboard() const { useRichTextClipboard; }
	void setUseRichTextClipboard(bool flag)
	{
		useRichTextClipboard = flag;
	}

	bool getEmbedFonts() const { return useOutlines; }
	void setEmbedFonts(bool flag) { useOutlines = flag; }

	std::string toDebugString() const override;
};

}
#endif /* DISPLAY_OBJECT_TEXTFIELD_H */
