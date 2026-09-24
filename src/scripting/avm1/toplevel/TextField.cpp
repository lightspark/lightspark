/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include <initializer_list>
#include <sstream>

#include "backends/graphics.h"
#include "display_object/DisplayObject.h"
#include "display_object/TextField.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/Matrix.h"
#include "scripting/avm1/toplevel/TextField.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::text_field` crate.

AVM1TextField::AVM1TextField
(
	AVM1Activation& act,
	const GcPtr<TextField>& textField
) : AVM1DisplayObject
(
	act,
	textField,
	act.getPrototypes().textField->proto
) {}

template<size_t swfVersion, EnableIf
<(
	swfVersion >= 5 &&
	swfVersion <= 10
), bool> = false>
static constexpr AVM1PropFlags allowSWFVersion()
{
	switch (swfVersion)
	{
		case 5: return AVM1PropFlags::Version5; break;
		case 6: return AVM1PropFlags::Version6; break;
		case 7: return AVM1PropFlags::Version7; break;
		case 8: return AVM1PropFlags::Version8; break;
		case 9: return AVM1PropFlags::Version9; break;
		case 10: return AVM1PropFlags::Version10; break;
		default: return AVM1PropFlags(0); break;
	}
}

template<bool dontEnumAndDel = true>
constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

template<>
constexpr auto protoFlags<false> = AVM1PropFlags(0);

template<size_t swfVersion, bool dontEnumAndDel = true>
constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
	allowSWFVersion<swfVersion>()
);

#define TEXTFIELD_FUNC_PROTO(name, ...) \
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextField, name, propFlags<__VA_ARGS__>)

#define TEXTFIELD_PROP_PROTO(name, funcName, ...) AVM1_PROPERTY_TYPE_PROTO \
( \
	AVM1TextField, \
	name, \
	funcName, \
	propFlags<__VA_ARGS__> \
)

#define TEXTFIELD_GETTER_PROTO(name, funcName, ...) AVM1_GETTER_TYPE_PROTO \
( \
	AVM1TextField, \
	name, \
	funcName, \
	propFlags<__VA_ARGS__> \
)

using AVM1TextField;

static constexpr auto protoDecls =
{
	TEXTFIELD_FUNC_PROTO(replaceSel, 6),
	TEXTFIELD_FUNC_PROTO(getTextFormat, 6),
	TEXTFIELD_FUNC_PROTO(setTextFormat, 6),
	TEXTFIELD_FUNC_PROTO(removeTextField, 6),
	TEXTFIELD_FUNC_PROTO(getNewTextFormat, 6),
	TEXTFIELD_FUNC_PROTO(setNewTextFormat, 6),
	// NOTE: `getDepth()` is defined in `AVM1DisplayObject`.
	TEXTFIELD_FUNC_PROTO(replaceText, 7),

	TEXTFIELD_PROP_PROTO(gridFitType, GridFitType, 8, false),
	TEXTFIELD_PROP_PROTO(antiAliasType, AntiAliasType, 8, false),
	TEXTFIELD_PROP_PROTO(thickness, Thickness, 8, false),
	TEXTFIELD_PROP_PROTO(sharpness, Sharpness, 8, false),
	// NOTE: `filters` is defined in `AVM1DisplayObject`.
	TEXTFIELD_PROP_PROTO(scroll, Scroll),
	TEXTFIELD_GETTER_PROTO(maxscroll, MaxScroll),
	TEXTFIELD_PROP_PROTO(borderColor, BorderColor),
	TEXTFIELD_PROP_PROTO(backgroundColor, BackgroundColor),
	TEXTFIELD_PROP_PROTO(textColor, TextColor),
	// NOTE: `tabEnabled` isn't a builtin property of `TextField`.
	// NOTE: `tabIndex` is defined in `AVM1DisplayObject`.
	TEXTFIELD_PROP_PROTO(autoSize, AutoSize),
	TEXTFIELD_PROP_PROTO(text, Text),
	TEXTFIELD_PROP_PROTO(type, Type),
	TEXTFIELD_PROP_PROTO(htmlText, HtmlText),
	TEXTFIELD_PROP_PROTO(variable, Variable),
	TEXTFIELD_PROP_PROTO(hscroll, HScroll),
	TEXTFIELD_GETTER_PROTO(maxHScroll, MaxHScroll),
	TEXTFIELD_PROP_PROTO(maxChars, MaxChars),
	TEXTFIELD_PROP_PROTO(embedFonts, EmbedFonts),
	TEXTFIELD_PROP_PROTO(html, Html),
	TEXTFIELD_PROP_PROTO(border, Border),
	TEXTFIELD_PROP_PROTO(background, Background),
	TEXTFIELD_PROP_PROTO(wordWrap, WordWrap),
	TEXTFIELD_PROP_PROTO(password, Password),
	TEXTFIELD_PROP_PROTO(multiline , Multiline),
	TEXTFIELD_PROP_PROTO(selectable, Selectable),
	TEXTFIELD_GETTER_PROTO(length, Length),
	TEXTFIELD_GETTER_PROTO(bottomScroll, BottomScroll),
	TEXTFIELD_GETTER_PROTO(textWidth, TextWidth),
	TEXTFIELD_GETTER_PROTO(textHeight, TextHeight),
	TEXTFIELD_PROP_PROTO(restrict, Restrict),
	TEXTFIELD_PROP_PROTO(condenseWhite, CondenseWhite),
	TEXTFIELD_PROP_PROTO(mouseWheelEnabled, MouseWheelEnabled),
	TEXTFIELD_PROP_PROTO(styleSheet, StyleSheet)
};

#undef TEXTFIELD_FUNC_PROTO
#undef TEXTFIELD_PROP_PROTO
#undef TEXTFIELD_GETTER_PROTO

GcPtr<AVM1SystemClass> AVM1TextField::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeEmptyClass(superProto);
	AVM1DisplayObject::defineInteractiveProto(ctx, _class);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextField, AVM1TextField, replaceSel)
{
	tiny_string text;
	AVM1_ARG_UNPACK(text);

	// NOTE: Despite what the AS1/2 docs state, an initial call to
	// `Selection.setFocus()` isn't required. Instead, it appears to
	// default to inserting at the begining of the text.
	//TODO: Verify the exact behaviour of this.
	auto sel = _this->tryGetSel().valueOr(TextSelection(0));
	_this->replaceText(sel, text);
	_this->setSel(sel.getStart() + text.numChars());
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextField, AVM1TextField, getTextFormat)
{
	size_t beginIndex;
	size_t endIndex;
	AVM1_ARG_UNPACK(beginIndex, -1)(endIndex, -1);

	return NEW_GC_PTR(act.getGcCtx(), AVM1TextFormat
	(
		act,
		_this->getTextFormat(beginIndex, endIndex)
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextField, AVM1TextField, setTextFormat)
{
	if (args.empty())
		return AVM1Value::undefinedVal;

	AVM1_ARG_UNPACK_NAMED(unpacker);

	NullableGcPtr<AVM1TextFormat> textFormat;
	unpacker.unpackRev(textFormat);

	size_t beginIndex;
	size_t endIndex;
	unpacker(beginIndex, -1)(endIndex, -1);

	if (textFormat.isNull())
		return AVM1Value::undefinedVal;

	_this->setTextFormat(beginIndex, endIndex, *textFormat);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextField, AVM1TextField, removeTextField)
{
	removeDisplayObject(act, _this);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextField, AVM1TextField, getNewTextFormat)
{
	return NEW_GC_PTR(act.getGcCtx(), AVM1TextFormat
	(
		act,
		_this->getNewTextFormat()
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextField, AVM1TextField, setNewTextFormat)
{
	NullableGcPtr<AVM1TextFormat> textFormat;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(textFormat));

	_this->setNewTextFormat(*textFormat);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextField, AVM1TextField, replaceText)
{
	size_t from;
	size_t to;
	tiny_string text
	AVM1_ARG_UNPACK(from, -1)(to, -1)(text);
	_this->replaceText(from, to, text);
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, GridFitType)
{
	switch (_this->getGridFitType())
	{
		default:
		case GridFitType::None: return "none"; break;
		case GridFitType::Pixel: return "pixel"; break;
		case GridFitType::SubPixel: return "subpixel"; break;
	}
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, GridFitType)
{
	auto newType = value.toString(act);
	if (newType == "none")
		_this->setGridFitType(GridFitType::None);
	else if (newType == "pixel")
		_this->setGridFitType(GridFitType::Pixel);
	else if (newType == "subpixel")
		_this->setGridFitType(GridFitType::SubPixel);
	// NOTE: In Flash Player, setting `gridFitType` to an invalid value
	// does nothing.
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, AntiAliasType)
{
	return _this->isAdvancedAA() ? "advanced" : "normal";
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, AntiAliasType)
{
	auto newType = value.toString(act);
	if (newType == "advanced")
		_this->setAdvancedAA(true);
	else if (newType == "normal")
		_this->setAdvancedAA(false);

	// NOTE: In Flash Player, setting `antiAliasType` to an invalid value
	// does nothing.
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Thickness)
{
	return _this->getThickness();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Thickness)
{
	_this->setThickness(value.toNumber(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Sharpness)
{
	return _this->getSharpness();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Sharpness)
{
	_this->setSharpness(value.toNumber(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Scroll)
{
	return number_t(_this->getScrollV());
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Scroll)
{
	_this->setScrollV(value.toNumber(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, MaxScroll)
{
	return _this->getMaxScrollV();
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, BorderColor)
{
	return number_t(_this->getBorderColor().toUInt());
}
	return _this->getThickness();
	_this->setThickness(value.toNumber(act));

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, BorderColor)
{
	_this->setBorderColor(value.toUInt32(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, BackgroundColor)
{
	return number_t(_this->getBackgroundColor().toUInt());
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, BackgroundColor)
{
	_this->setBackgroundColor(value.toUInt32(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, TextColor)
{
	return number_t(_this->getTextColor().toUInt());
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, TextColor)
{
	_this->setTextColor(value.toUInt32(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, AutoSize)
{
	switch (_this->autoSize)
	{
		case AS_NONE: return "none"; break;
		case AS_LEFT: return "left"; break;
		case AS_RIGHT: return "right"; break;
		case AS_CENTER: return "center"; break;
	}
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, AutoSize)
{
	if (value.is<bool>())
	{
		bool val = value.toBool(act, act.getSwfVersion());
		_this->setAutoSize(val ? AS_LEFT : AS_NONE);
		return;
	}

	auto mode = value.toString(act);
	if (mode.caselessEquals("left"))
		_this->setAutoSize(AS_LEFT);
	if (mode.caselessEquals("right"))
		_this->setAutoSize(AS_RIGHT);
	if (mode.caselessEquals("center"))
		_this->setAutoSize(AS_CENTER);
	else
		_this->setAutoSize(AS_NONE);
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Text)
{
	Locker l(*_this->linemutex);
	return _this->getText();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Text)
{
	_this->updateText(value.toString(act));
	_this->legacy = false;
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Type)
{
	return _this->isReadOnly() ? "dynamic" : "input";
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Type)
{
	auto type = value.toString(act);
	if (type.caselessEquas("dynamic"))
		_this->setReadOnly(true);
	else if(type.caselessEquals("input"))
		_this->setReadOnly(false);
	else
		LOG(LOG_ERROR, "`TextField.type` setter: invalid type: " << type);
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, HtmlText)
{
	return _this->getHtmlText();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, HtmlText)
{
	_this->setHtmlText(value.toString(act));
	_this->legacy = false;
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Variable)
{
	// NOTE: An empty `variable` name returns `null`, rather than
	// `undefined`.
	if (_this->getVarName().empty())
		return AVM1Value::nullVal;
	return _this->getVarName();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Variable)
{
	if (value.isNullOrUndefined())
		_this->setVarName("");
	else
		_this->setVarName(value.toString(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, HScroll)
{
	return number_t(_this->getScrollH());
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, HScroll)
{
	// NOTE: In SWF 8, and earlier, the clamping behaviour of `hscroll`
	// is the one done below. In SWF 9, and later, the clamping behaviour
	// is way more complicated. See Ruffle's #4634.
	_this->setScrollH(iclamp
	(
		value.toInt32(act),
		0,
		_this->getMaxScrollH()
	));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, MaxHScroll)
{
	return number_t(_this->getMaxScrollH());
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, MaxChars)
{
	if (!_this->getMaxChars())
		return AVM1Value::nullVal;
	return number_t(_this->getMaxChars());
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, MaxChars)
{
	_this->setMaxChars(value.toInt32(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, EmbedFonts)
{
	return _this->getUseOutlines();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, EmbedFonts)
{
	_this->setUseOutlines(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Html)
{
	return _this->isHTML();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Html)
{
	_this->setHTML(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Border)
{
	return _this->border;
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Border)
{
	_this->setBorder(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Background)
{
	return _this->background;
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Background)
{
	_this->setBackground(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, WordWrap)
{
	return _this->wordWrap;
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, WordWrap)
{
	_this->setWordWrap(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Password)
{
	return _this->password;
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Password)
{
	_this->setPassword(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Multiline)
{
	return _this->multiline;
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Multiline)
{
	_this->setMultiline(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Selectable)
{
	return _this->isSelectable();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Selectable)
{
	_this->setSelectable(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Length)
{
	return number_t(_this->getTextLength());
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, BottomScroll)
{
	return number_t(_this->getBottomScrollV());
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, TextWidth)
{
	return number_t(_this->textWidth);
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, TextHeight)
{
	return number_t(_this->textHeight);
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Restrict)
{
	if (_this->getRestirct().empty())
		return AVM1Value::nullVal;
	return _this->getRestrict();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, Restrict)
{
	if (value.isNullOrUndefined())
	{
		_this->setRestrict("");
		return;
	}

	// NOTE: The AS1/2 docs state that an empty string means you can't
	// enter any characters. This isn't true, it just means that
	// `restrict` returns `null`.
	_this->setRestrict(value.toString(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, CondenseWhite)
{
	return _this->getCondenseWhite();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, CondenseWhite)
{
	_this->setCondenseWhite(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, MouseWheelEnabled)
{
	return _this->isMouseWheelEnabled();
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, MouseWheelEnabled)
{
	_this->setMouseWheelEnabled(value.toBool(act, act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(AVM1TextField, AVM1TextField, StyleSheet)
{
	auto styleSheet = _this->getStyleSheet();
	if (styleSheet.isNull())
		return AVM1Value::undefinedVal;
	return styleSheet->toAVM1Object(act);
}

AVM1_SETTER_TYPE_BODY(AVM1TextField, AVM1TextField, StyleSheet)
{
	_this->setStyleSheet(value.as<AVM1StyleSheet>());
}
