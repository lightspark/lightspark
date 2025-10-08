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

#ifndef SCRIPTING_AVM1_OBJECT_OBJECT_H
#define SCRIPTING_AVM1_OBJECT_OBJECT_H 1

#include <vector>

#include "scripting/avm1/prop_map.h"
#include "scripting/avm1/value.h"
#include "smartrefs.h"

// Based on Ruffle's `avm1::object::{,Script}Object`.

namespace lightspark
{

class tiny_string;
class AVM1Activation;
class AVM1Executable;
class AVM1Prop;
enum AVM1PropFlags;
class AVM1Object;
class GcContext
template<typename T>
class GcPtr;
template<typename T>
class NullableGcPtr;
template<typename T>
class Optional;

class AVM1Watcher
{
private:
	GcPtr<AVM1Executable> callback;
	AVM1Value userData;
public:
	AVM1Watcher
	(
		const GcPtr<AVM1Executable>& _callback,
		const AVM1Value& _userData
	) : callback(_callback), userData(_userData) {}

	AVM1Value call
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& oldValue,
		const AVM1Value& newValue,
		const GcPtr<AVM1Object>& _this
	) const;
}

class AVM1Object : public RefCountable
{
private:
	using PropMapType = AVM1PropMap<AVM1Prop>::MapType;

	AVM1PropMap<AVM1Prop> properties;
	std::vector<GcPtr<AVM1Object>> interfaces;
	AVM1PropMap<AVM1Watcher> watchers;
public:
	AVM1Object
	(
		const GcContext& ctx,
		const NullableGcPtr<AVM1Object>& proto
	);

	// Gets the value of a data property on this object.
	//
	// NOTE: This doesn't traverse the prototype chain, and ignores
	// virtual properties, and thus can't cause any side effects.
	AVM1Value getData
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;

	// Sets a data property on this object.
	//
	// NOTE: This doesn't traverse the prototype chain, and ignores
	// virtual properties, and thus can't cause any side effects.
	void setData
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& value
	);

	using GetOwnPropsType = std::vector<std::pair
	<
		PropMapType::key_type,
		AVM1Value
	>>;
	GetOwnPropsType getOwnProps() const;

	AVM1Value lookupProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		bool isSlashPath
	) const;

	// Get a named, non virtual property from this object, exclusively.
	//
	// NOTE: This function shouldn't inspect protptype chains, instead,
	// use `getStoredProp()` to do normal property lookup, and resolution.
	virtual Optional<AVM1Value> getLocalProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		bool isSlashPath
	) const;

	// Get a non slash path named property from either this object, or
	// it's prototype.
	AVM1Value getNonSlashPathProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const
	{
		return lookupProp(activation, name, false);
	}

	// Get a named property from either this object, or it's prototype.
	virtual AVM1Value getProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const
	{
		return lookupProp(activation, name, true);
	}

	// Get a non virtual property from either this object, or it's
	// prototype.
	AVM1Value getStoredProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;

	virtual void setLocalProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& value,
		const GcPtr<AVM1Object>& _this
	);

	// Set a named property on either this object, or it's prototype.
	virtual void setProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& value
	);

	// Call the underlying object.
	//
	// This function takes a `this` argument, which generally refers to
	// the object that has this property, although, it can be changed by
	// `Function.{apply,call}()`.
	virtual AVM1Value call
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& _this,
		const std::vector<AVM1Value>& args
	) const
	{
		return AVM1Value::Type::Undefined;
	}

	// Construct the underlying object, if it's a valid constructor, and
	// return the result.
	// NOTE: Calling this on something other than a constructor will
	// return a new `Undefined` object.
	virtual AVM1Value construct
	(
		AVM1Activation& activation,
		const std::vector<AVM1Value>& args
	) const
	{
		return AVM1Value::Type::Undefined;
	}

	// Takes an already existing object, and performs construction on it
	// using this constructor (if valid).
	virtual void constructInplace
	(
		AVM1Activation& activation,
		GcPtr<AVM1Object> _this,
		const std::vector<AVM1Value>& args
	) const {}


	// Call a method on this object.
	//
	// It's highly recommended to use this function to perform method
	// calls. It's equivalent to AVM1's `ActionCallMethod`. It'll take
	// care of getting the method, calculating it's base prototype for
	// `super` calls, and providing it with the correct `this` argument.
	virtual AVM1Value callMethod
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const std::vector<AVM1Value>& args,
		const AVM1ExecutionReason& reason
	) const;

	// Get a getter defined on this object.
	virtual NullableGcPtr<AVM1Object> getGetter
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;

	// Get a setter defined on this object.
	virtual NullableGcPtr<AVM1Object> getSetter
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;

	// Construct a host object of some kind, and return it.
	//
	// As the first step in object construction, the native constructor
	// is called on the prototype, to initialize an object. The prototype
	// may construct any object implementation it wants, with itself as
	// the new object's `__proto__`.
	// Then, the constructor is `call()`ed with the new object as it's
	// `this` to initialize the object.
	virtual GcPtr<AVM1Object> createBareObject
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& _this
	) const;

	// Delete a named proprty from this object.
	//
	// Returns `false`, if the property can't be deleted.
	virtual bool deleteProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;

	// Get the `__proto__` of a given object.
	//
	// The prototype is another object used to resolve methods accross
	// a class of multiple objects. It should also be accessable as
	// `__proto__` from `getProp()`.
	virtual AVM1Value getProto(AVM1Activation& activation) const
	{
		return getData(activation, "__proto__");
	}

	// Define a value on an object.
	//
	// Unlike setting a value, this function is intended to replace any
	// existing virtual, or builtin properties already installed on a
	// given object. As such, this shouldn't run any setters; the
	// resulting name slot should either be completely replaced with the
	// value, or completely untouched.
	//
	// It isn't guaranteed that all objects will accept value definitions,
	// especially if a property name conflicts with a builtin, such as
	// `__proto__`.
	virtual void defineValue
	(
		const tiny_string& name,
		const AVM1Value& value,
		const AVM1PropFlags& flags
	);

	// Set the flags of a given property
	//
	// Leaving `name` unspecified allows setting all properties on a
	// given object to the same set of flags.
	// Property flags can be set, cleared, or left as-is, using the pairs
	// of `{set,clear}Flags` arguments.
	virtual void setPropFlags
	(
		const Optional<tiny_string>& name,
		const AVM1PropFlags& setFlags,
		const AVM1PropFlags& clearFlags
	);

	// Define a virtual property onto a given object.
	//
	// A virtual property is a set of `{g,s}et()` functions that are
	// called when a given named property is retrieved, or stored on an
	// object. These functions are then responsible for providing, or
	// accepting the value that's given to, or taken from the AVM.
	//
	// It isn't guaranteed that all objects will accept virtual
	// properties, especially if a property name conflicts with a
	// builtin, such as `__proto__`.
	virtual void addProp
	(
		const tiny_string& name,
		const GcPtr<AVM1Object>& getter,
		const NullableGcPtr<AVM1Object>& setter,
		const AVM1PropFlags& flags
	);

	// Define a virtual property onto a given object.
	//
	// A virtual property is a set of `{g,s}et()` functions that are
	// called when a given named property is retrieved, or stored on an
	// object. These functions are then responsible for providing, or
	// accepting the value that's given to, or taken from the AVM.
	//
	// It isn't guaranteed that all objects will accept virtual
	// properties, especially if a property name conflicts with a
	// builtin, such as `__proto__`.
	virtual void addPropWithCase
	(
		const tiny_string& name,
		const GcPtr<AVM1Object>& getter,
		const NullableGcPtr<AVM1Object>& setter,
		const AVM1PropFlags& flags
	);

	// Calls the `watcher` of a given property, if it exists.
	virtual void callWatcher
	(
		AVM1Activation& activation,
		const tiny_string& name,
		AVM1Value& value,
		const GcPtr<AVM1Object>& _this,
	) const;

	// Set the `watcher` of a given property.
	//
	// The property doesn't need to exist at the time of this being
	// called.
	virtual void watchProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const GcPtr<AVM1Object>& callback,
		const AVM1Value& userData
	);

	// Remove any unassigned `watcher` from the given property.
	//
	// The return value indeicates if there was a watcher present before
	// this method was called.
	virtual bool unwatchProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	);

	// Checks if the object has a given named property.
	virtual bool hasProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;

	// Checks if the object has a given named property on itself (and
	// not, say, the object's prototype, or super class).
	virtual bool hasOwnProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;

	// Checks if the object has a given named property on itself that's
	// virtual.
	virtual bool hasOwnVirtual
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;

	// Checks if a named property will appear when enumerating the
	// object.
	virtual bool isPropEnumerable
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const;

	// Enumerate the object.
	virtual std::vector<tiny_string> getKeys
	(
		AVM1Activation& activation,
		bool includeHidden
	) const;

	// Enumerates all interfaces implemented by this object.
	virtual std::vector<GcPtr<AVM1Object>> getInterfaces() const
	{
		return interfaces;
	}

	// Sets the interface list for this object. (Only useful for
	// prototypes).
	virtual void setInterfaces
	(
		const GcContext& ctx,
		const std::vector<GcPtr<AVM1Object>>& ifaces
	)
	{
		interfaces = ifaces;
	}


	// Determine if this object is an instance of a class.
	//
	// The class is provided in the form of it's constructor function,
	// and the explicit prototype of that constructor. It's assumed that
	// they're already linked.
	//
	// Because ActionScript 2 added interfaces, this function can't
	// simply check the prototype chain, and be done with it. Each
	// interface represents a new, parallel prototype chain, which also
	// needs to be checked. You can't implement nested interfaces
	// (thankfully), but if you could, this'd support that too.
	virtual bool isInstanceOf
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& ctor,
		const GcPtr<AVM1Object>& prototype
	) const;

	template<typename T>
	bool is() const { return false; }

	template<typename T>
	T* as() const
	{
		return is<T>() ? static_cast<T*>(this) : nullptr;
	}

	// Check if this object is in the prototype chain of the supplied
	// object.
	virtual bool isPrototypeOf
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& other
	) const;

	// Gets the length of this object, as if it were an array.
	virtual size_t getLength(AVM1Activation& activation) const;

	// Sets the length of this object, as if it were an array.
	virtual void setLength(AVM1Activation& activation, size_t length) const;

	// Checks if this object as an element
	virtual bool hasElement(AVM1Activation& activation, size_t idx) const;

	// Gets a property of this object, as if it were an array.
	virtual AVM1Value getElement(AVM1Activation& activation, size_t idx) const;

	// Sets a property of this object, as if it were an array.
	virtual void setElement
	(
		AVM1Activation& activation,
		size_t idx,
		const AVM1Value& value
	) const;

	// Deletes a property of this object, as if it were an array.
	virtual bool deleteElement(AVM1Activation& activation, size_t idx) const;
};

// Perform a prototype lookup of a given object.
//
// This function returns both the `Value`, and the prototype depth that
// it was grabbed from. If the property didn't resolve, then it returns
// an empty `Optional`.
//
// The protype depth can, and should be used to populate the `depth`
// argument, which is necessary to make `super` work.
Optional<std::pair<AVM1Value, uint8_t>> searchPrototype
(
	AVM1Activation& activation,
	const AVM1Value& proto,
	const tiny_string& name,
	const GcPtr<AVM1Object>& _this,
	bool isSlashPath
);

// Finds the appropriate `__resolve()` method for an object, while also
// searching it's hierarchy too.
Optional<AVM1Value> findResolveMethod
(
	AVM1Activation& activation,
	const AVM1Value& proto
);

}
#endif /* SCRIPTING_AVM1_OBJECT_OBJECT_H */
