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

#ifndef SCRIPTING_FLASH_DISPLAY_BITMAPCONTAINER_H
#define SCRIPTING_FLASH_DISPLAY_BITMAPCONTAINER_H 1

#include "compat.h"
#include "memory_support.h"
#include "smartrefs.h"
#include "swftypes.h"
#include <vector>
#include "backends/graphics.h"

namespace lightspark
{
class BitmapFilter;
class BitmapContainer : public RefCountable, public ITextureUploadable
{
public:
	enum BITMAP_FORMAT { RGB15, RGB24, RGB32, ARGB32 };
protected:
	size_t stride;
	int32_t width;
	int32_t height;
	/* the pixel data in premultiplied, native-endian 32 bit
	 * ARGB format. stride is the number of bytes per row, may be
	 * larger than width. */
	std::vector<uint8_t, reporter_allocator<uint8_t>> data;
	// buffer to contain the 
	std::vector<uint8_t> data_colortransformed;
	uint32_t *getDataNoBoundsChecking(int32_t x, int32_t y) const;
public:
	TextureChunk bitmaptexture;
	int nanoVGImageHandle;
	BitmapContainer(MemoryAccount* m);
	~BitmapContainer();
	uint32_t getDataSize() const { return data.size(); }
	uint8_t* getData() { return &data[0]; }
	const uint8_t* getData() const { return &data[0]; }
	uint8_t* getDataColorTransformed() 
	{
		data_colortransformed.reserve(data.size());
		return &data_colortransformed[0];
	}
	// this creates a new byte array that has to be deleted by the caller
	uint8_t* getRectangleData(const RECT& sourceRect);
	bool fromRGB(uint8_t* rgb, uint32_t width, uint32_t height, BITMAP_FORMAT format, bool frompng = false);
	bool fromJPEG(uint8_t* data, int len, const uint8_t *tablesData=NULL, int tablesLen=0);
	bool fromJPEG(std::istream& s);
	bool fromPNG(std::istream& s);
	bool fromPNG(uint8_t* data, int len);
	bool fromGIF(uint8_t* data, int len, SystemState* sys);
	bool fromPalette(uint8_t* inData, uint32_t width, uint32_t height, uint32_t inStride, uint8_t* palette, unsigned numColors, unsigned paletteBPP);
	void fromRawData(uint8_t* data, uint32_t width, uint32_t height);
	// Clip sourceRect coordinates to this BitmapContainer. The
	// output coordinates can be used to access pixels in data
	// without out-of-bounds errors.
	void clipRect(const RECT& sourceRect, RECT& clippedRect) const;
	// Clip a rectangle to fit both source and destination
	// bitmaps.
	void clipRect(_R<BitmapContainer> source, const RECT& sourceRect,
		      int32_t destX, int32_t destY, RECT& outputSourceRect,
		      int32_t& outputX, int32_t& outputY) const;
	void setAlpha(int32_t x, int32_t y, uint8_t alpha);
	void setPixel(int32_t x, int32_t y, uint32_t color, bool setAlpha, bool ispremultiplied=true);
	uint32_t getPixel(int32_t x, int32_t y, bool premultiplied=true) const;
	std::vector<uint32_t> getPixelVector(const RECT& rect) const;
	void copyRectangle(_R<BitmapContainer> source, 
			   const RECT& sourceRect,
			   int32_t destX, int32_t destY,
			   bool mergeAlpha);
	void applyFilter(_R<BitmapContainer> source,
				const RECT& sourceRect,
				int32_t destX, int32_t destY,
				BitmapFilter* filter);
	void fillRectangle(const RECT& rect, uint32_t color, bool useAlpha);
	bool scroll(int32_t x, int32_t y);
	void floodFill(int32_t x, int32_t y, uint32_t color);
	int getWidth() const { return width; }
	int getHeight() const { return height; }
	bool isEmpty() const { return data.empty(); }
	void clear();

	//ITextureUploadable interface
	void sizeNeeded(uint32_t& w, uint32_t& h) const override { w=width; h=height; }
	uint8_t* upload(bool refresh) override;
	TextureChunk& getTexture() override;
	void uploadFence() override;

	bool checkTexture();
};

}
#endif /* SCRIPTING_FLASH_DISPLAY_BITMAPCONTAINER_H */
