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

#include <sstream>

#include "gc/context.h"
#include "gc/ptr.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/value.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::object::array_object::ArrayObject`.

AVM1Array::AVM1Array(AVM1Activation& activation) : AVM1Array
(
	activation.getGcCtx(),
	activation.getCtx().getPrototypes().array
) {}

AVM1Array::AVM1Array
(
	const GcContext& ctx,
	const GcPtr<AVM1Object>& arrayProto,
	const std::vector<AVM1Value>& elems
) : AVM1Object(ctx, arrayProto)
{
	size_t length = 0;

	for (const auto& elem : elems)
	{
		defineValue
		(
			(std::stringstream() << length++).str(),
			elem,
			AVM1PropFlags(0)
		);
	}

	defineValue
	(
		"length",
		number_t(length),
		AVM1PropFlags::DontEnum | AVM1PropFlags::DontDelete
	);
}

static Optional<ssize_t> parseIndex(const tiny_string& str)
{
	return str.tryToNumber<int32_t>();
}

void AVM1Array::setLocalProp
(
	AVM1Activation& activation,
	const tiny_string& name,
	const AVM1Value& value,
	const GcPtr<AVM1Object>& _this
)
{
	if (name == "length")
	{
		setLength(activation, value.toInt32(activation));
		AVM1Object::setLocalProp(activation, name, value, _this);
		return;
	}

	(void)parseIndex(name).andThen([&](ssize_t idx)
	{
		auto length = getLength(activation);
		if (idx >= length)
			setLength(activation, ++idx);
		return makeOptional(idx);
	});
	AVM1Object::setLocalProp(activation, name, value, _this);
}

void AVM1Array::setLength(AVM1Activation& activation, ssize_t length)
{
	auto oldLen = getData(activation, "length");
	if (!oldLen.is<number_t>())
	{
		AVM1Object::setLength(activation, length);
		return;
	}

	ssize_t _oldLen = oldLen.toInt32(activation);
	for (auto i = std::max<ssize_t>(length, 0); i < _oldLen; ++i)
		deleteElement(activation, i);

	AVM1Object::setLength(activation, length);
}

void AVM1Array::setElement
(
	AVM1Activation& activation,
	ssize_t idx,
	const AVM1Value& value
)
{
	auto length = getLength(activation);
	if (idx >= length)
		setLength(activation, ++idx);

	AVM1Object::setElement(activation, idx, value);
}
