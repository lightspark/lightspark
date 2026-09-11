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

#include <sstream>

#include "gc/context.h"
#include "gc/ptr.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/movieclip_ref.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/scope.h"
#include "scripting/avm1/value.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::function` crate.


#ifdef USE_STRING_ID
AVM1Value AVM1Execution::exec
(
	AVM1Activation& act,
	const tiny_string& name,
	const AVM1Value& _this,
	uint8_t depth,
	const std::vector<AVM1Value>& args,
	const AVM1ExecutionReason& reason,
	const GcPtr<AVM1Object>& callee
) const
{
	auto sys = act.getSys();
	auto id = sys->getUniqueStringId(name, act.isCaseSensitive());
	return exec(act, id, _this, depth, args, reason, callee);
}
#endif


void AVM1Function::loadThis
(
	AVM1Activation& activation,
	const AVM1Value& _this,
	uint8_t& preloadReg
)
{
	if (!(flags & AVM1FuncFlags::PreloadThis))
		return;

	bool suppress = flags & AVM1FuncFlags::SuppressThis;
	// NOTE: The `this` register is set to `undefined`, if both flags
	// are set.
	activation.setLocalReg
	(
		preloadReg++,
		suppress ? AVM1Value::undefinedVal : _this
	);
}

void AVM1Function::loadArgs
(
	AVM1Activation& activation,
	const std::vector<AVM1Value>& args,
	const NullableGcPtr<AVM1Object>& caller,
	uint8_t& preloadReg
)
{
	bool preload = flags & AVM1FuncFlags::PreloadArgs;
	bool suppress = flags & AVM1FuncFlags::SuppressArgs;
	if (suppress && !preload)
		return;

	auto arguments = GC_NEW(activation.getGcCtx()) AVM1Array
	(
		activation,
		args
	);

	arguments->defineValue
	(
		"callee",
		activation.getCallee(),
		AVM1PropFlags::DontEnum
	);

	arguments->defineValue
	(
		"caller",
		!caller.isNull() ? AVM1Value(caller) : AVM1Value::nullVal,
		AVM1PropFlags::DontEnum
	);

	// Contrary to `this`, and `super`, setting both flags is the same as
	// just setting `preload`.
	if (preload)
		activation.setLocalReg(preloadReg++, arguments);
	else
		activation.forceDefineLocal("arguments", arguments);
}

void AVM1Function::loadSuper
(
	AVM1Activation& activation,
	const NullableGcPtr<AVM1Object>& _this,
	uint8_t depth,
	uint8_t& preloadReg
)
{
	bool preload = flags & AVM1FuncFlags::PreloadArgs;
	bool suppress = flags & AVM1FuncFlags::SuppressArgs;

	if (!preload && suppress)
		return;

	auto ctx = activation.getGcCtx();
	// TODO: `super` should only be defined, if this is a method call
	// (`depth > 0`).
	// `f[""]()` emits an `ActionCallMethod` instruction, causing `this`
	// to be `undefined`, but `super` is a function; what is it?
	auto super = _this.isNull() ? nullptr : GC_NEW(ctx) AVM1SuperObject
	(
		_this,
		depth
	);

	// NOTE: The `super` register is set to `undefined`, if both flags
	// are set.
	if (preload)
	{
		auto superVal =
		(
			super != nullptr ?
			AVM1Value(super) :
			AVM1Value::undefinedVal
		);
		activation.setLocalReg(preloadReg++, superVal);
	}
	else if (super != nullptr)
		activation.forceDefineLocal("super", super);

}

void AVM1Function::loadRoot(AVM1Activation& activation, uint8_t& preloadReg)
{
	if (!(flags & AVM1FuncFlags::PreloadRoot))
		return;

	auto baseClip = activation.getBaseClip();
	activation.setLocalReg
	(
		preloadReg++,
		getBaseClip->getAVM1Root()->asAVM1Object(activation)
	);
}

void AVM1Function::loadParent(AVM1Activation& activation, uint8_t& preloadReg)
{
	if (!(flags & AVM1FuncFlags::PreloadParent))
		return;

	auto parent = activation.getBaseClip()->getAVM1Parent();

	// NOTE: If `_parent` is `undefined` (because it's a root clip), it
	// actually doesn't get pushed, with `_global` taking it's place
	// instead.
	if (parent == nullptr)
		return;

	activation.setLocalReg
	(
		preloadReg++,
		parent->asAVM1Object(activation)
	);
}

void AVM1Function::loadGlobal(AVM1Activation& activation, uint8_t& preloadReg)
{
	if (!(flags & AVM1FuncFlags::PreloadGlobal))
		return;

	activation.setLocalReg
	(
		preloadReg++,
		activation.getCtx()->getGlobal()
	);
}

AVM1Value AVM1Function::exec
(
	AVM1Activation& activation,
	#ifdef USE_STRING_ID
	uint32_t name,
	#else
	const tiny_string& name,
	#endif
	const AVM1Value& _this,
	uint8_t depth,
	const std::vector<AVM1Value>& args,
	const AVM1ExecutionReason& reason,
	const GcPtr<AVM1Object>& callee
) const
{
	NullableGcPtr<AVM1Object> thisObj = _this;

	bool isClosure = activation.getSwfVersion() >= 6;
	auto thisDispObj = thisObj.asOpt().transformOr
	(
		activation.tryGetTargetClip(),
		[](const AVM1Object& _this)
		{
			return _this->as<DisplayObject>();
		}
	);

	auto _baseClip = thisDispObj;
	if (isClosure || reason == AVM1ExecutionReason::Special)
	{
		_baseClip = baseClip->resolveRef
		(
			activation
		).transformOr(_baseClip, [](const auto& pair)
		{
			return pair.second;
		});
	}

	auto wrk = activation.getSys()->worker;
	auto gcCtx = activation.getGcCtx();
	auto ctx = activation.getCtx();

	// Function calls in SWF 6, and later are proper closures, and close
	// over the scope that the function was defined in:
	// - Use the SWF version of the SWF that the function was defined in.
	// - Use the base clip from when the function was defined.
	// - Close over the scope that the function was defined in.
	auto _swfVersion = swfVersion;
	auto parentScope = scope;

	// Function calls in SWF 5 *aren't* closures, and reuse `this`'
	// settings, regardless of the function's origin:
	// - Use `this`' SWF version.
	// - Use `this`' base clip.
	// - Create a new scope, using the given base clip. Previous scopes
	// aren't closed over.
	if (!isClosure)
	{
		_swfVersion = std::max<uint8_t>(_baseClip->getSwfVersion(), 5);
		parentScope = GC_NEW(gcCtx) AVM1Scope
		(
			ctx.getGlobalScope(),
			_baseClip->asAVM1Object()
		);
	}

	bool inheritedThis = flags &
	(
		AVM1FuncFlags::PreloadThis |
		AVM1FuncFlags::SuppressThis
	);


	AVM1Activation frame
	(
		ctx
		activation.getID().makeFunctionID
		(
			name,
			reason,
			wrk->limits.max_recursion
		),
		_swfVersion,
		GC_NEW(gcCtx) AVM1Scope(parentScope, activation),
		constPool,
		_baseClip,
		inheritedThis ? activation.getThis() : _this,
		callee
	);

	frame.allocLocalRegs(regCount);

	// The caller is the previous callee.
	auto argsCaller = activation.getCallee();

	uint8_t preloadReg = 1;
	loadThis(frame, _this, preloadReg);
	loadArgs(frame, args, argsCaller, preloadReg);
	loadThis(frame, thisObj, depth, preloadReg);
	loadRoot(frame, preloadReg);
	loadParent(frame, preloadReg);
	loadGlobal(frame, preloadReg);

	// TODO: What happens if the arg registers clash with the preloaded
	// registers? What gets done last?
	auto it = this->args.begin();
	auto it2 = args.begin();
	for (; it != this->args.end(); ++it, std::next(it2, it2 != args.end()))
	{
		// NOTE: Any unassigned args are set to `undefined` in order to
		// prevent assignments from leaking into the parent scope.
		auto value = it2 != args.end() ? *it2 : AVM1Value::undefinedVal;
		auto arg = *it;

		if (arg.reg)
			frame.setLocalReg(arg.reg, value);
		else
			frame.forceDefineLocal(arg.name, value);
	}

	return frame.runActions(data);
}

AVM1_FUNCTION_BODY(AVM1NativeFunction, emptyFunc)
{
	return AVM1Value::undefinedVal;
}


AVM1FunctionObject::AVM1FunctionObject
(
	AVM1Activation& activation,
	const GcPtr<AVM1Execution>& _func,
	const AVM1NativeFunc _ctor,
	const GcPtr<AVM1Object>& funcProto,
	const GcPtr<AVM1Object>& proto
) : AVM1FunctionObject(activation, _func, _ctor, funcProto)
{
	proto->defineValue("constructor", func, AVM1PropFlags::DontEnum);
	proto->defineValue("prototype", proto, AVM1PropFlags(0));
}

AVM1Value AVM1FunctionObject::call
(
	AVM1Activation& activation,
	const tiny_string& name,
	const GcPtr<AVM1Object>& callee,
	const AVM1Value& _this,
	const std::vector<AVM1Value>& args
) const
{
	return func->exec
	(
		activation,
		name,
		_this,
		0,
		args,
		AVM1ExecutionReason::FunctionCall,
		callee
	)
}

AVM1Value AVM1FunctionObject::callCtor
(
	AVM1Activation& activation,
	const GcPtr<AVM1Object>& callee,
	GcPtr<AVM1Object> _this,
	const std::vector<AVM1Value>& args
) const
{
	_this->defineValue("__constructor__", callee, AVM1PropFlags::DontEnum);

	if (activation.getSwfVersion() < 7)
		_this->defineValue("constructor", callee, AVM1PropFlags::DontEnum);

	return asCtor->exec
	(
		activation,
		"[ctor]",
		_this,
		1,
		args,
		AVM1ExecutionReason::FunctionCall,
		callee
	);
}

AVM1Value AVM1FunctionObject::construct
(
	AVM1Activation& activation,
	const GcPtr<AVM1Object>& callee,
	const std::vector<AVM1Value>& args
) const
{
	auto ctx = activation.getGcCtx();
	auto _this = GC_NEW(ctx) AVM1Object(activation, callee->getProp
	(
		activation,
		"prototype"
	));

	auto ret = callCtor(activation, callee, _this, args);

	// Propagate the return value, if we're calling a native ctor.
	return isNativeCtor() ? ret : _this;
}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1FunctionObject;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1FunctionObject, call, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1FunctionObject, apply, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1FunctionObject::createClass
(
	AVM1DeclContext& ctx
)
{
	auto _class = ctx.makeNativeClassWithProto
	(
		ctor,
		function,
		ctx.getFuncProto()
	);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1FunctionObject, ctor)
{
	if (args.empty())
		return _this;
	return args[0];
}

AVM1_FUNCTION_BODY(AVM1FunctionObject, function)
{
	if (!args.empty())
		return args[0];
	// NOTE: Calling `Function()` seems to return a prototypeless, bare
	// `Object`.
	return NEW_GC_PTR(act.getGcCtx(), AVM1Object(act.getGcCtx()));
}

AVM1_FUNCTION_TYPE_BODY(AVM1FunctionObject, AVM1FunctionObject, call)
{
	NullableGcPtr<AVM1Object> thisObj;
	AVM1_ARG_UNPACK(thisObj, act.getGlobal());

	// NOTE This doesn't use `AVM1Object::call()` because `super()` only
	// works with direct calls.
	return _this->func->exec
	(
		act,
		"[Anonymous]",
		thisObj,
		1,
		args.tryGetFirst(2),
		AVM1ExecutionReason::FunctionCall,
		_this
	);
}

AVM1_FUNCTION_TYPE_BODY(AVM1FunctionObject, AVM1FunctionObject, apply)
{
	NullableGcPtr<AVM1Object> thisObj;
	NullableGcPtr<AVM1Object> argsObj;
	AVM1_ARG_UNPACK(thisObj, act.getGlobal())(argsObj);

	size_t size = !argsObj.isNull() ? argsObj->getLength(act) : 0;

	std::vector<AVM1Value> funcArgs;
	funcArgs.reserve(size);

	for (size_t i = 0; i < size; ++i)
		funcArgs.emplace_back(argsObj->getElement(act, i));

	// NOTE This doesn't use `AVM1Object::call()` because `super()` only
	// works with direct calls.
	return _this->func->exec
	(
		act,
		"[Anonymous]",
		thisObj,
		1,
		funcArgs,
		AVM1ExecutionReason::FunctionCall,
		_this
	);
}
