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
#include "scripting/avm1/toplevel/ConvolutionFilter.h"
#include "swftypes.h"
#include "utils/span.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::convolution_filter` crate.

GcPtr<AVM1BitmapFilter> AVM1ConvolutionFilter::cloneImpl(AVM1Activation& act) const
{
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1ConvolutionFilter(act));
	ret->divisor = divisor;
	ret->bias = bias;
	ret->matrix = matrix;
	ret->matrixSize = matrixSize;
	ret->color = color;
	ret->clamp = clamp;
	ret->preserveAlpha = preserveAlpha;
	return ret;
}

FILTER AVM1ConvolutionFilter::toFilterImpl() const
{
	CONVOLUTIONFILTER filter;
	filter.Divisor = divisor;
	filter.Bias = bias;
	if (matrixSize > Vector2u8())
		filter.Matrix = new FLOAT[matrixSize.x * matrixSize.y];
	for (size_t i = 0; i < matrixSize.x * matrixSize.y; ++i)
		filter.Matrix[i] = matrix[i];

	filter.DefaultColor = color;
	filter.MatrixX = matrixSize.x;
	filter.MatrixY = matrixSize.y;
	filter.Clamp = clamp;
	filter.PreserveAlpha = preserveAlpha;

	FILTER ret;
	ret.FilterID = FILTER_CONVOLUTION;
	ret.ConvolutionFilter = filter;
	return ret;
}

void AVM1ConvolutionFilter::setMatrix
(
	AVM1Activation& act,
	Optional<const AVM1Value&> value
)
{
	if (!value.hasValue())
		return;

	// NOTE, PLAYER-SPECIFIC: In Flash Player 11, only "true" `Object`s
	// resize the matrix, but in modern versions, `String`s will also
	// resize the matrix (which'll cause the matrix to be filled with
	// `NaN`s, since they have a `length`, but no elements).
	auto obj = value->toObject(act);
	size_t size = imax(obj->getLength(act), 0);

	matrix = std::vector<number_t>(0, size);

	for (size_t i = 0; i < size; ++i)
		matrix[i] = obj->getElement(act, i).toNumber(act);
	resizeMatrix();
}

void AVM1ConvolutionFilter::setAlpha
(
	AVM1Activation& act,
	Optional<const AVM1Value&> value
)
{
	if (!value.hasValue())
		return;

	auto num = value->toNumber(act);
	// NOTE: `dclamp()` isn't used here because we want to always return
	// the smallest value between `0`, and `1`, but if the value is `NaN`,
	// then `dclamp()` will propagate the `NaN`, rather than returning
	// either `0`, or `1`.
	// So, clamp the value using `fm{in,ax}()` instead.
	color.Alpha = std::fmin(std::fmax(num, 0.0), 1.0) * 255;
}

AVM1ConvolutionFilter::AVM1ConvolutionFilter
(
	AVM1Activationt& act
) : AVM1BitmapFilter
(
	act.getGcCtx(),
	act.getPrototypes().convolutionFilter->proto
),
divisor(1),
bias(0),
matrixSize(0, 0),
color(0, 0),
clamp(true),
preserveAlpha(true)
{
}

AVM1ConvolutionFilter::AVM1ConvolutionFilter
(
	AVM1Activationt& act,
	const Span<AVM1Value>& args
) : AVM1ConvolutionFilter(act)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	int32_t matrixX;
	int32_t matrixY;
	unpacker(matrixX, 0)(matrixY, 0);

	_this->setMatrixSize
	(
		iclamp(matrixX, 0, 15),
		iclamp(matrixY, 0, 15)
	);

	setMatrix(unpacker.tryUnpack());
	divisor = unpacker.tryUnpack<float>().orElseIf(!args.empty(), [&]
	{
		return std::accumulate
		(
			matrix.begin(),
			matrix.end(),
			0.0
		);
	}).valueOr(1);

	unpacker(bias, 0)(preserveAlpha, true)(clamp, true);
	color = unpacker.tryUnpack
	<
		uint32_t
	>().transformOr(RGBA(0, 0), [&](uint32_t val)
	{
		// NOTE: If the `color` argument is supplied in the AVM1 ctor,
		// then `alpha` is set to `1`, despite the AS1/2 docs stating
		// otherwise.
		// This doesn't happen in AVM2.
		return RGBA(val, 255);
	});
	
	setAlpha(act, unpacker.tryUnpack());
}

using AVM1ConvolutionFilter;

constexpr auto protoFlags = AVM1PropFlags::Version8;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_TYPE_PROTO(AVM1ConvolutionFilter, matrixX, MatrixX, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ConvolutionFilter, matrixY, MatrixY, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ConvolutionFilter, divisor, Divisor, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ConvolutionFilter, bias, Bias, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ConvolutionFilter, preserveAlpha, PreserveAlpha, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ConvolutionFilter, clamp, Clamp, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ConvolutionFilter, color, Color, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ConvolutionFilter, alpha, Alpha, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1ConvolutionFilter::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1ConvolutionFilter, ctor)
{
	new (_this.getPtr()) AVM1ConvolutionFilter(act, args);
	return _this;
}

AVM1_GETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, MatrixX)
{
	return number_t(_this->matrixSize.x);
}

AVM1_SETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, MatrixX)
{
	_this->setMatrixSize
	(
		iclamp(value.toInt32(act), 0, 255),
		_this->matrixSize.y
	);
}

AVM1_GETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, MatrixY)
{
	return number_t(_this->matrixSize.y);
}

AVM1_SETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, MatrixY)
{
	_this->setMatrixSize
	(
		_this->matrixSize.x,
		iclamp(value.toInt32(act), 0, 255)
	);
}

AVM1_GETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Matrix)
{
	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, _this->matrix));
}

AVM1_SETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Matrix)
{
	_this->setMatrix(act, value);
}

AVM1_GETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Divisor)
{
	return _this->divisor;
}

AVM1_SETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Divisor)
{
	_this->divisor = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Bias)
{
	return _this->bias;
}

AVM1_SETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Bias)
{
	_this->bias = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, PreserveAlpha)
{
	return _this->preserveAlpha;
}

AVM1_SETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, PreserveAlpha)
{
	_this->preserveAlpha = value.toBool(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Clamp)
{
	return _this->clamp;
}

AVM1_SETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Clamp)
{
	_this->clamp = value.toBool(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Color)
{
	return number_t(_this->color.toRGB().toUInt());
}

AVM1_SETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Color)
{
	_this->color = RGBA
	(
		value.toUInt32(act),
		_this->color.Alpha
	);
}

AVM1_GETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Alpha)
{
	return number_t(_this->color.Alpha) / 255;
}

AVM1_SETTER_TYPE_BODY(AVM1ConvolutionFilter, AVM1ConvolutionFilter, Alpha)
{
	_this->setAlpha(act, value);
}
