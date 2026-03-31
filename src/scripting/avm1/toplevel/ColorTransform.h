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

#ifndef SCRIPTING_AVM1_TOPLEVEL_COLORTRANSFORM_H
#define SCRIPTING_AVM1_TOPLEVEL_COLORTRANSFORM_H 1

#include "backends/colortransformbase.h"
#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"

// Based on Ruffle's `avm1::globals::color_transform` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;

class AVM1ColorTransform : public AVM1Object, public ColorTransformBase
{
public:
	AVM1ColorTransform
	(
		AVM1Activation& act,
		const ColorTransformBase& ct = {}
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_PROPERTY_TYPE_DECL(AVM1ColorTransform, RedMultiplier);
	AVM1_PROPERTY_TYPE_DECL(AVM1ColorTransform, GreenMultiplier);
	AVM1_PROPERTY_TYPE_DECL(AVM1ColorTransform, BlueMultiplier);
	AVM1_PROPERTY_TYPE_DECL(AVM1ColorTransform, AlphaMultiplier);
	AVM1_PROPERTY_TYPE_DECL(AVM1ColorTransform, RedOffset);
	AVM1_PROPERTY_TYPE_DECL(AVM1ColorTransform, GreenOffset);
	AVM1_PROPERTY_TYPE_DECL(AVM1ColorTransform, BlueOffset);
	AVM1_PROPERTY_TYPE_DECL(AVM1ColorTransform, AlphaOffset);
	AVM1_PROPERTY_TYPE_DECL(AVM1ColorTransform, RGB);
	AVM1_FUNCTION_TYPE_DECL(AVM1ColorTransform, concat);
	AVM1_FUNCTION_DECL(toString);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_COLORTRANSFORM_H */
