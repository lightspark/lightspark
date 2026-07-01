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

#include "backends/input.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/AsBroadcaster.h"
#include "scripting/avm1/toplevel/Key.h"
#include "scripting/flash/ui/keycodes.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::key` crate.

using AVM1Key;

constexpr auto objFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

static constexpr auto objDecls =
{
	AVM1Decl("ALT", AS3KEYCODE_ALTERNATE, objFlags),
	AVM1Decl("BACKSPACE", AS3KEYCODE_BACKSPACE, objFlags),
	AVM1Decl("CAPSLOCK", AS3KEYCODE_CAPS_LOCK, objFlags),
	AVM1Decl("CONTROL", AS3KEYCODE_CONTROL, objFlags),
	AVM1Decl("DELETEKEY", AS3KEYCODE_DELETE, objFlags),
	AVM1Decl("DOWN", AS3KEYCODE_DOWN, objFlags),
	AVM1Decl("END", AS3KEYCODE_END, objFlags),
	AVM1Decl("ENTER", AS3KEYCODE_ENTER, objFlags),
	AVM1Decl("ESCAPE", AS3KEYCODE_ESCAPE, objFlags),
	AVM1Decl("HOME", AS3KEYCODE_HOME, objFlags),
	AVM1Decl("INSERT", AS3KEYCODE_INSERT, objFlags),
	AVM1Decl("LEFT", AS3KEYCODE_LEFT, objFlags),
	AVM1Decl("PGDN", AS3KEYCODE_PAGE_DOWN, objFlags),
	AVM1Decl("PGUP", AS3KEYCODE_PAGE_UP, objFlags),
	AVM1Decl("RIGHT", AS3KEYCODE_RIGHT, objFlags),
	AVM1Decl("SHIFT", AS3KEYCODE_SHIFT, objFlags),
	AVM1Decl("SPACE", AS3KEYCODE_SPACE, objFlags),
	AVM1Decl("TAB", AS3KEYCODE_TAB, objFlags),
	AVM1Decl("UP", AS3KEYCODE_UP, objFlags),

	AVM1_FUNCTION_PROTO(isDown, objFlags),
	AVM1_FUNCTION_PROTO(isToggled, objFlags),
	AVM1_FUNCTION_PROTO(getCode, objFlags),
	AVM1_FUNCTION_PROTO(getAscii, objFlags)
};

AVM1Key::AVM1Key
(
	AVM1DeclContext& ctx,
	const GcPtr<AsBroadcasterFuncs>& bcastFuncs,
	const GcPtr<AVM1Object>& arrayProto
) : AVM1Object(ctx.getGcCtx(), ctx.getObjectProto())
{
	bcastFuncs->init(ctx.getGcCtx(), this, arrayProto);
	ctx.definePropsOn(this, objDecls);
}

AVM1_FUNCTION_BODY(AVM1Key, isDown)
{
	AS3KeyCode key;
	AVM1_ARG_UNPACK.unpack<int32_t>(key);
	return act.getSys()->getInputThread()->isKeyDown(key);
}

AVM1_FUNCTION_BODY(AVM1Key, isToggled)
{
	AS3KeyCode key;
	AVM1_ARG_UNPACK.unpack<int32_t>(key);
	return act.getSys()->getInputThread()->isKeyToggled(key);
}

AVM1_FUNCTION_BODY(AVM1Key, getCode)
{
	return number_t(act.getSys()->getInputThread()->getLastKeyCode());
}

AVM1_FUNCTION_BODY(AVM1Key, getAscii)
{
	return number_t(act.getSys()->getInputThread()->getLastKeyChar());
}
