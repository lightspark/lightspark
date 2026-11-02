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

#ifndef SCRIPTING_AVM1_TOPLEVEL_NUMBER_H
#define SCRIPTING_AVM1_TOPLEVEL_NUMBER_H 1

#include <limits>

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "swftypes.h"

// Based on Ruffle's `avm1::globals::number` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;

class AVM1Number : public AVM1Object
{
	using NumLimits = std::numeric_limits<double>;
private:
	number_t num;
public:
	static constexpr number_t NaN = NumLimits::quiet_NaN();
	static constexpr number_t Infinity = NumLimits::infinity();

	AVM1Number
	(
		const GcContext& ctx,
		GcPtr<AVM1Object> proto,
		number_t _num = 0
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	// Implements the `Number` constructor.
	AVM1_FUNCTION_DECL(ctor);

	// Implements `Number()`.
	AVM1_FUNCTION_DECL(number);

	AVM1_FUNCTION_TYPE_DECL(AVM1Number, toString);
	AVM1_FUNCTION_TYPE_DECL(AVM1Number, valueOf);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_NUMBER_H */
