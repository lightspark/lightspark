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

#include "scripting/avm1/activation.h"
#include "scripting/avm1/callable_value.h"
#include "scripting/avm1/object/object.h"
#include "tiny_string.h"

using namespace lightspark;

// Based on Ruffle's `avm1::callable_value::CallableValue`.

AVM1Value AVM1UnCallable::callWithDefaultThis
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& defaultThis,
	const tiny_string& name,
	const std::vector<AVM1Value>& args
)
{
	if (!value.is<AVM1Object>())
		return AVM1Value::undefinedVal;
	return value.as<AVM1Object>()->call(act, name, defaultThis, args);
}

AVM1Value AVM1Callable::callWithDefaultThis
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& defaultThis,
	const tiny_string& name,
	const std::vector<AVM1Value>& args
)
{
	if (!value.is<AVM1Object>())
		return AVM1Value::undefinedVal;
	return value.as<AVM1Object>()->call(act, name, _this, args);
}
