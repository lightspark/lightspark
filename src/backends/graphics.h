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
#define TWIPS_SCALING_FACTOR 1.0/20.0

#include "forwards/swftypes.h"
#include "forwards/backends/cachedsurface.h"
#include "forwards/backends/geometry.h"
#include "interfaces/backends/graphics.h"
#include "interfaces/threading.h"
#include "compat.h"
#include <vector>
#include "smartrefs.h"
#include "swftypes.h"
#include <cairo.h>
#include <pango/pango.h>
#include "backends/geometry.h"
#include "memory_support.h"

namespace lightspark
{
class RenderThread;
class SurfaceState;
class DisplayObject;
	
enum SMOOTH_MODE { SMOOTH_NONE=0, SMOOTH_SUBPIXEL=1, SMOOTH_ANTIALIAS=2 };

struct RectF
{
	Vector2f min;
	Vector2f max;
	Vector2f tl() const { return min; }
	Vector2f tr() const { return Vector2f(max.x, min.y); }
	Vector2f bl() const { return Vector2f(min.x, max.y); }
	Vector2f br() const { return max; }
	Vector2f size() const { return max - min; }
	RectF _union(const RectF& r) const
	{
		return RectF {
			Vector2f(
				dmin(min.x, r.min.x),
				dmin(min.y, r.min.y)
			),
			Vector2f(
				dmax(max.x, r.max.x),
				dmax(max.y, r.max.y)
			)

		};
	}
	RectF operator*(const MATRIX& r) const
	{
		auto p0 = r.multiply2D(tl());
		auto p1 = r.multiply2D(bl());
		auto p2 = r.multiply2D(tr());
		auto p3 = r.multiply2D(br());
		return RectF {
			Vector2f(
				dmin(dmin(p0.x, p1.x), dmin(p2.x, p3.x)),
				dmin(dmin(p0.y, p1.y), dmin(p2.y, p3.y))
			),
			Vector2f(
				dmax(dmax(p0.x, p1.x), dmax(p2.x, p3.x)),
				dmax(dmax(p0.y, p1.y), dmax(p2.y, p3.y))
			)
		};
	}
	RectF& operator*=(const MATRIX& r) { return *this = *this * r; }
};

class TextureChunk
{
friend class GLRenderContext;
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
	void setChunks(uint8_t* buf);
	uint32_t width = 0;
	uint32_t height = 0;
	number_t xContentScale = 1; // scale the bitmap content was generated for
	number_t yContentScale = 1;
	number_t xOffset = 0; // texture topleft from Shape origin
	number_t yOffset = 0;
};

class ColorTransformBase
{
public:
	number_t redMultiplier;
	number_t greenMultiplier;
	number_t blueMultiplier;
	number_t alphaMultiplier;
	number_t redOffset;
	number_t greenOffset;
	number_t blueOffset;
	number_t alphaOffset;
	ColorTransformBase():
		redMultiplier(1.0),
		greenMultiplier(1.0),
		blueMultiplier(1.0),
		alphaMultiplier(1.0),
		redOffset(0.0),
		greenOffset(0.0),
		blueOffset(0.0),
		alphaOffset(0.0)
	{}
	
	ColorTransformBase(const ColorTransformBase& r)
	{
		*this=r;
	}
	ColorTransformBase& operator=(const ColorTransformBase& r)
	{
		redMultiplier=r.redMultiplier;
		greenMultiplier=r.greenMultiplier;
		blueMultiplier=r.blueMultiplier;
		alphaMultiplier=r.alphaMultiplier;
		redOffset=r.redOffset;
		greenOffset=r.greenOffset;
		blueOffset=r.blueOffset;
		alphaOffset=r.alphaOffset;
		return *this;
	}
	bool operator==(const ColorTransformBase& r)
	{
		return redMultiplier==r.redMultiplier &&
				greenMultiplier==r.greenMultiplier &&
				blueMultiplier==r.blueMultiplier &&
				alphaMultiplier==r.alphaMultiplier &&
				redOffset==r.redOffset &&
				greenOffset==r.greenOffset &&
				blueOffset==r.blueOffset &&
				alphaOffset==r.alphaOffset;
	}
	void fillConcatenated(DisplayObject* src, bool ignoreBlendMode=false);
	void applyTransformation(uint8_t* bm, uint32_t size) const;
	uint8_t* applyTransformation(BitmapContainer* bm);
	bool isIdentity() const
	{
		return (redMultiplier==1.0 &&
				greenMultiplier==1.0 &&
				blueMultiplier==1.0 &&
				alphaMultiplier==1.0 &&
				redOffset==0.0 &&
				greenOffset==0.0 &&
				blueOffset==0.0 &&
				alphaOffset==0.0);
	}
	void resetTransformation()
	{
		redMultiplier=1.0;
		greenMultiplier=1.0;
		blueMultiplier=1.0;
		alphaMultiplier=1.0;
		redOffset=0.0;
		greenOffset=0.0;
		blueOffset=0.0;
		alphaOffset=0.0;
	}
	
	// returning r,g,b,a values are between 0.0 and 1.0
	void applyTransformation(const RGBA &color, float& r, float& g, float& b, float &a)
	{
		a = std::max(0.0f,std::min(255.0f,float((color.Alpha * alphaMultiplier * 255.0f)/255.0f + alphaOffset)))/255.0f;
		r = std::max(0.0f,std::min(255.0f,float((color.Red   *   redMultiplier * 255.0f)/255.0f +   redOffset)))/255.0f;
		g = std::max(0.0f,std::min(255.0f,float((color.Green * greenMultiplier * 255.0f)/255.0f + greenOffset)))/255.0f;
		b = std::max(0.0f,std::min(255.0f,float((color.Blue  *  blueMultiplier * 255.0f)/255.0f +  blueOffset)))/255.0f;
	}
	ColorTransformBase multiplyTransform(const ColorTransformBase& r)
	{
		ColorTransformBase ret;
		ret.redMultiplier = redMultiplier * r.redMultiplier;
		ret.greenMultiplier = greenMultiplier * r.greenMultiplier;
		ret.blueMultiplier = blueMultiplier * r.blueMultiplier;
		ret.alphaMultiplier = alphaMultiplier * r.alphaMultiplier;

		ret.redOffset = redOffset + redMultiplier * r.redOffset;
		ret.greenOffset = greenOffset + greenMultiplier * r.greenOffset;
		ret.blueOffset = blueOffset + blueMultiplier * r.blueOffset;
		ret.alphaOffset = alphaOffset + alphaMultiplier * r.alphaOffset;
		return ret;
	}
};

struct RefreshableSurface
{
	IDrawable* drawable;
	_NR<DisplayObject> displayobject;
};
struct RenderDisplayObjectToBitmapContainer
{
	MATRIX initialMatrix;
	_NR<CachedSurface> cachedsurface;
	_NR<BitmapContainer> bitmapcontainer;
	bool smoothing;
	AS_BLENDMODE blendMode;
	ColorTransformBase* ct;
	std::list<ITextureUploadable*> uploads;
	std::list<RefreshableSurface> surfacesToRefresh;
};

class IDrawable
{
protected:
	int32_t width;
	int32_t height;
	float xContentScale;
	float yContentScale;
	SurfaceState* state;
public:
	IDrawable(float w, float h, float x, float y,
			  float xs, float ys, float xcs, float ycs,
			  bool _ismask, bool _cacheAsBitmap,
			  float _scaling, float a,
			  const ColorTransformBase& _colortransform,
			  SMOOTH_MODE _smoothing,
			  AS_BLENDMODE _blendmode,
			  const MATRIX& _m);
	IDrawable(float _width, float _height, float _xContentScale, float _yContentScale,SurfaceState* _state)
		:width(_width),height(_height), xContentScale(_xContentScale), yContentScale(_yContentScale),state(_state)
	{
		// state will be deleted in cachedSurface
	}
	virtual ~IDrawable();
	/*
	 * This method returns a raster buffer of the image
	 * The various implementation are responsible for applying the
	 * masks
	 */
	virtual uint8_t* getPixelBuffer(bool* isBufferOwner=nullptr, uint32_t* bufsize=nullptr)=0;
	virtual bool isCachedSurfaceUsable(const DisplayObject*) const {return true;}
	int32_t getWidth() const { return width; }
	int32_t getHeight() const { return height; }
	float getXContentScale() const { return xContentScale; }
	float getYContentScale() const { return yContentScale; }
	SurfaceState* getState() const { return state; }
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
	void contentScale(number_t& x, number_t& y) const override;
	void contentOffset(number_t& x, number_t& y) const override;
	DisplayObject* getOwner() { return owner.getPtr(); }
};

/**
	The base class for render jobs based on cairo
	Stores an internal copy of the data to be rendered
*/
class CairoRenderer: public IDrawable
{
protected:
	static void cairoClean(cairo_t* cr);
	cairo_surface_t* allocateSurface(uint8_t*& buf);
	virtual void executeDraw(cairo_t* cr)=0;
	static void copyRGB15To24(uint32_t& dest, uint8_t* src);
	static void copyRGB24To24(uint32_t& dest, uint8_t* src);
public:
	CairoRenderer(const MATRIX& _m, float _x, float _y, float _w, float _h
				  , float _xs, float _ys
				  , bool _ismask, bool _cacheAsBitmap
				  , float _scaling, float _a
				  , const ColorTransformBase& _colortransform
				  , SMOOTH_MODE _smoothing
				  , AS_BLENDMODE _blendmode);
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
	static void adjustFillStyle(cairo_t* cr, const FILLSTYLE* style, const MATRIX& origmat, double scaleCorrection);
	static void executefill(cairo_t* cr, const FILLSTYLE* style, cairo_pattern_t* pattern, double scaleCorrection);
	static void executestroke(cairo_t* stroke_cr, const LINESTYLE2* style, cairo_pattern_t* pattern, double scaleCorrection, bool isMask, CairoTokenRenderer* th);
	static cairo_pattern_t* FILLSTYLEToCairo(const FILLSTYLE& style, double scaleCorrection, bool isMask);
	static bool cairoPathFromTokens(cairo_t* cr, const tokensVector &tokens, double scaleCorrection, bool skipFill, bool isMask, number_t xstart, number_t ystart, CairoTokenRenderer* th=nullptr, int* starttoken=nullptr);
	static void quadraticBezier(cairo_t* cr, double control_x, double control_y, double end_x, double end_y);
	/*
	   The tokens to be drawn
	*/
	const tokensVector tokens;
	/*
	 * This is run by CairoRenderer::execute()
	 */
	void executeDraw(cairo_t* cr) override;
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
			float _xs, float _ys,
			bool _ismask, bool _cacheAsBitmap,
			float _scaling, float _a,
			const ColorTransformBase& _colortransform,
			SMOOTH_MODE _smoothing, AS_BLENDMODE _blendmode,
			number_t _xstart, number_t _ystart);
	/*
	   Hit testing helper. Uses cairo to find if a point in inside the shape

	   @param tokens The tokens of the shape being tested
	   @param scaleFactor The scale factor to be applied
	   @param x The X in local coordinates
	   @param y The Y in local coordinates
	*/
	static bool hitTest(const tokensVector& tokens, float scaleFactor, const Vector2f& point, bool includeBoundsRect=false);
};

struct FormatText
{
	bool bullet {false};
	bool bold {false};
	bool italic {false};
	bool underline {false};
	enum ALIGNMENT {AS_NONE = 0, AS_LEFT, AS_RIGHT, AS_CENTER };
	ALIGNMENT align {AS_NONE};
	RGB fontColor {0x000000};
	uint32_t fontSize;
	tiny_string font;
	tiny_string url;
	tiny_string target;
};

struct textline
{
	tiny_string text;
	number_t autosizeposition;
	uint32_t textwidth;
	uint32_t height;
	FormatText format;
};

class FontTag;
class DLL_PUBLIC TextData
{
friend class CairoPangoRenderer;
protected:
	std::vector<textline> textlines;
public:
	/* the default values are from the spec for flash.text.TextField and flash.text.TextFormat */
	TextData() : width(100), height(100),leading(0), textWidth(0), textHeight(0), font("Times New Roman"),fontID(UINT32_MAX), scrollH(0), scrollV(1), background(false), backgroundColor(0xFFFFFF),
		border(false), borderColor(0x000000), multiline(false),isBold(false),isItalic(false), textColor(0x000000),
		autoSize(AS_NONE),align(AS_NONE), fontSize(12), wordWrap(false),caretblinkstate(false),isPassword(false),
		embeddedFont(nullptr)
	{}
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
	FontTag* embeddedFont;
	tiny_string getText(uint32_t line=UINT32_MAX) const;
	void setText(const char* text, bool firstlineonly=false);
	void appendText(const char* text, bool firstlineonly=false, const FormatText* format = nullptr);
	void appendFormatText(const char* text, const FormatText& format, bool firstlineonly=false);
	void getTextSizes(const tiny_string& text, number_t& tw, number_t& th);
	bool TextIsEqual(const std::vector<tiny_string>& lines) const;
	uint32_t getLineCount() const { return textlines.size(); }
	FontTag* checkEmbeddedFont(DisplayObject* d);
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
	static PangoRectangle lineExtents(PangoLayout *layout, int lineNumber);
public:
	CairoPangoRenderer(const TextData& _textData, const MATRIX& _m,
			int32_t _x, int32_t _y, int32_t _w, int32_t _h,
			float _xs, float _ys,
			bool _ismask, bool _cacheAsBitmap,
			float _s, float _a, 
			const ColorTransformBase& _colortransform,
			SMOOTH_MODE _smoothing,
			AS_BLENDMODE _blendmode,
			uint32_t _ci)
		: CairoRenderer(_m,_x,_y,_w,_h,_xs, _ys,_ismask,_cacheAsBitmap,_s,_a,
						_colortransform,
						_smoothing, _blendmode), textData(_textData),caretIndex(_ci) {}
	/**
		Helper. Uses Pango to find the size of the textdata
		@param _texttData The textData being tested
		@param w,h,tw,th are the (text)width and (text)height of the textData.
	*/
	static bool getBounds(const TextData& tData, const tiny_string& text, number_t& tw, number_t& th);
	static std::vector<LineData> getLineData(const TextData& _textData);
};

class RefreshableDrawable: public IDrawable
{
public:
	RefreshableDrawable(float _x, float _y, float _w, float _h
				  , float _xs, float _ys
				  , bool _ismask, bool _cacheAsBitmap
				  , float _scaling, float _a
				  , const ColorTransformBase& _colortransform
				  , SMOOTH_MODE _smoothing,AS_BLENDMODE _blendmode, const MATRIX& _m);
	//IDrawable interface
	uint8_t* getPixelBuffer(bool* isBufferOwner=nullptr, uint32_t* bufsize=nullptr) override { return nullptr; }
};

class BitmapRenderer: public IDrawable
{
protected:
	_NR<BitmapContainer> data;
public:
	BitmapRenderer(_NR<BitmapContainer> _data, float _x, float _y, float _w, float _h
				  , float _xs, float _ys
				  , bool _ismask, bool _cacheAsBitmap
				  , float _a
				  , const ColorTransformBase& _colortransform
				  , SMOOTH_MODE _smoothing,AS_BLENDMODE _blendmode, const MATRIX& _m);
	//IDrawable interface
	uint8_t* getPixelBuffer(bool* isBufferOwner=nullptr, uint32_t* bufsize=nullptr) override;
};

class InvalidateQueue
{
protected:
	_NR<DisplayObject> cacheAsBitmapObject;
public:
	InvalidateQueue(_NR<DisplayObject> _cacheAsBitmapObject=NullRef);
	virtual ~InvalidateQueue();
	//Invalidation queue management
	virtual void addToInvalidateQueue(_R<DisplayObject> d) = 0;
	_NR<DisplayObject> getCacheAsBitmapObject() const;
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
