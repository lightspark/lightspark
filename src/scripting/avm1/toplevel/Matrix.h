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

#ifndef SCRIPTING_AVM1_TOPLEVEL_MATRIX_H
#define SCRIPTING_AVM1_TOPLEVEL_MATRIX_H 1

#include "forwards/swftypes.h"
#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "utils/span.h"

// Based on Ruffle's `avm1::globals::matrix` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1Object;
class AVM1Value;

class AVM1Matrix
{
public:
	static MATRIX fromValue
	(
		AVM1Activation& act,
		const AVM1Value& value
	);

	static MATRIX fromGradientObject
	(
		AVM1Activation& act,
		GcPtr<AVM1Object> obj
	);

	static MATRIX fromObject
	(
		AVM1Activation& act,
		GcPtr<AVM1Object> obj
	);

	static MATRIX fromObjectOrIdent
	(
		AVM1Activation& act,
		GcPtr<AVM1Object> obj
	);

	static AVM1Value toValue
	(
		AVM1Activation& act,
		const MATRIX& mtx
	);

	static void applyToObject
	(
		AVM1Activation& act,
		GcPtr<AVM1Object> obj,
		const MATRIX& mtx
	);

	static MATRIX createBox
	(
		const Vector2f& scale,
		number_t rotation,
		const Vector2f& offset
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		GcPtr<AVM1Object> superProto
	);

	AVM1_FUNCTION_DECL(ctor);

	AVM1_FUNCTION_DECL(concat);
	AVM1_FUNCTION_DECL(invert);
	AVM1_FUNCTION_DECL(createBox);
	AVM1_FUNCTION_DECL(createGradientBox);
	AVM1_FUNCTION_DECL(clone);
	AVM1_FUNCTION_DECL(identity);
	AVM1_FUNCTION_DECL(rotate);
	AVM1_FUNCTION_DECL(translate);
	AVM1_FUNCTION_DECL(scale);
	AVM1_FUNCTION_DECL(deltaTransformPoint);
	AVM1_FUNCTION_DECL(transformPoint);
	AVM1_FUNCTION_DECL(toString);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_MATRIX_H */
