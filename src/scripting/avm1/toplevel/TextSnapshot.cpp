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
#include "scripting/avm1/toplevel/TextSnapshot.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::text_snapshot` crate.

AVM1TextSnapshot::AVM1TextSnapshot(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().textSnapshot->proto
) {}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly |
	AVM1PropFlags::Version6
);

using AVM1TextSnapshot;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextSnapshot, getCount, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextSnapshot, setSelected, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextSnapshot, getSelected, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextSnapshot, getText, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextSnapshot, getSelectedText, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextSnapshot, hitTestTextNearPos, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextSnapshot, setSelectColor, protoFlags);
	AVM1_FUNCTION_TYPE_PROTO(AVM1TextSnapshot, getTextRunInfo, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1TextSnapshot::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1TextSnapshot, ctor)
{
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextSnapshot, AVM1TextSnapshot, getCount)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextSnapshot.getCount()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextSnapshot, AVM1TextSnapshot, setSelected)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextSnapshot.setSelected()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextSnapshot, AVM1TextSnapshot, getSelected)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextSnapshot.getSelected()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextSnapshot, AVM1TextSnapshot, getText)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextSnapshot.getText()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextSnapshot, AVM1TextSnapshot, getSelectedText)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextSnapshot.getSelectedText()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextSnapshot, AVM1TextSnapshot, hitTestTextNearPos)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextSnapshot.hitTestTextNearPos()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextSnapshot, AVM1TextSnapshot, setSelectColor)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextSnapshot.setSelectColor()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1TextSnapshot, AVM1TextSnapshot, getTextRunInfo)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `TextSnapshot.getTextRunInfo()` is a stub.");
	return AVM1Value::undefinedVal;
}
