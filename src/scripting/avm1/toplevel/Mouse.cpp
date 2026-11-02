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

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/AsBroadcaster.h"
#include "scripting/avm1/toplevel/Mouse.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::mouse` crate.

using AVM1Mouse;

constexpr auto objFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(show, objFlags),
	AVM1_FUNCTION_PROTO(hide, objFlags)
};

AVM1Mouse::AVM1Mouse
(
	AVM1DeclContext& ctx,
	const GcPtr<AsBroadcasterFuncs>& bcastFuncs,
	const GcPtr<AVM1Object>& arrayProto
) : AVM1Object(ctx.getGcCtx(), ctx.getObjectProto())
{
	bcastFuncs->init(ctx.getGcCtx(), this, arrayProto);
	ctx.definePropsOn(this, objDecls);
}

AVM1_FUNCTION_BODY(AVM1Mouse, show)
{
	bool wasVisible = act.getSys()->isMouseVisible();
	act.getSys()->showMouseCursor(true);

	// NOTE: This returns a `Number`, not a `Boolean`.
	return number_t(!wasVisible);
}

AVM1_FUNCTION_BODY(AVM1Mouse, hide)
{
	bool wasVisible = act.getSys()->isMouseVisible();
	act.getSys()->showMouseCursor(false);

	// NOTE: This returns a `Number`, not a `Boolean`.
	return number_t(!wasVisible);
}
