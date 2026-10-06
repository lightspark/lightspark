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
#include "scripting/avm1/toplevel/ContextMenu.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::context_menu` crate.

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1ContextMenu;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_PROTO(copy, protoFlags),
	AVM1_FUNCTION_PROTO(hideBuiltInItems, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1ContextMenu::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1ContextMenu, ctor)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);
	_this->setProp
	(
		act,
		"onSelect",
		unpacker.unpack<GcPtr<AVM1Object>>()
	);

	auto builtInItems = NEW_GC_PTR(act.getGcCtx(), AVM1Object(act));

	builtInItems->setProp(act, "print", true);
	builtInItems->setProp(act, "forward_back", true);
	builtInItems->setProp(act, "rewind", true);
	builtInItems->setProp(act, "loop", true);
	builtInItems->setProp(act, "play", true);
	builtInItems->setProp(act, "quality", true);
	builtInItems->setProp(act, "zoom", true);
	builtInItems->setProp(act, "save", true);

	_this->setProp(act, "builtInItems", builtInItems);
	_this->setProp
	(
		act,
		"customItems",
		NEW_GC_PTR(act.getGcCtx(), AVM1Array(act))
	);

	return AVM1Value::undefinedVal;
}

GcPtr<AVM1Object> getBuiltInItems
(
	AVM1Activation& act,
	GcPtr<AVM1Object> _this
)
{
	return _this->setProp(act, "builtInItems").toObject(act);
}

AVM1_FUNCTION_BODY(AVM1ContextMenu, copy)
{
	auto _ctor = act.getPrototypes()->contextMenu->ctor;
	auto other = _ctor->construct(act,
	{
		_this->getProp(act, "onSelect").toObject(act)
	}).toObject(act);

	auto items = getBuiltInItems(act, _this);
	auto otherItems = getBuiltInItems(act, other);

	auto copyItem = [&](const tiny_string& name)
	{
		otherItems->setProp(act, name, items->getProp
		(
			act,
			name
		).toBool(act.getSwfVersion()));
	};

	copyItem("save");
	copyItem("zoom");
	copyItem("quality");
	copyItem("play");
	copyItem("loop");
	copyItem("rewind");
	copyItem("forward_back");
	copyItem("print");

	auto customItems = _this->getProp(act, "customItems").toObject(act);
	auto otherCustomItems = other->getProp(act, "customItems").toObject(act);

	size_t size = customItems->getLength(act);
	for (size_t i = 0; i < size; ++i)
	{
		otherCustomItems->setElem
		(
			act,
			i
			customItems->getElem(act, i)
		);
	}
	
	return other;
}

AVM1_FUNCTION_BODY(AVM1ContextMenu, hideBuiltInItems)
{
	auto builtInItems = getBuiltInItems(act, _this);

	builtInItems->setProp(act, "save", false);
	builtInItems->setProp(act, "zoom", false);
	builtInItems->setProp(act, "quality", false);
	builtInItems->setProp(act, "play", false);
	builtInItems->setProp(act, "loop", false);
	builtInItems->setProp(act, "rewind", false);
	builtInItems->setProp(act, "forward_back", false);
	builtInItems->setProp(act, "print", false);

	return AVM1Value::undefinedVal;
}
