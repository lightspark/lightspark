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

#ifndef SCRIPTING_AVM1_OBJECT_SUPER_H
#define SCRIPTING_AVM1_OBJECT_SUPER_H 1

#include "gc/ptr.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::object::super_object::SuperObject`.

namespace lightspark
{

class tiny_string;
class AVM1Activation;
enum AVM1ExecutionReason;
class AVM1Value;

// Implementation of AS2's `super` object.
//
// An `AVM1SuperObject` references all data from another object, but with
// a single layer of prototyping removed. It's as if the given `Object`
// was constructed with it's parent class.
class AVM1SuperObject : public AVM1Object
{
private:
	// The `Object` present as `this` throughout the super chain.
	GcPtr<AVM1Object> _this;

	// The prototype depth of the currently executing method.
	uint8_t depth;

	// Adds a niche, so that enums containing this type can use it for
	// their discriminant.
	uint8_t _niche;
public:
	// Construct a `super` for an incoming stack frame.
	AVM1SuperObject
	(
		const GcPtr<AVM1Object>& thisObj,
		uint8_t _depth
	) : _this(thisObj), depth(_depth), _niche(0) {}

	const GcPtr<AVM1Object>& getThis() const { return _this; }
	uint8_t getDepth() const { return depth; }

	const GcPtr<AVM1Object>& getBaseProto(AVM1Activation& activation) const;

	Optional<AVM1Value> getLocalProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		bool isSlashPath
	) const override;

	void setLocalProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& value,
		const GcPtr<AVM1Object>& _this
	) override
	{
		// NOTE: `super` can't have properties set on it.
		// TODO: What happens if you set `super.__proto__`?
	}

	AVM1Value call
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& _this,
		const std::vector<AVM1Value>& args
	) const override;

	AVM1Value callMethod
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const std::vector<AVM1Value>& args,
		const AVM1ExecutionReason& reason
	) const override;

	NullableGcPtr<AVM1Object> getGetter
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const override
	{
		return _this->getGetter(activation, name);
	}

	NullableGcPtr<AVM1Object> getSetter
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const override
	{
		return _this->getSetter(activation, name);
	}

	bool deleteProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const override
	{
		// NOTE: `super` can't have properties deleted from it.
		return false;
	}

	AVM1Value getProto(AVM1Activation& activation) const override;

	// NOTE: `super` can't have properties defined on it.
	void defineValue
	(
		const tiny_string& name,
		const AVM1Value& value,
		const AVM1PropFlags& flags
	) override
	{
		// NOTE: `super` can't have properties defined on it.
	}

	void addProp
	(
		const tiny_string& name,
		const GcPtr<AVM1Object>& getter,
		const NullableGcPtr<AVM1Object>& setter,
		const AVM1PropFlags& flags
	) override
	{
		// NOTE: `super` can't have properties defined on it.
	}

	void addPropWithCase
	(
		const tiny_string& name,
		const GcPtr<AVM1Object>& getter,
		const NullableGcPtr<AVM1Object>& setter,
		const AVM1PropFlags& flags
	) override
	{
		// NOTE: `super` can't have properties defined on it.
	}

	void callWatcher
	(
		AVM1Activation& activation,
		const tiny_string& name,
		AVM1Value& value,
		const GcPtr<AVM1Object>& thisObj,
	) const override
	{
		_this->callWatcher(activation, name, value, thisObj);
	}

	void watchProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const GcPtr<AVM1Object>& callback,
		const AVM1Value& userData
	) override
	{
		// NOTE: `super` can't have properties defined on it.
	}

	bool unwatchProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) override
	{
		// NOTE: `super` can't have properties defined on it.
		return false;
	}

	bool hasProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const override
	{
		// NOTE: `super` forwards property membership tests to it's
		// underlying object.
		return _this->hasProp(activation, name);
	}

	bool hasOwnProp
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const override
	{
		// NOTE: `super` forwards property membership tests to it's
		// underlying object, even though it can't be enumerated.
		return _this->hasOwnProp(activation, name);
	}

	bool hasOwnVirtual
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const override
	{
		// NOTE: `super` forwards property membership tests to it's
		// underlying object, even though it can't be enumerated.
		return _this->hasOwnVirtual(activation, name);
	}

	bool isPropEnumerable
	(
		AVM1Activation& activation,
		const tiny_string& name
	) const override
	{
		// NOTE: `super` forwards property membership tests to it's
		// underlying object, even though it can't be enumerated.
		return _this->isPropEnumerable(activation, name);
	}

	GetKeysType getKeys
	(
		AVM1Activation& activation,
		bool includeHidden
	) const override
	{
		return {};
	}

	std::vector<GcPtr<AVM1Object>> getInterfaces() const override
	{
		// NOTE: `super` doesn't implement interfaces.
		return {};
	}

	void setInterfaces(const std::vector<GcPtr<AVM1Object>>& ifaces) override
	{
		// NOTE: `super` probably can't have interfaces set on it.
	}

	ssize_t getLength(AVM1Activation& activation) const override
	{
		return 0;
	}

	void setLength(AVM1Activation& activation, ssize_t length) override
	{
		// NOTE: `super` can't have properties set on it.
	}

	bool hasElement(AVM1Activation& activation, ssize_t idx) const override
	{
		return false;
	}

	AVM1Value getElement(AVM1Activation& activation, ssize_t idx) const override;

	void setElement
	(
		AVM1Activation& activation,
		ssize_t idx,
		const AVM1Value& value
	) override
	{
		// NOTE: `super` can't have properties set on it.
	}

	bool deleteElement(AVM1Activation& activation, ssize_t idx) override
	{
		return false;
	}
};

}
#endif /* SCRIPTING_AVM1_OBJECT_SUPER_H */
