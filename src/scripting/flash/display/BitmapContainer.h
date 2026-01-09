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

#include "forwards/scripting/flash/geom/flashgeom.h"
#include "compat.h"
#include "memory_support.h"
#include "smartrefs.h"
#include "swftypes.h"
#include <vector>
#include <queue>
#include "backends/graphics.h"
#include "threading.h"


namespace lightspark
{
class Bitmap;
struct RenderDisplayObjectToBitmapContainer
{
	MATRIX initialMatrix;
	_NR<CachedSurface> cachedsurface;
	ColorTransformBase ct;
	RECT clipRect;
	RGBA backgroundcolor;
	AS_BLENDMODE blendMode;
	uint8_t qualityfactor;
	bool smoothing:1;
	bool hasClipRect:1;
	bool needsfill:1;
	bool needscopy:1;
};

struct BitmapContainerRenderData
{
	std::list<ITextureUploadable*> uploads;
	std::list<RefreshableSurface> surfacesToRefresh;
	std::queue<RenderDisplayObjectToBitmapContainer> rendercalls;
	std::list<Bitmap*> temporaryBitmaps;
	bool readpixels:1;
	bool needswait:1;
};

class BitmapFilter;
class BitmapContainer : public RefCountable
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
	// buffer to contain the pixels transformed by latest ColorTransformation
	std::vector<uint8_t> data_colortransformed;
	// color transformation values currently applied to data_colortransformed
	ColorTransformBase currentcolortransform;
	uint32_t *getDataNoBoundsChecking(int32_t x, int32_t y);
	uint8_t* getCurrentData();
	void checkModifiedTexture();
	bool hasModifiedData;
	bool hasModifiedTexture;
	bool needsclear;
	bool isfromtag;
	ATOMIC_INT32(currentrenderdata);
	BitmapContainerRenderData renderdata[2];
public:
	BitmapContainerRenderData* getRenderData() { return &renderdata[currentrenderdata];}
	BitmapContainerRenderData* swapRenderData()
	{
		currentrenderdata=1-currentrenderdata;
		return &renderdata[1-currentrenderdata];
	}
	Semaphore renderevent;
	TextureChunk bitmaptexture;
	int nanoVGImageHandle;
	RGBA nanoVGImageBackgroundcolor;
	cairo_pattern_t* cachedCairoPattern;
	BitmapContainer(MemoryAccount* m, bool _isfromtag=false);
	~BitmapContainer();
	uint32_t getDataSize() const { return data.size(); }
	uint8_t* getData() { return getCurrentData(); }
	uint8_t* getOriginalData() { return &data[0]; }
	uint8_t* getDataColorTransformed() 
	{
		data_colortransformed.reserve(data.size());
		return &data_colortransformed[0];
	}
	uint8_t *applyColorTransform(ColorTransform* ctransform);
	uint8_t *applyColorTransform(number_t redMulti, number_t greenMulti, number_t blueMulti, number_t alphaMulti,
								 number_t redOff, number_t greenOff, number_t blueOff, number_t alphaOff);
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
	uint32_t getPixel(int32_t x, int32_t y, bool premultiplied=true);
	std::vector<uint32_t> getPixelVector(const RECT& rect, bool premultiplied=true);
	void copyRectangle(_R<BitmapContainer> source, 
			   const RECT& sourceRect,
			   int32_t destX, int32_t destY,
			   bool mergeAlpha);
	void applyFilter(_R<BitmapContainer> source,
				const RECT& sourceRect,
				number_t destX, number_t destY,
				BitmapFilter* filter);
	void fillRectangle(const RECT& rect, uint32_t color, bool useAlpha);
	bool scroll(int32_t x, int32_t y);
	void floodFill(int32_t x, int32_t y, uint32_t color);
	int getWidth() const { return width; }
	int getHeight() const { return height; }
	bool isEmpty() const { return data.empty(); }
	bool isFromTag() const { return isfromtag; }

	bool checkTextureForUpload(SystemState* sys);
	void clone(BitmapContainer* c);
	void setModifiedData(bool modified);
	void setModifiedTexture(bool modified);
	bool getModifiedData() const { return hasModifiedData; }
	bool getModifiedTexture() const { return hasModifiedTexture; }
	void setNeedsClear(bool clear) { needsclear=clear; }
	bool getNeedsClear() const { return needsclear; }
	void addRenderCall(RenderDisplayObjectToBitmapContainer& call);
	void flushRenderCalls(RenderThread* renderthread, Bitmap* tempBitmap=nullptr, bool wait=true);
	void addTemporaryBitmap(RenderThread* renderthread, Bitmap* tempBitmap);
};

}
#endif /* SCRIPTING_FLASH_DISPLAY_BITMAPCONTAINER_H */
