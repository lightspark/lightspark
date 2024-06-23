/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "scripting/flash/display/BitmapContainer.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/geom/flashgeom.h"
#include "backends/rendering.h"
#include "backends/image.h"
#include "backends/decoder.h"
#include "backends/streamcache.h"
#include "swf.h"

using namespace std;
using namespace lightspark;

extern void nanoVGDeleteImage(int image);
BitmapContainer::BitmapContainer(MemoryAccount* m):stride(0),width(0),height(0),
	data(reporter_allocator<uint8_t>(m)),renderevent(0),
	nanoVGImageHandle(-1),cachedCairoPattern(nullptr)
{
}

BitmapContainer::~BitmapContainer()
{
	if (bitmaptexture.isValid())
	{
		RenderThread* rt = getSys()->getRenderThread();
		if (rt && rt->isStarted())
		{
			getSys()->getRenderThread()->releaseTexture(bitmaptexture);
		}
	}
	if (nanoVGImageHandle != -1)
		nanoVGDeleteImage(nanoVGImageHandle);
	if (cachedCairoPattern)
		cairo_pattern_destroy(cachedCairoPattern);
		
}

uint8_t* BitmapContainer::applyColorTransform(ColorTransform *ctransform)
{
	if (ctransform->isIdentity() && currentcolortransform.isIdentity())
		return getOriginalData();
	if (*ctransform==currentcolortransform)
		return getDataColorTransformed();
	currentcolortransform=*ctransform;
	return ctransform->applyTransformation(this);
}

uint8_t* BitmapContainer::applyColorTransform(number_t redMulti, number_t greenMulti, number_t blueMulti, number_t alphaMulti, number_t redOff, number_t greenOff, number_t blueOff, number_t alphaOff)
{
	if (redMulti==1.0 &&
		greenMulti==1.0 &&
		blueMulti==1.0 &&
		alphaMulti==1.0 &&
		redOff==0.0 &&
		greenOff==0.0 &&
		blueOff==0.0 &&
		alphaOff==0.0)
		return getData();
	if (redMulti==currentcolortransform.redMultiplier &&
		greenMulti==currentcolortransform.greenMultiplier &&
		blueMulti==currentcolortransform.blueMultiplier &&
		alphaMulti==currentcolortransform.alphaMultiplier &&
		redOff==currentcolortransform.redOffset &&
		greenOff==currentcolortransform.greenOffset &&
		blueOff==currentcolortransform.blueOffset &&
		alphaOff==currentcolortransform.alphaOffset)
		return getDataColorTransformed();
	currentcolortransform.redMultiplier=redMulti;
	currentcolortransform.greenMultiplier=greenMulti;
	currentcolortransform.blueMultiplier=blueMulti;
	currentcolortransform.alphaMultiplier=alphaMulti;
	currentcolortransform.redOffset=redOff;
	currentcolortransform.greenOffset=greenOff;
	currentcolortransform.blueOffset=blueOff;
	currentcolortransform.alphaOffset=alphaOff;
	uint8_t* src = getData();
	uint8_t* dst = getDataColorTransformed();
	uint32_t size = getWidth()*getHeight()*4;
	for (uint32_t i = 0; i < size; i+=4)
	{
		dst[i+3] = max(0,min(255,int(((number_t(src[i+3]) * currentcolortransform.alphaMultiplier) + currentcolortransform.alphaOffset))));
		dst[i+2] = max(0,min(255,int(((number_t(src[i+2]) * currentcolortransform. blueMultiplier) + currentcolortransform. blueOffset)*(number_t(dst[i+3])/255.0))));
		dst[i+1] = max(0,min(255,int(((number_t(src[i+1]) * currentcolortransform.greenMultiplier) + currentcolortransform.greenOffset)*(number_t(dst[i+3])/255.0))));
		dst[i  ] = max(0,min(255,int(((number_t(src[i  ]) * currentcolortransform.  redMultiplier) + currentcolortransform.  redOffset)*(number_t(dst[i+3])/255.0))));
	}
	return getDataColorTransformed();
}

uint8_t* BitmapContainer::getRectangleData(const RECT& sourceRect)
{
	RECT clippedSourceRect;
	clipRect(sourceRect, clippedSourceRect);

	int copyWidth = clippedSourceRect.Xmax - clippedSourceRect.Xmin;
	int copyHeight = clippedSourceRect.Ymax - clippedSourceRect.Ymin;

	int sx = clippedSourceRect.Xmin;
	int sy = clippedSourceRect.Ymin;
	uint8_t* res = new uint8_t[copyWidth * copyHeight * 4];
	for (int i=0; i<copyHeight; i++)
	{
		memcpy(&res[i*copyWidth*4],
				&data[(sy+i)*stride + 4*sx],
				4*copyWidth);
	}
	return res;
}

bool BitmapContainer::fromRGB(uint8_t* rgb, uint32_t w, uint32_t h, BITMAP_FORMAT format, bool frompng)
{
	if(!rgb)
		return false;

	width = w;
	height = h;
	size_t dataSize;
	if(format==ARGB32)
		CairoRenderer::convertBitmapWithAlphaToCairo(data, rgb, width, height, &dataSize, &stride,frompng);
	else
		CairoRenderer::convertBitmapToCairo(data, rgb, width, height, &dataSize, &stride, format==RGB15 ? 2 : (format==RGB24 ? 3 : 4));
	delete[] rgb;
	if(data.empty())
	{
		LOG(LOG_ERROR, "Error decoding image");
		return false;
	}

	return true;
}

bool BitmapContainer::fromJPEG(uint8_t *inData, int len, const uint8_t *tablesData, int tablesLen)
{
	assert(data.empty());
	/* flash uses signed values for width and height */
	uint32_t w,h;
	bool hasAlpha;
	uint8_t *rgb=ImageDecoder::decodeJPEG(inData, len, tablesData, tablesLen, &w, &h, &hasAlpha);
	assert_and_throw((int32_t)w >= 0 && (int32_t)h >= 0);
	BITMAP_FORMAT format=hasAlpha ? ARGB32 : RGB24;
	return fromRGB(rgb, (int32_t)w, (int32_t)h, format);
}

bool BitmapContainer::fromJPEG(std::istream &s)
{
	assert(data.empty());
	/* flash uses signed values for width and height */
	uint32_t w,h;
	bool hasAlpha;
	uint8_t *rgb=ImageDecoder::decodeJPEG(s, &w, &h, &hasAlpha);
	assert_and_throw((int32_t)w >= 0 && (int32_t)h >= 0);
	BITMAP_FORMAT format=hasAlpha ? ARGB32 : RGB24;
	return fromRGB(rgb, (int32_t)w, (int32_t)h, format);
}

bool BitmapContainer::fromPNG(std::istream &s)
{
	assert(data.empty());
	/* flash uses signed values for width and height */
	uint32_t w,h;
	bool hasAlpha;
	uint8_t *rgb=ImageDecoder::decodePNG(s, &w, &h,&hasAlpha);
	assert_and_throw((int32_t)w >= 0 && (int32_t)h >= 0);
	BITMAP_FORMAT format=hasAlpha ? ARGB32 : RGB24;
	return fromRGB(rgb, (int32_t)w, (int32_t)h, format,true);
}
bool BitmapContainer::fromPNG(uint8_t* data, int len)
{
	/* flash uses signed values for width and height */
	uint32_t w,h;
	bool hasAlpha;
	uint8_t *rgb=ImageDecoder::decodePNG(data,len, &w, &h,&hasAlpha);
	assert_and_throw((int32_t)w >= 0 && (int32_t)h >= 0);
	BITMAP_FORMAT format=hasAlpha ? ARGB32 : RGB24;
	return fromRGB(rgb, (int32_t)w, (int32_t)h, format,true);
}
bool BitmapContainer::fromGIF(uint8_t* data, int len, SystemState* sys)
{
#ifdef ENABLE_LIBAVCODEC
	MemoryStreamCache gifdata(sys);
	gifdata.append(data, len);
	gifdata.markFinished();
	std::streambuf *sbuf = gifdata.createReader();
	istream s(sbuf);
	FFMpegStreamDecoder* streamDecoder=new FFMpegStreamDecoder(nullptr,sys->getEngineData(),s,0,nullptr,gifdata.getReceivedLength());
	if(streamDecoder->videoDecoder)
	{
		uint32_t w,h;
		streamDecoder->videoDecoder->sizeNeeded(w,h);
		// TODO how are GIFs with multiple frames handled?
		if (streamDecoder->decodeNextFrame())
		{
			uint8_t* frame =streamDecoder->videoDecoder->upload(true);
			return fromRGB(frame, (int32_t)w, (int32_t)h, ARGB32,true);
		}
	}
	delete streamDecoder;
	delete sbuf;
#else
	LOG(LOG_ERROR,"can't decode gif image because ffmpeg is not available");
#endif
	return false;
}

bool BitmapContainer::fromPalette(uint8_t* inData, uint32_t w, uint32_t h, uint32_t inStride, uint8_t* palette, unsigned numColors, unsigned paletteBPP)
{
	assert(data.empty());
	if (!inData || !palette)
		return false;

	width = w;
	height = h;
	uint8_t *rgb=ImageDecoder::decodePalette(inData, w, h, inStride, palette, numColors, paletteBPP);
	return fromRGB(rgb, (int32_t)w, (int32_t)h, paletteBPP == 4 ? ARGB32 : RGB24);
}

void BitmapContainer::fromRawData(uint8_t* data, uint32_t width, uint32_t height)
{
	this->stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	this->width=width;
	this->height=height;
	uint32_t dataSize = stride * height;
	this->data.resize(dataSize);
	memcpy(this->data.data(),data,dataSize);
	
}

void BitmapContainer::clear()
{
	data.clear();
	data.shrink_to_fit();
	stride=0;
	width=0;
	height=0;
	bitmaptexture.makeEmpty();
}

// needs to be called in renderThread
bool BitmapContainer::checkTextureForUpload(SystemState* sys)
{
	if (isEmpty()) {
		return false;
	}

	if (!bitmaptexture.isValid())
		bitmaptexture=sys->getRenderThread()->allocateTexture(width, height, true,true);
	sys->getRenderThread()->loadChunkBGRA(bitmaptexture,width, height,currentcolortransform.isIdentity() ? getData() : getDataColorTransformed());
	return true;
}

void BitmapContainer::clone(BitmapContainer* c)
{
	memcpy (c->getOriginalData(),getOriginalData(),getWidth()*getHeight()*4);
	if (!currentcolortransform.isIdentity())
	{
		c->currentcolortransform = currentcolortransform;
		memcpy (c->getDataColorTransformed(),getDataColorTransformed(),getWidth()*getHeight()*4);
	}
}

void BitmapContainer::setAlpha(int32_t x, int32_t y, uint8_t alpha)
{
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;
	uint8_t* d = getCurrentData();
	uint32_t *p=reinterpret_cast<uint32_t *>(&d[y*stride + 4*x]);
	*p = ((uint32_t)alpha << 24) + (*p & 0xFFFFFF);
}

void BitmapContainer::setPixel(int32_t x, int32_t y, uint32_t color, bool setAlpha, bool ispremultiplied)
{
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;
	uint8_t* d = getCurrentData();
	uint32_t *p=reinterpret_cast<uint32_t *>(&d[y*stride + 4*x]);
	if(setAlpha)
	{
		if (ispremultiplied || ((color&0xff000000) == 0xff000000))
			*p=color;
		else
		{
			uint32_t res = 0;
			uint32_t alpha = ((color >> 24)&0xff);
			res |= ((((color >> 0) &0xff) * alpha)&0xff) << 0;
			res |= ((((color >> 8) &0xff) * alpha)&0xff) << 8;
			res |= ((((color >> 16) &0xff) * alpha)&0xff) << 16;
			res |= alpha<<24;
			*p=res;
		}
	}
	else
		*p=(*p & 0xff000000) | (color & 0x00ffffff);
}

uint32_t BitmapContainer::getPixel(int32_t x, int32_t y,bool premultiplied) const
{
	if (x < 0 || x >= width || y < 0 || y >= height)
		return 0;
	
	uint8_t* d = getCurrentData();
	const uint32_t *p=reinterpret_cast<const uint32_t *>(&d[y*stride + 4*x]);
	if (!premultiplied)
	{
		uint32_t res = 0;
		uint32_t alpha = ((*p >> 24)&0xff);
		if (alpha && alpha != 0xff)
		{
			// return value with "un-multiplied" alpha: ceiling(value*255/alpha)
			uint32_t b = ((*p) >> 0) &0xff;
			uint32_t g = ((*p) >> 8) &0xff;
			uint32_t r = ((*p) >> 16) &0xff;
			//x/y + (x % y != 0);
			res |= (((b*0xff)/alpha+((b*0xff)%alpha ? 1:0))&0xff) << 0;
			res |= (((g*0xff)/alpha+((g*0xff)%alpha ? 1:0))&0xff) << 8;
			res |= (((r*0xff)/alpha+((r*0xff)%alpha ? 1:0))&0xff) << 16;
			res |= alpha<<24;
			return res;
		}
	}
	return *p;
}

void BitmapContainer::copyRectangle(_R<BitmapContainer> source,
				    const RECT& sourceRect,
				    int32_t destX, int32_t destY,
				    bool mergeAlpha)
{
	RECT clippedSourceRect;
	int32_t clippedX;
	int32_t clippedY;
	clipRect(source, sourceRect, destX, destY, clippedSourceRect, clippedX, clippedY);

	int copyWidth = clippedSourceRect.Xmax - clippedSourceRect.Xmin;
	int copyHeight = clippedSourceRect.Ymax - clippedSourceRect.Ymin;

	if (copyWidth <= 0 || copyHeight <= 0)
		return;
	int sx = clippedSourceRect.Xmin;
	int sy = clippedSourceRect.Ymin;
	uint8_t *p = getCurrentData();
	if (mergeAlpha==false)
	{
		//Fast path using memmove
		for (int i=0; i<copyHeight; i++)
		{
			memmove(&p[(clippedY+i)*stride + 4*clippedX],
				&source->data[(sy+i)*source->stride + 4*sx],
				4*copyWidth);
		}
	}
	else
	{
		uint8_t* sourcedata = source->getCurrentData();
		bool needsdeletion = false;
		if (sourcedata == p)
		{
			// source and destination are the same BitmapContainer, so we operate on a copy
			// TODO check if it is really necessary (source/destination rectangles overlap)
			sourcedata = new uint8_t[data.size()];
			memcpy (sourcedata,p,data.size());
			needsdeletion = true;
		}
		// TODO check if there is a faster algorithm for this
		for (int i=0; i<copyHeight; i++)
		{
			for (int j=0; j<copyWidth; j++)
			{
				uint32_t* pdst = reinterpret_cast<uint32_t *>(&p[(clippedY+i)*stride+4*(j+clippedX)]);
				uint32_t* psrc = reinterpret_cast<uint32_t *>(&sourcedata[(sy+i)*source->stride+4*(j+sx)]);
				uint32_t srcalpha = ((*psrc >> 24)&0xff);
				uint32_t dstalpha = 0xff-srcalpha;
				uint32_t b = ((((*psrc)     ) &0xff) * srcalpha + (((*pdst)     ) &0xff) * dstalpha) / 0xff;
				uint32_t g = ((((*psrc) >> 8) &0xff) * srcalpha + (((*pdst) >> 8) &0xff) * dstalpha) / 0xff;
				uint32_t r = ((((*psrc) >>16) &0xff) * srcalpha + (((*pdst) >>16) &0xff) * dstalpha) / 0xff;
				uint32_t a = min (srcalpha + (((*pdst) >>24) &0xff) * dstalpha, 0xffU);
				*pdst = (b & 0xff) | ((g&0xff)<<8) | ((r&0xff)<<16) | (a<<24);
			}
		}
		if (needsdeletion)
			delete[] sourcedata;
	}
}

void BitmapContainer::applyFilter(_R<BitmapContainer> source,
				    const RECT& sourceRect,
				    number_t destX, number_t destY,
				    BitmapFilter* filter)
{
	RECT clippedSourceRect;
	int32_t clippedX;
	int32_t clippedY;
	clipRect(source, sourceRect, destX, destY, clippedSourceRect, clippedX, clippedY);
	filter->applyFilter(this,source.getPtr(),clippedSourceRect,destX,destY,1.0,1.0);
}

void BitmapContainer::fillRectangle(const RECT& inputRect, uint32_t color, bool useAlpha)
{
	RECT clippedRect;
	clipRect(inputRect, clippedRect);
	if (clippedRect.Ymin>=clippedRect.Ymax || clippedRect.Xmin>=clippedRect.Xmax)
		return;
	
	uint32_t realcolor = useAlpha ? color : (0xFF000000 | (color & 0xFFFFFF));
	// fill first line
	for(int32_t x=clippedRect.Xmin;x<clippedRect.Xmax;x++)
	{
		uint32_t offset=clippedRect.Ymin*stride + x*4;
		uint32_t* ptr=(uint32_t*)(getCurrentData()+offset);
		*ptr = realcolor;
	}
	// use memcpy to fill all other lines
	for(int32_t y=clippedRect.Ymin+1;y<clippedRect.Ymax;y++)
	{
		uint32_t offset=y*stride + clippedRect.Xmin*4;
		memcpy(getCurrentData()+offset,getData()+clippedRect.Ymin*stride+clippedRect.Xmin*4,(clippedRect.Xmax-clippedRect.Xmin)*4);
	}
}

bool BitmapContainer::scroll(int32_t x, int32_t y)
{
	int sourceX = imax(-x, 0);
	int sourceY = imax(-y, 0);

	int destX = imax(x, 0);
	int destY = imax(y, 0);

	int copyWidth = imax(width - abs(x), 0);
	int copyHeight = imax(height - abs(y), 0);

	if (copyWidth <= 0 && copyHeight <= 0)
		return false;

	uint8_t *dataBase = &data[0];
	for(int i=0; i<copyHeight; i++)
	{
		//Set the copy direction so that we don't
		//overwrite the destination region
		int row;
		if (y > 0)
			row = copyHeight - i - 1;
		else
			row = i;

		memmove(dataBase + (destY+row)*stride + 4*destX,
			dataBase + (sourceY+row)*stride + 4*sourceX,
			4*copyWidth);
	}

	return true;
}

inline uint32_t *BitmapContainer::getDataNoBoundsChecking(int32_t x, int32_t y) const
{
	uint8_t* d = getCurrentData();
	return (uint32_t*)&d[y*stride + 4*x];
}

uint8_t* BitmapContainer::getCurrentData() const
{
	return currentcolortransform.isIdentity() ? (uint8_t*)data.data() : (uint8_t*)data_colortransformed.data();
}

/*
 * Fill a connected area around (startX, startY) with the given color.
 *
 * Adapted from "A simple non-recursive scan line method" at
 * http://www.codeproject.com/Articles/6017/QuickFill-An-efficient-flood-fill-algorithm
 */
void BitmapContainer::floodFill(int32_t startX, int32_t startY, uint32_t color)
{
	struct LineSegment {
		LineSegment(int32_t _x1, int32_t _x2, int32_t _y, int32_t _dy) 
			: x1(_x1), x2(_x2), y(_y), dy(_dy) {}
		int32_t x1; // leftmost filled point on last line
		int32_t x2; // rightmost filled point on last line
		int32_t y;  // y coordinate (may be invalid!)
		int32_t dy; // vertical direction (1 or -1)
	};

	stack<LineSegment> segments;

	if (startX < 0 || startX >= width || startY < 0 || startY >= height)
		return;

	uint32_t seedColor = getPixel(startX, startY);

	// Comment on the codeproject.com: "needed in some cases" ???
	segments.push(LineSegment(startX, startX, startY+1, 1));
	// The starting point
	segments.push(LineSegment(startX, startX, startY, -1));

	while (!segments.empty())
	{
		int32_t left;
		LineSegment r = segments.top();
		segments.pop();
		if (r.y < 0 || r.y >= height)
			continue;

		assert(r.x1 <= r.x2);
		assert(r.x1 >= 0);
		assert(r.x2 < width);

		// current x-coordinate
		int t = r.x1;
		// pointer to the current pixel, keep in sync with t
		uint32_t *p = getDataNoBoundsChecking(r.x1, r.y);

		// extend left
		while (t >= 0 && *p == seedColor)
		{
			*p = color;
			p--;
			t--;
		}

		if (t >= r.x1)
		{
			// Did not extend to left. Skip over border if
			// any.
			while (t <= r.x2 && *p != seedColor)
			{
				p++;
				t++;
			}
			left = t;
		}
		else
		{
			// Extended past r.x1, push the segment on the
			// previous line
			left = t+1;
			if (left < r.x1)
			{
				segments.push(LineSegment(left, r.x1-1, r.y-r.dy, -r.dy));
			}

			t = r.x1 + 1;
		}

		// fill rightwards starting from r.x1 or the leftmost
		// filled point
		do
		{
			p = getDataNoBoundsChecking(t, r.y);
			while (t < width && *p == seedColor)
			{
				*p = color;
				p++;
				t++;
			}

			// push the segment on the next line
			if (t >= left+1)
				segments.push(LineSegment(left, t-1, r.y+r.dy, r.dy));

			// If extended past r.x2, push the segment on
			// the previous line
			if (t > r.x2+1)
			{
				segments.push(LineSegment(r.x2, t-1, r.y-r.dy, -r.dy));
				break; // we are done with this segment
			}

			// Skip forward
			p++;
			t++;
			while (t <= r.x2 && *p != seedColor)
			{
				p++;
				t++;
			}
			left = t;
		}
		while (t <= r.x2);
	}
}

void BitmapContainer::clipRect(const RECT& sourceRect, RECT& clippedRect) const
{
	clippedRect.Xmin = imax(sourceRect.Xmin, 0);
	clippedRect.Ymin = imax(sourceRect.Ymin, 0);
	clippedRect.Xmax = imax(imin(sourceRect.Xmax, getWidth()), 0);
	clippedRect.Ymax = imax(imin(sourceRect.Ymax, getHeight()), 0);
}

void BitmapContainer::clipRect(_R<BitmapContainer> source, const RECT& sourceRect,
			       int32_t destX, int32_t destY, RECT& outputSourceRect,
			       int32_t& outputX, int32_t& outputY) const
{
	int sLeft = imax(sourceRect.Xmin, 0);
	int sTop = imax(sourceRect.Ymin, 0);
	int sRight = imax(imin(sourceRect.Xmax, source->getWidth()), 0);
	int sBottom = imax(imin(sourceRect.Ymax, source->getHeight()), 0);

	int dLeft = destX;
	int dTop = destY;
	if (dLeft < 0)
	{
		sLeft += -dLeft;
		dLeft = 0;
	}
	if (dTop < 0)
	{
		sTop += -dTop;
		dTop = 0;
	}

	int clippedWidth = imax(imin(sRight - sLeft, getWidth() - dLeft), 0);
	int clippedHeight = imax(imin(sBottom - sTop, getHeight() - dTop), 0);

	outputSourceRect.Xmin = sLeft;
	outputSourceRect.Xmax = sLeft + clippedWidth;
	outputSourceRect.Ymin = sTop;
	outputSourceRect.Ymax = sTop + clippedHeight;
	
	outputX = dLeft;
	outputY = dTop;
}

std::vector<uint32_t> BitmapContainer::getPixelVector(const RECT& inputRect) const
{
	RECT rect;
	clipRect(inputRect, rect);

	std::vector<uint32_t> result;
	if ((rect.Xmax - rect.Xmin <= 0) || (rect.Ymax - rect.Ymin <= 0))
		return result;

	result.reserve((rect.Xmax - rect.Xmin)*(rect.Ymax - rect.Ymin));
	for (int32_t y=rect.Ymin; y<rect.Ymax; y++)
	{
		for (int32_t x=rect.Xmin; x<rect.Xmax; x++)
		{
			result.push_back(*getDataNoBoundsChecking(x, y));
		}
	}

	return result;
}
