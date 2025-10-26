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

#ifndef SCRIPTING_AVM1_ACTIVATION_H
#define SCRIPTING_AVM1_ACTIVATION_H 1

#include <functional>
#include <memory>
#include <vector>

#include <tsl/ordered_map.h>

#include "gc/ptr.h"
#include "scripting/avm1/value.h"
#include "tiny_string.h"

// Based on Ruffle's `avm1::activation::Activation`.

namespace lightspark
{

class Any;
class AVM1CallableValue;
class AVM1Context;
class AVM1ExecutionReason;
class AVM1Object;
enum AVM1PropFlags;
class AVM1Scope;
class DisplayObject;
class DisplayObjectContainer;
class GcContext;
template<typename T>
class Impl;
template<typename T>
class Optional;
class Request;
enum RequestMethod;
class Stage;
enum TextEncoding;
class SystemState;

// Represents a single activation, of a given AVM1 function, or keyframe.
class AVM1Activation
{
public:
		class Identifier
		{
		private:
			Identifier* parent { nullptr };
			tiny_string name;
			uint16_t depth { 0 };
			uint16_t funcCount { 0 };
			uint8_t specialCount { 0 };

			Identifier
			(
				const Identifier* _parent,
				const tiny_string& _name
				uint16_t _depth,
				uint16_t _funcCount,
				uint8_t _specialCount
			) :
			parent(_parent),
			name(_name),
			depth(_depth),
			funcCount(_funcCount),
			specialCount(_specialCount) {}
		public:
			Identifier(const tiny_string& _name) : name(_name) {}
			Identifier makeChildID(const tiny_string& name) const
			{
				return Identifier
				(
					this,
					name,
					depth + 1,
					funcCount,
					specialCount
				);
			}

			Identifier makeFuncID
			(
				const tiny_string& name
				const AVM1ExecutionReason& reason,
				uint16_t maxRecursionDepth
			) const;

			Identifier* getParent() const { return parent; }
			const tiny_string& getName() const { return name; }
			uint16_t getDepth() const { return depth; }
			uint16_t getFuncCount() const { return funcCount; }
			uint8_t getSpecialCount() const { return specialCount; }
		};
private:
	// The SWF version of a given function.
	//
	// Certain AVM1 operations change their behaviour based on the version
	// of the SWF file they were defined in. For example, case sensitivity
	// changes based on the SWF version.
	uint8_t swfVersion;

	// All defined local variables in this stack frame.
	GcPtr<AVM1Scope> scope;

	// The current, in use constant pool.
	std::vector<uint32_t> constPool;

	// The value of `this`.
	//
	// While this isn't *usually* modified, ActionScript does allow `this`
	// to be modified.
	AVM1Value _this;

	// The function object being called.
	NullableGcPtr<AVM1Object> callee;

	// Local registers, if any.
	//
	// `NullRef` indicates a function that uses the global register set.
	// A non-null value indicates the presence of local registers, even
	// if none exist. i.e. `makeOptional({})` means no registers should
	// exist at all.
	//
	// Registers use 1-based indexing; `r0` doesn't exist. Therefore this
	// vector, while nominally starting at 0, actually starts at 1.
	//
	// NOTE: The registers are stored in a `std::shared_ptr` so that
	// rescopes (e.g. `with`) can use the same register set.
	std::shared_ptr<std::vector<AVM1Value>> localRegs;

	// The base clip of this stack frame.
	// This will be the `MovieClip` that contains the byte code.
	GcPtr<DisplayObject> baseClip;

	// The current target `DisplayObject` of this stack frame.
	// This can be changed with `tellTarget()` (done via
	// `ActionSetTarget{,2}`).
	NullableGcPtr<DisplayObject> targetClip;

	// Whether the base clip was removed when we started this frame.
	bool baseClipUnloaded;

	AVM1Context& ctx;

	// An identifier to refer to this activation, when debugging.
	// This is usually the name of a function (if known), or some static
	// name, to indicate where in the code it is (e.g. a `with` block).
	Identifier id;

	// Run a single action from a given index.
	//
	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	Optional<AVM1Value> doAction
	(
		const std::vector<uint8_t>& code,
		size_t& idx,
		bool& isImplicit
	);

	void stackPush(const AVM1Value& value);

	// AVM1 instructions/actions.
	void actionAdd();
	void actionAdd2();
	void actionAnd();
	void actionAsciiToChar();
	void actionBitAnd();
	void actionBitLShift();
	void actionBitOr();
	void actionBitRShift();
	void actionBitURShift();
	void actionBitXor();

	void actionCharToAscii();
	void actionCloneSprite();
	bool actionCall();
	bool actionCallFunction();
	bool actionCallMethod();
	void actionCastOp();
	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionConstantPool(const std::vector<uint8_t>& code, size_t& idx);

	void actionDecrement();
	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionDefineFunction
	(
		const std::vector<uint8_t>& code,
		size_t& idx,
		size_t actionLen,
		bool isVer2 = false
	);
	void actionDefineLocal();
	void actionDefineLocal2();
	void actionDelete();
	void actionDelete2();
	void actionDivide();

	void actionEndDrag();
	void actionEnumerate();
	void actionEnumerate2();
	void actionEquals();
	void actionEquals2();
	void actionExtends();

	void actionGetMember();
	void actionGetProperty();
	void actionGetTime();
	void actionGetVariable();
	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionGetURL(const std::vector<uint8_t>& code, size_t& idx);
	void actionGetURL2(const std::vector<uint8_t>& code, size_t& idx);
	void actionGotoFrame(const std::vector<uint8_t>& code, size_t& idx);
	void actionGotoFrame2(const std::vector<uint8_t>& code, size_t& idx);
	void actionGotoLabel(const std::vector<uint8_t>& code, size_t& idx);

	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionIf(const std::vector<uint8_t>& code, size_t& idx);
	void actionIncrement();
	void actionInitArray();
	void actionInitObject();
	void actionImplementsOp();
	void actionInstanceOf();

	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionJump(const std::vector<uint8_t>& code, size_t& idx);

	void actionLess();
	void actionLess2();

	void actionGreater();

	void actionMBAsciiToChar();
	void actionMBCharToAscii();
	void actionMBStringExtract();
	void actionMBStringLength();
	void actionMultiply();
	void actionModulo();

	void actionNot();
	void actionNextFrame();
	bool actionNewMethod();
	bool actionNewObject();

	void actionOr();

	void actionPlay();
	void actionPreviousFrame();
	void actionPop();
	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionPush
	(
		const std::vector<uint8_t>& code,
		size_t& idx,
		size_t actionLen
	);
	void actionPushDuplicate();

	void actionRandomNumber();
	void actionRemoveSprite();
	AVM1Value actionReturn();

	void actionSetMember();
	void actionSetProperty();
	void actionSetVariable();
	void actionStrictEquals();
	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionSetTarget(const std::vector<uint8_t>& code, size_t& idx);
	void actionSetTarget2();
	void actionStackSwap();
	void actionStartDrag();
	void actionStop();
	void actionStopSounds();
	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionStoreRegister(const std::vector<uint8_t>& code, size_t& idx);
	void actionStringAdd();
	void actionStringEquals();
	void actionStringExtract();
	void actionStringGreater();
	void actionStringLength();
	void actionStringLess();
	void actionSubtract();

	void actionTargetPath();
	void actionThrow();
	void actionToggleQuality();
	void actionToInteger();
	void actionToNumber();
	void actionToString();
	void actionTrace();
	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	Optional<AVM1Value> actionTry
	(
		const std::vector<uint8_t>& code,
		size_t& idx,
		size_t actionLen
	);
	void actionTypeOf();

	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionWaitForFrame(const std::vector<uint8_t>& code, size_t& idx);
	void actionWaitForFrame2(const std::vector<uint8_t>& code, size_t& idx);
	Optional<AVM1Value> actionWith
	(
		const std::vector<uint8_t>& code,
		size_t& idx,
		size_t actionLen
	);

	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	void actionUnknown
	(
		const std::vector<uint8_t>& code,
		size_t& idx,
		size_t actionLen
	);

	// Checks that the base clip (script executing clip) still exists.
	// If the base clip is removed during execution, return from this
	// activation.
	// This should be called after any action/instruction that could
	// potentially destroy a clip (gotos, `tellTarget()`, etc).
	bool baseClipExists() const;

	void setTarget(const tiny_string& target);

	AVM1Activation
	(
		AVM1Context& _ctx,
		const Identifier& _id,
		uint8_t _swfVersion,
		const GcPtr<AVM1Scope>& _scope,
		const std::vector<uint32_t>& _constPool,
		const GcPtr<DisplayObject>& _baseClip,
		const NullableGcPtr<DisplayObject>& _targetClip,
		bool _baseClipUnloaded,
		const AVM1Value& thisVal,
		const NullableGcPtr<AVM1Object>& _callee,
		const std::shared_ptr<std::vector<AVM1Value>>& _localRegs = nullptr
	) :
	ctx(_ctx),
	id(_id),
	swfVersion(_swfVersion),
	scope(_scope),
	constPool(_constPool),
	baseClip(_baseClip),
	targetClip(_targetClip),
	baseClipUnloaded(_baseClipUnloaded),
	_this(thisVal),
	callee(_callee),
	localRegs(_localRegs) {}
public:

	AVM1Activation
	(
		AVM1Context& ctx,
		const Identifier& id,
		uint8_t _swfVersion,
		const GcPtr<AVM1Scope>& scope,
		const std::vector<uint32_t>& constPool,
		const GcPtr<DisplayObject>& baseClip,
		const AVM1Value& _this,
		const NullableGcPtr<AVM1Object>& callee
	);

	// Create a new activation, to run a block of code with a given scope.
	AVM1Activation withNewScope
	(
		const tiny_string& name,
		const GcPtr<AVM1Scope>& _scope
	);

	// Construct an empty stack frame, with no code.
	//
	// This is used by the test runner, and callback methods
	// (`onEnterFrame()`) to create a base activation frame, with access
	// to the global context.
	//
	// NOTE: Using the constructed `AVM1Activation` directly to execute
	// arbitrary bytecode, and/or to define new local variables is a
	// logic error, and will corrupt the global scope.
	AVM1Activation
	(
		AVM1Context& ctx,
		const Identifier& id,
		const GcPtr<DisplayObject>& baseClip
	);

	// Construct an empty stack frame, with no code, running on the
	// root movie, in layer 0.
	AVM1Activation(AVM1Context& ctx, const Identifier& id);

	// Add a stack frame that executes code in a timeline scope.
	//
	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	AVM1Value runChildFrame
	(
		const tiny_string& name,
		const GcPtr<DisplayObject>& activeClip,
		const std::vector<uint8_t>& code
	);

	// Add a stack frame that executes code in an initializer scope.
	Any runChildFrameForClip
	(
		const tiny_string& name,
		const GcPtr<DisplayObject>& activeClip,
		uint8_t _swfVersion,
		const std::function<Any(AVM1Activation&)>& func
	);

	// TODO: Replace `code` with a span, once we write our own span
	// implementation.
	Optional<AVM1Value> runActions
	(
		const std::vector<uint8_t>& code,
		size_t offset = 0
	);

	SystemState* getSys() const;

	const GcContext& getGcCtx() const;
	GcContext& getGcCtx();

	const AVM1Context& getCtx() const { return ctx; }
	AVM1Context& getCtx() { return ctx; }

	// Retrive a given register value.
	//
	// If a given register doesn't exist, it returns `undefined`, which
	// is a valid register value.
	AVM1Value getCurrentReg(uint8_t regID) const;

	// Set a register to a given value.
	//
	// If a given register doesn't exist, it does nothing.
	void setCurrentReg(uint8_t regID, const AVM1Value& value);

	// Set a local register to a given value. Returns `true` if successful.
	//
	// If a given local register doesn't exist, it does nothing.
	bool setLocalReg(uint8_t regID, const AVM1Value& value);

	// Convert the enumerable properties of an `Object` into a set of
	// form values.
	//
	// This is needed in order to support form submission from Flash
	// Player via a few legacy methods, such as `ActionGetURL2`, or
	// `getURL()`.
	//
	// NOTE: This doesn't support user defined virtual properties.
	tsl::ordered_map<uint32_t, uint32_t> objectToFormVals
	(
		const GcPtr<AVM1Object>& obj
	);

	// Construct a request for a fetch operation, that may send `Object`
	// properties as form data in the request body, or URL.
	Request objectToRequest
	(
		const GcPtr<AVM1Object>& obj,
		const tiny_string& url,
		const Optional<RequestMethod>& method
	);

	// Convert the current local variable scope into set of form values.
	// This is needed in order to support form submission from Flash
	// Player via a few legacy methods, such as `ActionGetURL2`, or
	// `getURL()`.
	//
	// NOTE: This doesn't support user defined virtual properties.
	tsl::ordered_map<uint32_t, uint32_t> localsToFormVals();

	// Construct a request for a fetch operation, that may send local
	// variables as form data in the request body, or URL.
	Request localsToRequest
	(
		const tiny_string& url,
		const Optional<RequestMethod>& method
	);

	// Resolves a target value to a `DisplayObject`, relative to the
	// starting `DisplayObject`.
	//
	// This is used by any action, or function with an argument that can
	// either be a `DisplayObject`, or a string path that references a
	// `DisplayObject`.
	//
	// For example, `removeMovieClip(target)` takes either a string, or
	// a `DisplayObject`.
	//
	// This can be an `Object`, `.` path, `/` path, or a weird combination
	// of the three. e.g. `_root/clip`, `clip.child._parent`,
	// `clip:child`, etc.
	// See Ruffle's `avm1/target_path` test for many different examples.
	//
	// A target path always resolves on the display list. It can also
	// look in the prototype chain, but not the scope chain.
	NullableGcPtr<DisplayObject> resolveTargetClip
	(
		const GcPtr<DisplayObject>& start,
		const AVM1Value& target,
		bool allowEmpty
	);

	// Resolves a target path string to an object.
	// This only returns `AVM1Object`s, non-object values return `nullptr`.
	//
	// This can be either a `/` path, `.`, or a weird combination of the
	// two. e.g. `_root/clip`, `clip.child._parent`, `clip:child`, etc.
	// See Ruffle's `avm1/target_path` test for many different examples.
	//
	// A target path always resolves on the display list. It can also
	// look in the prototype chain, but not the scope chain.
	NullableGcPtr<AVM1Object> resolveTargetPath
	(
		const GcPtr<DisplayObject>& root,
		const GcPtr<AVM1Object>& start,
		const tiny_string& _path,
		bool hasSlash,
		bool first = true
	);

	using ResolveVarPathType = Optional<std::pair
	<
		GcPtr<AVM1Object>,
		tiny_string
	>>;
	// Resolves a path for variable bindings.
	// Returns both the parent object that this variable belongs to, and
	// the variable name.
	// Returns `nullOpt`, if the path doesn't point to a valid object.
	ResolveVarPathType resolveVariablePath
	(
		const GcPtr<DisplayObject>& clip,
		const tiny_string& path
	);

	// Gets the value, referenced by a target path string.
	//
	// This can either be a normal variable name, `/` path, `.` path,
	// or a weird combination thereof.
	// e.g. `_root/clip.foo`, `clip:child:_parent`, and `bar`.
	// See Ruffle's `avm1/target_path` test for many different examples.
	//
	// It first tries to resolve the string as a target path, with a
	// variable name, such as `foo/bar/baz:biz`. The last `:`, or `.`
	// denotes the variable name, with everything before it denoting the
	// path of the target object. Note that the variable name on the
	// right can contain a `/` in this case. This path (minus the
	// variable name) is recursively resolved on the scope chain.
	// If the path doesn't resolve on any scope, then `undefined` is
	// returned.
	//
	// If there's no variable name, but the path contains `/`s, it'll
	// try to resolve on the scope chain. If that fails, then it's
	// treated as a variable name, and falls back to the variable case
	// (i.e. `a/b` would be treated as a variable called `a/b`, rather
	// than as a path).
	//
	// And finally, if none of the above it's a normal variable name,
	// resolved on the scope chain.
	Impl<AVM1CallableValue> getVariable(const tiny_string& path);

	// Sets the value, referenced by a target path string.
	//
	// This can either be a normal variable name, `/` path, `.` path,
	// or a weird combination thereof.
	// e.g. `_root/clip.foo`, `clip:child:_parent`, and `bar`.
	// See Ruffle's `avm1/target_path` test for many different examples.
	//
	// It first tries to resolve the string as a target path, with a
	// variable name, such as `foo/bar/baz:biz`. The last `:`, or `.`
	// denotes the variable name, with everything before it denoting the
	// path of the target object. Note that the variable name on the
	// right can contain a `/` in this case. This path (minus the
	// variable name) is recursively resolved on the scope chain.
	// If the path doesn't resolve on any scope, then the set fails, and
	// returns early. Otherwise, the variable name is either created, or
	// overwritten on the target scope.
	//
	// This differs from `getVariable()` because `/` paths that have no
	// variable segment are invalid. e.g. `a/b` sets a property called
	// `a/b` on the stack frame, rather than traversing the display list.
	//
	// If the string couldn't resolve to a path, it's considered a normal
	// variable name, and is set on the scope chain.
	void setVariable(const tiny_string& path, const AVM1Value& value);

	// Resolve a level by ID.
	//
	// If the level doesn't exist, then it'll be created, and instantiated
	// with a script object.
	GcPtr<DisplayObject> getOrCreateLevel(int32_t level);

	// Returns a reference to the `DisplayObject` that's the parent of
	// the root.
	NullableGcPtr<DisplayObjectContainer> getStageContainer() const;
	NullableGcPtr<Stage> getStage() const;

	// Tries to resolve a level by ID. Returns `NullGc` if it doesn't
	// exist.
	NullableGcPtr<DisplayObject> getLevel(int32_t level);

	// The current target clip of the executing code.
	// Actions that affect `root` after an invalid `tellTarget()` will
	// use this.
	//
	// The `root` is relative to the base clip that contains the bytecode.
	GcPtr<DisplayObject> getTargetOrRootClip() const;

	// The current target clip of the executing code.
	// Actions that affect the base clip after an invalid `tellTarget()`
	// will use this.
	GcPtr<DisplayObject> getTargetOrBaseClip() const;

	// Get the value of `_root`.
	NullableGcPtr<AVM1Object> getRootObject() const;

	// Returns whether property keys are case sensitive, based on the
	// current SWF version.
	bool isCaseSensitive() const { return swfVersion > 6; }

	// Resolve a particular named local variable within this activation.
	//
	// NOTE: Because scopes are `Object` chains, the same rules for
	// `AVM1Object::getProp()` still apply here.
	Impl<AVM1CallableValue> resolveVariable(const tiny_string& name);

	// Returns the suggested string encoding for actions/instructions.
	// For SWF 6, and later, this is always UTF-8.
	// For SWF 5, and earlier, this is locale dependent, and we default
	// to using ANSI/Windows-1252.
	TextEncoding getEncoding() const;

	// Returns the SWF version of the action, or function being executed.
	uint8_t getSwfVersion() const { return swfVersion; }

	// Returns the AVM1 local variable scope.
	const GcPtr<AVM1Scope>& getScope() const { return scope; }

	// Replace the current scope, with a new one.
	void setScope(const GcPtr<AVM1Scope>& _scope) { scope = _scope; }

	void setScopeToClip(const GcPtr<DisplayObject>& clip);

	// Whether this activation is running in a local scope.
	bool inLocalScope() const;

	// Gets the base clip of this stack frame.
	// This is the `MovieClip` that contains the executing bytecode.
	const GcPtr<DisplayObject>& getBaseClip() const { return baseClip; }

	// Gets the current target clip of this stack frame.
	// This is the `MovieClip` that `ActionGotoFrame`, and other
	// actions/instructions apply to.
	// Changed via `ActionSetTarget{,2}`.
	const NullableGcPtr<DisplayObject>& getTargetClip() const
	{
		return targetClip;
	}

	// Changes the target clip.
	void getTargetClip(const NullableGcPtr<DisplayObject>& clip)
	{
		targetClip = clip;
	}

	// Define a local property on the activation.
	//
	// If this activation is running in a local scope, and `name` is a
	// path string, it'll resolve it as a target path, and set the
	// variable on the mentioned object to `value`.
	// Otherwise, it'll create, or set the property specified by `name`
	// to `value` on the local scope. This doesn't traverse the scope
	// chain. Any properties that're earlier in the scope chain, with
	// the same name are shadowed.
	void defineLocal(const tiny_string& name, const AVM1Value& value);

	// Define a local property on the activation.
	//
	// This inserts a value as a stored property on the local scope. If
	// the property already exists, it'll be overwritten. This is used
	// internally to initalize objects.
	void forceDefineLocal(const tiny_string& name, const AVM1Value& value);

	// Returns the value of `this`, as a reference.
	const AVM1Value& getThis() const { return _this; }

	void allocateLocalRegs(uint8_t regs);

	const std::vector<uint32_t>& getConstPool() const { return constPool; }
	void setConstPool(const std::vector<uint32_t>& pool)
	{
		constPool = pool;
	}
};

}
#endif /* SCRIPTING_AVM1_ACTIVATION_H */
