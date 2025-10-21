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
#include "utils/optional.h"

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

AVM1Value AVM1Activation::runActions(const std::vector<uint8_t>& code)
{
	if (!id.funcCount && !id.specialCount)
		ctx.startTime = compat_now();

	for (size_t i = 0; i < code.size();)
	{
		auto ret = doAction(code, i);
		if (ret.hasValue())
			return *ret;
	}

	return AVM1Value::undefinedVal;
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

Optional<AVM1Value> AVM1Activation::doAction
(
	const std::vector<uint8_t>& code,
	size_t& idx
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
		return AVM1Value::undefinedVal;

	uint8_t opcode = code[idx++];
	uint16_t length = 0;
	if (opcode >= 0x80)
	{
		length = static_cast<uint16_t*>(code.data())[idx];
		idx += 2;
	}

	switch (opcode)
	{
		// `ActionEnd`.
		case 0x00: return AVM1Value::undefinedVal; break;
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
		case 0x8e: actionDefineFunction(code, idx, true); break;
		case 0x8f: return actionTry(code, idx); break;

		case 0x94: actionWith(code, idx); break;
		case 0x96: actionPush(code, idx); break;
		case 0x99: actionJump(code, idx); break;
		case 0x9a: actionGetURL2(code, idx); break;
		case 0x9b: actionDefineFunction(code, idx); break;
		case 0x9d: actionIf(code, idx); break;
		case 0x9e:
			if (!actionCall())
				return AVM1Value::undefinedVal;
			break;
		case 0x9f: actionGotoFrame2(code, idx); break;

		default: actionUnknown(code, idx); break;
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
	// ActionScript 1 logical and.
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
	// SWF4 `ord()` function.
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
			auto frameNum = clip->frameLabelToNum(frame, *this);
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

}

bool AVM1Activation::actionCallMethod()
{

}

void AVM1Activation::actionCastOp()
{

}

void AVM1Activation::actionConstantPool
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

void AVM1Activation::actionDecrement()
{

}

void AVM1Activation::actionDefineFunction
(
	const std::vector<uint8_t>& code,
	size_t& idx,
	bool isVer2 = false
)
{

}

void AVM1Activation::actionDefineLocal()
{

}

void AVM1Activation::actionDefineLocal2()
{

}

void AVM1Activation::actionDelete()
{

}

void AVM1Activation::actionDelete2()
{

}

void AVM1Activation::actionDivide()
{

}

void AVM1Activation::actionEndDrag()
{

}

void AVM1Activation::actionEnumerate()
{

}

void AVM1Activation::actionEnumerate2()
{

}

void AVM1Activation::actionEquals()
{

}

void AVM1Activation::actionEquals2()
{

}

void AVM1Activation::actionExtends()
{

}

void AVM1Activation::actionGetMember()
{

}

void AVM1Activation::actionGetProperty()
{

}

void AVM1Activation::actionGetTime()
{

}

void AVM1Activation::actionGetVariable()
{

}

void AVM1Activation::actionGetURL
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

void AVM1Activation::actionGetURL2
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

void AVM1Activation::actionGotoFrame
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

void AVM1Activation::actionGotoFrame2
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

void AVM1Activation::actionGotoLabel
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

void AVM1Activation::actionIf(const std::vector<uint8_t>& code, size_t& idx)
{

}

void AVM1Activation::actionIncrement()
{

}

void AVM1Activation::actionInitArray()
{

}

void AVM1Activation::actionInitObject()
{

}

void AVM1Activation::actionImplementsOp()
{

}

void AVM1Activation::actionInstanceOf()
{

}

void AVM1Activation::actionJump(const std::vector<uint8_t>& code, size_t& idx)
{

}

void AVM1Activation::actionLess()
{

}

void AVM1Activation::actionLess2()
{

}

void AVM1Activation::actionGreater()
{

}

void AVM1Activation::actionMBAsciiToChar()
{

}

void AVM1Activation::actionMBStringExtract()
{

}

void AVM1Activation::actionMBStringLength()
{

}

void AVM1Activation::actionMultiply()
{

}

void AVM1Activation::actionModulo()
{

}

void AVM1Activation::actionNot()
{

}

void AVM1Activation::actionNextFrame()
{

}

bool AVM1Activation::actionNewMethod()
{

}

bool AVM1Activation::actionNewObject()
{

}

void AVM1Activation::actionOr()
{

}

void AVM1Activation::actionPlay()
{

}

void AVM1Activation::actionPreviousFrame()
{

}

void AVM1Activation::actionPush(const std::vector<uint8_t>& code, size_t& idx)
{

}

void AVM1Activation::actionPushDuplicate()
{

}

void AVM1Activation::actionRandomNumber()
{

}

void AVM1Activation::actionRemoveSprite()
{

}

AVM1Value AVM1Activation::actionReturn()
{

}

void AVM1Activation::actionSetMember()
{

}

void AVM1Activation::actionSetProperty()
{

}

void AVM1Activation::actionSetVariable()
{

}

void AVM1Activation::actionStrictEquals()
{

}

void AVM1Activation::actionSetTarget()
{

}

void AVM1Activation::actionSetTarget2()
{

}

void AVM1Activation::actionStackSwap()
{

}

void AVM1Activation::actionStartDrag()
{

}

void AVM1Activation::actionStop()
{

}

void AVM1Activation::actionStopSounds()
{

}

void AVM1Activation::actionStoreRegister
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

void AVM1Activation::actionStringAdd()
{

}

void AVM1Activation::actionStringEquals()
{

}

void AVM1Activation::actionStringExtract()
{

}

void AVM1Activation::actionStringGreater()
{

}

void AVM1Activation::actionStringLength()
{

}

void AVM1Activation::actionStringLess()
{

}

void AVM1Activation::actionSubtract()
{

}

void AVM1Activation::actionTargetPath()
{

}

void AVM1Activation::actionThrow()
{

}

void AVM1Activation::actionToggleQuality()
{

}

void AVM1Activation::actionToInteger()
{

}

void AVM1Activation::actionToNumber()
{

}

void AVM1Activation::actionToString()
{

}

Optional<AVM1Value> AVM1Activation::actionTry
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}

void AVM1Activation::actionTypeOf()
{

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
	size_t& idx
)
{

}

void AVM1Activation::actionUnknown
(
	const std::vector<uint8_t>& code,
	size_t& idx
)
{

}
