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

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/value.h"
#include "scripting/avm1/toplevel/Error.h"
#include "tiny_string.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::error` crate.

using AVM1Error;

static constexpr auto protoDecls =
{
	AVM1Decl("message", "Error"),
	AVM1Decl("name", "Error"),
	AVM1_FUNCTION_PROTO(toString)
};

GcPtr<AVM1SystemClass> AVM1Error::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Error, ctor)
{
	AVM1Value message;
	AVM1_ARG_UNPACK(message);

	if (!message.isUndefined())
		_this->setProp(act, "message", message);

	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Error, toString)
{
	return _this->getProp(act, "message").toString(act);
}
