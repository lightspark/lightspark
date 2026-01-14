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
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::text_format` crate.

AVM1TextFormat::AVM1TextFormat(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().textFormat->proto
) {}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1TextFormat;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextFormat, getTextExtent, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, font, Font),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, size, Size),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, color, Color),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, url, URL),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, target, Target),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, bold, Bold),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, italic, Italic),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, underline, Underline),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, align, Align),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, leftMargin, LeftMargin),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, rightMargin, RightMargin),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, indent, Indent),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, leading, Leading),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, blockIndent, BlockIndent),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, tabStops, TabStops),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, bullet, Bullet),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, display, Display),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, kerning, Kerning),
	AVM1_PROPERTY_TYPE_PROTO(AVM1TextFormat, letterSpacing, LetterSpacing)
};

GcPtr<AVM1SystemClass> AVM1TextFormat::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1TextFormat, ctor)
{
	AVM1_ARG_UNPACK(_this->font).tryUnpack<Pixels>
	(
		_this->size
	).unpackArgs
	(
		_this->color,
		_this->bold,
		_this->italic,
		_this->underline,
		_this->url,
		_this->target,
		_this->align
	).tryUnpackArgsAs<Pixels>
	(
		_this->leftMargin,
		_this->rightMargin,
		_this->indent,
		_this->leading
	);

	_this->display = TextDisplay::Block;
	return _this;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, getTextExtent)
{
	tiny_string text;
	Optional<number_t> width;
	AVM1_ARG_UNPACK(text)(width);

	TextField textField
	(
		act.getSys(),
		act.getBaseClip()->getMovie(),
		Vector2Twips(),
		Vector2Twips(width.valueOr(0), 0)
	);

	textField.setAutoSize(AS_LEFT);
	textField.setWordWrap(width.hasValue());
	textField.setNewTextFormat(_this);
	textField.setText(text);

	auto metrics = textField.tryGetMetrics().valueOrThrow
	(
		"TextFormat.getTextExtent(): "
		"All text boxes must have at least 1 line."
	);

	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1Object(act));
	ret->setData(act, "ascent", metrics.ascent.toPx());
	ret->setData(act, "descent", metrics.descent.toPx());
	ret->setData(act, "width", metrics.size.x.toPx());
	ret->setData(act, "height", metrics.size.y.toPx());
	ret->setData(act, "textFieldHeight", textField.getHeight());
	ret->setData(act, "textFieldWidth", textField.getWidth());
	return ret;
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Font)
{
	return _this->font.transformOr(AVM1Value::nullVal);
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Font)
{
	if (value.isNullOrUndefined())
		_this->font.reset();
	else
		_this->font = value.toString(act).substr(0, 64);
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Size)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->size.transformOr(nullVal, [&](const auto& size)
	{
		return size.toPx();
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Size)
{
	if (value.isNullOrUndefined())
		_this->size.reset();
	else
	{
		_this->size = Twips::fromPx
		(
			act.getSwfVersion() < 8 ?
			value.toInt32(act) :
			roundToEven(value.toNumber(act))
		);
	}
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Color)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->color.transformOr(nullVal, [&](const auto& color)
	{
		return color.toUInt();
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Color)
{
	if (value.isNullOrUndefined())
		_this->color.reset();
	else
		_this->color = RGBA(value.toUInt32(act));
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, URL)
{
	return _this->url.transformOr(AVM1Value::nullVal);
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, URL)
{
	if (value.isNullOrUndefined())
		_this->url.reset();
	else
		_this->url = value.toString(act);
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Target)
{
	return _this->target.transformOr(AVM1Value::nullVal);
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Target)
{
	if (value.isNullOrUndefined())
		_this->target.reset();
	else
		_this->target = value.toString(act);
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Bold)
{
	return _this->bold.transformOr(AVM1Value::nullVal);
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Bold)
{
	if (value.isNullOrUndefined())
		_this->bold.reset();
	else
		_this->bold = value.toBool(act.getSwfVersion());
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Italic)
{
	return _this->italic.transformOr(AVM1Value::nullVal);
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Italic)
{
	if (value.isNullOrUndefined())
		_this->italic.reset();
	else
		_this->italic = value.toBool(act.getSwfVersion());
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Underline)
{
	return _this->underline.transformOr(AVM1Value::nullVal);
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Underline)
{
	if (value.isNullOrUndefined())
		_this->underline.reset();
	else
		_this->underline = value.toBool(act.getSwfVersion());
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Align)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->align.transformOr(nullVal, [&](const auto& align)
	{
		switch (align)
		{
			case TextAlign::Left: return "left"; break;
			case TextAlign::Right: return "right"; break;
			case TextAlign::Center: return "center"; break;
			case TextAlign::Justify: return "justify"; break;
		}
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Align)
{
	if (value.isNullOrUndefined())
		return;

	auto align = value.toString(act);
	if (align.caselessEquals("left"))
		_this->align = TextAlign::Left;
	else if (align.caselessEquals("right"))
		_this->align = TextAlign::Right;
	else if (align.caselessEquals("Center"))
		_this->align = TextAlign::Center;
	else if (align.caselessEquals("justify"))
		_this->align = TextAlign::Justify;
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, LeftMargin)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->leftMargin.transformOr(nullVal, [&](const auto& leftMargin)
	{
		return leftMargin.toPx();
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, LeftMargin)
{
	if (value.isNullOrUndefined())
		_this->leftMargin.reset();
	else
	{
		_this->leftMargin = Twips::fromPx
		(
			act.getSwfVersion() < 8 ?
			imax(value.toInt32(act), 0) :
			roundToEven(dmax(value.toNumber(act), 0))
		);
	}
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, RightMargin)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->rightMargin.transformOr(nullVal, [&](const auto& rightMargin)
	{
		return rightMargin.toPx();
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, RightMargin)
{
	if (value.isNullOrUndefined())
		_this->rightMargin.reset();
	else
	{
		_this->rightMargin = Twips::fromPx
		(
			act.getSwfVersion() < 8 ?
			imax(value.toInt32(act), 0) :
			roundToEven(dmax(value.toNumber(act), 0))
		);
	}
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Indent)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->indent.transformOr(nullVal, [&](const auto& indent)
	{
		return indent.toPx();
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Indent)
{
	if (value.isNullOrUndefined())
		_this->indent.reset();
	else
	{
		_this->indent = Twips::fromPx
		(
			act.getSwfVersion() < 8 ?
			imax(value.toInt32(act), 0) :
			roundToEven(value.toNumber(act))
		);
	}
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Leading)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->leading.transformOr(nullVal, [&](const auto& leading)
	{
		return leading.toPx();
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Leading)
{
	if (value.isNullOrUndefined())
		_this->leading.reset();
	else
	{
		_this->leading = Twips::fromPx
		(
			act.getSwfVersion() < 8 ?
			imax(value.toInt32(act), 0) :
			roundToEven(value.toNumber(act))
		);
	}
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, BlockIndent)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->blockIndent.transformOr(nullVal, [&](const auto& blockIndent)
	{
		return blockIndent.toPx();
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, BlockIndent)
{
	if (value.isNullOrUndefined())
		_this->blockIndent.reset();
	else
	{
		_this->blockIndent = Twips::fromPx
		(
			act.getSwfVersion() < 8 ?
			imax(value.toInt32(act), 0) :
			roundToEven(value.toNumber(act))
		);
	}
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, TabStops)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->tabStops.transformOr(nullVal, [&](const auto& tabStops)
	{
		return NEW_GC_PTR(act.getGcCtx AVM1Array(act, tabStops));
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, TabStops)
{
	if (!value.is<AVM1Object>())
	{
		_this->tabStops.reset();
		return;
	}

	std::vector<Twips> tabStops;
	auto obj = value.toObject(act);
	auto size = obj->getLength(act);
	tabStops.reserve(size);

	for (size_t i = 0; i < size; ++i)
	{
		auto elem = obj->getElement(act, i);
		tabStops.emplace_back(roundToEven(elem.toNumber(act)));
	}
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Bullet)
{
	return _this->bullet.transformOr(AVM1Value::nullVal);
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Bullet)
{
	if (value.isNullOrUndefined())
		_this->bullet.reset();
	else
		_this->bullet = value.toBool(act.getSwfVersion());
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Display)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->display.transformOr(nullVal, [&](const auto& display)
	{
		switch (display)
		{
			case TextDisplay::Block: return "block"; break;
			case TextDisplay::Inline: return "inline"; break;
			case TextDisplay::None: return "none"; break;
		}
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Display)
{
	if (value.isNullOrUndefined())
	{
		_this->display = TextDisplay::Block;
		return;
	}

	auto display = value.toString(act);
	if (display == "none")
		_this->display = TextDisplay::None;
	else if (display == "inline")
		_this->display = TextDisplay::Inline;
	else
		_this->display = TextDisplay::Block;
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Kerning)
{
	return _this->kerning.transformOr(AVM1Value::nullVal);
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, Kerning)
{
	if (value.isNullOrUndefined())
		_this->kerning.reset();
	else
		_this->kerning = value.toBool(act.getSwfVersion());
}

AVM1_GETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, LetterSpacing)
{
	const auto& nullVal = AVM1Value::nullVal;
	return _this->letterSpacing.transformOr(nullVal, [&](const auto& letterSpacing)
	{
		return letterSpacing.toPx();
	});
}

AVM1_SETTER_TYPE_BODY(AVM1TextFormat, AVM1TextFormat, LetterSpacing)
{
	if (value.isNullOrUndefined())
		_this->letterSpacing.reset();
	else
		_this->letterSpacing = roundToEven(value.toNumber(act));
}
