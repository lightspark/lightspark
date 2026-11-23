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

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/Accessibility.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::accessibility` crate.

constexpr auto protoFlags =
(
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly |
	AVM1PropFlags::Version6
);

using AVM1Accessibility;

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(isActive, protoFlags),
	AVM1_FUNCTION_PROTO(sendEvent, protoFlags),
	AVM1_FUNCTION_PROTO(updateProperties, protoFlags)
};

AVM1Accessibility::AVM1Accessibility(AVM1DeclContext& ctx) : AVM1Object
(
	ctx.getGcCtx(),
	ctx.getObjectProto()
)
{
	ctx.definePropsOn(this, objDecls);
}

AVM1_FUNCTION_BODY(AVM1Accessibility, isActive)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Accessibility.isActive()` is a stub.");
	return false;
}

AVM1_FUNCTION_BODY(AVM1Accessibility, sendEvent)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Accessibility.sendEvent()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Accessibility, updateProperties)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `Accessibility.updateProperties()` is a stub.");
	return AVM1Value::undefinedVal;
}
