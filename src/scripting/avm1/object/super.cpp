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

#include "gc/context.h"
#include "gc/ptr.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/object/super.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/scope.h"
#include "scripting/avm1/value.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::object::super_object::SuperObject`.


const GcPtr<AVM1Object>& AVM1SuperObject::getBaseProto
(
	AVM1Activation& activation
) const
{
	auto proto = _this;
	for (size_t i = 0; i < depth; ++i)
		proto = proto->getProto(activation);
	return proto;
}

Optional<AVM1Value> AVM1SuperObject::getLocalProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	bool isSlashPath
) const
{
	return {};
}

AVM1Value AVM1SuperObject::call
(
	AVM1Activation& act,
	const tiny_string& name,
	const AVM1Value&,
	const std::vector<AVM1Value>& args
) const
{
	return getBaseProto(act)->getProp(act, "__constructor__")->tryAs
	<
		AVM1FunctionObject
	>().transformOr(AVM1Value::undefinedVal, [&](const auto& ctor)
	{
		return ctor.asCtor()->exec
		(
			act,
			name,
			_this,
			depth,
			args,
			AVM1ExecutionReason::FunctionCall,
			ctor
		);
	});
}

AVM1Value AVM1SuperObject::callMethod
(
	AVM1Activation& activation,
	const tiny_string& name,
	const std::vector<AVM1Value>& args,
	const AVM1ExecutionReason& reason
) const
{
	auto pair = searchPrototype
	(
		activation,
		getProto(activation),
		_this,
		false
	);

	if (!pair.hasValue())
		return AVM1Value::undefinedVal;

	GcPtr<AVM1Object> method = pair->first;
	auto _depth = pair->second;

	if (!method->is<AVM1Executable>())
		return method->call(activation, name, _this);

	return method->as<AVM1Executable>()->exec
	(
		activation,
		name,
		_this,
		depth + _depth,
		args,
		reason,
		method
	);
}

AVM1Value AVM1SuperObject::getProto(AVM1Activation& activation) const
{
	return getBaseProto(activation)->getProto(activation);
}

AVM1Value AVM1SuperObject::getElement
(
	AVM1Activation& activation,
	ssize_t idx
) const
{
	return AVM1Value::undefinedVal;
}
