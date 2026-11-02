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

#ifndef SCRIPTING_AVM1_TOPLEVEL_COLOR_H
#define SCRIPTING_AVM1_TOPLEVEL_COLOR_H 1

#include "scripting/avm1/function.h"

// Based on Ruffle's `avm1::globals::color` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1Context;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Object;
class AVM1Value;
class DisplayObject;
class GcContext;

class AVM1Color
{
public:
	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);


	AVM1_FUNCTION_DECL(ctor);
	// Gets the target `DisplayObject` of this `Color`.
	static NullableGcPtr<DisplayObject> getTarget(AVM1_GETTER_ARGS);
	AVM1_PROPERTY_DECL(RGB);
	AVM1_PROPERTY_DECL(Transform);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_COLOR_H */
