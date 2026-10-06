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

#ifndef SCRIPTING_AVM1_TOPLEVEL_MATH_H
#define SCRIPTING_AVM1_TOPLEVEL_MATH_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::globals::math` crate.

namespace lightspark
{

class AVM1DeclContext;

class AVM1Math : public AVM1Object
{
public:
	AVM1Math(AVM1DeclContext& ctx);

	AVM1_FUNCTION_DECL(abs);
	AVM1_FUNCTION_DECL(min);
	AVM1_FUNCTION_DECL(max);
	AVM1_FUNCTION_DECL(sin);
	AVM1_FUNCTION_DECL(cos);
	AVM1_FUNCTION_DECL(atan2);
	AVM1_FUNCTION_DECL(tan);
	AVM1_FUNCTION_DECL(exp);
	AVM1_FUNCTION_DECL(log);
	AVM1_FUNCTION_DECL(sqrt);
	AVM1_FUNCTION_DECL(round);
	AVM1_FUNCTION_DECL(random);
	AVM1_FUNCTION_DECL(floor);
	AVM1_FUNCTION_DECL(ceil);
	AVM1_FUNCTION_DECL(atan);
	AVM1_FUNCTION_DECL(asin);
	AVM1_FUNCTION_DECL(acos);
	AVM1_FUNCTION_DECL(pow);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_MATH_H */
