/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include "display_object/DisplayObject.h"
#include "display_object/Stage.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/AsBroadcaster.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::stage` crate.

using AVM1Stage;

static constexpr auto objDecls =
{
	AVM1Decl("align", getAlign, setAlign),
	AVM1Decl("height", getHeight, nullptr),
	AVM1Decl("scaleMode", getScaleMode, setScaleMode),
	AVM1Decl("displayState", getDisplayState, setDisplayState),
	AVM1Decl("showMenu", getShowMenu, setShowMenu),
	AVM1Decl("width", getWidth, nullptr)
};

AVM1Stage::AVM1Stage
(
	AVM1DeclContext& ctx,
	const GcPtr<AsBroadcasterFuncs>& bcastFuncs,
	const GcPtr<AVM1Object>& arrayProto
) : AVM1Object(ctx.getGcCtx(), ctx.getObjectProto())
{
	bcastFuncs->init(ctx.getGcCtx(), this, arrayProto);
	ctx.definePropsOn(this, objDecls);
}

AVM1_FUNCTION_BODY(AVM1Stage, getAlign)
{
	auto align = act.getSys()->stage->getAlign();
	std::string str;

	// Match the string values returned by Flash Player.
	// It's actually possible for `align` to return a bogus value, like
	// "LTRB", which acts the same as "TL" (since top left alignment
	// takes priority).
	// NOTE: The string order is different between AVM1, and AVM2!
	if (align & StageAlign::Left)
		str.push_back('L');
	if (align & StageAlign::Top)
		str.push_back('T');
	if (align & StageAlign::Right)
		str.push_back('R');
	if (align & StageAlign::Bottom)
		str.push_back('B');
	return str;
}

AVM1_FUNCTION_BODY(AVM1Stage, setAlign)
{
	tiny_string alignStr;
	AVM1_ARG_UNPACK(alignStr);

	StageAlign align(0);

	bool validPrefix = alignStr.tryStripPrefix
	(
		"bottom"
	).andThen([&](const auto& str)
	{
		align = StageAlign::Bottom;
		return makeOptional(str);
	}).orElse([&]
	{
		auto str = alignStr.tryStripPrefix("top");
		if (str.hasValue())
			align = StageAlign::Top;
		return str;
	}).valueOr("").startsWith('_');

	if (validPrefix && alignStr.endsWith("left"))
		align |= StageAlign::Left;
	else if (validPrefix && alignStr.endsWith("right"))
		align |= StageAlign::Right;

	act.getSys()->stage->setAlign(align);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Stage, getHeight)
{
	return number_t(act.getSys()->stage->internalGetHeight());
}

AVM1_FUNCTION_BODY(AVM1Stage, getScaleMode)
{
	switch (act.getSys()->stage->getScaleMode())
	{
		case StageScale::ExactFit: return "exactFit"; break;
		case StageScale::ShowAll: return "showAll"; break;
		case StageScale::NoBorder: return "noBorder"; break;
		case StageScale::NoScale: return "noScale"; break;
	}
}

AVM1_FUNCTION_BODY(AVM1Stage, setScaleMode)
{
	tiny_string scaleModeStr;
	AVM1_ARG_UNPACK(scaleModeStr);

	auto stage = act.getSys()->stage;

	if (scaleModeStr.caselessEquals("exactFit"))
		stage->setScaleMode(StageScale::ExactFit);
	else if (scaleModeStr.caselessEquals("noBorder"))
		stage->setScaleMode(StageScale::NoBorder);
	else if (scaleModeStr.caselessEquals("noScale"))
		stage->setScaleMode(StageScale::NoScale);
	else
		stage->setScaleMode(StageScale::ShowAll);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Stage, getDisplayState)
{
	if (act.getSys()->stage->isFullscreen())
		return "fullScreen";
	return "normal";
}

AVM1_FUNCTION_BODY(AVM1Stage, setDisplayState)
{
	tiny_string dispStateStr;
	AVM1_ARG_UNPACK(dispStateStr);

	auto stage = act.getSys()->stage;
	if (dispStateStr.caselessEquals("fullScreen"))
		stage->setDisplayState(DisplayState::FullScreen);
	else if (dispStateStr.caselessEquals("normal"))
		stage->setDisplayState(DisplayState::Normal);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Stage, getShowMenu)
{
	return act.getSys()->stage->getShowMenu();
}

AVM1_FUNCTION_BODY(AVM1Stage, setShowMenu)
{
	bool showMenu;
	AVM1_ARG_UNPACK(showMenu, true);

	act.getSys()->stage->setShowMenu(showMenu);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Stage, getWidth)
{
	return number_t(act.getSys()->stage->internalGetWidth());
}
