/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef BACKENDS_GRAPHICS_H
#define BACKENDS_GRAPHICS_H 1

#define CHUNKSIZE_REAL 126 // 1 pixel on each side is used for clamping to edge
#define CHUNKSIZE 128

#include "compat.h"
#include <vector>
#include "swftypes.h"
#include "threading.h"
#include <cairo.h>
#include <pango/pango.h>
#include "backends/geometry.h"
#include "memory_support.h"

namespace lightspark
{

enum SMOOTH_MODE { SMOOTH_NONE=0, SMOOTH_SUBPIXEL=1, SMOOTH_ANTIALIAS=2 };

class DisplayObject;
class InvalidateQueue;
class ColorTransform;

class TextureChunk
{
friend class GLRenderContext;
friend class CairoRenderContext;
friend class RenderThread;
private:
	/*
	 * For GLRenderContext texId is an OpenGL texture id and chunks is an array of used
	 * chunks inside such texture.
	 * For CairoRenderContext texId is an arbitrary id for the texture and chunks is
	 * not used.
	 */
	uint32_t* chunks = nullptr;
	uint32_t texId = 0;
	TextureChunk(uint32_t w, uint32_t h);
public:
	TextureChunk() {}
	TextureChunk(const TextureChunk& r);
	TextureChunk& operator=(const TextureChunk& r);
	~TextureChunk();
	bool resizeIfLargeEnough(uint32_t w, uint32_t h);
	uint32_t getNumberOfChunks() const { return ((width+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL)*((height+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL); }
	bool isValid() const { return chunks; }
	void makeEmpty();
	uint32_t width = 0;
	uint32_t height = 0;
	float xContentScale = 1; // scale the bitmap content was generated for
	float yContentScale = 1;
	float xOffset = 0; // texture topleft from Shape origin
	float yOffset = 0;
};

class CachedSurface
{
public:
	CachedSurface():tex(nullptr),xOffset(0),yOffset(0),xOffsetTransformed(0),yOffsetTransformed(0),widthTransformed(0),heightTransformed(0),alpha(1.0),rotation(0.0),xscale(1.0),yscale(1.0),
		redMultiplier(1.0), greenMultiplier(1.0), blueMultiplier(1.0), alphaMultiplier(1.0), redOffset(0.0), greenOffset(0.0), blueOffset(0.0), alphaOffset(0.0)
		,blendmode(BLENDMODE_NORMAL),isMask(false),smoothing(SMOOTH_MODE::SMOOTH_ANTIALIAS),isChunkOwner(true),isValid(false),isInitialized(false),wasUpdated(false){}
	~CachedSurface()
	{
		if (isChunkOwner && tex)
			delete tex;
	}
	TextureChunk* tex;
	int32_t xOffset;
	int32_t yOffset;
	int32_t xOffsetTransformed;
	int32_t yOffsetTransformed;
	int32_t widthTransformed;
	int32_t heightTransformed;
	float alpha;
	float rotation;
	float xscale;
	float yscale;
	float redMultiplier;
	float greenMultiplier;
	float blueMultiplier;
	float alphaMultiplier;
	float redOffset;
	float greenOffset;
	float blueOffset;
	float alphaOffset;
	MATRIX matrix;
	_NR<DisplayObject> mask;
	AS_BLENDMODE blendmode;
	bool isMask;
	SMOOTH_MODE smoothing;
	bool isChunkOwner;
	bool isValid;
	bool isInitialized;
	bool wasUpdated;
};


class ITextureUploadable
{
private:
	bool queued;
protected:
	~ITextureUploadable(){}
public:
	ITextureUploadable():queued(false) {}
	virtual void sizeNeeded(uint32_t& w, uint32_t& h) const=0;
	virtual void contentScale(float& x, float& y) const {x = 1; y = 1;}
	// Texture topleft from Shape origin
	virtual void contentOffset(float& x, float& y) const {x = 0; y = 0;}
	/*
		Upload data to memory mapped to the graphics card (note: size is guaranteed to be enough
	*/
	virtual uint8_t* upload(bool refresh)=0;
	virtual TextureChunk& getTexture()=0;
	/*
		Signal the completion of the upload to the texture
		NOTE: fence may be called on shutdown even if the upload has not happen, so be ready for this event
	*/
	virtual void uploadFence()
	{
		queued=false;
	}
	void setQueued() {queued=true;}
	bool getQueued() const { return queued;}
};

class IDrawable
{
public:
	enum MASK_MODE { HARD_MASK = 0, SOFT_MASK };
	struct MaskData
	{
		IDrawable* m;
		MASK_MODE maskMode;
		MaskData(IDrawable* _m, MASK_MODE _mm):m(_m),maskMode(_mm){}
	};
protected:
	/*
	 * The masks to be applied
	 */
	std::vector<MaskData> masks;
	int32_t width;
	int32_t height;
	/*
	   The minimal x coordinate for all the points being drawn, in local coordinates
	*/
	int32_t xOffset;
	/*
	   The minimal y coordinate for all the points being drawn, in local coordinates
	*/
	int32_t yOffset;
	int32_t xOffsetTransformed;
	int32_t yOffsetTransformed;
	int32_t widthTransformed;
	int32_t heightTransformed;
	float rotation;
	float alpha;
	float xscale;
	float yscale;
	float xContentScale;
	float yContentScale;
	float redMultiplier;
	float greenMultiplier;
	float blueMultiplier;
	float alphaMultiplier;
	float redOffset;
	float greenOffset;
	float blueOffset;
	float alphaOffset;
	bool isMask;
	_NR<DisplayObject> mask;
	SMOOTH_MODE smoothing;
	/**
	  The whole transformation matrix that is applied to the rendered object
	*/
	MATRIX matrix;
public:
	IDrawable(int32_t w, int32_t h, int32_t x, int32_t y,
		int32_t rw, int32_t rh, int32_t rx, int32_t ry, float r,
		float xs, float ys, float xcs, float ycs,
		bool im, _NR<DisplayObject> _mask,
		float a, const std::vector<MaskData>& m,
		float _redMultiplier,float _greenMultiplier,float _blueMultiplier,float _alphaMultiplier,
		float _redOffset,float _greenOffset,float _blueOffset,float _alphaOffset, SMOOTH_MODE _smoothing,
		const MATRIX& _m):
		masks(m),width(w),height(h),xOffset(x),yOffset(y),xOffsetTransformed(rx),yOffsetTransformed(ry),widthTransformed(rw),heightTransformed(rh),rotation(r),
		alpha(a), xscale(xs), yscale(ys), xContentScale(xcs), yContentScale(ycs),
		redMultiplier(_redMultiplier),greenMultiplier(_greenMultiplier),blueMultiplier(_blueMultiplier),alphaMultiplier(_alphaMultiplier),
		redOffset(_redOffset),greenOffset(_greenOffset),blueOffset(_blueOffset),alphaOffset(_alphaOffset),
		isMask(im),mask(_mask),smoothing(_smoothing), matrix(_m) {}
	virtual ~IDrawable();
	/*
	 * This method returns a raster buffer of the image
	 * The various implementation are responsible for applying the
	 * masks
	 */
	virtual uint8_t* getPixelBuffer(bool* isBufferOwner=nullptr, uint32_t* bufsize=nullptr)=0;
	/*
	 * This method creates a cairo path that can be used as a mask for
	 * another object
	 */
	virtual void applyCairoMask(cairo_t* cr, int32_t offsetX, int32_t offsetY) const = 0;
	virtual bool isCachedSurfaceUsable(const DisplayObject*) const {return true;}
	int32_t getWidth() const { return width; }
	int32_t getHeight() const { return height; }
	int32_t getWidthTransformed() const { return widthTransformed; }
	int32_t getHeightTransformed() const { return heightTransformed; }
	int32_t getXOffset() const { return xOffset; }
	int32_t getYOffset() const { return yOffset; }
	int32_t getXOffsetTransformed() const { return xOffsetTransformed; }
	int32_t getYOffsetTransformed() const { return yOffsetTransformed; }
	float getRotation() const { return rotation; }
	float getAlpha() const { return alpha; }
	float getXScale() const { return xscale; }
	float getYScale() const { return yscale; }
	float getXContentScale() const { return xContentScale; }
	float getYContentScale() const { return yContentScale; }
	bool getIsMask() const { return isMask; }
	_NR<DisplayObject> getMask() const { return mask; }
	SMOOTH_MODE getSmoothing() const { return smoothing; }
	float getRedMultiplier() const { return redMultiplier; }
	float getGreenMultiplier() const { return greenMultiplier; }
	float getBlueMultiplier() const { return blueMultiplier; }
	float getAlphaMultiplier() const { return alphaMultiplier; }
	float getRedOffset() const { return redOffset; }
	float getGreenOffset() const { return greenOffset; }
	float getBlueOffset() const { return blueOffset; }
	float getAlphaOffset() const { return alphaOffset; }
	MATRIX& getMatrix() { return matrix; }
};

class AsyncDrawJob: public IThreadJob, public ITextureUploadable
{
private:
	IDrawable* drawable;
	/**
	 * The DisplayObject owning this render request. We incRef/decRef it
	 * in our constructor/destructor to make sure that it does not go away
	 */
	_R<DisplayObject> owner;
	uint8_t* surfaceBytes;
	bool uploadNeeded;
	bool isBufferOwner;
public:
	/*
	 * @param o The DisplayObject that is being rendered. It is a reference to
	 * make sure the object survives until the end of the rendering
	 * @param d IDrawable to be rendered asynchronously. The pointer is now
	 * owned by this instance
	 */
	AsyncDrawJob(IDrawable* d, _R<DisplayObject> o);
	~AsyncDrawJob();
	//IThreadJob interface
	void execute() override;
	void threadAbort() override;
	void jobFence() override;
	//ITextureUploadable interface
	uint8_t* upload(bool refresh) override;
	void sizeNeeded(uint32_t& w, uint32_t& h) const override;
	TextureChunk& getTexture() override;
	void uploadFence() override;
	void contentScale(float& x, float& y) const override;
	void contentOffset(float& x, float& y) const override;
	DisplayObject* getOwner() { return owner.getPtr(); }
};

/**
	The base class for render jobs based on cairo
	Stores an internal copy of the data to be rendered
*/
class CairoRenderer: public IDrawable
{
protected:
	/*
	   The scale to be applied in both the x and y axis.
	   Useful to adapt points defined in pixels and twips (1/20 of pixel)
	*/
	const float scaleFactor;
	static void cairoClean(cairo_t* cr);
	cairo_surface_t* allocateSurface(uint8_t*& buf);
	virtual void executeDraw(cairo_t* cr)=0;
	static void copyRGB15To24(uint32_t& dest, uint8_t* src);
	static void copyRGB24To24(uint32_t& dest, uint8_t* src);
public:
	CairoRenderer(const MATRIX& _m, int32_t _x, int32_t _y, int32_t _w, int32_t _h
				  , int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r
				  , float _xs, float _ys
				  , bool _im, _NR<DisplayObject> mask
				  , float _s, float _a, const std::vector<MaskData>& m
				  , float _redMultiplier, float _greenMultiplier, float _blueMultiplier, float _alphaMultiplier
				  , float _redOffset, float _greenOffset, float _blueOffset, float _alphaOffset
				  , SMOOTH_MODE _smoothing);
	//IDrawable interface
	uint8_t* getPixelBuffer(bool* isBufferOwner=nullptr, uint32_t* bufsize=nullptr) override;
	bool isCachedSurfaceUsable(const DisplayObject*) const override;
	/*
	 * Converts data (which is in RGB format) to the format internally used by cairo.
	 */
	static void convertBitmapToCairo(std::vector<uint8_t, reporter_allocator<uint8_t>>& data, uint8_t* inData, uint32_t width,
					 uint32_t height, size_t* dataSize, size_t* stride, uint32_t bpp);
	/*
	 * Converts data (which is in ARGB or RGBA(png) format) to the format internally used by cairo.
	 */
	static void convertBitmapWithAlphaToCairo(std::vector<uint8_t, reporter_allocator<uint8_t>>& data, uint8_t* inData, uint32_t width,
			uint32_t height, size_t* dataSize, size_t* stride, bool frompng);
};

class CairoTokenRenderer : public CairoRenderer
{
private:
	static cairo_pattern_t* FILLSTYLEToCairo(const FILLSTYLE& style, double scaleCorrection, bool isMask);
	static bool cairoPathFromTokens(cairo_t* cr, const tokensVector &tokens, double scaleCorrection, bool skipFill, bool isMask, number_t xstart, number_t ystart, int* starttoken=nullptr);
	static void quadraticBezier(cairo_t* cr, double control_x, double control_y, double end_x, double end_y);
	/*
	   The tokens to be drawn
	*/
	const tokensVector tokens;
	/*
	 * This is run by CairoRenderer::execute()
	 */
	void executeDraw(cairo_t* cr) override;
	void applyCairoMask(cairo_t* cr, int32_t offsetX, int32_t offsetY) const override;
	number_t xstart;
	number_t ystart;
public:
	/*
	   CairoTokenRenderer constructor

	   @param _o Owner of the surface _t. See comments on 'owner' member.
	   @param _t GL surface where the final drawing will be uploaded
	   @param _g The tokens to be drawn. This is copied internally.
	   @param _m The whole transformation matrix
	   @param _s The scale factor to be applied in both the x and y axis
	   @param _a The alpha factor to be applied
	   @param _ms The masks that must be applied
	   @param _smoothing indicates if the tokens should be rendered with antialiasing
	*/
	CairoTokenRenderer(const tokensVector& _g, const MATRIX& _m,
			int32_t _x, int32_t _y, int32_t _w, int32_t _h,
			int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r,
			float _xs, float _ys,
			bool _im, _NR<DisplayObject> _mask,
			float _s, float _a, const std::vector<MaskData>& _ms,
			float _redMultiplier, float _greenMultiplier, float _blueMultiplier, float _alphaMultiplier,
			float _redOffset, float _greenOffset, float _blueOffset, float _alphaOffset,
			SMOOTH_MODE _smoothing,number_t _xstart, number_t _ystart);
	/*
	   Hit testing helper. Uses cairo to find if a point in inside the shape

	   @param tokens The tokens of the shape being tested
	   @param scaleFactor The scale factor to be applied
	   @param x The X in local coordinates
	   @param y The Y in local coordinates
	*/
	static bool hitTest(const tokensVector& tokens, float scaleFactor, number_t x, number_t y);
};
struct textline
{
	tiny_string text;
	number_t autosizeposition;
	uint32_t textwidth;
};

class DLL_PUBLIC TextData
{
friend class CairoPangoRenderer;
protected:
	std::vector<textline> textlines;
public:
	/* the default values are from the spec for flash.text.TextField and flash.text.TextFormat */
	TextData() : width(100), height(100),leading(0), textWidth(0), textHeight(0), font("Times New Roman"),fontID(UINT32_MAX), scrollH(0), scrollV(1), background(false), backgroundColor(0xFFFFFF),
		border(false), borderColor(0x000000), multiline(false),isBold(false),isItalic(false), textColor(0x000000),
		autoSize(AS_NONE),align(AS_NONE), fontSize(12), wordWrap(false),caretblinkstate(false),isPassword(false) {}
	uint32_t width;
	uint32_t height;
	int32_t leading;
	uint32_t textWidth;
	uint32_t textHeight;
	tiny_string font;
	uint32_t fontID;
	int32_t scrollH; // pixels, 0-based
	int32_t scrollV; // lines, 1-based
	bool background;
	RGB backgroundColor;
	bool border;
	RGB borderColor;
	bool multiline;
	bool isBold;
	bool isItalic;
	RGB textColor;
	enum ALIGNMENT {AS_NONE = 0, AS_LEFT, AS_RIGHT, AS_CENTER };
	ALIGNMENT autoSize;
	ALIGNMENT align;
	uint32_t fontSize;
	bool wordWrap;
	bool caretblinkstate;
	bool isPassword;
	tiny_string getText(uint32_t line=UINT32_MAX) const;
	void setText(const char* text);
	uint32_t getLineCount() const { return textlines.size(); }
};

class LineData {
public:
	LineData(int32_t x, int32_t y, int32_t _width,
		 int32_t _height, int32_t _firstCharOffset, int32_t _length,
		 number_t _ascent, number_t _descent, number_t _leading,
		 number_t _indent):
		extents(x, x+_width, y, y+_height), 
		firstCharOffset(_firstCharOffset), length(_length),
		ascent(_ascent), descent(_descent), leading(_leading),
		indent(_indent) {}
	// position and size
	RECT extents;
	// Offset of the first character on this line
	int32_t firstCharOffset;
	// length of the line in characters
	int32_t length;
	number_t ascent;
	number_t descent;
	number_t leading;
	number_t indent;
};

class CairoPangoRenderer : public CairoRenderer
{
	/*
	 * This is run by CairoRenderer::execute()
	 */
	void executeDraw(cairo_t* cr) override;
	TextData textData;
	uint32_t caretIndex;
	static void pangoLayoutFromData(PangoLayout* layout, const TextData& tData, const tiny_string& text);
	void applyCairoMask(cairo_t* cr, int32_t offsetX, int32_t offsetY) const override;
	static PangoRectangle lineExtents(PangoLayout *layout, int lineNumber);
public:
	CairoPangoRenderer(const TextData& _textData, const MATRIX& _m,
			int32_t _x, int32_t _y, int32_t _w, int32_t _h,
			int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r,
			float _xs, float _ys,
			bool _im, _NR<DisplayObject> _mask,
			float _s, float _a, const std::vector<MaskData>& _ms,
			float _redMultiplier, float _greenMultiplier, float _blueMultiplier, float _alphaMultiplier,
			float _redOffset, float _greenOffset, float _blueOffset, float _alphaOffset,
			SMOOTH_MODE _smoothing,uint32_t _ci)
		: CairoRenderer(_m,_x,_y,_w,_h,_rx,_ry,_rw,_rh,_r,_xs, _ys,_im,_mask,_s,_a,_ms,
						_redMultiplier, _greenMultiplier, _blueMultiplier, _alphaMultiplier,
						_redOffset, _greenOffset, _blueOffset, _alphaOffset,
						_smoothing), textData(_textData),caretIndex(_ci) {}
	/**
		Helper. Uses Pango to find the size of the textdata
		@param _texttData The textData being tested
		@param w,h,tw,th are the (text)width and (text)height of the textData.
	*/
	static bool getBounds(const TextData& tData, const tiny_string& text, number_t& tw, number_t& th);
	static std::vector<LineData> getLineData(const TextData& _textData);
};

class BitmapRenderer: public IDrawable
{
protected:
	_NR<BitmapContainer> data;
public:
	BitmapRenderer(_NR<BitmapContainer> _data, int32_t _x, int32_t _y, int32_t _w, int32_t _h
				  , int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r
				  , float _xs, float _ys
				  , bool _im, NullableRef<DisplayObject> mask
				  , float _a, const std::vector<MaskData>& m
				  , float _redMultiplier, float _greenMultiplier, float _blueMultiplier, float _alphaMultiplier
				  , float _redOffset, float _greenOffset, float _blueOffset, float _alphaOffset
				  , SMOOTH_MODE _smoothing, const MATRIX& _m);
	//IDrawable interface
	uint8_t* getPixelBuffer(bool* isBufferOwner=nullptr, uint32_t* bufsize=nullptr) override;
	void applyCairoMask(cairo_t* cr, int32_t offsetX, int32_t offsetY) const override {}
};

class InvalidateQueue
{
protected:
	_NR<DisplayObject> cacheAsBitmapObject;
public:
	InvalidateQueue(_NR<DisplayObject> _cacheAsBitmapObject=NullRef, bool issoftwarequeue=false):cacheAsBitmapObject(_cacheAsBitmapObject),isSoftwareQueue(issoftwarequeue) {}
	virtual ~InvalidateQueue(){}
	//Invalidation queue management
	virtual void addToInvalidateQueue(_R<DisplayObject> d) = 0;
	_NR<DisplayObject> getCacheAsBitmapObject() const { return cacheAsBitmapObject; }
	bool isSoftwareQueue;
};

class SoftwareInvalidateQueue: public InvalidateQueue
{
public:
	SoftwareInvalidateQueue(_NR<DisplayObject> _cacheAsBitmapObject):InvalidateQueue(_cacheAsBitmapObject,true) {}
	std::list<_R<DisplayObject>> queue;
	void addToInvalidateQueue(_R<DisplayObject> d) override;
};

class CharacterRenderer : public ITextureUploadable
{
	uint8_t* data;
	uint32_t width;
	uint32_t height;
	TextureChunk chunk;
public:
	CharacterRenderer(uint8_t *d, uint32_t w, uint32_t h):data(d),width(w),height(h) {}
	virtual ~CharacterRenderer() { delete[] data; }
	//ITextureUploadable interface
	void sizeNeeded(uint32_t& w, uint32_t& h) const override { w=width; h=height;}
	uint8_t* upload(bool refresh) override;
	TextureChunk& getTexture() override;
};

}
#endif /* BACKENDS_GRAPHICS_H */
