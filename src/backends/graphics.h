/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#define CHUNKSIZE 128

#include "compat.h"
#include "backends/lsopengl.h"
#include <vector>
#include "swftypes.h"
#include "threading.h"
#include <cairo.h>
#include <pango/pango.h>
#include "backends/geometry.h"
#include "memory_support.h"

namespace lightspark
{

class DisplayObject;
class InvalidateQueue;

class TextureBuffer
{
private:
	GLuint texId;
	GLenum filtering;
	uint32_t allocWidth;
	uint32_t allocHeight;
	uint32_t width;
	uint32_t height;
	uint32_t horizontalAlignment;
	uint32_t verticalAlignment;
	bool inited;
	uint32_t nearestPOT(uint32_t a) const;
	void setAllocSize(uint32_t w, uint32_t h);
public:
	/**
	  	TextureBuffer constructor

		@param initNow Create right now the texture (can be true only if created inside the Render Thread)
		@param width The requested width
		@param height The requested height
		@param filtering The requested texture filtering from OpenGL enumeration
	*/
	TextureBuffer(bool initNow, uint32_t width=0, uint32_t height=0, GLenum filtering=GL_NEAREST);
	/**
	  	TextureBuffer destructor

		Destroys the GL resources allocated for this texture
		@pre Should be run inside the RenderThread or shutdown should be already run
	*/
	~TextureBuffer();
	/**
	   	Return the texture id

		@ret The OpenGL texture id
	*/
	GLuint getId() {return texId;}
	/**
	  	Initialize the texture using new values

		@param width The requested width
		@param height The requested height
		@param filtering The requested texture filtering from OpenGL enumeration
		@pre Running inside the RenderThread
	*/
	void init(uint32_t width, uint32_t height, GLenum filtering=GL_NEAREST);
	/**
	  	Frees the GL resources

		@pre Running inside the RenderThread
	*/
	void shutdown();
	/**
		Bind as the current texture

		@pre Running inside the RenderThread
	*/
	void bind();
	/**
		Unbind the current texture

		@pre Running inside the RenderThread
	*/
	void unbind();
	/**
		Set the given uniform with the coordinate scale of the current texture

		@pre Running inside the RenderThread
	*/
	void setTexScale(GLuint uniformLocation);

	void resize(uint32_t width, uint32_t height);
	/**
		Request a minimum alignment for width and height
	*/
	void setRequestedAlignment(uint32_t w, uint32_t h);
	uint32_t getAllocWidth() const { return allocWidth;}
	uint32_t getAllocHeight() const { return allocHeight;}
};

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
	uint32_t* chunks;
	uint32_t texId;
	TextureChunk(uint32_t w, uint32_t h);
public:
	TextureChunk():chunks(NULL),texId(0),width(0),height(0){}
	TextureChunk(const TextureChunk& r);
	TextureChunk& operator=(const TextureChunk& r);
	~TextureChunk();
	bool resizeIfLargeEnough(uint32_t w, uint32_t h);
	uint32_t getNumberOfChunks() const { return ((width+CHUNKSIZE-1)/CHUNKSIZE)*((height+CHUNKSIZE-1)/CHUNKSIZE); }
	bool isValid() const { return chunks; }
	void makeEmpty();
	uint32_t width;
	uint32_t height;
};

class CachedSurface
{
public:
	CachedSurface():xOffset(0),yOffset(0),alpha(1.0){}
	TextureChunk tex;
	int32_t xOffset;
	int32_t yOffset;
	float alpha;
};

class ITextureUploadable
{
protected:
	~ITextureUploadable(){}
public:
	virtual void sizeNeeded(uint32_t& w, uint32_t& h) const=0;
	/*
		Upload data to memory mapped to the graphics card (note: size is guaranteed to be enough
	*/
	virtual void upload(uint8_t* data, uint32_t w, uint32_t h) const=0;
	virtual const TextureChunk& getTexture()=0;
	/*
		Signal the completion of the upload to the texture
		NOTE: fence may be called on shutdown even if the upload has not happen, so be ready for this event
	*/
	virtual void uploadFence()=0;
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
	float alpha;
public:
	IDrawable(int32_t w, int32_t h, int32_t x, int32_t y, float a, const std::vector<MaskData>& m):
		masks(m),width(w),height(h),xOffset(x),yOffset(y),alpha(a){}
	virtual ~IDrawable(){}
	/*
	 * This method returns a raster buffer of the image
	 * The various implementation are responsible for applying the
	 * masks
	 */
	virtual uint8_t* getPixelBuffer()=0;
	/*
	 * This method creates a cairo path that can be used as a mask for
	 * another object
	 */
	virtual void applyCairoMask(cairo_t* cr, int32_t offsetX, int32_t offsetY) const = 0;
	int32_t getWidth() const { return width; }
	int32_t getHeight() const { return height; }
	int32_t getXOffset() const { return xOffset; }
	int32_t getYOffset() const { return yOffset; }
	float getAlpha() const { return alpha; }
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
	void execute();
	void threadAbort();
	void jobFence();
	//ITextureUploadable interface
	void upload(uint8_t* data, uint32_t w, uint32_t h) const;
	void sizeNeeded(uint32_t& w, uint32_t& h) const;
	const TextureChunk& getTexture();
	void uploadFence();
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
	/**
	  The whole transformation matrix that is applied to the rendered object
	*/
	MATRIX matrix;
	/*
	 * There are reports (http://lists.freedesktop.org/archives/cairo/2011-September/022247.html)
	 * that cairo is not threadsafe, and I have encountered some spurious crashes, too.
	 * So we use a global lock for all cairo calls until this issue is sorted out.
	 * TODO: CairoRenderes are enqueued as IThreadJobs, therefore this mutex
	 *       will serialize the thread pool when all thread pool workers are executing CairoRenderers!
	 */
	static StaticRecMutex cairoMutex;
	static void cairoClean(cairo_t* cr);
	cairo_surface_t* allocateSurface(uint8_t*& buf);
	virtual void executeDraw(cairo_t* cr)=0;
public:
	CairoRenderer(const MATRIX& _m, int32_t _x, int32_t _y, int32_t _w, int32_t _h, float _s, float _a, const std::vector<MaskData>& m);
	//IDrawable interface
	uint8_t* getPixelBuffer();
	/*
	 * Converts data (which is in RGB format) to the format internally used by cairo.
	 */
	static void convertBitmapToCairo(std::vector<uint8_t, reporter_allocator<uint8_t>>& data, uint8_t* inData, uint32_t width,
			uint32_t height, size_t* dataSize, size_t* stride);
	/*
	 * Converts data (which is in ARGB format) to the format internally used by cairo.
	 */
	static void convertBitmapWithAlphaToCairo(std::vector<uint8_t, reporter_allocator<uint8_t>>& data, uint8_t* inData, uint32_t width,
			uint32_t height, size_t* dataSize, size_t* stride);
};

class CairoTokenRenderer : public CairoRenderer
{
private:
	static cairo_pattern_t* FILLSTYLEToCairo(const FILLSTYLE& style, double scaleCorrection);
	static bool cairoPathFromTokens(cairo_t* cr, const std::vector<GeomToken>& tokens, double scaleCorrection, bool skipFill);
	static void quadraticBezier(cairo_t* cr, double control_x, double control_y, double end_x, double end_y);
	/*
	   The tokens to be drawn
	*/
	const std::vector<GeomToken> tokens;
	/*
	 * This is run by CairoRenderer::execute()
	 */
	void executeDraw(cairo_t* cr);
	void applyCairoMask(cairo_t* cr, int32_t offsetX, int32_t offsetY) const;
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
	*/
	CairoTokenRenderer(const std::vector<GeomToken>& _g, const MATRIX& _m,
			int32_t _x, int32_t _y, int32_t _w, int32_t _h,
		    float _s, float _a, const std::vector<MaskData>& _ms)
		: CairoRenderer(_m,_x,_y,_w,_h,_s,_a,_ms),tokens(_g){}
	/*
	   Hit testing helper. Uses cairo to find if a point in inside the shape

	   @param tokens The tokens of the shape being tested
	   @param scaleFactor The scale factor to be applied
	   @param x The X in local coordinates
	   @param y The Y in local coordinates
	*/
	static bool hitTest(const std::vector<GeomToken>& tokens, float scaleFactor, number_t x, number_t y);
};

class TextData
{
public:
	/* the default values are from the spec for flash.text.TextField and flash.text.TextFormat */
	TextData() : width(100), height(100), textWidth(0), textHeight(0), font("Times New Roman"), background(false), backgroundColor(0xFFFFFF),
		border(false), borderColor(0x000000), multiline(false), textColor(0x000000),
		autoSize(AS_NONE), fontSize(12), wordWrap(false) {}
	uint32_t width;
	uint32_t height;
	uint32_t textWidth;
	uint32_t textHeight;
	tiny_string text;
	tiny_string font;
	bool background;
	RGB backgroundColor;
	bool border;
	RGB borderColor;
	bool multiline;
	RGB textColor;
	enum AUTO_SIZE {AS_NONE = 0, AS_LEFT, AS_RIGHT, AS_CENTER };
	AUTO_SIZE autoSize;
	uint32_t fontSize;
	bool wordWrap;
};

class CairoPangoRenderer : public CairoRenderer
{
	static StaticMutex pangoMutex;
	/*
	 * This is run by CairoRenderer::execute()
	 */
	void executeDraw(cairo_t* cr);
	TextData textData;
	static void pangoLayoutFromData(PangoLayout* layout, const TextData& tData);
	void applyCairoMask(cairo_t* cr, int32_t offsetX, int32_t offsetY) const;
public:
	CairoPangoRenderer(const TextData& _textData, const MATRIX& _m,
			int32_t _x, int32_t _y, int32_t _w, int32_t _h, float _s, float _a, const std::vector<MaskData>& _ms)
		: CairoRenderer(_m,_x,_y,_w,_h,_s,_a,_ms), textData(_textData) {}
	/**
		Helper. Uses Pango to find the size of the textdata
		@param _texttData The textData being tested
		@param w,h,tw,th are the (text)width and (text)height of the textData.
	*/
	static bool getBounds(const TextData& _textData, uint32_t& w, uint32_t& h, uint32_t& tw, uint32_t& th);
};

class InvalidateQueue
{
public:
	virtual ~InvalidateQueue(){};
	//Invalidation queue management
	virtual void addToInvalidateQueue(_R<DisplayObject> d) = 0;
};

class SoftwareInvalidateQueue: public InvalidateQueue
{
public:
	std::list<_R<DisplayObject>> queue;
	void addToInvalidateQueue(_R<DisplayObject> d);
};

};
#endif /* BACKENDS_GRAPHICS_H */
