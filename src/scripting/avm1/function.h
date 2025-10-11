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

#ifndef SCRIPTING_AVM1_FUNCTION_H
#define SCRIPTING_AVM1_FUNCTION_H 1

#include <vector>

#include "scripting/avm1/object/object.h"

#define AVM1_FUNCTION_ARGS \
	( \
		AVM1Activation& act, \
		const GcPtr<AVM1Object>& _this, \
		const std::vector<AVM1Value>& args \
	)
#define AVM1_FUNCTION_DECL(name) \
	static AVM1Value name(AVM1_FUNCTION_ARGS)

#define AVM1_FUNCTION_BODY(class, name) \
	AVM1Value class::name(AVM1_FUNCTION_ARGS)

// Based on Ruffle's `avm1::function` crate.

namespace lightspark
{

class tiny_string;
class AVM1Activation;
struct AVM1FuncArg;
class AVM1MovieClipRef;
class AVM1Scope;
class AVM1Value;
class GcContext
template<typename T>
class GcPtr;
template<typename T>
class NullableGcPtr;

// The signature of a native function in Lightspark's code.
using AVM1NativeFunc = AVM1Value(*)(AVM1_FUNCTION_ARGS);

// Indicates the reason for executing an `AVM1Execution`.
enum class AVM1ExecutionReason
{
	// This execution is a normal function call from a script's bytecode.
	FunctionCall,

	// This execution is a special, internal function call from the player,
	// such as {g,s}etters, `{toString,valueOf}()`, or event handlers.
	Special,
};

// The interface of an AVM1 function.
//
// The function can either be defined in Lightspark's runtime, or in the
// AVM1 bytecode itself.
class AVM1Executable
{
public:
	// Execute the given code.
	//
	// NOTE: Execution isn't guaranteed to have completed when this
	// function returns. If on stack execution is possible, then this
	// function returns a value that must be pushed onto the stack.
	// Otherwise, you must create a new stack frame, and execute the
	// action data yourself.
	virtual AVM1Value exec
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
	) const = 0;

	#ifdef USE_STRING_ID
	AVM1Value exec
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& _this,
		uint8_t depth,
		const std::vector<AVM1Value>& args,
		const AVM1ExecutionReason& reason,
		const GcPtr<AVM1Object>& callee
	) const;
	#endif
};

enum class AVM1FuncFlags : uint16_t
{
	PreloadThis = 1 << 0,
	SuppressThis = 1 << 1,
	PreloadArgs = 1 << 2,
	SuppressArgs = 1 << 3,
	PreloadSuper = 1 << 4,
	SuppressSuper = 1 << 5,
	PreloadRoot = 1 << 6,
	PreloadParent = 1 << 7,
	PreloadGlobal = 1 << 8,
};

// A function defined in the AVM1 runtime, done via
// `ActionDefineFunction{,2}`.
class AVM1Function : public AVM1Execution
{
private:
	// The version of the SWF that generated this function.
	uint8_t swfVersion;

	// The bytecode of the function.
	std::vector<uint8_t> data;

	// The name of the function, if it isn't anonymous.
	uint32_t name;

	// The number of registers to allocate for this function's private
	// register set. Any registers beyond this will be provided by the
	// global register set.
	uint8_t regCount;

	// The arguments of the function.
	std::vector<AVM1FuncArg> args;

	// The scope that the function was defined in.
	GcPtr<AVM1Scope> scope;

	// The constant pool that this function executes with.
	std::vector<uint32_t> constPool;

	// The base `MovieClip` that the function was defined in.
	// This is the `MovieClip` that contains the bytecode.
	GcPtr<AVM1MovieClipRef> baseClip;

	// The flags that define the preloated registers of the function.
	AVM1FuncFlags flags;

	void loadThis
	(
		AVM1Activation& activation,
		const AVM1Value& _this,
		uint8_t& preloadReg
	);

	void loadArgs
	(
		AVM1Activation& activation,
		const std::vector<AVM1Value>& args,
		const NullableGcPtr<AVM1Object>& caller,
		uint8_t& preloadReg
	);

	void loadSuper
	(
		AVM1Activation& activation,
		const NullableGcPtr<AVM1Object>& _this,
		uint8_t depth,
		uint8_t& preloadReg
	);

	void loadRoot(AVM1Activation& activation, uint8_t& preloadReg);
	void loadParent(AVM1Activation& activation, uint8_t& preloadReg);
	void loadGlobal(AVM1Activation& activation, uint8_t& preloadReg);
public:
	// Construct a function from an `ActionDefineFunction2` action.
	AVM1Function
	(
		uint8_t _swfVersion,
		const std::vector<uint8_t> actions,
		uint32_t _name,
		uint8_t _regCount,
		const std::vector<AVM1FuncArg>& _args,
		const GcPtr<AVM1Scope>& _scope,
		const std::vector<uint32_t>& pool,
		const GcPtr<AVM1MovieClipRef>& _baseClip,
		const AVM1FuncFlags& _flags = AVM1FuncFlags(0)
	) :
	swfVersion(_swfVersion),
	data(actions),
	name(_name),
	regCount(_regCount),
	args(_args),
	scope(_scope),
	constPool(pool),
	baseClip(_baseClip),
	flags(_flags) {}

	AVM1Value exec
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
	) const override;

	uint8_t getSwfVersion() const { return swfVersion; }
	const tiny_string& getName() const { return name; }
	const GcPtr<AVM1Scope>& getScope() const { return scope; }
	uint8_t getRegCount() const { return regCount; }
};

struct AVM1FuncArg
{
	// The register that the argument will be preloaded into.
	//
	// NOTE: If `reg` is 0, then this argument will be stored in a named
	// variable in the function activation, and can be accessed with
	// `Action{G,S}etVariable`.
	// Otherwise, the argument is loaded into a register, and pust be
	// accessed with `Action{Push,StoreRegister}`.
	uint8_t reg;

	// The name of the argument.
	uint32_t name;
};

// A native function in Lightspark's code.
class AVM1NativeFunction : public AVM1Executable
{
private:
	AVM1NativeFunc func;

	// A dummy function that does nothing, and returns `undefined`.
	AVM1_FUNCTION_DECL(emptyFunc);
public:
	AVM1NativeFunction(const AVM1NativeFunc _func = nullptr) :
	func(_func != nullptr ? _func : emptyFunc) {}

	AVM1Value exec
	(
		AVM1Activation& act,
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
	) const override
	{
		return func(act, _this.toObject(act), args);
	}
};

class AVM1FunctionObject : public AVM1Object
{
private:
	// The function that'll be called when this object is called.
	GcPtr<AVM1Execution> func;

	// The function that'll be called when this object is constructed.
	//
	#if 1
	// If `ctor` is null, then it falls back to `function`.
	#else
	// If `ctor` is null, then it falls back to the underlying function
	// itself.
	#endif
	AVM1NativeFunc ctor;
public:
	// Construct a function, without a prototype.
	AVM1FunctionObject
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Execution>& _func,
		const AVM1NativeFunc _ctor,
		const GcPtr<AVM1Object>& funcProto
	) : AVM1Object(activation, funcProto), func(_func), ctor(_ctor) {}

	// Construct a function with any combination of normal, and
	// constructor parts.
	//
	// Since prototypes need to link back to themselves, this constructor
	// builds both objects itself.
	//
	// `funcProto` refers to the function's implicit prototype, and
	// `proto` refers to the function's explicit prototype.
	// The function, and it's prototype will be linked to each other.
	AVM1FunctionObject
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Execution>& _func,
		const AVM1NativeFunc _ctor,
		const GcPtr<AVM1Object>& funcProto,
		const GcPtr<AVM1Object>& proto
	);

	// Constructs a function, that does nothing.
	AVM1FunctionObject
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& funcProto,
		const GcPtr<AVM1Object>& proto
	) : AVM1FunctionObject
	(
		activation,
		AVM1NativeFunction(),
		nullptr,
		funcProto,
		proto
	) {}

	// Construct a function from AVM1 bytecode, and it's associated
	// prototypes.
	AVM1FunctionObject
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Function>& func,
		const GcPtr<AVM1Object>& funcProto,
		const GcPtr<AVM1Object>& proto
	) : AVM1FunctionObject
	(
		activation,
		func
		nullptr,
		funcProto,
		proto
	) {}

	// Construct a function from a native function, and it's associated
	// prototypes.
	AVM1FunctionObject
	(
		AVM1Activation& activation,
		const AVM1NativeFunc func,
		const GcPtr<AVM1Object>& funcProto,
		const GcPtr<AVM1Object>& proto
	) : AVM1FunctionObject
	(
		activation,
		AVM1NativeFunction(func),
		nullptr,
		funcProto,
		proto
	) {}

	// Construct a native constructor from native functions, and their
	// associated prototypes.
	//
	// This differs from the native function ctor in 2 important ways.
	// - When called through `new`, the return value will always become
	// the result of the operation. Therefore, native constructors should
	// generally return either `_this`, if the object was successfully
	// constructed, or `undefined` if not.
	// - When called as a normal function, `func` will be called, instead
	// of `ctor`. If it's `nullptr`, then the return value will be
	// `undefined`.
	AVM1FunctionObject
	(
		AVM1Activation& activation,
		const AVM1NativeFunc ctor,
		const AVM1NativeFunc func,
		const GcPtr<AVM1Object>& funcProto,
		const GcPtr<AVM1Object>& proto
	) : AVM1FunctionObject
	(
		activation,
		AVM1NativeFunction(func),
		ctor,
		funcProto,
		proto
	) {}

	constexpr operator const GcPtr<AVM1Executable>&() const { return func; }
	constexpr operator GcPtr<AVM1Executable>&() { return func; }

	GcPtr<AVM1Executable> asCtor() const
	{
		if (ctor != nullptr)
			return AVM1NativeFunction(ctor);
		return func;
	}

	bool isNativeCtor() const { return ctor != nullptr; }

	AVM1Value call
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const GcPtr<AVM1Object>& callee,
		const AVM1Value& _this,
		const std::vector<AVM1Value>& args
	) const;

	AVM1Value callCtor
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& callee,
		GcPtr<AVM1Object> _this,
		const std::vector<AVM1Value>& args
	) const;

	AVM1Value construct
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& callee,
		const std::vector<AVM1Value>& args
	) const;

	void constructInplace
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& callee,
		GcPtr<AVM1Object> _this,
		const std::vector<AVM1Value>& args
	) const
	{
		// Always ignore the constructor's return value.
		(void)callCtor(activation, callee, _this, args);
	}

	AVM1Value call
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& _this,
		const std::vector<AVM1Value>& args
	) const override
	{
		return call(activation, name, this, _this, args);
	}

	AVM1Value construct
	(
		AVM1Activation& activation,
		const std::vector<AVM1Value>& args
	) const override
	{
		return construct(activation, this, args);
	}

	void constructInplace
	(
		AVM1Activation& activation,
		GcPtr<AVM1Object> _this,
		const std::vector<AVM1Value>& args
	) const override
	{
		constructInplace(activation, this, _this, args);
	}
};

}
#endif /* SCRIPTING_AVM1_FUNCTION_H */
