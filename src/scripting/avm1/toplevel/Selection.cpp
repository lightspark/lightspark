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

#include "display_object/Stage.h"
#include "display_object/TextField.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/AsBroadcaster.h"
#include "scripting/avm1/toplevel/Selection.h"
#include "swf.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::selection` crate.

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

using AVM1Selection;

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(getBeginIndex, protoFlags),
	AVM1_FUNCTION_PROTO(getEndIndex, protoFlags),
	AVM1_FUNCTION_PROTO(getCaretIndex, protoFlags),
	AVM1_FUNCTION_PROTO(getFocus, protoFlags),
	AVM1_FUNCTION_PROTO(setFocus, protoFlags),
	AVM1_FUNCTION_PROTO(setSelection, protoFlags)
};

AVM1Selection::AVM1Selection
(
	AVM1DeclContext& ctx,
	const GcPtr<AsBroadcasterFuncs>& bcastFuncs,
	const GcPtr<AVM1Object>& arrayProto
) : AVM1Object(ctx.getGcCtx(), ctx.getObjectProto())
{
	bcastFuncs->init(ctx.getGcCtx(), this, arrayProto);
	ctx.definePropsOn(this, objDecls);
}

static Optional<TextSelection> getCurrentSel(AVM1Activation& act)
{
	auto focusTarget = act.getStage()->getFocusTarget();
	if (focusTarget.isNull())
		return {};
	return focusTarget->tryAs<TextField>().andThen([&](const auto& focus)
	{
		return focus->tryGetSel();
	});
}

AVM1_FUNCTION_BODY(AVM1Selection, getBeginIndex)
{
	return getCurrentSel(act).transformOr(number_t(-1), [&](const auto& sel)
		return sel.getStart();
	});
}

AVM1_FUNCTION_BODY(AVM1Selection, getEndIndex)
{
	return getCurrentSel(act).transformOr(number_t(-1), [&](const auto& sel)
		return sel.getEnd();
	});
}

AVM1_FUNCTION_BODY(AVM1Selection, getCaretIndex)
{
	return getCurrentSel(act).transformOr(number_t(-1), [&](const auto& sel)
		return sel.getCaret();
	});
}

AVM1_FUNCTION_BODY(AVM1Selection, getFocus)
{
	auto focusTarget = act.getStage()->getFocusTarget();
	if (focusTarget.isNull())
		return AVM1Value::nullVal;
	return focusTarget->toAVM1ValueOrUndef(act).toString(act);
}

AVM1_FUNCTION_BODY(AVM1Selection, setFocus)
{
	if (args.empty())
		return false;
	// NOTE: `setFocusTarget(NullGc)` always returns `true`.
	if (args[0].isNullOrUndefined())
		return act.getStage()->setFocusTarget(NullGc);

	auto obj = act.resolveTargetClip
	(
		act.getTargetOrRootClip(),
		args[0],
		false
	);

	if (obj.isNull() || !obj->is<InteractiveObject>())
		return false;
	return act.getStage()->setFocusTarget(obj->as<InteractiveObject>());
}

AVM1_FUNCTION_BODY(AVM1Selection, setSelection)
{
	auto focusTarget = act.getStage()->getFocusTarget();
	if (focusTarget.isNull() || !focusTarget->is<TextField>())
		return AVM1Value::undefinedVal;

	auto textField = focusTarget->as<TextField>();
	ssize_t start;
	ssize_t end;
	AVM1_ARG_UNPACK(start, 0)(end, -1);

	textField->setSel(start, end);
	return AVM1Value::undefinedVal;
}
