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

#ifndef SCRIPTING_AVM1_RUNTIME_H
#define SCRIPTING_AVM1_RUNTIME_H 1

#include <vector>

#include "forwards/scripting/flash/display/DisplayObject.h"
#include "forwards/swftypes.h"
#include "smartrefs.h"

// Based on Ruffle's `avm1::runtime::Avm1`.

namespace lightspark
{

class tiny_string;
class AVM1Activation;
class AVM1Function;
class AVM1Object;
class AVM1Scope;
union AVM1Value;
class DisplayObject;
class MovieClip;
template<typename T>
class Optional;

class AVM1Context
{
private:
	using RegistryType = std::unordered_map<uint32_t, _NR<AVM1Function>>;

	SystemState* sys;

	// The version of Flash Player we're trying to mimic.
	uint8_t playerVersion;

	// The constant pool to use for new `Activation`s, from sources that
	// don't close over the pool they were defined with.
	std::vector<uint32_t> constPool;

	// The global scope.
	_NR<AVM1Scope> globalScope;

	// The system built-ins that're used internally to construct new
	// objects.
	AVM1SystemPrototypes* prototypes;

	// Cached functions for `ASBroadcaster`.
	ASBroadcasterFuncs* broadcasterFuncs;

	// The operand stack (shared across functions).
	std::vector<AVM1Value> stack;

	// The register slots (also shared across functions).
	// NOTE: Functions defined with `ActionDefineFunction2` don't use
	// these slots.
	std::vector<AVM1Value> registers;

	// If a serious error occured, or the user requested it, the VM may
	// be halted.
	// This completely prevents anymore actions from running.
	bool halted;

	// Whether a `Mouse` listener's been registered.
	bool hasMouseListener;

	// The list of all `MovieClip`s, in execution order.
	_NR<DisplayObject> clipExecList;

	// The mappings between symbol names, and constructors registered
	// with `Object.registerClass()`.
	// NOTE: Because SWF 6, and 7+ use different case sensitivity rules,
	// Flash Player keeps 2 registries, one case sensitive, and
	// the other case insensitive.
	RegistryType ctorRegistryCaseSensitive;
	RegistryType ctorRegistryCaseInsensitive;

	// If `get{Bounds,Rect}()` is called on a `MovieClip` with invalid
	// bounds, and the target space is the same as the origin space, but
	// the target isn't the `MovieClip` itself, the call can either
	// return the default invalid bounds (all corners have `INT32_MAX`
	// twips), or a special invalid bounds (all corners have `INT32_MIN`
	// twips).
	//
	// This `bool` is used for this edge case. If `true`, the special
	// invalid bounds is returned, rather than teh default invalid bounds.
	//
	// This `bool` is set to `true`, if `get{Bounds,Rect}()` are called
	// on a `MovieClip` with either an `Activation`, or root movie SWF
	// version above 7. It's an internal state that changes irreversibly.
	// This means that the `getBounds()` result of a `MovieClip` can
	// change by calling `getBounds()` on a different `MovieClip`.
	//
	// There are more examples of this in Ruffle's
	// `movieclip_invalid_get_bounds_*` tests.
	bool useSpecialInvalidBounds;

	const RegistryType& getCtorRegistry(uint8_t swfVersion) const
	{
		if (swfVersion > 6)
			return ctorRegistryCaseSensitive;
		return ctorRegistryCaseInsensitive;
	}

	RegistryType& getCtorRegistry(uint8_t swfVersion)
	{
		if (swfVersion > 6)
			return ctorRegistryCaseSensitive;
		return ctorRegistryCaseInsensitive;
	}
public:
	AVM1Context(SystemState* _sys, uint8_t _playerVersion);

	// Add a stack frame, that executes code in a timeline scope.
	//
	// This creates a new stack frame.
	void runStackFrameForAction
	(
		MovieClip* activeClip,
		const tiny_string& name,
		const std::vector<uint8_t>& code
	);

	// Add a stack frame, that executes code in an initializer scope.
	//
	// This creates a new stack frame.
	void runWithStackFrameForClip
	(
		MovieClip* activeClip,
		std::function<void(AVM1Activation&)> func
	);

	// Add a stack frame, that executes code in an initializer scope.
	//
	// This creates a new stack frame.
	void runStackFrameForInitAction
	(
		MovieClip* activeClip,
		const std::vector<uint8_t>& code
	);

	// Add a stack frame, that executes code in a timeline scope, for an
	// object method, such as an event handler.
	//
	// This creates a new stack frame.
	void runStackFrameForMethod
	(
		MovieClip* activeClip,
		AVM1Object* obj,
		const tiny_string& name,
		const std::vector<AVM1Value>& args
	);

	void notifySystemListeners
	(
		MovieClip* activeClip,
		const tiny_string& broadcasterName,
		const tiny_string& method,
		const std::vector<AVM1Value>& args
	);

	SystemState* getSys() const { return sys; }
	uint8_t getPlayerVersion() const { return playerVersion; }
	void setPlayerVersion(uint8_t version) { playerVersion = version; }

	// Returns `true` if the `Mouse` object has a registered listener.
	bool getHasMouseListener() const { return hasMouseListener; }

	// Halts the VM, preventing the execution of further actions.
	//
	// If the VM is currently evaluating an action, it'll continue until
	// the action realizes that it's been halted. If an immediate halt
	// is required, an exception must be thrown.
	//
	// This is usually used when a serious error, or infinite loop occurs.
	void halt();

	size_t stackSize() const { return stack.size(); }
	void clearStack() { stack.reset(); }
	void push(const AVM1Value& value) { stack.push_back(value); }
	AVM1Value pop();

	// Get a reference to `_global`.
	AVM1Object* getGlobal() const { return globalScope->getLocalsPtr(); }

	// Get a reference to the global scope.
	const RefPtr<AVM1Scope>& getGlobalScope() const { return globalScope; }

	// Get the system built-in prototypes for this instance.
	const AVM1SystemPrototypes& getPrototypes() const { return prototypes; }

	// Get the constant pool to use for new `Activation`s, from sources
	// that don't close over the pool they were defined with.
	const std::vector<uint32_t>& getConstPool() const { return constPool; }

	// Set the constant pool to use for new `Activation`s, from sources
	// that don't close over the pool they were defined with.
	void setConstPool(const std::vector<uint32_t>& pool) { constPool = pool; }

	const ASBroadcasterFuncs& getBroadcasterFuncs() const
	{
		return broadcasterFuncs;
	}

	Optional<const AVM1Value&> getReg(size_t idx) const;
	Optional<AVM1Value&> getReg(size_t idx);

	// Find all `DisplayObject`s with negative depth recursively.
	//
	// If an object is pending removal, due to being removed by a
	// `RemoveObject` tag on the previous frame, while it had an `unload`
	// event listener attached, AVM1 requires that the object be kept
	// around for an extra frame.
	//
	// This gets called at the start of each frame, in order to create a
	// list of objects that're to be removed.
	void findDisplayObjectsPendingRemoval
	(
		DisplayObject* obj,
		std::vector<_R<DisplayObject>>& vec
	);

	// Remove all `DisplayObject`s that're pending removal.
	void removePending();

	// Run a singal frame.
	void runFrame();

	// Adds a `MovieClip` to the execution list.
	//
	// This should be called whenever a `MovieClip` is created, and
	// controls the execution order for AVM1 movies.
	void addToExecList(const RefPtr<MovieClip>& clip);

	AVM1Function* getRegisteredCtor
	(
		uint8_t swfVersion,
		const tiny_string& symbol
	) const;

	AVM1Function* getRegisteredCtor
	(
		uint8_t swfVersion,
		uint32_t symbol
	) const;

	void registerCtor
	(
		uint8_t swfVersion,
		const tiny_string& symbol,
		const _NR<AVM1Function>& ctor
	);

	void registerCtor
	(
		uint8_t swfVersion,
		uint32_t symbol,
		const _NR<AVM1Function>& ctor
	);

	bool getUseSpecialInvalidBounds() const
	{
		return useSpecialInvalidBounds;
	}

	void activateUseSpecialInvalidBounds()
	{
		useSpecialInvalidBounds = true;
	}
};

}
#endif /* SCRIPTING_AVM1_RUNTIME_H */
