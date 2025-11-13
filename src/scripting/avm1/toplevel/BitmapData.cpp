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

#include "3rdparty/perlinnoise/PerlinNoise.hpp"
#include "backends/graphics.h"
#include "backends/bitmap/bitmap_data.h"
#include "display_object/DisplayObject.h"
#include "display_object/MovieClip.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/BitmapData.h"
#include "scripting/avm1/toplevel/Matrix.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::bitmap_data` crate.

NullableGcPtr<BitmapContainer>& AVM1BitmapData::getContainer()
{
	return data->getContainer();
}

const NullableGcPtr<BitmapContainer>& AVM1BitmapData::getContainer() const
{
	return data->getContainer();
}

bool AVM1BitmapData::isDisposed() const
{
	return data->isDisposed();
}

#define BITMAP_DATA_GETTER(func) BITMAP_DATA_FUNC(get##func)

#define BITMAP_DATA_FUNC(func) [](AVM1_FUNCTION_ARGS) \
{ \
	auto bitmapData = _this->as<AVM1BitmapData>(); \
	if (bitmapData.isNull() || bitmapData->isDisposed()) \
		return -1.0; \
	return func(act, bitmapData, args); \
}

#define BITMAP_DATA_GETTER_PROTO(name, funcName, ...) \
	AVM1Decl(#name, BITMAP_DATA_GETTER(funcName), ##__VA_ARGS__)

#define BITMAP_DATA_FUNCTION_PROTO(name, ...) \
	AVM1Decl(#name, BITMAP_DATA_FUNC(name), ##__VA_ARGS__)

using AVM1BitmapData;

static constexpr auto protoDecls =
{
	BITMAP_DATA_GETTER_PROTO(height, Height),
	BITMAP_DATA_GETTER_PROTO(width, Width),
	BITMAP_DATA_GETTER_PROTO(transparent, Transparent),
	BITMAP_DATA_GETTER_PROTO(rectangle, Rectangle),
	BITMAP_DATA_FUNCTION_PROTO(getPixel),
	BITMAP_DATA_FUNCTION_PROTO(getPixel32),
	BITMAP_DATA_FUNCTION_PROTO(setPixel),
	BITMAP_DATA_FUNCTION_PROTO(setPixel32),
	AVM1_FUNCTION_PROTO(copyChannel),
	AVM1_FUNCTION_PROTO(fillRect),
	BITMAP_DATA_FUNCTION_PROTO(clone),
	BITMAP_DATA_FUNCTION_PROTO(dispose),
	BITMAP_DATA_FUNCTION_PROTO(floodFill),
	AVM1_FUNCTION_PROTO(noise),
	BITMAP_DATA_FUNCTION_PROTO(colorTransform),
	BITMAP_DATA_FUNCTION_PROTO(getColorBoundsRect),
	BITMAP_DATA_FUNCTION_PROTO(perlinNoise),
	BITMAP_DATA_FUNCTION_PROTO(applyFilter),
	BITMAP_DATA_FUNCTION_PROTO(draw),
	BITMAP_DATA_FUNCTION_PROTO(hitTest),
	BITMAP_DATA_FUNCTION_PROTO(generateFilterRect),
	BITMAP_DATA_FUNCTION_PROTO(copyPixels),
	BITMAP_DATA_FUNCTION_PROTO(merge),
	BITMAP_DATA_FUNCTION_PROTO(paletteMap),
	BITMAP_DATA_FUNCTION_PROTO(pixelDissolve),
	BITMAP_DATA_FUNCTION_PROTO(scroll),
	BITMAP_DATA_FUNCTION_PROTO(threshold),
	AVM1_FUNCTION_PROTO(compare)
};

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(loadBitmap);
};

#undef BITMAP_DATA_GETTER
#undef BITMAP_DATA_FUNC
#undef BITMAP_DATA_GETTER_PROTO
#undef BITMAP_DATA_FUNCTION_PROTO

GcPtr<AVM1SystemClass> AVM1BitmapData::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	ctx.definePropsOn(_class->proto, objDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1BitmapData, ctor)
{
	std::pair<uint32_t, uint32_t> _size;
	bool transparency;
	uint32_t fillColor;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(_size)
	(
		transparency,
		true
	)(fillColor, UINT32_MAX));

	Vector2u size(_size.first, _size.second);

	if (!isSizeValid(act.getSwfVersion(), size))
	{
		LOG
		(
			LOG_ERROR,
			"AVM1BitmapData::ctor(): "
			"Invalid `BitmapData` size: " << size.x << 'x' <<
			size.y
		);
		return AVM1Value::undefinedVal;
	}

	data = NEW_GC_PTR(act.getGcCtx(), BitmapData
	(
		act.getGcCtx(),
		size,
		transparency,
		fillColor
	));
	return _this;
}

BITMAP_DATA_GETTER_BODY(Height)
{
	return number_t(_this->data->getHeight());
}

BITMAP_DATA_GETTER_BODY(Width)
{
	return number_t(_this->data->getHeight());
}

BITMAP_DATA_GETTER_BODY(Transparent)
{
	return _this->data->isTransparent();
}

BITMAP_DATA_GETTER_BODY(Rectangle)
{
	auto ctor = act.getPrototypes().rectangle->ctor;
	return ctor->construct(act,
	{
		0.0,
		0.0,
		_this->data->getWidth(),
		_this->data->getHeight()
	});
}

BITMAP_DATA_FUNCTION_BODY(getPixel)
{
	Vector2u pos;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(pos.x)(pos.y), -1.0);
	auto container = _this->getContainer();
	container->flushRenderCalls(act.getSys()->getRenderThread());
	int32_t ret = container->getPixel(pos, false, false);
	return number_t(ret);
}

BITMAP_DATA_FUNCTION_BODY(getPixel32)
{
	Vector2u pos;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(pos.x)(pos.y), -1.0);
	auto container = _this->getContainer();
	container->flushRenderCalls(act.getSys()->getRenderThread());
	int32_t ret = container->getPixel(pos, true, false);
	return number_t(ret);
}

BITMAP_DATA_FUNCTION_BODY(setPixel)
{
	Vector2u pos;
	uint32_t color;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(pos.x)(pos.y)(color), -1.0);
	auto container = _this->getContainer();
	container->setPixel(pos, color, false, false);
	_this->data->notifyUsers();
	return AVM1Value::undefinedVal;
}

BITMAP_DATA_FUNCTION_BODY(setPixel32)
{
	Vector2u pos;
	uint32_t color;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(pos.x)(pos.y)(color), -1.0);
	auto data = _this->data;
	auto container = _this->getContainer();
	container->setPixel(pos, color, data->isTransparent(), false);
	data->notifyUsers();
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1BitmapData, copyChannel)
{
	NullableGcPtr<AVM1Object> rectObj;
	NullableGcPtr<AVM1BitmapData> source;
	NullableGcPtr<AVM1Object> sourceRectObj;
	NullableGcPtr<AVM1Object> destPointObj;
	int32_t sourceChannel;
	int32_t destChannel;
	AVM1_ARG_UNPACK(source).unpack<GcPtr<AVM1Object>>
	(
		sourceRectObj
	).unpack<GcPtr<AVM1Object>>
	(
		destPointObj
	)(sourceChannel)(destChannel);

	auto data = _this->as<BitmapData>();
	if (data.isNull())
		return -1.0;

	if (data->isDisposed() || source.isNull())
		return AVM1Value::undefinedVal;

	// TODO: What happens, if `source` is disposed?
	Vector2 destPoint
	(
		destPointObj->getProp(act, "x").toInt32(act),
		destPointObj->getProp(act, "y").toInt32(act)
	);

	RECT sourceRect
	(
		sourceRectObj->getProp(act, "x").toInt32(act),
		sourceRectObj->getProp(act, "y").toInt32(act),
		sourceRectObj->getProp(act, "width").toInt32(act),
		sourceRectObj->getProp(act, "height").toInt32(act)
	);

	data->copyChannel
	(
		sourceBitmap->getData(),
		destPoint,
		sourceRect,
		sourceChannel,
		destChannel
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1BitmapData, fillRect)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);
	auto rectObj = unpacker.unpack<GcPtr<AVM1Object>>();

	auto data = _this->as<BitmapData>()
	if (data.isNull() || data->isDisposed())
		return -1.0;

	uint32_t color;
	AVM1_ARG_CHECK(unpacker(color));

	RECT rect
	(
		rectObj->getProp(act, "x").toInt32(act),
		rectObj->getProp(act, "y").toInt32(act),
		rectObj->getProp(act, "width").toInt32(act),
		rectObj->getProp(act, "height").toInt32(act)
	);

	data->fillRect(rect, color);
	return AVM1Value::undefinedVal;
}

BITMAP_DATA_FUNCTION_BODY(clone)
{
	return NEW_GC_PTR(act.getGcCtx(), AVM1BitmapData
	(
		_this->getLocalProp(act, "__proto__", false),
		NEW_GC_PTR(act.getGcCtx(), BitmapData(*_this->data))
	));
}

BITMAP_DATA_FUNCTION_BODY(dispose)
{
	_this->data->dispose();
	return AVM1Value::undefinedVal;
}

BITMAP_DATA_FUNCTION_BODY(floodFill)
{
	std::pair<int32_t, int32_t> pos;
	uint32_t color;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(pos)(color));

	_this->data->floodFill
	(
		Vector2(pos.first, pos.second),
		color
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1BitmapData, noise)
{	
	AVM1Value seedVal;
	uint32_t low;
	uint32_t high;
	BitmapChannelOpts channelOpts;
	bool grayScale;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(seedVal)(low, 0)(high, 0xff)
	(
		channelOpts,
		BitmapChannelOpts::RGB
	)(grayScale, false));

	_this->data->noise
	(
		seedVal.toInt32(act),
		low,
		std::max<uint8_t>(high, low),
		channelOpts,
		grayScale
	);
	return AVM1Value::undefinedVal;
}

BITMAP_DATA_FUNCTION_BODY(draw)
{
	AVM1Value sourceVal;
	MATRIX matrix;
	CXFORMWITHALPHA cxform;
	AS_BLENDMODE blendMode;
	Optional<RectF> clipRect;
	bool smooth;

	AVM1_ARG_UNPACK(sourceVal)(matrix)(cxform)
	(
		blendMode,
		BLENDMODE_NORMAL
	)(clipRect)(smooth, false);

	auto sourceObj = sourceVal.toObject(act);
	auto source = sourceObj->as<DisplayObject>();
	bool needsCopy = false;
	if (source.isNull() && sourceObj->is<AVM1BitmapData>())
	{
		auto bitmapData = sourceObj->as<AVM1BitmapData>();
		// Fast path: Use a temporary `Bitmap` to render the `BitmapData`
		// (the `Bitmap` will be destroyed in the render thread).
		source = NEW_GC_PTR(act.getGcCtx(), Bitmap());
		source->as<Bitmap>()->setupTemporaryBitmap(bitmapData->data);
		needscopy = bitmapData->getContainer() == _this->getContainer();
	}
	else
	{
		LOG
		(
			LOG_ERROR,
			"BitmapData.draw(): Unexpected source: object: " <<
			sourceObj->toDebugStr() << ", value; " << sourceVal
		);
		_this->data->notifyUsers();
		return AVM1Value::undefinedVal;
	}

	_this->data->drawDisplayObject
	(
		source,
		matrix,
		smooth,
		blendMode,
		cxform,
		clipRect,
		needscopy
	);

	if (_this->data->hasUsers())
	{
		_this->getContainer()->flushRenderCalls
		(
			act.getSys()->getRenderThread(),
			source->as<Bitmap>()
		);
	}
	_this->data->notifyUsers();
	return AVM1Value::undefinedVal;
}

BITMAP_DATA_FUNCTION_BODY(colorTransform)
{
}

BITMAP_DATA_FUNCTION_BODY(getColorBoundsRect)
{
}

BITMAP_DATA_FUNCTION_BODY(perlinNoise)
{
}

BITMAP_DATA_FUNCTION_BODY(applyFilter)
{
}

BITMAP_DATA_FUNCTION_BODY(hitTest)
{
}

BITMAP_DATA_FUNCTION_BODY(generateFilterRect)
{
}

BITMAP_DATA_FUNCTION_BODY(copyPixels)
{
}

BITMAP_DATA_FUNCTION_BODY(merge)
{
}

BITMAP_DATA_FUNCTION_BODY(paletteMap)
{
}

BITMAP_DATA_FUNCTION_BODY(pixelDissolve)
{
}

BITMAP_DATA_FUNCTION_BODY(scroll)
{
}

BITMAP_DATA_FUNCTION_BODY(threshold)
{
}

AVM1_FUNCTION_BODY(AVM1BitmapData, compare)
{
}

AVM1_FUNCTION_BODY(AVM1BitmapData, loadBitmap)
{
}
