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

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/toplevel/AsBroadcaster.h"
#include "scripting/avm1/runtime.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::as_broadcaster` crate.

void initImpl
(
	const GcPtr<AsBroadcasterFuncs>& funcs,
	const GcPtr<AVM1Object>& broadcaster,
	const GcPtr<AVM1Object>& arrayProto
);

constexpr auto objFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AsBroadcaster;

static constexpr auto objDecls = makeArray
(
	AVM1_FUNCTION_PROTO(initialize, objFlags),
	AVM1_FUNCTION_PROTO(addListener, objFlags, false),
	AVM1_FUNCTION_PROTO(removeListener, objFlags, false),
	AVM1_FUNCTION_PROTO(broadcastMessage, objFlags, false)
);

CreateClassType AsBroadcaster::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	// NOTE: Despite the AS1/2 docs stating that `AsBroadcaster` has no
	// constructor, Flash Player will accept an expression like
	// `new AsBroadcaster();`, which returns a newly created `Object`
	// in that case.
	auto _class = ctx.makeEmptyClass(superProto);
	auto& gcCtx = ctx.getGcCtx();

	auto defineAsObj = [&](size_t idx) -> GcPtr<AVM1Object>
	{
		auto obj = objDecls[idx].definePropOn
		(
			gcCtx,
			_class->ctor,
			ctx->getFuncProto()
		).as<AVM1Object>();

		if (!obj.isNull())
			return obj;

		throw RunTimeException
		(
			"AsBroadcaster::createClass(): Expected an `Object` for a "
			"broadcaster function."
		);
	};
	
	(void)defineAsObj(0);
	return std::make_pair(NEW_GC_PTR(gcCtx, AsBroadcasterFuncs
	(
		gcCtx,
		// `addListener`.
		defineAsObj(1),
		// `removeListener`.
		defineAsObj(2),
		// `broadcastMessage`.
		defineAsObj(3)
	)), _class);
}

void AsBroadcasterFuncs::init
(
	GcContext& ctx,
	const GcPtr<AVM1Object>& broadcaster,
	const GcPtr<AVM1Object>& arrayProto
) const
{
	initImpl(ctx, this, broadcaster, arrayProto);
}

AVM1_FUNCTION_BODY(AsBroadcaster, addListener)
{
	auto listeners = _this->getProp
	(
		act, "_listeners"
	).as<AVM1Object>();

	AVM1Value newListener;
	AVM1_ARG_UNPACK(newListener);

	if (listeners.isNull())
		return true;

	auto size = listeners->getLength(act);

	bool found = false;
	for (size_t i = 0; i < size; ++i)
	{
		if (listeners->getElement(act, i) == newListener)
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		listeners->callMethod
		(
			act,
			"push",
			{ newListener },
			AVM1ExecutionReason::FunctionCall
		);
	}

	return true;
}

AVM1_FUNCTION_BODY(AsBroadcaster, removeListener)
{
	auto listeners = _this->getProp
	(
		act, "_listeners"
	).as<AVM1Object>();

	AVM1Value oldListener;
	AVM1_ARG_UNPACK(oldListener);

	if (listeners.isNull())
		return false;

	auto size = listeners->getLength(act);

	for (size_t i = 0; i < size; ++i)
	{
		if (listeners->getElement(act, i) != oldListener)
			continue;
		listeners->callMethod
		(
			act,
			"splice",
			{ number_t(i), 1.0 },
			AVM1ExecutionReason::FunctionCall
		);
		return true;
	}

	return false;
}

AVM1_FUNCTION_BODY(AsBroadcaster, broadcastMessage)
{
	if (args.empty())
		return AVM1Value::undefinedVal;
	return broadcastImpl
	(
		act,
		_this,
		// TODO: Make `args` a `Span`.
		{ std::next(args.begin()), args.end() },
		args[0].toString(act)
	);
}

AVM1_FUNCTION_BODY
(
	AsBroadcaster,
	broadcastImpl,
	const tiny_string& methodName
)
{
	auto listeners = _this->getProp
	(
		act, "_listeners"
	).as<AVM1Object>();

	if (listeners.isNull())
		return false;

	auto size = listeners->getLength(act);

	for (size_t i = 0; i < size; ++i)
	{
		auto listener = listeners->getElement(act, i);
		if (listener.isPrimitive())
			continue;

		listener.toObject(act)->callMethod
		(
			act,
			methodName,
			args,
			AVM1ExecutionReason::Special
		);
	}

	return size > 0;
}

AVM1_FUNCTION_BODY(AsBroadcaster, initialize)
{
	if (args.empty())
		return AVM1Value::undefinedVal;

	initImpl
	(
		act.getGcCtx(),
		act.getCtx().getBroadcasterFuncs(act.getSwfVersion()),
		args[0].toObject(act),
		act.getPrototypes().array->proto
	);
	return AVM1Value::undefinedVal;
}

void initImpl
(
	GcContext& ctx,
	const GcPtr<AsBroadcasterFuncs>& funcs,
	const GcPtr<AVM1Object>& broadcaster,
	const GcPtr<AVM1Object>& arrayProto
)
{
	broadcaster->defineValue("_listeners", NEW_GC_PTR(ctx, AVM1Array
	(
		ctx,
		arrayProto
	)), AVM1PropFlags::DontEnum);

	broadcaster->defineValue
	(
		"addListener",
		funcs->addListener,
		objFlags
	);

	broadcaster->defineValue
	(
		"removeListener",
		funcs->removeListener,
		objFlags
	);

	broadcaster->defineValue
	(
		"broadcastMessage",
		funcs->broadcastMessage,
		objFlags
	);
}
