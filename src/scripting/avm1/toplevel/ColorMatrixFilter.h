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

#ifndef SCRIPTING_AVM1_TOPLEVEL_COLORMATRIXFILTER_H
#define SCRIPTING_AVM1_TOPLEVEL_COLORMATRIXFILTER_H 1

#include <array>

#include "backends/geometry.h"
#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/toplevel/BitmapFilter.h"
#include "swftypes.h"

// Based on Ruffle's `avm1::globals::color_matrix_filter` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class FILTER;
class GcContext;
template<typename T>
class Span;
template<typename T>
class Optional;

class AVM1ColorMatrixFilter : public AVM1BitmapFilter
{
private:
	GcPtr<AVM1BitmapFilter> cloneImpl(AVM1Activation& act) const override;
	FILTER toFilterImpl() const override;

	std::array<float, 20> matrix;
public:
	AVM1ColorMatrixFilter(AVM1Activationt& act);
	AVM1ColorMatrixFilter
	(
		AVM1Activationt& act,
		const Span<AVM1Value>& args
	);

	void setMatrix
	(
		AVM1Activation& act,
		Optional<const AVM1Value&> value
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);

	AVM1_PROPERTY_TYPE_DECL(AVM1ColorMatrixFilter, Matrix);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_COLORMATRIXFILTER_H */
