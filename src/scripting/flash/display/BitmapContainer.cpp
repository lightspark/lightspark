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

#include "scripting/flash/display/BitmapContainer.h"
#include "backends/rendering_context.h"
#include "backends/image.h"

using namespace std;
using namespace lightspark;

BitmapContainer::BitmapContainer(MemoryAccount* m):stride(0),dataSize(0),width(0),height(0),
	data(reporter_allocator<uint8_t>(m))
{
}

bool BitmapContainer::fromRGB(uint8_t* rgb, uint32_t w, uint32_t h, BITMAP_FORMAT format)
{
	if(!rgb)
		return false;

	width = w;
	height = h;
	if(format==ARGB32)
		CairoRenderer::convertBitmapWithAlphaToCairo(data, rgb, width, height, &dataSize, &stride);
	else
		CairoRenderer::convertBitmapToCairo(data, rgb, width, height, &dataSize, &stride, format==RGB15);
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
	uint8_t *rgb=ImageDecoder::decodePNG(s, &w, &h);
	assert_and_throw((int32_t)w >= 0 && (int32_t)h >= 0);
	return fromRGB(rgb, (int32_t)w, (int32_t)h, RGB24);
}

bool BitmapContainer::fromPalette(uint8_t* inData, uint32_t w, uint32_t h, uint32_t inStride, uint8_t* palette, unsigned numColors, unsigned paletteBPP)
{
	assert(data.empty());
	if (!inData || !palette)
		return false;

	width = w;
	height = h;
	uint8_t *rgb=ImageDecoder::decodePalette(inData, w, h, inStride, palette, numColors, paletteBPP);
	return fromRGB(rgb, (int32_t)w, (int32_t)h, RGB24);
}

void BitmapContainer::clear()
{
	data.clear();
	data.shrink_to_fit();
	stride=0;
	dataSize=0;
	width=0;
	height=0;
}

void BitmapContainer::setAlpha(int32_t x, int32_t y, uint8_t alpha)
{
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;

	uint32_t *p=reinterpret_cast<uint32_t *>(&data[y*stride + 4*x]);
	*p = ((uint32_t)alpha << 24) + (*p & 0xFFFFFF);
}

void BitmapContainer::setPixel(int32_t x, int32_t y, uint32_t color, bool setAlpha)
{
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;

	uint32_t *p=reinterpret_cast<uint32_t *>(&data[y*stride + 4*x]);
	if(setAlpha)
		*p=color;
	else
		*p=(*p & 0xff000000) | (color & 0x00ffffff);
}

uint32_t BitmapContainer::getPixel(int32_t x, int32_t y)
{
	if (x < 0 || x >= width || y < 0 || y >= height)
		return 0;

	const uint32_t *p=reinterpret_cast<const uint32_t *>(&data[y*stride + 4*x]);
	return *p;
}

void BitmapContainer::copyRectangle(_R<BitmapContainer> source,
				    int32_t srcLeft, int32_t srcTop,
				    int32_t destLeft, int32_t destTop,
				    int32_t rectWidth, int32_t rectHeight,
				    bool mergeAlpha)
{
	// Ensure that the coordinates are valid
	int sLeft = imax(srcLeft, 0);
	int sTop = imax(srcTop, 0);
	int sRight = imin(sLeft+rectWidth, source->getWidth());
	int sBottom = imin(sTop+rectHeight, source->getHeight());

	int dLeft = destLeft;
	int dTop = destTop;
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

	int copyWidth = imin(sRight - sLeft, width - dLeft);
	int copyHeight = imin(sBottom - sTop, height - dTop);

	if (copyWidth <= 0 || copyHeight <= 0)
		return;

	if (mergeAlpha==false)
	{
		//Fast path using memmove
		for (int i=0; i<copyHeight; i++)
		{
			memmove(&data[(dTop+i)*stride + 4*dLeft],
				&source->data[(sTop+i)*source->stride + 4*sLeft],
				4*copyWidth);
		}
	}
	else
	{
		//Slow path using Cairo
		CairoRenderContext ctxt(&data[0], width, height);
		ctxt.simpleBlit(dLeft, dTop, &source->data[0],
				source->getWidth(), source->getHeight(),
				sLeft, sTop, copyWidth, copyHeight);
	}
}

void BitmapContainer::fillRectangle(int32_t x, int32_t y, int32_t rectWidth, int32_t rectHeight, uint32_t color, bool useAlpha)
{
	//Clip rectangle
	int32_t rectX=x;
	int32_t rectY=y;
	int32_t rectW=rectWidth;
	int32_t rectH=rectHeight;
	if(rectX<0)
	{
		rectW+=rectX;
		rectX = 0;
	}
	if(rectY<0)
	{
		rectH+=rectY;
		rectY = 0;
	}
	if(rectX + rectW > width)
		rectW = width - rectX;
	if(rectY + rectH > height)
		rectH = height - rectY;

	for(int32_t i=0;i<rectH;i++)
	{
		for(int32_t j=0;j<rectW;j++)
		{
			uint32_t offset=(i+rectY)*stride + (j+rectX)*4;
			uint32_t* ptr=(uint32_t*)(getData()+offset);
			if (useAlpha)
				*ptr = color;
			else
				*ptr = 0xFF000000 | (color & 0xFFFFFF);
		}
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
