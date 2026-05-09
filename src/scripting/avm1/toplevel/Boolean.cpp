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

#include <sstream>

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/value.h"
#include "scripting/avm1/toplevel/Boolean.h"
#include "tiny_string.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::boolean` crate.

AVM1Boolean::AVM1Boolean
(
	const GcContext& ctx,
	GcPtr<AVM1Object> proto,
	bool _value
) : AVM1Object(ctx, proto), value(_value) {}

using AVM1Boolean;

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1Boolean, toString, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Boolean, valueOf, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1Boolean::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, boolean, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Boolean, ctor)
{
	bool value;
	AVM1_ARG_UNPACK(value, false);

	INPLACE_NEW_GC_PTR(act.getGcCtx(), _this, AVM1Boolean
	(
		act.getGcCtx(),
		_this->getProto(act).toObject(act),
		value
	));

	return _this;
}

AVM1_FUNCTION_BODY(AVM1Boolean, boolean)
{
	// When `Boolean()` is called as a function, it returns the value
	// itself, converted into a bool.
	// NOTE: If `Boolean()` is called without an argument, it returns
	// `undefined`.
	bool value;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(value));
	return value;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Boolean, AVM1Boolean, toString)
{
	// NOTE: The object must be a `Boolean`. This is because
	// `Boolean.prototype.toString.call()` returns `undefined` for non
	// `Boolean` objects.
	return _this->value ? "true" : "false";
}

AVM1_FUNCTION_TYPE_BODY(AVM1Boolean, AVM1Boolean, valueOf)
{
	// NOTE: The object must be a `Boolean`. This is because
	// `Boolean.prototype.valueOf.call()` returns `undefined` for non
	// `Boolean` objects.
	return _this->value;
}
