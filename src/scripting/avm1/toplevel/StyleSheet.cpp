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

#include "backends/graphics.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/TextFormat.h"
#include "scripting/avm1/toplevel/StyleSheet.h"
#include "scripting/avm1/toplevel/toplevel.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::style_sheet` crate.

AVM1StyleSheet::AVM1StyleSheet(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().styleSheet->proto
) {}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly |
	AVM1PropFlags::Version7
);

using AVM1StyleSheet;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1StyleSheet, getStyle, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1StyleSheet, setStyle, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1StyleSheet, clear, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1StyleSheet, getStyleNames, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1StyleSheet, transform, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1StyleSheet, parseCSS, protoFlags),
	AVM1_NAME_FUNCTION_TYPE_PROTO(AVM1StyleSheet, parse, parseCSS, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1StyleSheet::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1StyleSheet, ctor)
{
	INPLACE_NEW_GC_PTR(act.getGcCtx(), _this, AVM1StyleSheet(act));
	return _this;
}

static AVM1Value copyProps(AVM1Activation& act, const AVM1Value& value)
{
	if (!value.is<AVM1Object>())
		return AVM1Value::nullVal;

	auto obj = value.toObject(act);
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1Object(act));

	auto keys = obj->getKeys(act, false);
	for (const auto& key : keys)
		ret->setProp(act, key, obj->getStoredProp(act, key));
	return ret;
}

static GcPtr<AVM1Object> getCSS
(
	AVM1Activation& act,
	GcPtr<AVM1Object> _this
)
{
	return _this->getStoredProp(act, "_css").toObject(act);
}

AVM1_FUNCTION_TYPE_BODY(AVM1StyleSheet, AVM1StyleSheet, getStyle)
{
	auto css = getCSS(act, _this);
	tiny_string name;
	AVM1_ARG_UNPACK(name);
	return copyProps(act, css->getProp(act, name));
}

AVM1_FUNCTION_TYPE_BODY(AVM1StyleSheet, AVM1StyleSheet, setStyle)
{
	auto tryCreateArrayProp = [&](const tiny_string& name)
	{
		if (_this->hasProp(act, name))
			return;
		_this->setProp(act, name, NEW_GC_PTR
		(
			act.getGcCtx(),
			AVM1Array(act)
		));
	};

	tryCreateArrayProp("_styles");
	tryCreateArrayProp("_css");

	auto css = getCSS(act, _this);
	auto styles = _this->getStoredProp(act, "_styles").toObject(act);

	tiny_string name;
	AVM1Value obj;
	AVM1_ARG_UNPACK(name)(obj);

	css->setProp(act, name, copyProps(act, obj));

	auto _textFormat = _this->callMethod
	(
		act,
		"transform",
		{ copyProps(act, obj) },
		AVM1ExecutionReason::Special
	);
	styles->setProp(act, name, _textFormat);

	if (!value.is<AVM1Object>())
		return AVM1Value::undefinedVal;

	auto textFormat = value.toObject(act)->as<AVM1TextFormat>();
	if (!textFormat.isNull())
		_this->setStyle(name.lowercase(), textFormat);

	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1StyleSheet, AVM1StyleSheet, clear)
{
	_this->setProp(act, "_styles", NEW_GC_PTR
	(
		act.getGcCtx(),
		AVM1Array(act)
	));

	_this->setProp(act, "_css", NEW_GC_PTR
	(
		act.getGcCtx(),
		AVM1Array(act)
	));
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1StyleSheet, AVM1StyleSheet, getStyleNames)
{
	auto keys = getCSS(act, _this)->getKeys(act, false);
	return NEW_GC_PTR(act.getGcCtx(), AVM1Array
	(
		act,
		{ keys.begin(), keys.end() }
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1StyleSheet, AVM1StyleSheet, transform)
{
	TextFormat textFormat
	{
		.display = TextDisplay::Block,
		.kerning = false
	};

	NullableGcPtr<AVM1Object> styleObj;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK.withStrict()
	(
		styleObj,
		NullGc
	), AVM1Value::nullVal);

	if (styleObj.isNull())
	{
		return NEW_GC_PTR(act.getGcCtx(), AVM1TextFormat
		(
			act,
			textFormat
		));
	}

	auto getStyle = [&](const tiny_string& name) -> Optional<AVM1Value>
	{
		auto ret = styleObj->getStoredProp(act, name);
		if (value.is<AVM1Undefined>())
			return {};
		return value;
	};

	auto parseColor = [&](const tiny_string& str) -> Optional<RGBA>
	{
		auto strippedStr = str.stripPrefix("#");
		if (strippedStr.numChars() != 6)
			return {};
		return strippedStr.tryToNumber
		<
			uint32_t
		>(16).transform([&](auto val)
		{
			return RGBA(val);
		});
	};

	auto parseSuffixedInt = [&](const AVM1Value& val)
	{
		return parseIntImpl(act, val, {}).toInt32(act);
	};

	auto parseSuffixedNumber = [&](const AVM1Value& val)
	{
		return parseIntImpl(act, val, {}).toNumber(act);
	};

	getStyle("color").transform([&](const auto& val)
	{
		return val.as<tiny_string>();
	}).andThen([&](const auto& str)
	{
		textFormat.color = parseColor(str);
		return str;
	});

	getStyle("display").andThen([&](const auto& val)
	{
		auto display = val.toString(act);
		if (display == "none")
			textFormat.display = TextDisplay::None;
		else if (display == "inline")
			textFormat.display = TextDisplay::Inline;
		else
			textFormat.display = TextDisplay::Block;
		return val;
	});

	getStyle("fontFamily").andThen([&](const auto& family)
	{
		if (!family.toBool(act.getSwfVersion()))
			return val;
		textFormat.font = parseFontList(family.toString(act));
		return val;
	});

	getStyle("fontSize").andThen([&](const auto& val)
	{
		auto size = parseSuffixedInt(val);
		if (size > 0)
			textFormat.size = size;
		return val;
	});

	getStyle("fontStyle").andThen([&](const auto& val)
	{
		auto style = val.toString(act)
		if (style != "normal" && style != "italic")
			return val;
		textFormat.italic = style == "italic";
		return val;
	});

	getStyle("fontWeight").andThen([&](const auto& val)
	{
		auto weight = val.toString(act)
		if (weight != "normal" && style != "bold")
			return val;
		textFormat.bold = weight == "bold";
		return val;
	});

	getStyle("kerning").andThen([&](const auto& val)
	{
		textFormat.kerning = val.visit(makeVisitor
		(
			[](const tiny_string& str) { return str == "true"; },
			[](const auto&) -> bool { return parseSuffixedInt(val); }
		));
		return val;
	});

	getStyle("leading").andThen([&](const auto& val)
	{
		if (!val.toBool(act.getSwfVersion()))
			return val;
		textFormat.leading = parseSuffixedInt(val);
		return val;
	});

	getStyle("letterSpacing").andThen([&](const auto& val)
	{
		if (!val.toBool(act.getSwfVersion()))
			return val;
		textFormat.letterSpacing = parseSuffixedNumber(val);
		return val;
	});

	getStyle("leftMargin").andThen([&](const auto& val)
	{
		if (!val.toBool(act.getSwfVersion()))
			return val;
		textFormat.leftMargin = imax(parseSuffixedInt(val), 0);
		return val;
	});

	getStyle("rightMargin").andThen([&](const auto& val)
	{
		if (!val.toBool(act.getSwfVersion()))
			return val;
		textFormat.rightMargin = imax(parseSuffixedInt(val), 0);
		return val;
	});

	getStyle("textAlign").andThen([&](const auto& val)
	{
		auto align = val.toString(act).lowercase();
		if (align == "left")
			textFormat.align = TextAlign::Left
		else if (align == "center")
			textFormat.align = TextAlign::Center;
		else if (align == "right")
			textFormat.align = TextAlign::Right;
		else if (align == "justify")
			textFormat.align = TextAlign::Justify;
		return val;
	});

	getStyle("textDecoration").andThen([&](const auto& val)
	{
		auto decoration = val.toString(act)
		if (decoration != "none" && decoration != "underline")
			return val;
		textFormat.underline = decoration == "underline";
		return val;
	});

	getStyle("indent").andThen([&](const auto& val)
	{
		if (!val.toBool(act.getSwfVersion()))
			return val;
		textFormat.indent = parseSuffixedInt(val);
		return val;
	});

	return NEW_GC_PTR(act.getGcCtx(), AVM1TextFormat(act, textFormat));
}

AVM1_FUNCTION_TYPE_BODY(AVM1StyleSheet, AVM1StyleSheet, parseCSS)
{
	tiny_string source;
	AVM1_ARG_UNPACK(source);

	CSSParser::ParseType css;
	try
	{
		css = CSSParser(source).parse();
	}
	catch (CSSExcpetion& e)
	{
		return false;
	}

	for (const auto& pair : css)
	{
		const auto& sel = pair.first;
		if (sel.empty())
			continue;

		const auto& props = pair.second;
		auto obj = NEW_GC_PTR(act.getGcCtx(), AVM1Object(act));
		for (const auto& _pair : props)
		{
			obj->setProp
			(
				act,
				dashesToCamelCase(_pair.first),
				_pair.second
			);
		}
		setStyle(act, _this, { sel, obj });
	}
	return true;
}
