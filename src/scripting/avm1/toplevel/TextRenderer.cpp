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
#include "scripting/avm1/toplevel/TextRenderer.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::text_renderer` crate.

constexpr auto objFlags =
(
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::Version8
);

constexpr auto objFlagsV9 =
(
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::Version9
);

using AVM1TextRenderer;

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(setAdvanceAntialiasingTable, objFlags),
	AVM1_PROPERTY_PROTO(DisplayMode, objFlagsV9),
	AVM1_PROPERTY_PROTO(MaxLevel, objFlags)
};

GcPtr<AVM1SystemClass> AVM1TextRenderer::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1TextRenderer, setAdvanceAntialiasingTable)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextRenderer.setAdvanceAntialiasingTable()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_BODY(AVM1TextRenderer, DisplayMode)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextRenderer.displayMode`'s getter is a stub.");
	return "default";
}

AVM1_SETTER_BODY(AVM1TextRenderer, DisplayMode)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextRenderer.displayMode`'s setter is a stub.");
}

AVM1_GETTER_BODY(AVM1TextRenderer, MaxLevel)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextRenderer.maxLevel`'s getter is a stub.");
	return 4;
}

AVM1_SETTER_BODY(AVM1TextRenderer, MaxLevel)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextRenderer.maxLevel`'s setter is a stub.");
}
