/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

bool BitmapContainer::fromRGB(uint8_t* rgb, uint32_t w, uint32_t h, bool hasAlpha)
{
	if(!rgb)
		return false;

	width = w;
	height = h;
	if(hasAlpha)
		CairoRenderer::convertBitmapWithAlphaToCairo(data, rgb, width, height, &dataSize, &stride);
	else
		CairoRenderer::convertBitmapToCairo(data, rgb, width, height, &dataSize, &stride);
	delete[] rgb;
	if(data.empty())
	{
		LOG(LOG_ERROR, "Error decoding image");
		return false;
	}

	return true;
}

bool BitmapContainer::fromJPEG(uint8_t *inData, int len)
{
	assert(data.empty());
	/* flash uses signed values for width and height */
	uint32_t w,h;
	bool hasAlpha;
	uint8_t *rgb=ImageDecoder::decodeJPEG(inData, len, &w, &h, &hasAlpha);
	assert_and_throw((int32_t)w >= 0 && (int32_t)h >= 0);
	return fromRGB(rgb, (int32_t)w, (int32_t)h, hasAlpha);
}

bool BitmapContainer::fromJPEG(std::istream &s)
{
	assert(data.empty());
	/* flash uses signed values for width and height */
	uint32_t w,h;
	bool hasAlpha;
	uint8_t *rgb=ImageDecoder::decodeJPEG(s, &w, &h, &hasAlpha);
	assert_and_throw((int32_t)w >= 0 && (int32_t)h >= 0);
	return fromRGB(rgb, (int32_t)w, (int32_t)h, hasAlpha);
}

bool BitmapContainer::fromPNG(std::istream &s)
{
	assert(data.empty());
	/* flash uses signed values for width and height */
	uint32_t w,h;
	uint8_t *rgb=ImageDecoder::decodePNG(s, &w, &h);
	assert_and_throw((int32_t)w >= 0 && (int32_t)h >= 0);
	return fromRGB(rgb, (int32_t)w, (int32_t)h, false);
}

void BitmapContainer::reset()
{
	data.clear();
	data.shrink_to_fit();
	stride=0;
	dataSize=0;
	width=0;
	height=0;
}
