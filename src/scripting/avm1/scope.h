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

#ifndef SCRIPTING_AVM1_SCOPE_H
#define SCRIPTING_AVM1_SCOPE_H 1

#include "gc/ptr.h"
#include "gc/resource.h"

// Based on Ruffle's `avm1::scope::Scope`.

namespace lightspark
{

class tiny_string;
class GcContext;
class AVM1Activation
class AVM1CallableValue;
class AVM1DisplayObject;
class AVM1Global;
class AVM1Object;
class AVM1Value;
template<typename T>
class Impl;
template<typename T>
class Optional;

// The type of scope.
enum class AVM1ScopeClass
{
	// The global scope.
	Global,

	// A timeline scope. All timeline actions run with the current clip
	// object, instead of a local scope. The timeline scope can be
	// changed with `tellTarget()`.
	Target,

	// A local scope. Local scopes are inherited whenever a closure is
	// defined.
	Local,

	// A scope that's been created using `with`. `with` scopes aren't
	// inherited when closures are defined.
	With,
};

// The scope chain of an AVM1 context.
class AVM1Scope : public GcResource
{
private:
	NullableGcPtr<AVM1Scope> parent;
	AVM1ScopeClass _class;
	GcPtr<AVM1Object> values;
public:
	// Contructs/Creates an arbitrary scope.
	AVM1Scope
	(
		const GcContext& ctx,
		const GcPtr<AVM1Scope>& parent,
		const AVM1ScopeClass& type,
		const GcPtr<AVM1Object>& values
	);
	AVM1Scope
	(
		const GcPtr<AVM1Scope>& parent,
		const AVM1ScopeClass& type,
		const GcPtr<AVM1Object>& values
	);

	// Constructs a global scope (A parentless scope).
	AVM1Scope(const GcPtr<AVM1Global>& globals);

	// Constructs a target/timeline scope.
	AVM1Scope
	(
		const GcPtr<AVM1Scope> parent,
		const GcPtr<AVM1DisplayObject>& clip
	);

	// Constructs a child/local scope of another scope.
	AVM1Scope(const GcPtr<AVM1Scope>& parent);

	// Constructs a `with` scope, used as the scope in a `with` block.
	//
	// `with` blocks add an object to the top of the scope chain, so that
	// unqualified references will attempt to resolve that object first.
	AVM1Scope
	(
		const GcPtr<AVM1Scope>& parent,
		const GcPtr<AVM1Object>& withObj
	) : AVM1Scope(parent, AVM1ScopeClass::With, withObj) {}

	AVM1Scope(const AVM1Scope& other) : AVM1Scope
	(
		other.parent,
		other._class,
		other.values
	)
	{
	}

	AVM1Scope& operator=(const AVM1Scope& other)
	{
		parent = other.parent;
		_class = other._class;
		values = other.values;
		return *this;
	}

	// Replaces the target/timeline scope with another object.
	void setTargetScope(const GcPtr<AVM1DisplayObject>& clip);

	// Returns the parent scope.
	const NullableGcPtr<AVM1Scope>& getParentScope() const { return parent; }

	// Returns the current local scope object.
	const GcPtr<AVM1Object>& getLocals() const { return values; }
	const AVM1Object* getLocalsPtr() const { return values.getPtr(); }
	AVM1Object* getLocalsPtr() { return values.getPtr(); }

	// Returns the class type.
	AVM1ScopeClass getClass() const { return _class; }

	bool isGlobalScope() const { return _class == AVM1ScopeClass::Global; }
	bool isTargetScope() const { return _class == AVM1ScopeClass::Target; }
	bool isLocalScope() const { return _class == AVM1ScopeClass::Local; }
	bool isWithScope() const { return _class == AVM1ScopeClass::With; }

	template<typename F>
	void forEachScope(F&& callback) const
	{
		if (callback(*this) && !parent.isNull())
			parent->forEachScope(callback);
	}

	template<typename F>
	void forEachScope(F&& callback)
	{
		if (callback(*this) && !parent.isNull())
			parent->forEachScope(callback);
	}

	// Gets/Resolves a specific variable on the scope chain.
	//
	// Since scopes are just object chains, the same rules for
	// `AVM1Object::getProp()` still apply here.
	Impl<AVM1CallableValue> getVar
	(
		AVM1Activation& act,
		const tiny_string& name
	) const
	{
		return resolveRecursive(act, name, true);
	}

	// Recursively resolve a variable on the scope chain.
	// See `AVM1Scope::getVar()` for details.
	Impl<AVM1CallableValue> resolveRecursive
	(
		AVM1Activation& act,
		const tiny_string& name,
		bool isTopLevel
	) const;

	// Sets a specific variable on the scope chain.
	//
	// This traverses the scope chain, in search of a variable. If a
	// variable already exists, it get's overwritten. The traversal
	// ends when a target scope is reached, which represents the
	// `MovieClip` timeline the code is executing in.
	// If no variable is found, it'll be created in the current
	// target scope.
	void setVar
	(
		AVM1Activation& act,
		const tiny_string& name,
		const AVM1Value& value
	);

	// Defines a named local variable on the scope.
	//
	// If the variable doesn't exist on the local scope, it'll be created.
	// Otherwise, the existing variable will be set to `value`, which
	// doesn't traverse the scope chain. Any variables that have the same
	// name, but are deeper in the chain get shadowed.
	void defineLocal
	(
		AVM1Activation& act,
		const tiny_string& name,
		const AVM1Value& value
	);

	// Creates a local variable on the activation.
	//
	// This inserts the value as a stored variable on the local
	// scope. If the variable already exists, it'll be forcefully
	// overwritten. Used internally for initializing objects.
	void forceDefineLocal(const tiny_string& name, const AVM1Value& value);
	#ifdef USE_STRING_ID
	void forceDefineLocal(uint32_t nameID, const AVM1Value& value);
	#endif

	// Deletes a variable from the scope.
	bool deleteVar(AVM1Activation& act, const tiny_string& name);
};

}
#endif /* SCRIPTING_AVM1_SCOPE_H */
