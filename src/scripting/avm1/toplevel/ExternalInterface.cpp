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

#include "backends/extscriptobject.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/ExternalInterface.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"
#include "utils/span.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::external_interface` crate.

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

using AVM1ExternalInterface;

static constexpr auto objDecls =
{
	AVM1_GETTER_PROTO(available, Available, protoFlags),
	AVM1_FUNCTION_PROTO(addCallback, protoFlags),
	AVM1_FUNCTION_PROTO(call, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1ExternalInterface::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeEmptyClass(superProto);
	ctx.definePropsOn(_class->ctor, objDecls);
	return _class;
}

AVM1_GETTER_BODY(AVM1ExternalInterface, Available)
{
	return act.getSys()->extScriptObject != nullptr;
}

AVM1_FUNCTION_BODY(AVM1ExternalInterface, addCallback)
{
	auto extScriptObj = act.getSys()->extScriptObject;
	if (extScriptObj == nullptr || args.size() < 3)
		return false;

	tiny_string methodName;
	AVM1Value instance;
	NullableGcPtr<AVM1Object> method;

	AVM1_ARG_UNPACK_MULTIPLE(methodName, instance, method);

	if (method.isNullOrUndefined())
		return false;

	extScriptObj->setMethod(methodName, NEW_GC_PTR
	(
		act.getGcCtx(),
		ExtAVM1Callback(act.getGcCtx(), instance, method)
	));
}

AVM1_FUNCTION_BODY(AVM1ExternalInterface, call)
{
	auto extScriptObj = act.getSys()->extScriptObject;
	if (extScriptObj == nullptr)
		return AVM1Value::nullVal;

	tiny_string name;
	AVM1_ARG_UNPACK(name);

	auto argSpan = makeSpan(args).trySubSpan(1);
	std::vector<ExtVariant> extArgs(argSpan.begin(), argSpan.end());

	return *extScriptObj->callExternal
	(
		name,
		makeSpan(extArgs)
	).andThen([&](const auto& val) -> Optional<AVM1Value>
	{
		return val.toAVM1Value(act);
	}).orElse([&]
	{
		LOG
		(
			LOG_INFO,
			"AVM1: External function \"" << name <<
			"\" failed, returning `null`."
		);
		return AVM1Value::nullVal;
	});
}
