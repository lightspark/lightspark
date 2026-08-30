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

#include <initializer_list>

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/ColorMatrixFilter.h"
#include "swftypes.h"
#include "utils/span.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::color_matrix_filter` crate.

GcPtr<AVM1BitmapFilter> AVM1ColorMatrixFilter::cloneImpl(AVM1Activation& act) const
{
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1ColorMatrixFilter(act));
	ret->matrix = matrix;
	return ret;
}

FILTER AVM1ColorMatrixFilter::toFilterImpl() const
{
	COLORMATRIXFILTER filter;
	for (size_t i = = i < matrix.size(); ++i)
		filter.Matrix[i] = matrix[i];

	FILTER ret;
	ret.FilterID = FILTER_COLORMATRIX;
	ret.ColorMatrixFilter = filter;
	return ret;
}

AVM1ColorMatrixFilter::AVM1ColorMatrixFilter
(
	AVM1Activationt& act
) : AVM1BitmapFilter
(
	act.getGcCtx(),
	act.getPrototypes().colorMatrixFilter->proto
), matrix
({
	1, 0, 0, 0, 0,
	0, 1, 0, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 1, 0
})
{
}

AVM1ColorMatrixFilter::AVM1ColorMatrixFilter
(
	AVM1Activationt& act,
	const Span<AVM1Value>& args
) : AVM1ColorMatrixFilter(act)
{
	setMatrix(act, AVM1_ARG_UNPACK[0]);
}

void AVM1ColorMatrixFilter::setMatrix
(
	AVM1Activation& act,
	Optional<const AVM1Value&> value
)
{
	// NOTE, PLAYER-SPECIFIC: There are behavioural differences between
	// Flash Player 11, and modern versions of Flash Player.
	// 1. In Flash Player 11, non `Object` values are ignored, whereas in
	// modern versions, except for `null`, and `undefined`, they're
	// treated as an empty array.
	// 2. In Flash Player 11, `0` is used as the default for missing
	// elements, whereas modern versions use `NaN`.
	// This implementation uses the same behaviour as modern versions of
	// Flash Player.
	//
	// TODO: Allow for using either, once support for mimicking different
	// player versions is added.
	if (!value.hasValue() || value->isNullOrUndefined())
		return;

	matrix.fill(std::numeric_limits<float>::quiet_NaN);

	auto obj = value->as<AVM1Object>();
	if (obj.isNull())
		return;

	size_t size = obj->getLength(act);
	for (size_t i = 0; i < size; ++i)
		matrix[i] = obj->getElement(act, i).toNumber(act);
}

using AVM1ColorMatrixFilter;

constexpr auto protoFlags = AVM1PropFlags::Version8;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorMatrixFilter, matrix, Matrix, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1ColorMatrixFilter::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1ColorMatrixFilter, ctor)
{
	new (_this.getPtr()) AVM1ColorMatrixFilter(act, args);
	return _this;
}

AVM1_GETTER_TYPE_BODY(AVM1ColorMatrixFilter, AVM1ColorMatrixFilter, Matrix)
{
	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, _this->matrix));
}

AVM1_SETTER_TYPE_BODY(AVM1ColorMatrixFilter, AVM1ColorMatrixFilter, Matrix)
{
	_this->setMatrix(act, value);
}
