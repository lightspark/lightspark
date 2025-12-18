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

#ifndef SCRIPTING_AVM1_TOPLEVEL_POINT_H
#define SCRIPTING_AVM1_TOPLEVEL_POINT_H 1

#include "forwards/swftypes.h"
#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "utils/span.h"

// Based on Ruffle's `avm1::globals::point` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1Object;
class AVM1Value;

class AVM1Point
{
public:
	static GcPtr<AVM1Object> toObject
	(
		AVM1Activation& act,
		const Vector2f& point
	);

	static GcPtr<AVM1Object> makePoint
	(
		AVM1Activation& act,
		const Span<AVM1Value>& args
	);

	static Vector2f fromValue
	(
		AVM1Activation& act,
		const AVM1Value& value
	);

	static Vector2f fromObject
	(
		AVM1Activation& act,
		const GcPtr<AVM1Object>& obj
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);

	// Prototype methods.
	AVM1_GETTER_DECL(Length);
	AVM1_FUNCTION_DECL(clone);
	AVM1_FUNCTION_DECL(offset);
	AVM1_FUNCTION_DECL(equals);
	AVM1_FUNCTION_DECL(subtract);
	AVM1_FUNCTION_DECL(add);
	AVM1_FUNCTION_DECL(normalize);
	AVM1_FUNCTION_DECL(toString);

	// Object methods.
	AVM1_FUNCTION_DECL(distance);
	AVM1_FUNCTION_DECL(polar);
	AVM1_FUNCTION_DECL(interpolate);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_POINT_H */
