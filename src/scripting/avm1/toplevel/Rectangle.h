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

#ifndef SCRIPTING_AVM1_TOPLEVEL_RECTANGLE_H
#define SCRIPTING_AVM1_TOPLEVEL_RECTANGLE_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "utils/span.h"

// Based on Ruffle's `avm1::globals::rectangle` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1Object;
class AVM1Value;
struct RectF;

class AVM1Rectangle
{
public:
	static GcPtr<AVM1Object> toObject
	(
		AVM1Activation& act,
		const RectF& rect
	);

	static GcPtr<AVM1Object> makeRect
	(
		AVM1Activation& act,
		const Span<AVM1Value>& args
	);

	static RectF fromValue
	(
		AVM1Activation& act,
		const AVM1Value& value
	);

	static RectF fromObject
	(
		AVM1Activation& act,
		const GcPtr<AVM1Object>& obj
	);

	static Vector2f getMin
	(
		AVM1Activation& act,
		const AVM1Value& value
	);

	static Vector2f getMin
	(
		AVM1Activation& act,
		const GcPtr<AVM1Object>& obj
	);

	static Vector2f getMax
	(
		AVM1Activation& act,
		const AVM1Value& value
	);

	static Vector2f getMax
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
	AVM1_FUNCTION_DECL(clone);
	AVM1_FUNCTION_DECL(setEmpty);
	AVM1_FUNCTION_DECL(isEmpty);
	AVM1_PROPERTY_DECL(Left);
	AVM1_PROPERTY_DECL(Right);
	AVM1_PROPERTY_DECL(Top);
	AVM1_PROPERTY_DECL(Bottom);
	AVM1_PROPERTY_DECL(TopLeft);
	AVM1_PROPERTY_DECL(BottomRight);
	AVM1_PROPERTY_DECL(Size);
	AVM1_FUNCTION_DECL(inflate);
	AVM1_FUNCTION_DECL(inflatePoint);
	AVM1_FUNCTION_DECL(offset);
	AVM1_FUNCTION_DECL(offsetPoint);
	AVM1_FUNCTION_DECL(contains);
	AVM1_FUNCTION_DECL(containsPoint);
	AVM1_FUNCTION_DECL(containsRectangle);
	AVM1_FUNCTION_DECL(intersection);
	AVM1_FUNCTION_DECL(intersects);
	AVM1_FUNCTION_DECL(_union);
	AVM1_FUNCTION_DECL(equals);
	AVM1_FUNCTION_DECL(toString);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_RECTANGLE_H */
