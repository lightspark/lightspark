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
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/ContextMenuItem.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::context_menu_item` crate.

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1ContextMenuItem;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_PROTO(copy, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1ContextMenuItem::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1ContextMenuItem, ctor)
{
	tiny_string caption;
	Optional<AVM1Value> callback;
	bool spearatorBefore;
	bool enabled;
	bool visible;
	AVM1_ARG_UNPACK(caption)(callback)
	(
		separatorBefore,
		false
	)(enabled, false)(visible, true);

	_this->setProp(act, "caption", caption);

	callback.andThen([&](const auto& callback)
	{
		_this->setProp(act, "onSelect", callback);
		return callback;
	});

	_this->setProp(act, "separatorBefore", separatorBefore);
	_this->setProp(act, "enabled", enabled);
	_this->setProp(act, "visible", visible);

	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1ContextMenuItem, copy)
{
	auto _ctor = act.getPrototypes()->contextMenuItem->ctor;
	auto swfVersion = act.getSwfVersion();

	auto caption = _this->getProp(act, "caption").toString(act);
	auto callback = _this->getProp(act, "onSelect").toObject(act);
	bool enabled = _this->getProp(act, "enabled").toBool(swfVersion);
	bool separatorBefore = _this->getProp(act, "separatorBefore").toBool(swfVersion);
	bool visible = _this->getProp(act, "visible").toBool(swfVersion);
	
	return _ctor->construct(act,
	{
		caption,
		callback,
		separatorBefore,
		enabled,
		visible
	});
}
