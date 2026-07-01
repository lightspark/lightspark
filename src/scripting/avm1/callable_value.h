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

#ifndef SCRIPTING_AVM1_CALLABLE_VALUE_H
#define SCRIPTING_AVM1_CALLABLE_VALUE_H 1

#include <vector>

#include "gc/ptr.h"
#include "scripting/avm1/value.h"

// Based on Ruffle's `avm1::callable_value::CallableValue`.

namespace lightspark
{

class tiny_string;
class AVM1Activation;
class AVM1Object;

class AVM1CallableValue
{
public:
	virtual ~AVM1CallableValue() = 0;
	virtual bool isCallable() const = 0;
	virtual const AVM1Value& getValue() const = 0;

	virtual AVM1Value callWithDefaultThis
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& defaultThis,
		const tiny_string& name,
		const std::vector<AVM1Value>& args
	) = 0;

	operator const AVM1Value&() const { return getValue(); }
};

class AVM1UnCallable : public AVM1CallableValue
{
private:
	AVM1Value value;
public:
	AVM1UnCallable(const AVM1Value& _value) : value(_value) {}
	bool isCallable() const override { return false; }
	const AVM1Value& getValue() const override { return value; }

	AVM1Value callWithDefaultThis
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& defaultThis,
		const tiny_string& name,
		const std::vector<AVM1Value>& args
	) override;
};

class AVM1Callable : public AVM1CallableValue
{
private:
	GcPtr<AVM1Object> _this;
	AVM1Value value;
public:
	AVMCallable
	(
		const GcPtr<AVM1Object>& thisObj,
		const AVM1Value& _value
	) : _this(thisObj), value(_value) {}

	bool isCallable() const override { return true; }
	const AVM1Value& getValue() const override { return value; }

	AVM1Value callWithDefaultThis
	(
		AVM1Activation& activation,
		const GcPtr<AVM1Object>& defaultThis,
		const tiny_string& name,
		const std::vector<AVM1Value>& args
	) override;
};

}
#endif /* SCRIPTING_AVM1_CALLABLE_VALUE_H */
