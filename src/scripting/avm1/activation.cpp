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

#include "display_object/DisplayObject.h"
#include "display_object/DisplayObjectContainer.h"
#include "display_object/Stage.h"
#include "gc/context.h"
#include "parsing/encoding.h"
#include "request.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/movieclip_ref.h"
#include "scripting/avm1/object.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/scope.h"
#include "swf.h"
#include "utils/any.h"
#include "utils/impl.h"
#include "utils/optional.h"
#include "utils/type_traits.h"

using namespace lightspark;

// Based on Ruffle's `avm1::activation::Activation`.

AVM1Activation::Identifier AVM1Activation::Identifier::makeFuncID
(
	const tiny_string& name
	const AVM1ExecutionReason& reason,
	uint16_t maxRecursionDepth
) const
{
	using ExceptionType = AVM1Exception::Type;
	using Reason = AVM1ExecutionReason;
	bool isFuncCall = reason == Reason::FunctionCall;
	bool isSpecialCall = reason == Reason::Special;
	uint16_t _funcCount = funcCount + isFuncCall;
	uint8_t _specialCount = specialCount + + isSpecialCall;

	if (isFuncCall && _funcCount >= maxRecursionDepth)
		throw AVM1Exception(ExceptionType::MaxFunctionRecursion);
	else if (isSpecialCall && specialCount == 65)
		throw AVM1Exception(ExceptionType::MaxSpecialRecursion);

	return Identifier(this, name, depth + 1, _funcCount, _specialCount);
}

AVM1Activation::AVM1Activation
(
	AVM1Context& ctx,
	const Identifier& id,
	uint8_t swfVersion,
	const GcPtr<AVM1Scope>& _scope,
	const std::vector<uint32_t>& _constPool,
	const GcPtr<DisplayObject>& _baseClip,
	const AVM1Value& _this,
	const NullableGcPtr<AVM1Object>& callee
) : AVM1Activation
(
	ctx,
	id,
	swfVersion,
	scope,
	constPool,
	baseClip,
	// `targetClip`.
	baseClip,
	// `baseClipUnloaded`.
	baseClip->isAVM1Removed(),
	_this,
	callee
)
{
}

AVM1Activation AVM1Activation::withNewScope
(
	const tiny_string& name,
	const GcPtr<AVM1Scope>& _scope
)
{
	return AVM1Activation
	(
		ctx,
		id.makeChildID(name),
		swfVersion,
		_scope,
		constPool,
		baseClip,
		baseClipUnloaded,
		_this,
		callee,
		localRegs
	);
}

AVM1Activation::AVM1Activation
(
	AVM1Context& ctx,
	const Identifier& id,
	const GcPtr<DisplayObject>& baseClip
) : AVM1Activation
(
	ctx,
	id,
	baseClip->getSwfVersion(),
	ctx.getGlobalScope(),
	ctx.getConstPool(),
	baseClip,
	ctx.getGlobal(),
	nullptr
)
{
}

AVM1Activation::AVM1Activation
(
	AVM1Context& ctx,
	const Identifier& id
) : AVM1Activation
(
	ctx,
	id,
	// [b0nk, Dinnerbone]: We have 3 options here, in the case that there's
	// no root clip.
	// 1. Don't execute anything (throw, and let the caller handle it).
	// 2. Execute something with a temporary orphaned clip.
	// 3. Execute something with no clip at all.
	// Neither myself, nor Dinnerbone are even sure if it's possible to
	// get into this state, since AVM2 is the only one that can remove
	// the root clip (as far as either of us can tell). So, just assert
	// for now.
	// If we ever see this assert fail in the wild, we can look into
	// what approach it expects.
	ctx.getStage()->getRoot().asOpt().orElse([&]
	{
		assert(("AVM1 should always have a root clip", false));
		return {};
	})
)
{
}

AVM1Value AVM1Activation::runChildFrame
(
	const tiny_string& name,
	const GcPtr<DisplayObject>& activeClip,
	const std::vector<uint8_t>& code
)
{
	AVM1Activation parentAct
	(
		ctx,
		id.makeChildID("[Actions Parent]"),
		activeClip
	);

	GcPtr<AVM1DisplayObject> clipObj = activeClip;

	AVM1Activation childAct
	(
		parentAct.ctx,
		parentAct.id.makeChildID(name),
		activeClip->getSwfVersion(),
		NEW_GC_PTR(parentAct.getGcCtx(), AVM1Scope
		(
			parentAct.getScope(),
			clipObj
		)),
		parentAct.ctx.getConstPool(),
		activeClip,
		clipObj,
		nullptr
	);

	return childAct.runActions(code);
}

Any AVM1Activation::runChildFrameForClip
(
	const tiny_string& name,
	const GcPtr<DisplayObject>& activeClip,
	uint8_t _swfVersion,
	const std::function<Any(AVM1Activation&)>& func
)
{
	GcPtr<AVM1DisplayObject> clipObj = activeClip;

	AVM1Activation act
	(
		ctx,
		id.makeChildID(name),
		_swfVersion,
		NEW_GC_PTR(parentAct.getGcCtx(), AVM1Scope
		(
			ctx.getGlobalScope(),
			clipObj
		)),
		ctx.getConstPool(),
		activeClip,
		clipObj,
		nullptr
	);

	return func(act);
}

Optional<AVM1Value> AVM1Activation::runActions
(
	const std::vector<uint8_t>& code,
	size_t offset
)
{
	if (!id.funcCount && !id.specialCount)
		ctx.startTime = compat_now();

	for (size_t i = offset; i < code.size();)
	{
		bool isImplicit = false;
		auto ret = doAction(code, i, isImplicit);
		if (ret.hasValue() || isImplicit)
			return ret;
	}

	return {};
}

SystemState* AVM1Activation::getSys() const
{
	return ctx.getSys();
}

const GcContext& AVM1Activation::getGcCtx() const
{
	return ctx.getGcCtx();
}

GcContext& AVM1Activation::getGcCtx()
{
	return ctx.getGcCtx();
}

using ReadVec = std::vector<uint8_t>;
#ifdef USE_PAIR
template<typename T>
using ReadType = std::pair<T, size_t>;
using ReadIdx = size_t;
#else
template<typename T>
using ReadType = T;
using ReadIdx = size_t&;
#endif

template<typename T, EnableIf<std::is_integral<T>::value, bool> = false>
static ReadType<T> readInt(const ReadVec& code, ReadIdx idx)
{
	T ret(0);
	// NOTE: SWF values are stored in little endian.
	// This method of reading a little endian value is both portable,
	// and efficient.
	for (size_t i = 0; i < sizeof(T); ++i)
		ret |= T(code[idx++]) << (i * CHAR_BIT);
	#ifdef USE_PAIR
	return std::make_pair(ret, idx);
	#else
	return ret;
	#endif
}

template<typename T, EnableIf<std::is_floating_point<T>::value, bool> = false>
static ReadType<T> readFloat(const ReadVec& code, ReadIdx idx);

template<>
static ReadType<float> readFloat(const ReadVec& code, ReadIdx idx);
{
	return *static_cast<float*>(&readInt<uint32_t>(code, idx));
}

template<>
static ReadType<double> readFloat(const ReadVec& code, ReadIdx idx);
{
	// NOTE: For some reason, `Double`s are stored as a pair of 32 bit
	// little endian values, with the first word being the high word,
	// and the second word being the low word.
	#if 1
	auto ret = readInt<uint64_t>(code, idx);
	ret = (ret << 32) | (ret >> 32);
	#else
	auto ret =
	(
		uint64_t(readInt<uint32_t>(code, idx) << 32) |
		readInt<uint32_t>(code, idx)
	);
	#endif
	return *static_cast<double*>(&ret);
}

static ReadType<tiny_string> readStr(const ReadVec& code, ReadIdx idx)
{
	tiny_string ret(static_cast<const char*>(&code[idx]), true);
	#ifdef USE_PAIR
	return std::make_pair(ret, idx + ret.numBytes() + 1);
	#else
	idx += ret.numBytes() + 1;
	return ret;
	#endif
}

static ReadType<uint32_t> readStrID
(
	SystemState* sys,
	const ReadVec& code,
	ReadIdx idx,
	bool caseSensitive = true
)
{
	return sys->getUniqueStringId(readStr(code, idx), caseSensitive);
}

static ReadType<tiny_string> readSwfStr
(
	SystemState* sys,
	const TextEncoding& encoding,
	const ReadVec& code,
	ReadIdx idx
)
{
	auto ret = sys->getTextEncodingManager()->decode
	(
		static_cast<const char*>(&code[idx]),
		encoding
	);
	#ifdef USE_PAIR
	return std::make_pair(ret, idx + ret.numBytes() + 1);
	#else
	idx += ret.numBytes() + 1;
	return ret;
	#endif
}

static ReadType<uint32_t> readSwfStrID
(
	SystemState* sys,
	const TextEncoding& encoding,
	const ReadVec& code,
	ReadIdx idx,
	bool caseSensitive = true
)
{
	return sys->getUniqueStringId(readSwfStr
	(
		sys,
		encoding,
		code,
		idx
	), caseSensitive);
}

Optional<AVM1Value> AVM1Activation::doAction
(
	const std::vector<uint8_t>& code,
	size_t& idx,
	bool& isImplicit
)
{
	using ExceptionType = AVM1Exception::Type;

	if (!(++ctx.actionsExecuted % 2000))
	{
		auto delta = compat_now() - ctx.startTime;
		auto wrk = ctx.getSys()->worker;
		if (delta.getSecs() >= wrk->limits.script_timeout)
			throw AVM1Exception(ExceptionType::ScriptTimeout);
	}

	// NOTE: Executing beyond the end of a function acts as an implicit
	// return.
	if (idx >= code.size())
	{
		isImplicit = true;
		return {};
	}

	uint8_t opcode = code[idx++];
	uint16_t length = 0;
	if (opcode >= 0x80)
		length = readInt<uint16_t>(code, idx);

	switch (opcode)
	{
		// `ActionEnd`.
		case 0x00: isImplicit = true; return {}; break;
		case 0x04: actionNextFrame(); break;
		case 0x05: actionPreviousFrame(); break;
		case 0x06: actionPlay(); break;
		case 0x07: actionStop(); break;
		case 0x08: actionToggleQuality(); break;
		case 0x09: actionStopSounds(); break;
		case 0x0a: actionAdd(); break;
		case 0x0b: actionSubtract(); break;
		case 0x0c: actionMultiply(); break;
		case 0x0d: actionDivide(); break;
		case 0x0e: actionEquals(); break;
		case 0x0f: actionLess(); break;

		case 0x10: actionAnd(); break;
		case 0x11: actionOr(); break;
		case 0x12: actionNot(); break;
		case 0x13: actionStringEquals(); break;
		case 0x14: actionStringLength(); break;
		case 0x15: actionStringExtract(); break;
		case 0x17: actionPop(); break;
		case 0x18: actionToInteger(); break;
		case 0x1c: actionSetVariable(); break;
		case 0x1d: actionGetVariable(); break;

		case 0x20: actionSetTarget2(); break;
		case 0x21: actionStringAdd(); break;
		case 0x22: actionGetProperty(); break;
		case 0x23: actionSetProperty(); break;
		case 0x24: actionCloneSprite(); break;
		case 0x25: actionRemoveSprite(); break;
		case 0x26: actionTrace(); break;
		case 0x27: actionStartDrag(); break;
		case 0x28: actionEndDrag(); break;
		case 0x29: actionStringLess(); break;
		case 0x2a: actionThrow(); break;
		case 0x2b: actionCastOp(); break;
		case 0x2c: actionImplementsOp(); break;

		case 0x30: actionRandomNumber(); break;
		case 0x31: actionMBStringLength(); break;
		case 0x32: actionCharToAscii(); break;
		case 0x33: actionAsciiToChar(); break;
		case 0x34: actionGetTime(); break;
		case 0x35: actionMBStringExtract(); break;
		case 0x36: actionMBCharToAscii(); break;
		case 0x37: actionMBAsciiToChar(); break;
		case 0x3a: actionDelete(); break;
		case 0x3b: actionDelete2(); break;
		case 0x3c: actionDefineLocal(); break;
		case 0x3d:
			if (!actionCallFunction())
				return AVM1Value::undefinedVal;
			break;
		case 0x3e: return actionReturn(); break;
		case 0x3f: actionModulo(); break;

		case 0x40:
			if (!actionNewObject())
				return AVM1Value::undefinedVal;
			break;
		case 0x41: actionDefineLocal2(); break;
		case 0x42: actionInitArray(); break;
		case 0x43: actionInitObject(); break;
		case 0x44: actionTypeOf(); break;
		case 0x45: actionTargetPath(); break;
		case 0x46: actionEnumerate(); break;
		case 0x47: actionAdd2(); break;
		case 0x48: actionLess2(); break;
		case 0x49: actionEquals2(); break;
		case 0x4a: actionToNumber(); break;
		case 0x4b: actionToString(); break;
		case 0x4c: actionPushDuplicate(); break;
		case 0x4d: actionStackSwap(); break;
		case 0x4e: actionGetMember(); break;
		case 0x4f: actionSetMember(); break;

		case 0x50: actionIncrement(); break;
		case 0x51: actionDecrement(); break;
		case 0x52:
			if (!actionCallMethod())
				return AVM1Value::undefinedVal;
			break;
		case 0x53:
			if (!actionNewMethod())
				return AVM1Value::undefinedVal;
			break;
		case 0x54: actionInstanceOf(); break;
		case 0x55: actionEnumerate2(); break;

		case 0x60: actionBitAnd(); break;
		case 0x61: actionBitOr(); break;
		case 0x62: actionBitXor(); break;
		case 0x63: actionBitLShift(); break;
		case 0x64: actionBitRShift(); break;
		case 0x65: actionBitURShift(); break;
		case 0x66: actionStrictEquals(); break;
		case 0x67: actionGreater(); break;
		case 0x68: actionStringGreater(); break;
		case 0x69: actionExtends(); break;

		case 0x81: actionGotoFrame(code, idx); break;
		case 0x83: actionGetURL(code, idx); break;
		case 0x87: actionStoreRegister(code, idx); break;
		case 0x88: actionConstantPool(code, idx); break;
		case 0x8a: actionWaitForFrame(code, idx); break;
		case 0x8b: actionSetTarget(code, idx); break;
		case 0x8c: actionGotoLabel(code, idx); break;
		case 0x8d: actionWaitForFrame2(code, idx); break;
		// `ActionDefineFunction2`.
		case 0x8e: actionDefineFunction(code, idx, length, true); break;
		case 0x8f: return actionTry(code, idx, length); break;

		case 0x94: actionWith(code, idx, length); break;
		case 0x96: actionPush(code, idx, length); break;
		case 0x99: actionJump(code, idx); break;
		case 0x9a: actionGetURL2(code, idx); break;
		case 0x9b: actionDefineFunction(code, idx, length); break;
		case 0x9d: actionIf(code, idx); break;
		case 0x9e:
			if (!actionCall())
				return AVM1Value::undefinedVal;
			break;
		case 0x9f: actionGotoFrame2(code, idx); break;

		default: actionUnknown(code, idx, length); break;
	}

	return {};
}

void AVM1Activation::stackPush(const AVM1Value& value)
{
	auto obj = value.tryAs<AVM1Object>();

	if (obj.isNull() || !obj->is<AVM1DisplayObject>())
	{
		ctx.push(value);
		return;
	}

	try
	{
		// NOTE: If the cached clip in a `MovieClipRef` is invalidated
		// (which falls back to path based resolution), it *never*
		// switches back to cache based resolution.
		ctx.push(NEW_GC_PTR(ctx.getGcCtx(), AVM1MovieClipRef
		(
			*this,
			obj->as<AVM1DisplayObject>(),
			false
		)));
	}
	catch (...)
	{
		ctx.push(value);
	}
}

void AVM1Activation::actionAdd()
{
	ctx.push
	(
		ctx.pop().toNumber(*this) +
		ctx.pop().toNumber(*this)
	);
}

void AVM1Activation::actionAdd2()
{
	// ECMA-262 2nd edition sec. 11.6.1.
	auto a = ctx.pop().toPrimitive(*this);
	auto b = ctx.pop().toPrimitive(*this);

	ctx.push([&] -> AVM1Value
	{
		if (a.is<tiny_string>() || b.is<tiny_string>())
			return a.toString(*this) + b.toString(*this);
		return a.toNumber(*this) + b.toNumber(*this);
	}());
}

void AVM1Activation::actionAnd()
{
	// ActionScript 1 logical and operator.
	auto a = ctx.pop();
	auto b = ctx.pop();

	// NOTE: Flash Player diverges from the SWF spec, and *always* returns
	// a `bool`, even in SWF 4.
	ctx.push(b.toBool(swfVersion) && a.toBool(swfVersion));
}


// Based on Rust's `char_try_from_u32()`.
static bool isValidCodePoint(uint32_t ch)
{
	return ch < 0x110000 && (ch < 0xd800 || ch >= 0xe000);
}

void AVM1Activation::actionAsciiToChar()
{
	// NOTE: In SWF 6, and later, this operates on UTF-16 code units.
	// NOTE: In SWF 5, and earlier, this *always* operates on bytes,
	// regardless of the locale encoding.
	auto ch = ctx.pop().toUInt16(*this);

	ch = swfVersion < 6 || isValidCodePoint(ch) ? ch : 0xfffd;

	tiny_string str;
	if (ch != '\0')
		str = tiny_string::fromChar(ch);
	ctx.push(str);
}

void AVM1Activation::actionCharToAscii()
{
	// SWF 4 `ord()` function.
	// NOTE: In SWF 6, and later, this operates on UTF-16 code units.
	// NOTE: In SWF 5, and earlier, this *always* operates on bytes,
	// regardless of the locale encoding.
	auto str = ctx.pop().toString(*this);

	ctx.push(number_t(str.empty() ? '\0' : str[0]));
}

void AVM1Activation::actionCloneSprite()
{
	auto depth = ctx.pop().toInt32(*this);
	auto target = ctx.pop().toString(*this);
	auto sourceClip = resolveTargetClip
	(
		getTargetOrRootClip(),
		// Source target.
		ctx.pop(),
		true
	);

	if (!sourceClip.isNull() || !sourceClip->is<MovieClip>())
	{
		LOG(LOG_ERROR, "actionCloneSprite: Source isn't a `MovieClip`");
		return;
	}

	AVM1MovieClip::cloneSprite
	(
		sourceClip->as<MovieClip>(),
		ctx,
		target,
		depth,
		nullptr
	);
}

void AVM1Activation::actionBitAnd()
{
	ctx.push(number_t
	(
		ctx.pop().toInt32(*this) &
		ctx.pop().toInt32(*this)
	));
}

void AVM1Activation::actionBitLShift()
{
	// NOTE: Only 5 bits are used for the shift count.
	auto a = ctx.pop().toInt32(*this) & 0x1f;
	auto b = ctx.pop().toInt32(*this);
	ctx.push(number_t(b << a));
}

void AVM1Activation::actionBitOr()
{
	ctx.push(number_t
	(
		ctx.pop().toInt32(*this) |
		ctx.pop().toInt32(*this)
	));
}

void AVM1Activation::actionBitRShift()
{
	// NOTE: Only 5 bits are used for the shift count.
	auto a = ctx.pop().toUInt32(*this) & 0x1f;
	auto b = ctx.pop().toInt32(*this);
	ctx.push(number_t(b << a));
}

void AVM1Activation::actionBitURShift()
{
	// NOTE: Only 5 bits are used for the shift count.
	auto a = ctx.pop().toUInt32(*this) & 0x1f;
	auto b = ctx.pop().toUInt32(*this);
	auto ret = b << a;

	// NOTE: In SWF 8-9, `ActionBitURShift` actually returns a signed
	// value.
	if (swfversion == 8 || swfversion == 9)
		ctx.push(number_t(int32_t(ret)));
	else
		ctx.push(number_t(ret));
}

void AVM1Activation::actionBitXor()
{
	ctx.push(number_t
	(
		ctx.pop().toInt32(*this) ^
		ctx.pop().toInt32(*this)
	));
}

bool AVM1Activation::actionCall()
{
	// Runs any actions/instructions on the given frame.
	auto arg = ctx.pop();
	auto target = getTargetOrRootClip();

	// The argument can be either a frame number, or a path to a
	// `MovieClip`, with a frame number.
	auto callFrame = [&] -> Optional<std::pair<GcPtr<MovieClip>, uint32_t>>>
	{
		// Frame number on the current clip.
		if (arg.is<number_t>() && target->is<MovieClip>())
			return std::make_pair(target, arg.toUint32(*this));

		// An optional path to a `MovieClip`, and frame number/label,
		// such as "/clip:framelabel".
		auto pair = resolveVariablePath(target, arg.toString(*this));
		if (!pair.hasValue() || !pair->first->is<AVM1MovieClip>())
			return {};

		auto clip = pair->first->as<MovieClip>();
		const auto& frame = pair->second;
		// TODO: Use `AVM1Value`'s `number_t` to `uint32_t` conversion,
		// once thats moved out of `AVM1Value`.
		return frame.tryToNumber<number_t>().andThen([&](uint32_t frame)
		{
			// First, try to parse it as a frame number.
			return makeOptional(std::make_pair(clip, frame));
		}).orElse([&]
		{
			auto frameNum = clip->frameLabelToNum(*this, frame);
			if (!frameNum.hasValue())
				return {};
			// Otherwise, it's a frame label.
			return std::make_pair(clip, *frameNum);
		});
	}();


	if (!callFrame.hasValue())
	{
		LOG(LOG_ERROR, "actionCall: Invalid call.");
		return baseClipExists();
	}

	const auto& clip = callFrame->first;
	const auto& frame = callFrame->second;
	if (frame > 0xffff)
		return baseClipExists();

	clip->forEachFrameAction(frame, [&](const auto& action)
	{
		runChildFrame("[Frame Call]", clip, action.getData());
	});

	return baseClipExists();
}

bool AVM1Activation::actionCallFunction()
{
	auto funcName = ctx.pop().toString(*this);
	auto numArgs = std::min<size_t>
	(
		ctx.pop().toUint32(*this),
		ctx.stackSize()
	);

	std::vector<AVM1Value> args;
	args.reserve(numArgs);

	for (size_t i = 0; i < numArgs; ++i)
	{
		auto arg = ctx.pop();
		if (arg.is<AVM1MovieClip>())
			args.push_back(arg.toObject(*this));
		else
			args.push_back(arg);
	}

	stackPush(getVariable(funcName)->callWithDefaultThis
	(
		*this,
		getTargetOrRootClip()->toAVM1Object(*this),
		funcName,
		args
	));

	// After any function call, execution of this frame stops, if the
	// base clip doesn't exist. For example, a `_root.gotoAndStop()` call
	// that moves the timeline to a frame where the clip was removed.
	return baseClipExists();
}

bool AVM1Activation::actionCallMethod()
{
	auto methodNameVal = ctx.pop();
	auto objVal = ctx.pop();
	auto numArgs = std::min<size_t>
	(
		ctx.pop().toUint32(*this),
		ctx.stackSize()
	);

	std::vector<AVM1Value> args;
	args.reserve(numArgs);

	for (size_t i = 0; i < numArgs; ++i)
	{
		auto arg = ctx.pop();
		if (arg.is<AVM1MovieClip>())
			args.push_back(arg.toObject(*this));
		else
			args.push_back(arg);
	}

	// Can't call a method on a `null`/`undefined` `this`.
	if (objVal.isNullOrUndefined())
	{
		stackPush(AVM1Value::undefinedVal);
		return false;
	}

	auto obj = objVal.toObject(*this);
	tiny_string methodName;
	if (!methodNameVal.is<UndefinedVal>())
		methodName = methodNameVal.toString(*this);

	using Reason = AVM1ExecutionReason;
	const auto& undefVal = AVM1Value::undefinedVal;
	stackPush
	(
		methodName.empty() ?
		// `undefined`/empty method name; call `this` as a function.
		obj->call(*this, "[Anonymous]", undefVal, args) :
		// Call `this[methodName]`.
		obj->callMethod(*this, methodName, args, Reason::FunctionCall)
	);

	return baseClipExists();
}

void AVM1Activation::actionCastOp()
{
	auto objVal = ctx.pop();
	auto ctor = ctx.pop().toObject(*this);

	bool instanceOf = objVal.visit(makeVisitor
	(
		[&](const GcPtr<AVM1Object>& obj)
		{
			auto proto = ctor->getProp(*this, "prototype");
			return obj->isInstanceOf(*this, ctor, proto);
		},
		[&](const GcPtr<AVM1MovieClipRef>& clip)
		{
			auto obj = clip->toObject(*this);
			auto proto = ctor->getProp(*this, "prototype");
			return obj->isInstanceOf(*this, ctor, proto);
		},
		[&](const auto&) { return false; }
	));

	ctx.push(instanceOf ? objVal : AVM1Value::nullVal);
}

void AVM1Activation::actionConstantPool
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{
	auto sys = ctx.getSys();
	auto encoding = getEncoding();
	auto poolSize = readInt<uint16_t>(code, idx);

	constPool.clear();
	constPool.reserve(poolSize);
	for (size_t i = 0; i < poolSize; ++i)
		constPool.push_back(readSwfStrID(sys, encoding, code, idx));

	ctx.setConstPool(constPool);
}

void AVM1Activation::actionDecrement()
{
	ctx.push(ctx.pop().toNumber(*this) - 1.0);
}

void AVM1Activation::actionDefineFunction
(
	const std::vector<uint8_t>& code,
	size_t& idx,
	size_t actionLen,
	bool isVer2
)
{
	bool caseSensitive = isCaseSensitive();
	auto name = readStr(code, idx);
	auto numArgs = readInt<uint16_t>(code, idx);
	auto regCount = isVer2 ? code[idx++] : 0;
	AVM1FuncFlags flags = isVer2 ? readInt<AVM1FuncFlags>(code, idx) : 0;

	std::vector<AVM1FuncArg> args;
	args.reserve(numArgs);

	auto sys = ctx.getSys();
	for (size_t i = 0; i < numArgs; ++i)
	{
		args.emplace_back
		(
			// `reg`.
			isVer2 ? code[idx++] : 0,
			// `name`.
			readStrID(sys, code, idx, caseSensitive)
		);
	}

	// NOTE: `codeLength` isn't included in `ActionDefineFunction{,2}`'s
	// action length.
	auto codeLength = readInt<uint16_t>(code, idx);
	actionLen += codeLength;

	auto& gcCtx = ctx.getGcCtx();
	auto funcCodeIt = std::next(code.begin(), idx);
	auto func = NEW_GC_PTR(gcCtx, AVM1Function
	(
		gcCtx,
		swfVersion,
		// TODO: Replace this with a span, once we write our own span
		// implementation.
		std::vector<uint8_t>(funcCodeIt, funcCodeIt + codeLength),
		sys->getUniqueStringId(name, caseSensitive),
		regCount,
		args,
		scope,
		constPool,
		// NOTE: `baseClip` should always be a `MovieClip`, which means
		// it can't throw.
		NEW_GC_PTR(gcCtx, AVM1MovieClipRef
		(
			*this,
			baseClip->toAVM1Object(*this)
		)),
		flags
	));

	auto funcObj = NEW_GC_PTR(gcCtx, AVM1FunctionObject
	(
		*this,
		func,
		ctx.getPrototypes().function->proto,
		NEW_GC_PTR(gcCtx, AVM1Object
		(
			gcCtx,
			ctx.getPrototypes().object->proto
		))
	));

	if (!name.empty())
		defineLocal(name, funcObj);
	else
		ctx.push(funcObj);
}

void AVM1Activation::actionDefineLocal()
{
	// NOTE: If the property doesn't exist on the local object's
	// prototype chain, then it's created on the local object.
	// Otherwise, the property is set (including calling virtual
	// setters).
	// Despite not being in the SWF 19 spec, `.` paths, and `/` paths
	// are also supported, and affect the related object in the same way
	// as `ActionSetVariable`.
	auto val = ctx.pop();
	auto name = ctx.pop().toString(*this);
	defineLocal(name, val);
}

void AVM1Activation::actionDefineLocal2()
{
	// NOTE: If the property doesn't exist on the local object's
	// prototype chain, then it's created on the local object.
	// Otherwise, the property is left unchanged.
	// Despite not being in the SWF 19 spec, `.` paths, and `/` paths
	// are also supported, and affect the related object in the same way
	// as `ActionSetVariable`, if the variable doesn't already exist on
	// the mentioned object.
	auto name = ctx.pop().toString(*this);
	if (inLocalScope() || name.findFirst(":.") == tiny_string::npos)
	{
		scope->defineLocal(*this, name, AVM1Value::undefinedVal);
		return;
	}

	auto var = getVariable(name);
	if (!var->isCallable() && var->getValue().is<UndefinedVal>())
		setVariable(name, AVM1Value::undefinedVal);
}

void AVM1Activation::actionDelete()
{
	auto name = ctx.pop().toString(*this);
	ctx.push(ctx.pop().visit(makeVisitor
	(
		[&](const GcPtr<AVM1Object>& obj)
		{
			return obj->deleteProp(*this, name);
		},
		[&](const GcPtr<AVM1MovieClipRef>& clip)
		{
			auto obj = clip->toObject(*this);
			return obj->deleteProp(*this, name);
		},
		[&](const auto& val)
		{
			LOG
			(
				LOG_ERROR,
				"actionDelete: Can't delete property " <<
				name << " from " << val
			);
			return false;
		}
	));
}

void AVM1Activation::actionDelete2()
{
	auto name = ctx.pop().toString(*this);

	// NOTE: This isn't in the SWF 19 spec, but this returns a `bool`,
	// based on whether the variable was actually deleted, or not.
	ctx.push(scope->deleteVar(*this, name));
}

void AVM1Activation::actionDivide()
{
	// 1. Pops value A off the stack.
	auto a = ctx.pop().toNumber(*this);

	// 2. Pops value B off the stack.
	// 3. Converts A, and B to floating-point; non-numeric values evaluate
	// to 0.
	auto b = ctx.pop().toNumber(*this);

	// 6. If A is zero, the result NaN, Infinity, or -Infinity is pushed
	// to the stack, in SWF 5 and later. In SWF 4, the result is the
	// string #ERROR#.
	if (a == 0.0 && swfVersion < 5)
	{
		ctx.push("#ERROR#");
		return;
	}

	// 4. Divides B, by A.
	// 5. Pushes the result, B/A, to the stack.
	ctx.push(b / a);
}

void AVM1Activation::actionEndDrag()
{
	ctx.getSys()->getInputThread()->stopDrag();
}

void AVM1Activation::actionEnumerate()
{
	auto name = ctx.pop().toString(*this);

	// A sentinel value that indicates the end of enumeration.
	ctx.push(AVM1Value::undefinedVal);

	ctx.pop().visit(makeVisitor
	(
		[&](const GcPtr<AVM1Object>& obj)
		{
			auto keys = obj->getKeys(*this, false);
			for (auto it = keys.rbegin(); it != keys.rend(); ++it)
				stackPush(*it);
		},
		[&](const GcPtr<AVM1MovieClipRef>& clip)
		{
			auto obj = clip->toObject(*this);
			auto keys = obj->getKeys(*this, false);
			for (auto it = keys.rbegin(); it != keys.rend(); ++it)
				stackPush(*it);
		},
		[&](const auto&)
		{
			LOG(LOG_ERROR, "actionEnumerate: Can't enumerate properties of " << name);
		}
	));
}

void AVM1Activation::actionEnumerate2()
{
	// A sentinel value that indicates the end of enumeration.
	ctx.push(AVM1Value::undefinedVal);

	ctx.pop().visit(makeVisitor
	(
		[&](const GcPtr<AVM1Object>& obj)
		{
			auto keys = obj->getKeys(*this, false);
			for (auto it = keys.rbegin(); it != keys.rend(); ++it)
				stackPush(*it);
		},
		[&](const GcPtr<AVM1MovieClipRef>& clip)
		{
			auto obj = clip->toObject(*this);
			auto keys = obj->getKeys(*this, false);
			for (auto it = keys.rbegin(); it != keys.rend(); ++it)
				stackPush(*it);
		},
		[&](const auto& val)
		{
			LOG(LOG_ERROR, "actionEnumerate2: Can't enumerate " << val);
		}
	));
}

void AVM1Activation::actionEquals()
{
	// ActionScript 1 equality operator.
	// NOTE: If both values are converted to `NaN`, then it'll always
	// return `false`. Which differs from `ActionEquals2`'s behaviour.
	auto a = ctx.pop().toNumber(*this);
	auto b = ctx.pop().toNumber(*this);

	// NOTE: Flash Player diverges from the SWF spec, and *always* returns
	// a `bool`, even in SWF 4.
	ctx.push(b == a);
}

void AVM1Activation::actionEquals2()
{
	// SWF 5, and later equality operator.
	auto a = ctx.pop();
	auto b = ctx.pop();
	ctx.push(b.isEqual(a, *this));
}

void AVM1Activation::actionExtends()
{
	auto superClass = ctx.pop().toObject(*this);
	auto subClass = ctx.pop().toObject(*this);

	// TODO: What happens if we try to extend an `Object` that has no
	// `prototype`? e.g. `class Foo extends Object.prototype`, or
	// `class Foo extends 5`.
	auto superProto = superClass.getProp(*this "prototype").toObject(*this);
	auto subProto = NEW_GC_PTR(ctx.getGcCtx(), AVM1Object
	(
		ctx.getGcCtx(),
		superProto
	));

	subProto.defineValue
	(
		"constructor",
		superClass,
		AVM1PropFlags::DontEnum
	);

	subProto.defineValue
	(
		"__constructor__",
		superClass,
		AVM1PropFlags::DontEnum
	);

	subClass.setProp(*this, "prototype", subProto);
}

void AVM1Activation::actionGetMember()
{
	auto name = ctx.pop().toString(*this);
	auto obj = ctx.pop().toObject(*this);

	stackPush(obj->getNonSlashPathProp(*this, name));
}

void AVM1Activation::actionGetProperty()
{
	auto propIdx = ctx.pop().toNumber(*this);
	auto path = ctx.pop();
	auto clip = resolveTargetClip
	(
		getTargetOrBaseClip(),
		path,
		!targetClip.isNull()
	);

	if (clip.isNull())
	{
		LOG(LOG_ERROR, "actionGetProperty: Invalid target " << path);
		ctx.push(AVM1Value::undefinedVal);
		return;
	}

	if (!std::isfinite(propIdx) || propIdx <= -1.0)
	{
		LOG(LOG_ERROR, "actionGetProperty: Invalid property " << propIdx);
		ctx.push(AVM1Value::undefinedVal);
		return;
	}

	auto prop = *ctx.getDisplayProps().getByIndex(propIdx);
	ctx.push(prop.get(*this, clip));
}

void AVM1Activation::actionGetTime()
{
	auto sys = ctx.getSys();
	uint64_t now = sys->now().absDiff(ctx.startTime).toMs();
	ctx.push(number_t(now));
}

void AVM1Activation::actionGetVariable()
{
	auto varPath = ctx.pop().toString(*this);
	stackPush(getVariable(varPath));
}

void AVM1Activation::actionGetURL
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{
	auto sys = ctx.getSys();
	auto url = readSwfStr(sys, getEncoding(), code, idx);
	auto target = readSwfStr(sys, getEncoding(), code, idx);

	bool isLevelName =
	(
		target.startsWith("_level") &&
		target.numChars() > 6
	);

	if (!isLevelName)
	{
		(void)parseFSCommand(url).andThen([&](const tiny_string& cmd)
		{
			sys->extIfaceManager->handleFSCommand(cmd, target);
			return makeOptional(str);
		}).orElse([&]
		{
			sys->openPageInBrowser(url, target);
			return nullOpt;
		});
		return;
	}

	auto levelID = target.substr
	(
		6,
		tiny_string::npos
	).tryParseNumber<int32_t>();

	if (!levelID.hasValue())
	{
		LOG(LOG_ERROR, "actionGetURL: Couldn't parse level ID " << target);
		return;
	}

	if (!url.empty())
	{
		sys->loaderManager->loadMovieIntoClip
		(
			getOrCreateLevel(levelID),
			Request(RequestMethod::Get, url),
			{},
			AVM1MovieLoaderData()
		);
		return;
	}

	auto level = getLevel(levelID);
	if (level.isNull() || !level->is<MovieClip>())
		return;

	// NOTE: A blank URL on a level target is an unload.
	level->as<MovieClip>()->AVM1unloadMovie(ctx);
}

static Optional<RequestMethod> fromSendVars(const AVM1GetURLFlags& flags)
{
	using URLFlags = AVM1GetURLFlags;
	switch (flags & URLFlags::MethodMask)
	{
		default:
		case URLFlags::MethodNone: return {}; break;
		case URLFlags::MethodGet: return RequestMethod::Get; break;
		case URLFlags::MethodPost: return RequestMethod::Post; break;
	}
}

void AVM1Activation::actionGetURL2
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{
	using URLFlags = AVM1GetURLFlags;
	URLFlags flags = code[idx++];
	// TODO: Support `LoadVariablesFlag`.
	// TODO: What happens if there's only a single string?
	auto sys = ctx.getSys();
	auto targetVal = ctx.pop();
	auto target = targetVal.toString(*this);
	auto url = ctx.pop().toString(*this);

	auto fsCmd = parseFSCommand(url);

	if (fsCmd.hasValue())
	{
		// `target` is an fscommand.
		sys->extIfaceManager->handleFSCommand(*fsCmd, target);
		return;
	}

	auto levelID = [&] -> int32_t
	{
		bool isLevelName =
		(
			target.startsWith("_level") &&
			target.numChars() >= 6
		);

		if (!isLevelName)
			return -1;

		return target.substr
		(
			6,
			tiny_string::npos
		).tryParseNumber<number_t>().valueOr
		(
			-(target.numChars() > 6)
		);
	}();

	NullableGcPtr<DisplayObject> clipTarget = [&]
	{
		if (levelID < 0)
			return nullptr;
		if (!(flags & (URLFlags::LoadTarget | URLFlags::LoadVariables)))
			return nullptr;

		return targetVal.isPrimitive() ? resolveTargetClip
		(
			getTargetOrRootClip(),
			targetVal,
			true
		) : targetVal.as<AVM1Object>()->as<DisplayObject>();
	}();

	auto getURL = [&]
	{
		// `getURL()` call.
		auto vars = fromSendVars(flags).andThen([&](const auto& method)
		{
			return makeOptional(std::make_pair
			(
				method,
				localsToFormVals()
			));
		});
		sys->navigateToURL(url, target, vars);
	};

	bool isLoadVars = flags & URLFlags::LoadVariables;
	bool isLoadTarget = flags & URLFlags::LoadTarget;

	if (isLoadVars)
	{
		// `loadVariables{,Num}()` call.
		// Depending on the situation, it'll open a link in the browser
		// instead.
		bool _isLoadVars = isLoadTarget || levelID >= 0 ||
		(
			!targetVal.isPrimitive() &&
			clipTarget == baseClip->getAVM1Root()
		);
		if (!_loadVars)
		{
			getURL();
			return;
		}

		if (clipTarget.isNull())
			return;

		sys->loaderManager->loadFormIntoAVM1Object
		(
			clipTarget->toAVM1Object(*this),
			localsToRequest(url, fromSendVars(flags))
		);
		return;
	}

	if (!isLoadTarget && levelID < 0)
	{
		getURL();
		return;
	}

	// `{,un}loadMovie{,Num}()` call.
	if (!url.empty() && levelID < 0)
		return;

	auto request =
	(
		isLoadTarget ?
		localsToRequest(url, fromSendVars(flags)) :
		Request(RequestMethod::Get, url)
	);

	if (!url.empty() && clipTarget.isNull())
	{
		// NOTE: The level target is created while loading the movie.
		sys->loaderManager->loadMovie
		(
			target,
			request,
			{},
			AVM1MovieLoaderData()
		);
		return;
	}
	else if (!url.empty())
	{
		sys->loaderManager->loadMovieIntoClip
		(
			clipTarget,
			request,
			{},
			AVM1MovieLoaderData()
		);
		return;
	}

	if (clipTarget.isNull() || !clipTarget->is<MovieClip>())
		return;

	// NOTE: A blank URL on a level target is an unload.
	clipTarget->as<MovieClip>()->AVM1unloadMovie(ctx);
}

void AVM1Activation::actionGotoFrame
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{
	auto frame = readInt<uint16_t>(code, idx);

	if (targetClip.isNull())
	{
		LOG(LOG_ERROR, "ActionGotoFrame failed. Reason: Invalid target.");
		return;
	}

	auto clip = targetClip->as<MovieClip>();
	if (clip.isNull())
	{
		LOG(LOG_ERROR, "ActionGotoFrame failed. Reason: Target isn't a `MovieClip`.");
		return;
	}

	clip->AVM1gotoFrame(frame, true, !clip->state.stop_FP, true);
}

void AVM1Activation::actionGotoFrame2
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{
	// SWF 4, and later `gotoAnd{Play,Stop}()`.
	// The argument can be either a frame number, or a frame label.
	auto flags = code[idx++];

	auto clip = getTargetOrRootClip()->as<MovieClip>();
	if (clip.isNull())
	{
		LOG(LOG_ERROR, "actionGotoFrame2: Target isn't a `MovieClip`.");
		return;
	}

	bool isPlaying = flags & 1;
	uint16_t sceneOffset = flags & 2 ? readInt<uint16_t>(code, idx) : 0;
	auto frame = ctx.pop();
	(void)AVM1MovieClip::gotoFrame
	(
		*this,
		clip,
		frame,
		!isPlaying,
		sceneOffset
	);
}

void AVM1Activation::actionGotoLabel
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{
	auto sys = ctx.getSys();
	auto label = readSwfStr(sys, getEncoding(), code, idx);

	if (targetClip.isNull())
	{
		LOG(LOG_ERROR, "actionGotoLabel: Invalid target.");
		return;
	}

	auto clip = targetClip->as<MovieClip>();
	if (clip.isNull())
	{
		LOG(LOG_ERROR, "actionGotoLabel: Target isn't a `MovieClip`.");
		return;
	}

	clip->AVM1gotoFrameLabel(label, true, true);
}

void AVM1Activation::actionIf(const std::vector<uint8_t>& code, size_t& idx)
{
	auto offset = readInt<int16_t>(code, idx);
	if (ctx.pop().toBool(swfVersion))
		idx = clampTmpl<size_t>(idx + offset, 0, code.size());
}

void AVM1Activation::actionIncrement()
{
	ctx.push(ctx.pop().toNumber(*this) + 1.0);
}

void AVM1Activation::actionInitArray()
{
	auto numArgs = ctx.pop().toNumber(*this);

	if (numArgs < 0 || numArgs > INT32_MAX)
	{
		// NOTE: `ActionInitArray` pops no args, and pushes `undefined`,
		// if `numArgs` is out of range.
		ctx.push(AVM1Value::undefinedVal);
		return;
	}

	size_t _numArgs = numArgs;

	std::vector<AVM1Value> args;
	args.reserve(_numArgs);

	for (size_t i = 0; i < _numArgs; ++i)
		args.push_back(ctx.pop());

	ctx.push(NEW_GC_PTR(ctx.getGcCtx(), AVM1Array(*this, args)));
}

void AVM1Activation::actionInitObject()
{
	auto numProps = ctx.pop().toNumber(*this);

	if (numProps < 0 || numProps > INT32_MAX)
	{
		// NOTE: `ActionInitObject` pops no args, and pushes `undefined`,
		// if `numProps` is out of range.
		ctx.push(AVM1Value::undefinedVal);
		return;
	}

	auto obj = NEW_GC_PTR(ctx.getGcCtx(), AVM1Object
	(
		ctx.getGcCtx(),
		ctx.getPrototypes().object->proto
	));

	size_t _numProps = numProps;
	for (size_t i = 0; i < _numProps; ++i)
	{
		auto value = ctx.pop();
		auto name = ctx.pop().toString(*this);
		obj->setProp(*this, name, value);
	}

	ctx.push(obj);
}

void AVM1Activation::actionImplementsOp()
{
	auto ctor = ctx.pop().toObject(*this);
	// NOTE, PLAYER-SPECIFIC: In older versions of Flash Player (at least
	// FP 9), `count` is *always* converted to a `Number`, even if it's
	// an `Object`. But at some point, this was changed, and instead
	// prints the following in the trace log: "Parameters of type Object
	// are no longer coerced into the required primitive type - number.".
	// Newer versions of Flash Player only convert primitives, and treat
	// `Object`s as `0`.
	size_t count = [&]
	{
		auto count = ctx.pop();
		if (count.isPrimitive())
			return count.toUInt32(*this);
		LOG(LOG_ERROR, "actionImplementsOp: Can't convert non-primitives to `Number`. Returning 0 instead.");
		return 0;
	}();

	count = std::min(count, ctx.stackSize());

	// Bail, if there aren't any interfaces.
	if (!count)
		return;

	std::vector<GcPtr<AVM1Object>> ifaces;
	// TODO: If one of the interfaces isn't an `Object`, do we leave the
	// entire stack dirty, or do we clean it up?
	for (size_t i = 0; i < count; ++i)
		ifaces.push_back(ctx.pop().toObject(*this));

	auto proto = ctor.getProp(*this, "prototype").toObject(*this);
	proto->setInterfaces(ifaces);
}

void AVM1Activation::actionInstanceOf()
{
	auto ctor = ctx.pop().toObject(*this);
	auto obj = ctx.pop();

	ctx.push(obj.visit(makeVisitor
	(
		[&](const GcPtr<AVM1Object>& obj)
		{
			auto proto = ctor->getProp(*this, "prototype");
			return obj->isInstanceOf(*this, ctor, proto);
		},
		[&](const GcPtr<AVM1MovieClipRef>& clip)
		{
			auto obj = clip->toObject(*this);
			auto proto = ctor->getProp(*this, "prototype");
			return obj->isInstanceOf(*this, ctor, proto);
		},
		[&](const auto&) { return false; }
	)));
}

void AVM1Activation::actionJump(const std::vector<uint8_t>& code, size_t& idx)
{
	auto offset = readInt<int16_t>(code, idx);
	idx = clampTmpl<size_t>(idx + offset, 0, code.size());
}

void AVM1Activation::actionLess()
{
	// ActionScript 1 less than operator.
	// NOTE: If both values are converted to `NaN`, then it'll always
	// return `false`. Which differs from `ActionLess2`'s behaviour.
	auto a = ctx.pop().toNumber(*this);
	auto b = ctx.pop().toNumber(*this);

	// NOTE: Flash Player diverges from the SWF spec, and *always* returns
	// a `bool`, even in SWF 4.
	ctx.push(b < a);
}

void AVM1Activation::actionLess2()
{
	// ECMA-262 2nd edition sec. 11.8.1.
	auto a = ctx.pop();
	auto b = ctx.pop();

	ctx.push(b.isLess(a, *this));
}

void AVM1Activation::actionGreater()
{
	// ECMA-262 2nd edition sec. 11.8.2.
	auto a = ctx.pop();
	auto b = ctx.pop();

	ctx.push(a.isLess(b, *this));
}

void AVM1Activation::actionMBAsciiToChar()
{
	// NOTE: In SWF 6, and later, this operates on UTF-16 code units.
	// TODO: In SWF 5, and earlier, this operates on locale dependent
	// characters.
	uint32_t ch = ctx.pop().toUInt16(*this);

	tiny_string str;
	if (ch != '\0')
	{
		// NOTE: Unpaired surrogates get turned into a replacement
		// character.
		str = tiny_string::fromChar(isValidCodePoint(ch) ? ch : 0xfffd);
	}
	ctx.push(str);
}

void AVM1Activation::actionMBCharToAscii()
{
	// SWF 4 `mbord()` function.
	// NOTE: In SWF 6, and later, this operates on UTF-16 code units.
	// NOTE: In SWF 5, and earlier, this operates on locale dependent
	// characters.
	auto str = ctx.pop().toString(*this);

	ctx.push(number_t(str.empty() ? '\0' : str[0]));
}

void AVM1Activation::actionMBStringExtract()
{
	// SWF 4 `mbsubstring()` function.
	// NOTE: In SWF 6, and later, this operates on UTF-16 code units.
	// NOTE: In SWF 5, and earlier, this operates on locale dependent
	// characters.
	size_t len = ctx.pop().toInt32(*this);

	// NOTE: The start index is 1 based.
	size_t start = std::max<int32_t>(ctx.pop().toInt32(*this) - 1, 0);
	auto str = ctx.pop().toString(*this);

	if (start <= str.numChars())
		ctx.push(str.substr(start, len))
	else
		ctx.push("");
}

void AVM1Activation::actionMBStringLength()
{
	// SWF 4 `mblength()` function.
	ctx.push(number_t(ctx.pop().toString(*this).numChars()));
}

void AVM1Activation::actionMultiply()
{
	ctx.push
	(
		ctx.pop().toNumber(*this) *
		ctx.pop().toNumber(*this)
	);
}

void AVM1Activation::actionModulo()
{
	ctx.push
	(
		ctx.pop().toNumber(*this) %
		ctx.pop().toNumber(*this)
	);
}

void AVM1Activation::actionNot()
{
	// NOTE: Flash Player diverges from the SWF spec, and *always* returns
	// a `bool`, even in SWF 4.
	ctx.push(!ctx.pop().toBool(swfVersion));
}

void AVM1Activation::actionNextFrame()
{
	if (targetClip.isNull())
	{
		LOG(LOG_ERROR, "actionNextFrame: Invalid target.");
		return;
	}

	auto clip = targetClip->as<MovieClip>();
	if (clip.isNull())
	{
		LOG(LOG_ERROR, "actionNextFrame: Target isn't a `MovieClip`.");
		return;
	}

	clip->AVM1gotoFrame(clip->state.FP + 1, true, true);
}

bool AVM1Activation::actionNewMethod()
{
	auto methodNameVal = ctx.pop();
	auto objVal = ctx.pop();
	auto numArgs = std::min<size_t>
	(
		ctx.pop().toUInt32(*this),
		ctx.stackSize()
	);

	std::vector<AVM1Value> args;
	args.reserve(numArgs);

	for (size_t i = 0; i < numArgs; ++i)
	{
		auto arg = ctx.pop();
		if (arg.is<AVM1MovieClip>())
			args.push_back(arg.toObject(*this));
		else
			args.push_back(arg);
	}

	// Can't call a method on a `null`/`undefined` `this`.
	if (objVal.isNullOrUndefined())
	{
		ctx.push(AVM1Value::undefinedVal);
		return false;
	}

	auto obj = objVal.toObject(*this);
	tiny_string methodName;
	if (!methodNameVal.is<UndefinedVal>())
		methodName = methodNameVal.toString(*this);

	stackPush
	(
		methodName.empty() ?
		// `undefined`/empty method name; construct `this` as a function.
		obj->construct(*this, args) :
		obj->getProp(*this, methodName).visit(makeVisitor
		(
			[&](const GcPtr<AVM1Object>& ctor)
			{
				// Construct `this[methodName]`.
				return ctor->construct(*this, args);
			},
			[&](const auto& ctor)
			{
				LOG
				(
					LOG_ERROR,
					"actionNewMethod: Tried to construct "
					"with a non `Object` constructor " << ctor
				);
				return AVM1Value::undefinedVal;
			}
		))
	);

	return baseClipExists();
}

bool AVM1Activation::actionNewObject()
{
	auto funcName = ctx.pop().toString(*this);
	auto numArgs = std::min<size_t>
	(
		ctx.pop().toUInt32(*this),
		ctx.stackSize()
	);

	std::vector<AVM1Value> args;
	args.reserve(numArgs);

	for (size_t i = 0; i < numArgs; ++i)
	{
		auto arg = ctx.pop();
		if (arg.is<AVM1MovieClip>())
			args.push_back(arg.toObject(*this));
		else
			args.push_back(arg);
	}

	auto ctor = resolveVariable(funcName)->getValue().toObject(*this);
	stackPush(ctor->construct(*this, args));
	return baseClipExists();
}

void AVM1Activation::actionOr()
{
	// ActionScript 1 logical or operator.
	auto a = ctx.pop();
	auto b = ctx.pop();

	// NOTE: Flash Player diverges from the SWF spec, and *always* returns
	// a `bool`, even in SWF 4.
	ctx.push(b.toBool(swfVersion) || a.toBool(swfVersion));
}

void AVM1Activation::actionPlay()
{
	if (targetClip.isNull())
	{
		LOG(LOG_ERROR, "actionPlay: Invalid target.");
		return;
	}

	auto clip = targetClip->as<MovieClip>();
	if (clip.isNull())
	{
		LOG(LOG_ERROR, "actionPlay: Target isn't a `MovieClip`.");
		return;
	}

	clip->setPlaying();
}

void AVM1Activation::actionPreviousFrame()
{
	if (targetClip.isNull())
	{
		LOG(LOG_ERROR, "actionPreviousFrame: Invalid target.");
		return;
	}

	auto clip = targetClip->as<MovieClip>();
	if (clip.isNull())
	{
		LOG(LOG_ERROR, "actionPreviousFrame: Target isn't a `MovieClip`.");
		return;
	}

	clip->AVM1gotoFrame(clip->state.FP - 1, true, true);
}

void AVM1Activation::actionPop()
{
	(void)ctx.pop();
}

void AVM1Activation::actionPush
(
	const std::vector<uint8_t>& code,
	size_t& idx,
	size_t actionLen
)
{
	auto sys = ctx.getSys();
	auto enc = getEncoding();

	auto getConst = [&](uint16_t i) -> AVM1Value
	{
		if (i < constPool.size())
		{
			#ifdef USE_STRING_ID
			return constPool[i];
			#else
			return sys->getStringFromUniqueId(constPool[i], true);
			#endif
		}

		LOG
		(
			LOG_ERROR,
			"actionPush: Constant pool index " << i << " is "
			"out of range. Pool size: " << constPool.size()
		);
		return AVM1Value::undefinedVal;
	};

	size_t end = idx + actionLen;
	while (idx < end)
	{
		auto type = code[idx++];
		switch (type)
		{
			// `String`.
			case 0: ctx.push(readSwfStr(sys, enc, code, idx)); break;
			// `Float`.
			case 1: ctx.push(readFloat<float>(code, idx)); break;
			// `Null`.
			case 2: ctx.push(AVM1Value::nullVal); break;
			// `Undefined`.
			case 3: ctx.push(AVM1Value::undefinedVal); break;
			// `RegisterNumber`.
			// NOTE: `stackPush()` is used here to handle `MovieClipRef`s.
			case 4: stackPush(code[idx++]); break;
			// `Boolean`.
			case 5: ctx.push(bool(code[idx++])); break;
			// `Double`.
			case 6: ctx.push(readFloat<double>(code, idx)); break;
			// `Integer`.
			case 7: ctx.push(number_t(readInt<int32_t>(code, idx))); break;
			// `Constant8`.
			case 8: ctx.push(getConst(code[idx++])); break;
			// `Constant16`.
			case 9: ctx.push(getConst(readInt<uint16_t>(code, idx))); break;
			default:
			{
				// NOTE, PLAYER-SPECIFIC: Modern versions of Flash Player
				// exit on unknown value types. But in older versions of
				// Flash Player (at least FP 9), unknown value types are
				// ignored. We follow the lenient behaviour, due to some
				// SWFs relying on this behaviour.
				// TODO: Throw an exception, once support for mimicking
				// different player versions is added.
				LOG(LOG_ERROR, "actionPush: Invalid value type: " << type);
				break;
			}
		}
	}
}

void AVM1Activation::actionPushDuplicate()
{
	auto val = ctx.pop();
	ctx.push(val);
	ctx.push(val);
}

void AVM1Activation::actionRandomNumber()
{
	auto sys = ctx.getSys();
	// NOTE: The max value is clamped to the range `0 - INT32_MAX`.
	int32_t max = ctx.pop().toNumber(*this);
	ctx.push(number_t(max > 0 ? sys->randomFlashCompatible(max) : 0));
}

void AVM1Activation::actionRemoveSprite()
{
	auto target = ctx.pop();
	auto _targetClip = resolveTargetClip(getTargetOrRootClip(), target);
	if (_targetClip.isNull())
	{
		LOG(LOG_ERROR, "actionRemoveSprite: Source isn't a `DisplayObject`.");
		return;
	}

	AVM1DisplayObject::removeDisplayObject(*this, _targetClip);
	if (targetClip.isNull() || !targetClip->isAVM1Removed())
		return;

	// Revert the target to either the base clip, or `NullGc`, if the
	// base was also removed.
	setTargetClip(baseClip);
	scope->setTargetScope(getTargetOrRootClip()->toAVM1DisplayObject(*this));
}

AVM1Value AVM1Activation::actionReturn()
{
	return ctx.pop();
}

void AVM1Activation::actionSetMember()
{
	auto value = ctx.pop();
	auto name = ctx.pop().toString(*this);
	auto obj = ctx.pop().toObject(*this);
	obj->setProp(*this, name, value);
}

void AVM1Activation::actionSetProperty()
{
	auto propVal = ctx.pop();
	auto propIdx = ctx.pop().toNumber(*this);
	auto path = ctx.pop();
	auto clip = resolveTargetClip
	(
		getTargetOrBaseClip(),
		path,
		!targetClip.isNull()
	);

	const auto& dispProps = ctx.getDisplayProps();
	bool isValidProp =
	(
		std::isfinite(propIdx) &&
		propIdx >= 0.0 &&
		dispProps.isValidIndex(propIdx)
	);

	if (!isValidProp)
	{
		LOG(LOG_ERROR, "actionSetProperty: Invalid property " << propIdx);
		ctx.push(AVM1Value::undefinedVal);
		return;
	}

	auto prop = *dispProps.getByIndex(propIdx);
	if (clip.isNull() || prop.isReadOnly())
	{
		// NOTE: `propVal` needs to be converted even if the target isn't
		// valid, or the property is read-only. This behaviour has been
		// consistent since Flash Player 21. Earlier versions will usually
		// only do conversions when the input is valid, but in Flash Player
		// 19, and 20, conversions never happen, *at all*!!!
		switch (propIdx)
		{
			// `x`.
			case 0:
			// `y`.
			case 1:
			// `xscale`.
			case 2:
			// `yscale`.
			case 3:
			// `currentframe`.
			case 4:
			// `totalframes`.
			case 5:
			// `alpha`.
			case 6:
			// `visible`.
			case 7:
			// `width`.
			case 8:
			// `height`.
			case 9:
			// `rotation`.
			case 10:
			// `framesloaded`.
			case 12:
			{
				if (propVal.isNullOrUndefined())
					break;
				// Falls through.
			}
			/// `highquality`.
			case 16:
			/// `soundbuftime`.
			case 18:
			/// `xmouse`.
			case 20:
			/// `ymouse`.
			case 21:
			{
				// Convert to a `Number`.
				(void)propVal.toNumber(*this);
				break;
			}
			// `name`.
			case 13:
			// `quality`.
			case 19:
			{
				// Convert to a `String`.
				(void)propVal.toString(*this);
				break;
			}
			default:
				// No conversion.
				break;
		}
	}

	if (clip.isNull())
	{
		LOG(LOG_ERROR, "actionSetProperty: Invalid target " << path);
		ctx.push(AVM1Value::undefinedVal);
		return;
	}

	prop.set(*this, clip, propVal);
}

void AVM1Activation::actionSetVariable()
{
	// SWF 4 style variable.
	auto value = ctx.pop();
	auto varPath = ctx.pop().toString(*this);
	setVariable(varPath, value);
}

void AVM1Activation::actionStrictEquals()
{
	// This is the same as the normal equality operator, except the types
	// must match.
	ctx.push(ctx.pop() == ctx.pop());
}

void AVM1Activation::actionSetTarget
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{
	auto sys = ctx.getSys();
	auto target = readSwfStr(sys, getEncoding(), code, idx);
	setTarget(target);
}

void AVM1Activation::actionSetTarget2()
{
	auto target = ctx.pop();
	if (baseClip->isAVM1Removed())
	{
		setTargetClip(NullGc);
		return;
	}

	bool done = target.visit(makeVisitor
	(
		#ifdef USE_STRING_ID
		[&](uint32_t _target)
		#else
		[&](const tiny_string& _target)
		#endif
		{
			setTarget(_target);
			return true;
		},
		[&](const UndefinedVal&)
		{
			// NOTE: In SWF 6, and earlier, an `undefined` target will
			// cause `ActionSetTarget2` to set the target back to the
			// base clip.
			setTargetClip(swfVersion > 6 ? NullGc : baseClip);
			return false;
		},
		[&](const GcPtr<AVM1Object>& obj)
		{
			auto clip = obj->as<DisplayObject>();
			if (!clip.isNull())
			{
				// `MovieClip`s are targeted directly.
				setTargetClip(clip);
				return false;
			}
			// Other `Object`s are converted to a `String`.
			setTarget(target.toString(*this));
			return true;
		},
		[&](const GcPtr<AVM1MovieClipRef>&)
		{
			auto clip = target.toObject(*this)->as<DisplayObject>();
			if (!clip.isNull())
			{
				// `MovieClip`s can be targeted directly.
				setTargetClip(clip);
				return false;
			}
			// Other `Object`s are converted to a `String`.
			setTarget(target.toString(*this));
			return true;
		},
		[&](const auto&)
		{
			setTarget(target.toString(*this));
			return true;
		}
	));

	if (done)
		return;

	scope->setTargetScope(getTargetOrBaseClip()->toAVM1DisplayObject(*this));
}

void AVM1Activation::actionStackSwap()
{
	auto a = ctx.pop();
	auto b = ctx.pop();
	ctx.push(a);
	ctx.push(b);
}

void AVM1Activation::actionStartDrag()
{
	auto clip = resolveTargetClip(getTargetOrRootClip(), ctx.pop());
	bool lockCenter = ctx.pop().toInt32(*this);
	bool hasConstrain = ctx.pop().toInt32(*this);
	Optional<RectF> constrain;
	if (hasConstrain)
	{
		Vector2f max;
		Vector2f min;
		max.y = ctx.pop();
		max.x = ctx.pop();
		min.y = ctx.pop();
		min.x = ctx.pop();
		constrain = RectF { min, max };
	}

	if (targetClip.isNull())
	{
		LOG(LOG_ERROR, "actionStartDrag: Invalid target.");
		return;
	}

	AVM1MovieClip::startDragImpl(*this, clip, lockCenter, constrain);
}

void AVM1Activation::actionStop()
{
	if (targetClip.isNull())
	{
		LOG(LOG_ERROR, "actionGotoLabel: Invalid target.");
		return;
	}

	auto clip = targetClip->as<MovieClip>();
	if (clip.isNull())
	{
		LOG(LOG_ERROR, "actionGotoLabel: Target isn't a `MovieClip`.");
		return;
	}

	clip->setStopped();
}

void AVM1Activation::actionStopSounds()
{
	ctx.getSys()->audioManager->stopAllSounds();
}

void AVM1Activation::actionStoreRegister
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{
	// NOTE: The value *must* remain on the stack.
	auto value = ctx.pop();
	ctx.push(value);
	setCurrentReg(code[idx++], value);
}

void AVM1Activation::actionStringAdd()
{
	// SWF 4 `add` operator.
	// TODO: What does this return, when given non `String` operands.
	auto a = ctx.pop().toString(*this);
	auto b = ctx.pop().toString(*this);

	ctx.push(b + a);
}

void AVM1Activation::actionStringEquals()
{
	// SWF 4 `eq` operator.
	auto a = ctx.pop().toString(*this);
	auto b = ctx.pop().toString(*this);

	// NOTE: Flash Player diverges from the SWF spec, and *always* returns
	// a `bool`, even in SWF 4.
	ctx.push(b == a);
}

void AVM1Activation::actionStringExtract()
{
	// SWF 4 `substring()` function.
	size_t len = ctx.pop().toInt32(*this);

	// NOTE: The start index is 1 based.
	size_t start = std::max<int32_t>(ctx.pop().toInt32(*this) - 1, 0);
	auto str = ctx.pop().toString(*this);

	if (start <= str.numChars())
		ctx.push(str.substr(start, len))
	else
		ctx.push("");
}

void AVM1Activation::actionStringGreater()
{
	// SWF 4 `gt` operator.
	auto a = ctx.pop().toString(*this);
	auto b = ctx.pop().toString(*this);

	// NOTE: Flash Player diverges from the SWF spec, and *always* returns
	// a `bool`, even in SWF 4.
	ctx.push(b > a);
}

void AVM1Activation::actionStringLength()
{
	// SWF 4 `length()` function.
	// NOTE: In SWF 6, and later, this is the same as `String.length`
	// (which returns the number of UTF-16 code units).
	// NOTE: In SWF 5, and earlier, this returns the byte length, even
	// though the encoding is locale dependent.
	auto str = ctx.pop().toString(*this);
	ctx.push(number_t
	(
		swfVersion > 5 ?
		str.numChars() :
		str.numBytes()
	));
}

void AVM1Activation::actionStringLess()
{
	// SWF 4 `lt` operator.
	auto a = ctx.pop().toString(*this);
	auto b = ctx.pop().toString(*this);

	// NOTE: Flash Player diverges from the SWF spec, and *always* returns
	// a `bool`, even in SWF 4.
	ctx.push(b < a);
}

void AVM1Activation::actionSubtract()
{
	ctx.push
	(
		ctx.pop().toNumber(*this) -
		ctx.pop().toNumber(*this)
	);
}

void AVM1Activation::actionTargetPath()
{
	// Prints out the `.` path of the argument.
	// NOTE: The argument must be a `DisplayObject` (not a string path).
	auto clip = ctx.pop().toObject(*this)->as<DisplayObject>();
	if (clip.isNull())
	{
		ctx.push(AVM1Value::undefinedVal);
		return;
	}
	ctx.push(clip->AVM1GetPath());
}

void AVM1Activation::actionThrow()
{
	auto value = ctx.pop();
	tiny_string str;
	try
	{
		LOG_CALL("Thrown exception: " << value.toString(*this));
	}
	catch (...)
	{
		LOG_CALL("Thrown exception: undefined");
	}
	throw value;
}

void AVM1Activation::actionToggleQuality()
{
	// Toggle between `Low`, and `High`/`Best` quality.
	// NOTE: This action remembers whether the stage quality was `Best`,
	// or higher, so we have to preserve the high quality downscaling
	// flag, to make sure that we toggle back to the proper quality.
	auto stage = ctx.getStage();
	bool useDownScaling = stage->useHighQualityDownScaling();
	bool isHigh =
	(
		stage->quality == Quality::High ||
		stage->quality == Quality::Best
	);

	stage->setQuality
	(
		isHigh ? Quality::Low :
		useDownScaling ?
		Quality::Best :
		Quality::High
	);

	stage->setHighQualityDownScaling(useDownScaling);
}

void AVM1Activation::actionToInteger()
{
	ctx.push(number_t(ctx.pop().toInt32(*this)));
}

void AVM1Activation::actionToNumber()
{
	ctx.push(ctx.pop().toNumber(*this));
}

void AVM1Activation::actionToString()
{
	ctx.push(ctx.pop().toString(*this));
}

void AVM1Activation::actionTrace()
{
	// NOTE: `trace()` *always* prints `undefined`, even in SWF 6, and
	// earlier.
	auto value = ctx.pop();
	ctx.getSys()->trace
	(
		value.is<UndefinedVal>() ?
		"undefined" :
		value.toString(*this)
	);
}

Optional<AVM1Value> AVM1Activation::actionTry
(
	const std::vector<uint8_t>& code,
	size_t& idx,
	size_t actionLen
)
{
	auto sys = ctx.getSys();
	// NOTE: `ActionTry` must be at least 7 bytes long. If it's shorter,
	// it's an invalid action; treat it like an empty try block, and bail
	// earliy.
	if (actionLen < 7)
		return {};

	TryFlags flags = code[idx++];
	auto trySize = readInt<uint16_t>(code, idx);
	auto catchSize = readInt<uint16_t>(code, idx);
	auto finallySize = readInt<uint16_t>(code, idx);
	actionLen += trySize + catchSize + finallySize;

	struct CatchVar
	{
		bool isReg;
		union
		{
			tiny_string name;
			uint8_t reg;
		};
	} catchVar;

	catchVar.isReg = flags & TryFlags::CatchInRegister;
	if (catchVar.isReg)
		catchVar.reg = code[idx++];
	else
		catchVar.name = readSwfStr(sys, getEncoding(), code, idx);

	size_t tryStart = idx;
	size_t catchStart = tryStart + trySize;
	size_t finallyStart = catchStart + catchSize;
	idx = finallyStart + finallySize;

	Optional<AVM1Value> ret;
	try
	{
		ret = runActions(code, tryStart);
	}
	catch (AVM1Value& value)
	{
		if (!(flags & TryFlags::CatchBlock))
			return {};

		AVM1Activation act
		(
			ctx,
			id.makeChildID("[Catch]"),
			swfVersion,
			scope,
			constPool,
			baseClip,
			_this,
			callee
		);

		act.localRegs = localRegs;

		if (catchVar.isReg)
			act.setCurrentReg(catchVar.reg, value);
		else
			act.setVariable(catchVar.name, value);
		ret = act.runActions(code, catchStart);
	}

	if (!(flags & TryFlags::FinallyBlock))
		return ret;

	auto finallyRet = runActions(code, finallyStart);
	return finallyRet.hasValue() ? finallyRet : ret;
}

void AVM1Activation::actionTypeOf()
{
	ctx.push(ctx.pop().typeOf(*this));
}

void AVM1Activation::actionWaitForFrame
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

void AVM1Activation::actionWaitForFrame2
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

Optional<AVM1Value> AVM1Activation::actionWith
(
	const std::vector<uint8_t>& code,
	size_t& idx,
	size_t actionLen
)
{

}

void AVM1Activation::actionUnknown
(
	const std::vector<uint8_t>& code,
	size_t& idx,
	size_t actionLen
)
{

}
