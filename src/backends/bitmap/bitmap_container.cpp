/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2024, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include <stack>

#include "backends/bitmap/bitmap_container.h"
#include "backends/cachedsurface.h"
#include "backends/decoder.h"
#include "backends/image.h"
#include "backends/rendering.h"
#include "backends/streamcache.h"
#include "display_object/Bitmap.h"
#include "swf.h"

using namespace lightspark;

extern void nanoVGDeleteImage(int image, EngineData* engineData);
extern void nanoVGUpdateImage(int image, const uint8_t* data, EngineData* engineData);

BitmapContainer::BitmapContainer(MemoryAccount* m, bool _fromTag) :
stride(0),
data(reporter_allocator<uint8_t>(m)),
hasModifiedData(false),
hasModifiedTexture(false),
needsClear(true),
fromTag(_fromTag),
currentRenderData(0),
renderEvent(0),
nanoVGImageHandle(-1)
#ifdef ENABLE_CAIRO
,cachedCairoPattern(nullptr)
#endif
{
}

BitmapContainer::~BitmapContainer()
{
	auto sys = getSys();
	if (bitmapTexture.isValid())
	{
		auto rt = sys->getRenderThread();
		if (rt != nullptr && rt->isStarted())
			rt->releaseTexture(bitmapTexture);
	}

	if (nanoVGImageHandle != -1)
		nanoVGDeleteImage(nanoVGImageHandle, sys->getEngineData());
	#ifdef ENABLE_CAIRO
	if (cachedCairoPattern != nullptr)
		cairo_pattern_destroy(cachedCairoPattern);
	#endif
}

uint8_t* BitmapContainer::applyColorTransform(const ColorTransform& ct)
{
	if (ct.isIdentity() && currentColorTransform.isIdentity())
		return getOriginalData();
	if (ctransform == currentColorTransform)
		return getDataColorTransformed();
	checkModifiedTexture();

	setModifiedData(true);
	currentColorTransform = ct;
	return ct.applyTransformation(this);
}

uint8_t* BitmapContainer::applyColorTransform
(
	number_t redMulti,
	number_t greenMulti,
	number_t blueMulti,
	number_t alphaMulti,
	number_t redOff,
	number_t greenOff,
	number_t blueOff,
	number_t alphaOff
)
{
	bool isIdentity =
	(
		redMulti == 1 &&
		greenMulti == 1 &&
		blueMulti == 1 &&
		alphaMulti == 1 &&
		!redOff &&
		!greenOff &&
		!blueOff &&
		!alphaOff
	);
	if (isIdentity)
		return getData();

	auto& curCt = currentColorTransform;
	bool isSame =
	(
		redMulti == curCt.redMultiplier &&
		greenMulti == curCt.greenMultiplier &&
		blueMulti == curCt.blueMultiplier &&
		alphaMulti == curCt.alphaMultiplier &&
		redOff == curCt.redOffset &&
		greenOff == curCt.greenOffset &&
		blueOff == curCt.blueOffset &&
		alphaOff == curCt.alphaOffset
	);
	if (isSame)
		return getDataColorTransformed();

	curCt.redMultiplier = redMulti;
	curCt.greenMultiplier = greenMulti;
	curCt.blueMultiplier = blueMulti;
	curCt.alphaMultiplier = alphaMulti;
	curCt.redOffset = redOff;
	curCt.greenOffset = greenOff;
	curCt.blueOffset = blueOff;
	curCt.alphaOffset = alphaOff;
	auto src = getData();
	auto dst = getDataColorTransformed();
	size_t _size = size.x * size.y * 4;
	for (size_t i = 0; i < _size; i += 4)
	{
		dst[i + 3] = iclamp(src[i + 3] * curCt.alphaMultiplier + curCt.alphaOffset, 0, 255);
		dst[i + 2] = iclamp((src[i + 2] * curCt.blueMultiplier + curCt.blueOffset) * (number_t(dst[i + 3]) / 255));
		dst[i + 1] = iclamp((src[i + 1] * curCt.greenMultiplier + curCt.greenOffset) * (number_t(dst[i + 3]) / 255));
		dst[i] = iclamp((src[i] * curCt.redMultiplier + curCt.redOffset) * (number_t(dst[i + 3]) / 255));
	}
	return getDataColorTransformed();
}

uint8_t* BitmapContainer::getRectangleData(const Rect<int32_t>& sourceRect)
{
	auto rect = clipRect(sourceRect);
	auto copySize = rect.size();

	int sx = clippedSourceRect.Xmin;
	int sy = clippedSourceRect.Ymin;
	auto point = rect.min;
	auto ret = new uint8_t[copySize.x * copySize.x * 4];
	for (size_t i = 0; i < copySize.y; ++i)
	{
		memcpy
		(
			&ret[i * copySize.x * 4],
			&data[(point.y + i) * stride + 4 * point.x],
			copySize.x * 4
		);
	}
	return ret;
}

static std::pair<size_t, size_t> convertBitmapWithAlpha
(
	std::vector<uint8_t, reporter_allocator<uint8_t>>& data,
	uint8_t* inData,
	const Vector2u& size,
	bool fromPNG
)
{
	auto ret = std::make_pair
	(
		size.x * size.y * 4,
		size.x * 4
	);

	data.resize(ret.first);

	auto inSpan = Span<uint8_t>(inData, ret.first).as<uint32_t>();
	auto outSpan = makeSpan(data).as<uint32_t>();

	for (size_t i = 0; i < size.y; ++i)
	{
		for (size_t j = 0; j < size.x; ++j)
		{
			size_t idx = i * size.x + j;
			// PNGs are always decoded in RGBA
			outSpan[idx] = fromPNG ? rol
			(
				inSpan.atBE(idx),
				8
			) : inSpan[idx];
		}
	}
}

static inline uint32_t copyRGB15To24(uint8_t* src)
{
	// highest bit is ignored
	uint8_t r = (src[0] & 0x7C) >> 2;
	uint8_t g = ((src[0] & 0x03) << 3) + (src[1] & 0xE0 >> 5);
	uint8_t b = src[1] & 0x1F;

	r = r * 255 / 31;
	g = g * 255 / 31;
	b = b * 255 / 31;

	return uint32_t(r << 16 | g << 8 | b);
}

static inline uint32_t copyRGB24To24(uint8_t* src)
{
	return uint32_t(src[0] << 16 | src[1] << 8 | src[2]);
}

static std::pair<size_t, size_t> convertBitmap
(
	std::vector<uint8_t, reporter_allocator<uint8_t>>& data,
	uint8_t* inData,
	const Vector2u& size,
	size_t bpp
)
{
	auto ret = std::make_pair
	(
		size.x * size.y * 4,
		size.x * 4
	);

	data.resize(ret.first, 0);
	Span<uint8_t> inSpan(inData, ret.first);
	auto outSpan = makeSpan(data).as<uint32_t>();

	for (size_t i = 0; i < size.y; ++i)
	{
		for (size_t j = 0; j < size.x; ++j)
		{
			auto idx = i * size.x + j;
			// set the alpha channel to opaque
			uint32_t px = 0xff000000;
			// copy the RGB bytes to pdata
			switch (bpp)
			{
				case 2:
					px |= copyRGB15To24(&inData[idx * 2]);
					break;
				case 3:
					px |= copyRGB24To24(&inData[idx * 3]);
					break;
				case 4:
					px |= copyRGB24To24(&inData[idx * 4 + 1]);
					break;
			}
			outSpan[idx] = px;
		}
	}
}

bool BitmapContainer::fromRGB
(
	uint8_t* rgb,
	const Vector2u& _size,
	const BITMAP_FORMAT& format,
	bool fromPNG = false
);
{
	if (rgb == nullptr)
		return false;

	size = _size;
	size_t dataSize;
	auto pair = std::tie(dataSize, stride);
	if (format == ARGB32)
		pair = convertBitmapWithAlpha(data, rgb, size, fromPNG);
	else
	{
		auto bpp =
		(
			format == RGB15 ? 2 :
			format == RGB24 ? 3 : 4
		);
		convertBitmap(data, rgb, size, bpp);
	}

	delete[] rgb;

	if (data.empty())
	{
		LOG(LOG_ERROR, "Error decoding image");
		return false;
	}

	setNeedsClear(false);
	setModifiedData(true);
	return true;
}


bool BitmapContainer::fromJPEG
(
	Span<uint8_t> _data,
	Span<uint8_t> tablesData
)
{
	assert(data.empty());

	uint8_t* rgb;
	/* flash uses signed values for width and height */
	Vector2 _size;
	bool hasAlpha;

	std::tie
	(
		rgb,
		_size,
		hasAlpha
	) = ImageDecoder::decodeJPEG(_data, tablesData);
	assert_and_throw(_size >= Vector2());
	return fromRGB(rgb, _size, hasAlpha ? ARGB32 : RGB24);
}

bool BitmapContainer::fromJPEG(std::istream& s)
{
	assert(data.empty());

	uint8_t* rgb;
	/* flash uses signed values for width and height */
	Vector2 _size;
	bool hasAlpha;

	std::tie(rgb, _size, hasAlpha) = ImageDecoder::decodeJPEG(s);
	assert_and_throw(_size >= Vector2());
	return fromRGB(rgb, _size, hasAlpha ? ARGB32 : RGB24);
}

bool BitmapContainer::fromPNG(std::istream& s)
{
	assert(data.empty());

	uint8_t* rgb;
	/* flash uses signed values for width and height */
	Vector2 _size;
	bool hasAlpha;

	std::tie(rgb, _size, hasAlpha) = ImageDecoder::decodePNG(s);
	assert_and_throw(_size >= Vector2());
	return fromRGB(rgb, _size, hasAlpha ? ARGB32 : RGB24, true);
}

bool BitmapContainer::fromPNG(Span<uint8_t> _data)
{
	uint8_t* rgb;
	/* flash uses signed values for width and height */
	Vector2 _size;
	bool hasAlpha;

	std::tie(rgb, _size, hasAlpha) = ImageDecoder::decodePNG(_data);
	assert_and_throw(_size >= Vector2());
	return fromRGB(rgb, _size, hasAlpha ? ARGB32 : RGB24, true);
}

bool BitmapContainer::fromGIF(SystemState* sys, Span<uint8_t> _data)
{
	#ifdef ENABLE_LIBAVCODEC
	MemoryStreamCache gifData(sys);
	gifData.append(_data);
	gifData.markFinished();
	auto sBuf = gifData.createReader();
	std::istream s(sBuf);

	auto streamDecoder = new FFMpegStreamDecoder
	(
		nullptr,
		sys->getEngineData(),
		s,
		0,
		nullptr,
		gifData.getReceivedLength()
	);

	if
	(
		streamDecoder->videoDecoder != nullptr &&
		// TODO how are GIFs with multiple frames handled?
		streamDecoder->decodeNextFrame()
	)
	{
		return fromRGB
		(
			streamDecoder->videoDecoder->upload(true),
			streamDecoder->videoDecoder->sizeNeeded(),
			ARGB32,
			true
		);
	}

	delete streamDecoder;
	delete sbuf;
	#else
	LOG(LOG_ERROR,"can't decode gif image because ffmpeg is not available");
	#endif
	setNeedsClear(false);
	setModifiedData(true);
	return false;
}

bool BitmapContainer::fromPalette
(
	uint8_t* inData,
	const Vector2u& _size,
	size_t inStride,
	uint8_t* palette,
	size_t numColors,
	size_t paletteBPP
)
{
	assert(data.empty());
	if (inData == nullptr || palette == nullptr)
		return false;

	size = _size;
	auto rgb = ImageDecoder::decodePalette
	(
		inData,
		_size,
		inStride,
		palette,
		numColors,
		paletteBPP
	);

	return fromRGB(rgb, _size, paletteBPP == 4 ? ARGB32 : RGB24);
}

void BitmapContainer::fromRawData(uint8_t* _data, const Vector2u& _size)
{
	stride = _size.x * 4;
	size = _size;

	auto dataSize = stride * _size.y;
	data.resize(dataSize);
	setNeedsClear(false);

	if (_data == nullptr)
		return;

	setModifiedData(true);
	memcpy(data.data(), _data, dataSize);
}

// needs to be called in renderThread
bool BitmapContainer::checkTextureForUpload(SystemState* sys)
{
	if (isEmpty())
		return false;

	if (nanoVGImageHandle >= 0 && !hasModifiedData)
		return true;
	else if (nanoVGImageHandle >= 0)
	{
		nanoVGUpdateImage(nanoVGImageHandle,currentcolortransform.isIdentity() ? getData() : getDataColorTransformed(),sys->getEngineData());
		setModifiedData(false);
		return true;
	}

	auto rt = sys->getRenderThread();
	if (!bitmaptexture.isValid())
		bitmapTexture = rt->allocateTexture(size, true, true);
	rt->loadChunkBGRA
	(
		bitmapTexture,
		size,
		currentColorTransform.isIdentity() ?
		getData() :
		getColorTransformedData()
	);
	return true;
}

void BitmapContainer::clone(BitmapContainer& c)
{
	size_t _size = size.x * size.y * 4;
	checkModifiedTexture();
	memcpy(c.getOriginalData(), getOriginalData(), _size);

	if (!currentColorTransform.isIdentity())
	{
		c.currentColorTransform = currentColorTransform;
		memcpy
		(
			c.getColorTransformedData(),
			getColorTransformedData(),
			_size
		);
	}

	c.setModifiedData(true);
}

void BitmapContainer::setModifiedData(bool modified)
{
	assert(!modified || !hasModifiedTexture);
	hasModifiedData = modified;
}

void BitmapContainer::setModifiedTexture(bool modified)
{
	assert(!modified || !hasModifiedData);
	hasModifiedTexture = modified;
}

void BitmapContainer::addRenderCall(RenderDisplayObjectToBitmapContainer& call)
{
	getRenderData()->rendercalls.push(call);
}

void BitmapContainer::flushRenderCalls
(
	RenderThread* renderThread,
	Bitmap* tempBitmap,
	bool wait
)
{
	if (getRenderData()->rendercalls.empty())
		return;
	renderThread->renderBitmap(this, tempBitmap, wait);
}
void BitmapContainer::addRenderCallBitmap
(
	RenderThread* renderThread,
	Bitmap* tempBitmap
)
{
	renderThread->addRenderCallBitmap(this, tempBitmap);
}

void BitmapContainer::setAlpha(const Vector2& pos, uint8_t alpha)
{
	if (pos < Vector2() || pos >= size)
		return;

	auto _data = getCurrentData().as<RGBA>();
	_data[pos.y * size.x + pos.x].Alpha = alpha;
	setModifiedData(true);
}

void BitmapContainer::setPixel
(
	const Vector2& pos,
	const RGBA& color,
	bool setAlpha,
	bool isPremultiplied
)
{
	if (pos < Vector2() || pos >= size)
		return;

	checkModifiedTexture();

	auto& px = getCurrentData().as<RGBA>()[pos.y * size.x + pos.x];
	if (isPremultiplied || px.Alpha == 0xff)
		px = color;
	else
	{
		auto alpha = setAlpha ? color.Alpha : px.Alpha;
		px = RGBA
		(
			(color.Red * alpha + 0x7f) / 0xff,
			(color.Green * alpha + 0x7f) / 0xff,
			(color.Blue * alpha + 0x7f) / 0xff,
			alpha
		);
	}
	setModifiedData(true);
}

// values taken from ruffle, see https://github.com/ruffle-rs/ruffle/blob/master/core/src/bitmap/bitmap_data.rs
static constexpr uint32_t FLASH_PREMUL_FACTOR[256] =
{
	0, 16678912, 8339456, 5559638, 4169728, 3335783, 2779819, 2386603, 2086230, 1855488,
	1667892, 1518251, 1391151, 1285234, 1193302, 1111928, 1043895, 981113, 927744, 879275,
	834621, 795535, 759126, 726358, 695839, 668183, 642538, 618737, 596651, 576171, 555964,
	538706, 522104, 506319, 490557, 477321, 464038, 451353, 439544, 428244, 417582, 407500,
	397768, 388535, 379630, 371117, 363179, 355235, 348050, 340965, 334052, 327038, 321269,
	315077, 309159, 303586, 298189, 293092, 287981, 283080, 278251, 273892, 269268, 265179,
	261087, 256971, 253160, 249322, 245508, 242164, 238575, 235245, 231859, 228848, 225785,
	222712, 219616, 216827, 213985, 211432, 208835, 206075, 203750, 201196, 198895, 196223,
	194301, 191987, 189686, 187636, 185559, 183426, 181453, 179444, 177638, 175855, 174054,
	171948, 170489, 168695, 166889, 165365, 163519, 162045, 160508, 158970, 157429, 156150,
	154610, 153081, 151803, 150511, 148986, 147709, 146420, 145116, 143868, 142586, 141545,
	140277, 139194, 137957, 136954, 135676, 134652, 133621, 132604, 131577, 130552, 129527,
	128508, 127476, 126451, 125432, 124670, 123645, 122818, 121847, 121082, 120060, 119288,
	118263, 117502, 116720, 115967, 115195, 114424, 113655, 112893, 112125, 111356, 110563,
	109811, 109048, 108287, 107766, 107004, 106236, 105724, 104953, 104434, 103676, 102904,
	102375, 101879, 101119, 100604, 99834, 99321, 98813, 98112, 97533, 97019, 96509, 95994,
	95486, 94713, 94185, 93689, 93179, 92667, 92149, 91643, 91129, 90621, 90068, 89597,
	89342, 88829, 88318, 87804, 87294, 87034, 86523, 85994, 85499, 85245, 84732, 84222,
	83956, 83450, 82937, 82685, 82173, 81840, 81405, 80889, 80638, 80127, 79862, 79354,
	79103, 78590, 78332, 78077, 77565, 77308, 76795, 76541, 76284, 75766, 75518, 75262,
	74748, 74493, 74238, 73691, 73470, 73214, 72959, 72447, 72189, 71935, 71671, 71166,
	70911, 70651, 70399, 70140, 69886, 69615, 69116, 68861, 68603, 68350, 68093, 67839,
	67576, 67326, 67070, 66813, 66556, 66302, 66046, 65791, 65408,
};

RGBA BitmapContainer::getPixel
(
	const Vector2& pos,
	bool isPremultiplied = true
)
{
	if (pos < Vector2() || pos >= size)
		return RGBA();

	if (needsClear && !hasModifiedData)
		return nanoVGImageBackgroundColor;

	auto px =
	(
		hasModifiedData ?
		// Avoid the round trip to `RenderThread` as we already have the
		// current data in memory.
		makeSpan(data) :
		getCurrentData()
	).as<RGBA>()[pos.y * size.x + pos.x];

	if (isPremultiplied)
		return px;

	if (!px.Alpha || px.Alpha == 0xff)
		return px;

	// return value with "un-multiplied" alpha: algorithm taken from ruffle
	auto alphaFactor = FLASH_PREMUL_FACTOR[px.Alpha];
	return RGBA
	(
		uint32_t(px.Red * alphaFactor + 0x8000) >> 16,
		uint32_t(px.Green * alphaFactor + 0x8000) >> 16,
		uint32_t(px.Blue * alphaFactor + 0x8000) >> 16,
		px.Alpha
	);
}

void BitmapContainer::copyRectangle
(
	_R<BitmapContainer> source,
	const Rect<int32_t>& sourceRect,
	const Vector2& destPoint,
	bool mergeAlpha
)
{
	Rect<int32_t> rect;
	Vector2 pos;
	std::tie(rect, pos) = clipRect(source, sourceRect, destPoint);

	auto copySize = rect.size();

	if (copySize <= Vector2())
		return;

	int sx = clippedSourceRect.Xmin;
	int sy = clippedSourceRect.Ymin;
	auto _data = getCurrentData();

	setModifiedData(true);
	if (!mergeAlpha)
	{
		source->checkModifiedTexture();
		//Fast path using memmove
		for (size_t i = 0; i < copySize.y; ++i)
		{
			size_t dstIdx =
			(
				(pos.y + i) *
				source->stride +
				pos.x * 4
			);

			size_t srcIdx =
			(
				(rect.min.y + i) *
				source->stride +
				rect.min.x * 4
			);
			memmove
			(
				&_data[dstIdx],
				&source->data[srcIdx],
				copySize.x * 4
			);
		}
		return;
	}

	auto srcData = source->getCurrentData();
	bool needsDeletion = false;
	if (sourceData.getData() == _data.getData())
	{
		// source and destination are the same BitmapContainer, so we operate on a copy
		// TODO check if it is really necessary (source/destination rectangles overlap)
		sourceData = Span<uint8_t>
		(
			new uint8_t[data.size()],
			data.size()
		);

		memcpy
		(
			sourceData.getData(),
			_data.getData(),
			data.size()
		);
		needsDeletion = true;
	}

	auto dstData = _data.as<RGBA>();
	auto srcData = sourceData.as<RGBA>();
	// TODO check if there is a faster algorithm for this
	for (size_t i = 0; i < copySize.y; ++i)
	{
		for (size_t j = 0; i < copySize.x; ++j)
		{
			auto& dstPx = dstData
			[
				(pos.y + i) *
				size.x +
				pos.x
			];

			dstPx = dstPx.alphaBlend(srcData
			[
				(rect.min.y + i) *
				size.x +
				rect.min.x
			]);
		}
	}

	if (needsDeletion)
		delete[] sourceData.getData();
}

void BitmapContainer::applyFilter
(
	_R<BitmapContainer> source,
	const Rect<int32_t>& sourceRect,
	const Vector2f& destPoint,
	BitmapFilter& filter
)
{
	Rect<int32_t> rect;
	Vector2 pos;
	std::tie(rect, pos) = clipRect(source, sourceRect, destPoint);
	filter.applyFilter(this, source.getPtr(), rect, pos, Vector2f(1, 1));
	setModifiedData(true);
}

void BitmapContainer::fillRectangle
(
	const Rect<int32_t>& rect,
	const RGBA& color,
	bool useAlpha
)
{
	auto _rect = clipRect(rect);
	if (_rect.min >= _rect.max)
		return;

	RGBA realColor = useAlpha ? color : color.toRGB();
	auto _data = getCurrentData().as<RGBA>();
	auto span = getData().as<RGBA>();

	// fill first line
	for (ssize_t x = _rect.min.x; x < _rect.max.x; ++x)
		_data[_rect.min.y * size.x + x] = realColor;

	size_t lineSize = _rect.size().x * 4;
	// use memcpy to fill all other lines
	for (ssize_t y = _rect.min.y; y < _rect.max.y; ++y)
	{
		memcpy
		(
			&_data[y * size.x + _rect.min.x],
			&span[_rect.min.y * size.x + _rect.min.x],
			lineSize
		);
	}
	setModifiedData(true);
}

bool BitmapContainer::scroll(const Vector2& pos)
{
	auto srcPos = -pos.max(Vector2());
	auto destPos = pos.max(Vector2());
	auto copySize = (size - pos.abs()).max(Vector2());

	if (copySize <= Vector2())
		return false;

	auto _data = getCurrentData().as<RGBA>();
	for (size_t i = 0; i < copySize.y; ++i)
	{
		//Set the copy direction so that we don't
		//overwrite the destination region
		size_t row = copySize.y > 0 ? copySize.y - i - 1 : i;

		memmove
		(
			&_data[(destPos.y + row) * size.x + destPos.x],
			&_data[(srcPos.y + row) * size.x + srcPos.x],
			copySize.x * 4
		);
	}

	setModifiedData(true);
	return true;
}

inline Span<uint32_t> BitmapContainer::getDataNoBoundsChecking(const Vector2f& pos)
{
	return getCurrentData().as<uint32_t>().getLast
	(
		pos.y *
		size.x +
		pos.x
	)
}

Span<uint8_t> BitmapContainer::getCurrentData()
{
	checkModifiedTexture();
	return
	(
		currentColorTransform.isIdentity() ?
		makeSpan(data) :
		makeSpan(colorTransformedData)
	);
}

void BitmapContainer::checkModifiedTexture()
{
	if (!hasModifiedTexture)
		return;

	setModifiedTexture(false);
	incRef();
	getSys()->getRenderThread()->readPixelsToBimapContainer(_MR(this));
}

/*
 * Fill a connected area around (startX, startY) with the given color.
 *
 * Adapted from "A simple non-recursive scan line method" at
 * http://www.codeproject.com/Articles/6017/QuickFill-An-efficient-flood-fill-algorithm
 */
void BitmapContainer::floodFill(const Vector2& pos, const RGBA& color)
{
	struct LineSegment
	{
		LineSegment
		(
			int32_t _x1,
			int32_t _x2,
			int32_t _y,
			int32_t _dy
		) : x1(_x1), x2(_x2), y(_y), dy(_dy) {}
		int32_t x1; // leftmost filled point on last line
		int32_t x2; // rightmost filled point on last line
		int32_t y;  // y coordinate (may be invalid!)
		int32_t dy; // vertical direction (1 or -1)
	};

	std::stack<LineSegment> segments;

	if (pos < Vector2() || pos >= size)
		return;

	auto seedColor = getPixel(pos);

	// Comment on the codeproject.com: "needed in some cases" ???
	segments.push(LineSegment(pos.x, pos.x, pos.y + 1, 1));
	// The starting point
	segments.push(LineSegment(pos.x, pos.x, pos.y, -1));

	while (!segments.empty())
	{
		int32_t left;
		auto r = segments.top();
		segments.pop();
		if (r.y < 0 || r.y >= size.y)
			continue;

		assert(r.x1 <= r.x2);
		assert(r.x1 >= 0);
		assert(r.x2 < size.x);

		// current x-coordinate
		auto t = r.x1;
		// pointer to the current pixel, keep in sync with t
		auto p = getDataNoBoundsChecking(Vector2
		(
			r.x1,
			r.y
		)).as<RGBA>().data();

		// extend left
		for (; t >= 0 && *p == seedColor; --p, --t)
			*p = color;

		if (t >= r.x1)
		{
			// Did not extend to left. Skip over border if
			// any.
			for (; t <= r.x2 && *p != seedColor; ++p, ++t);
			left = t;
		}
		else
		{
			// Extended past r.x1, push the segment on the
			// previous line
			left = t + 1;
			if (left < r.x1)
			{
				segments.emplace
				(
					left,
					r.x1 - 1,
					r.y - r.dy,
					-r.dy
				);
			}

			t = r.x1 + 1;
		}

		// fill rightwards starting from r.x1 or the leftmost
		// filled point
		do
		{
			auto _p = getDataNoBoundsChecking(Vector2
			(
				t,
				r.y
			)).as<RGBA>();

			auto it = _p.begin();
			for (; t < size.x && *it == seedColor; ++it, ++t)
				*it = color;

			// push the segment on the next line
			if (t >= left+1)
			{
				segments.emplace
				(
					left,
					t - 1,
					r.y + r.dy,
					r.dy
				);
			}

			// If extended past r.x2, push the segment on
			// the previous line
			if (t > r.x2+1)
			{
				segments.emplace
				(
					r.x2,
					t - 1,
					r.y - r.dy,
					-r.dy
				);
				break; // we are done with this segment
			}

			// Skip forward
			++it;
			++t;
			for (; t <= r.x2 && *it != seedColor; ++it, ++t);
			left = t;
		}
		while (t <= r.x2);
	}

	setModifiedData(true);
}

Rect<int32_t> BitmapContainer::clipRect(const Rect<int32_t>& srcRect) const
{
	return Rect<int32_t>
	{
		sourceRect.min.max(Vector2()),
		sourceRect.max.clamp(Vector2(), size)
	};
}

void BitmapContainer::clipRect(_R<BitmapContainer> source, const RECT& sourceRect,
			       int32_t destX, int32_t destY, RECT& outputSourceRect,
			       int32_t& outputX, int32_t& outputY) const
std::pair<Rect<int32_t>, Vector2> BitmapContainer::clipRect
(
	_R<BitmapContainer> source,
	const Rect<int32_t>& srcRect,
	const Vector2& destPoint
)
{
	auto rect = source->clipRect(srcRect);
	auto destTL = destPoint;

	if (destTL.x < 0)
	{
		rect.min.x -= destTL.x;
		destTL.x = 0;
	}

	if (destTL.y < 0)
	{
		rect.min.y -= destTL.y;
		destTL.y = 0;
	}

	auto clippedSize = rect.size().clamp
	(
		Vector2(),
		size - rect.tl()
	);

	return std::make_pair(Rect<int32_t>
	{
		rect.tl(),
		rect.tl() + clippedSize
	}, rect.tl());
}

std::vector<uint32_t> BitmapContainer::getPixelVector(const RECT& inputRect, bool premultiplied)
std::vector<RGBA> BitmapContainer::getPixelVector
(
	const Rect<int32_t>& rect,
	bool isPremultiplied
)
{
	auto _rect = clipRect(rect);

	if (_rect.size() <= Vector2())
		return {};

	auto _size = _rect.size();
	std::vector<RGBA> ret;
	ret.reserve(_size.x * _size.y);
	for (ssize_t y = _rect.min.y; y < _rect.max.y; ++y)
	{
		for (ssize_t x = _rect.min.x; x < _rect.max.x; ++x)
		{
			Vector2 pos(x, y);
			ret.emplace_back
			(
				isPremultiplied ?
				getDataNoBoundsCheck(pos).front() :
				getPixel(pos, false)
			);
		}
	}

	return ret;
}
