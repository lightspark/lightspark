/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_TOPLEVEL_TRANSFORM_H
#define SCRIPTING_AVM1_TOPLEVEL_TRANSFORM_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"

// Based on Ruffle's `avm1::globals::transform` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1MovieClipRef;
class AVM1SystemClass;
class AVM1Value;
class DisplayObject;

#define TRANSFORM_PROP_DECL(name) AVM1_PROPERTY_TYPE_DECL \
( \
	AVM1Transform, \
	name, \
	GcPtr<DisplayObject> clip \
)

class AVM1Transform : public AVM1Object
{
private:
	NullableGcPtr<AVM1MovieClipRef> clip;
public:
	AVM1Transform
	(
		AVM1Activation& act,
		const NullableGcPtr<AVM1MovieClipRef>& _clip = NullGc
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	NullableGcPtr<DisplayObject> getClip() const;

	AVM1_FUNCTION_DECL(ctor);
	TRANSFORM_PROP_DECL(Matrix);
	TRANSFORM_PROP_DECL(ConcatenatedMatrix);
	TRANSFORM_PROP_DECL(ColorTransform);
	TRANSFORM_PROP_DECL(ConcatenatedColorTransform);
	TRANSFORM_PROP_DECL(PixelBounds);
};

#undef TRANSFORM_PROP_DECL

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_TRANSFORM_H */
