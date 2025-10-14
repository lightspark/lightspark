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

#ifndef SCRIPTING_AVM1_OBJECT_ARRAY_H
#define SCRIPTING_AVM1_OBJECT_ARRAY_H 1

#include <vector>

#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::object::array_object::ArrayObject`.

namespace lightspark
{

class tiny_string;
class AVM1Activation;
class AVM1Value;
class GcContext;
template<typename T>
class GcPtr;

class AVM1Array : public AVM1Object
{
public:
	AVM1Array(AVM1Activation& activation);

	AVM1Array
	(
		const GcContext& ctx,
		const GcPtr<AVM1Object>& arrayProto,
		const std::vector<AVM1Value>& elems = {}
	);

	void setLocalProp
	(
		AVM1Activation& activation,
		const tiny_string& name,
		const AVM1Value& value,
		const GcPtr<AVM1Object>& _this
	) override;

	void setLength(AVM1Activation& activation, ssize_t length) override;

	void setElement
	(
		AVM1Activation& activation,
		ssize_t idx,
		const AVM1Value& value
	) override;
};

}
#endif /* SCRIPTING_AVM1_OBJECT_ARRAY_H */
