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

#include "forwards/asobject.h"
#include "forwards/scripting/flash/display/DisplayObject.h"
#include "forwards/scripting/flash/system/flashsystem.h"
#include "forwards/scripting/toplevel/Global.h"
#include "forwards/swftypes.h"
#include "smartrefs.h"

// Based on Ruffle's `avm1::scope::Scope`.

namespace lightspark
{

union asAtom;
struct garbagecollectorstate;
struct multiname;
class ASObject;
class ASWorker;
class DisplayObject;
class Global;
enum GET_VARIABLE_RESULT;
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
class AVM1Scope : public RefCountable
{
private:
	_NR<AVM1Scope> parent;
	AVM1ScopeClass _class;
	asAtom values;
	uint32_t storedmembercount;

	// Replaces the current local scope object with another object.
	void setLocals(asAtom newLocals);
public:
	// Contructs/Creates an arbitrary scope.
	AVM1Scope
	(
		_NR<AVM1Scope> _parent,
		const AVM1ScopeClass& type,
		asAtom _values,
		bool valuesrefcounted=false
	) : parent(_parent), _class(type), values(_values),storedmembercount(1)
	{
		ASObject* o = asAtomHandler::getObject(values);
		if (o)
		{
			if(!valuesrefcounted)
				o->incRef();
			o->addStoredMember();
		}
	}

	// Constructs a global scope (A parentless scope).
	AVM1Scope(Global* globals);

	// Constructs a target/timeline scope.
	AVM1Scope(const _R<AVM1Scope>& _parent, DisplayObject* clip);

	// Constructs a child/local scope of another scope.
	AVM1Scope(const _R<AVM1Scope>& _parent, ASWorker* wrk);

	// Constructs a `with` scope, used as the scope in a `with` block.
	//
	// `with` blocks add an object to the top of the scope chain, so that
	// unqualified references will attempt to resolve that object first.
	AVM1Scope
	(
		const _R<AVM1Scope>& parent,
		asAtom withObj
	) : AVM1Scope(parent, AVM1ScopeClass::With, withObj) {}

	AVM1Scope(const AVM1Scope& other) : AVM1Scope
	(
		other.parent,
		other._class,
		asAtomHandler::invalidAtom
	)
	{
		setLocals(other.values);
	}

	AVM1Scope& operator=(const AVM1Scope& other)
	{
		parent = other.parent;
		_class = other._class;
		setLocals(other.values);
		return *this;
	}

	~AVM1Scope()
	{
	}

	// Creates a global scope (A parentless scope).
	static AVM1Scope makeGlobalScope(Global* globals)
	{
		return AVM1Scope(globals);
	}

	// Creates a child/local scope of another scope.
	static AVM1Scope makeLocalScope(const _R<AVM1Scope>& parent, ASWorker* wrk)
	{
		return AVM1Scope(parent, wrk);
	}

	// Creates a target/timeline scope.
	static AVM1Scope makeTargetScope(const _R<AVM1Scope>& parent, DisplayObject* clip)
	{
		return AVM1Scope(parent, clip);
	}

	// Creates a `with` scope, used as the scope in a `with` block.
	//
	// `with` blocks add an object to the top of the scope chain, so that
	// unqualified references will attempt to resolve that object first.
	static AVM1Scope makeWithScope(const _R<AVM1Scope>& parent, asAtom withObj)
	{
		return AVM1Scope(parent, withObj);
	}

	// Replaces the target/timeline scope with another object.
	void setTargetScope(DisplayObject* clip);

	// Returns the parent scope.
	_NR<AVM1Scope> getParentScope() const { return parent; }

	// Returns the current local scope object.
	asAtom getLocals() const { return values; }
	const ASObject* getLocalsPtr() const { return asAtomHandler::getObject(values); }
	ASObject* getLocalsPtr() { return asAtomHandler::getObject(values); }

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
	// `ASObject::AVM1getVariableByMultiname` still apply here.
	asAtom getVariableByMultiname
	(
		DisplayObject* baseClip,
		const multiname& name,
		const GET_VARIABLE_OPTION& options,
		ASWorker* wrk
	)
	{
		return resolveRecursiveByMultiname(baseClip, name, options, wrk, true);
	}

	// Recursively resolve a variable on the scope chain.
	// See `AVM1Scope::getVariableByMultiname` for details.
	asAtom resolveRecursiveByMultiname
	(
		DisplayObject* baseClip,
		const multiname& name,
		const GET_VARIABLE_OPTION& options,
		ASWorker* wrk,
		bool isTopLevel
	);

	// Sets a specific variable on the scope chain.
	//
	// This traverses the scope chain, in search of a variable. If a
	// variable already exists, it get's overwritten. The traversal
	// ends when a target scope is reached, which represents the
	// `MovieClip` timeline the code is executing in.
	// If no variable is found, it'll be created in the current
	// target scope.
	bool setVariableByMultiname
	(
		multiname& name,
		asAtom& value,
		const CONST_ALLOWED_FLAG& allowConst,
		ASWorker* wrk
	);

	// Defines a named local variable on the scope.
	//
	// If the variable doesn't exist on the local scope, it'll be created.
	// Otherwise, the existing variable will be set to `value`, which
	// doesn't traverse the scope chain. Any variables that have the same
	// name, but are deeper in the chain get shadowed.
	bool defineLocalByMultiname
	(
		multiname& name,
		asAtom& value,
		const CONST_ALLOWED_FLAG& allowConst,
		ASWorker* wrk
	);

	// Creates a local variable on the activation.
	//
	// This inserts the value as a stored variable on the local
	// scope. If the variable already exists, it'll be forcefully
	// overwritten. Used internally for initializing objects.
	bool forceDefineLocalByMultiname
	(
		multiname& name,
		asAtom& value,
		const CONST_ALLOWED_FLAG& allowConst,
		ASWorker* wrk
	);

	bool forceDefineLocal
	(
		const tiny_string& name,
		asAtom& value,
		const CONST_ALLOWED_FLAG& allowConst,
		ASWorker* wrk
	);

	bool forceDefineLocal
	(
		uint32_t nameID,
		asAtom& value,
		const CONST_ALLOWED_FLAG& allowConst,
		ASWorker* wrk
	);
	void addStoredMember()
	{
		++storedmembercount;
	}
	void removeStoredMember()
	{
		assert(storedmembercount);
		--storedmembercount;
	}
	void clearScope();

	// Deletes a variable from the scope.
	bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk);

	bool countAllCyclicMemberReferences(garbagecollectorstate& gcstate);
	void prepareShutdown();
};

}
#endif /* SCRIPTING_AVM1_SCOPE_H */
