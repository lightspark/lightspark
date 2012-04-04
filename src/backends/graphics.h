/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#define CHUNKSIZE 128

#include "compat.h"
#include "lsopengl.h"
#include <vector>
#include "swftypes.h"
#include "threading.h"
#include <cairo.h>
#include <pango/pango.h>
#include "backends/geometry.h"

namespace lightspark
{

class DisplayObject;

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

class MatrixApplier
{
private:
	struct packedMatrix
	{
		float data[4][4];
	};
	std::vector<packedMatrix> savedStack;
public:
	MatrixApplier();
	MatrixApplier(const MATRIX& m);
	void concat(const MATRIX& m);
	void unapply();
};

class TextureChunk
{
friend class RenderContext;
friend class RenderThread;
private:
	uint32_t texId;
	uint32_t* chunks;
	TextureChunk(uint32_t w, uint32_t h);
public:
	TextureChunk():texId(0),chunks(NULL),width(0),height(0){}
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

/**
	The base class for render jobs based on cairo
	Stores an internal copy of the data to be rendered
*/
class CairoRenderer: public ITextureUploadable, public IThreadJob
{
protected:
	virtual ~CairoRenderer();
	/**
	 * The ASObject owning this render request. We incRef/decRef it
	 * in our constructor/destructor to make sure that it does no go away
	 * (especially the CachedSurface reference below) while we do our work.
	 */
	ASObject* owner;
	/**
	  The target texture for the rendering, must be non const as the operation will update the size
	*/
	CachedSurface& surface;
	/**
	  The whole transformation matrix that is applied to the rendered object
	*/
	MATRIX matrix;
	/*
	   The minimal x coordinate for all the points being drawn, in local coordinates
	*/
	int32_t xOffset;
	/*
	   The minimal y coordinate for all the points being drawn, in local coordinates
	*/
	int32_t yOffset;
	float alpha;
	int32_t width;
	int32_t height;
	/*
	   A pointer to a memory buffer where cairo will draw
	*/
	uint8_t* surfaceBytes;
	/*
	   The scale to be applied in both the x and y axis.
	   Useful to adapt points defined in pixels and twips (1/20 of pixel)
	*/
	const float scaleFactor;
	bool uploadNeeded;
	/*
	 * There are reports (http://lists.freedesktop.org/archives/cairo/2011-September/022247.html)
	 * that cairo is not threadsafe, and I have encountered some spurious crashes, too.
	 * So we use a global lock for all cairo calls until this issue is sorted out.
	 * TODO: CairoRenderes are enqueued as IThreadJobs, therefore this mutex
	 *       will serialize the thread pool when all thread pool workers are executing CairoRenderers!
	 */
	static StaticMutex cairoMutex;
	static cairo_matrix_t MATRIXToCairo(const MATRIX& matrix);
	static void cairoClean(cairo_t* cr);
	cairo_surface_t* allocateSurface();
	virtual void executeDraw(cairo_t* cr)=0;
public:
	CairoRenderer(ASObject* _o, CachedSurface& _t, const MATRIX& _m,
				int32_t _x, int32_t _y, int32_t _w, int32_t _h, float _s, float _a);
	//ITextureUploadable interface
	void sizeNeeded(uint32_t& w, uint32_t& h) const;
	void upload(uint8_t* data, uint32_t w, uint32_t h) const;
	const TextureChunk& getTexture();
	void uploadFence();
	//IThreadJob interface
	void threadAbort();
	void jobFence();
	void execute();
	/*
	 * Converts data (which is in RGB format) to the format internally used by cairo.
	 */
	static void convertBitmapToCairo(std::vector<uint8_t>& data, uint8_t* inData, uint32_t width,
			uint32_t height, size_t* dataSize, size_t* stride);
	/*
	 * Converts data (which is in ARGB format) to the format internally used by cairo.
	 */
	static void convertBitmapWithAlphaToCairo(std::vector<uint8_t>& data, uint8_t* inData, uint32_t width,
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
public:
	/*
	   CairoTokenRenderer constructor

	   @param _o Owner of the surface _t. See comments on 'owner' member.
	   @param _t GL surface where the final drawing will be uploaded
	   @param _g The tokens to be drawn. This is copied internally.
	   @param _m The whole transformation matrix
	   @param _s The scale factor to be applied in both the x and y axis
	   @param _a The alpha factor to be applied
	*/
	CairoTokenRenderer(ASObject* _o, CachedSurface& _t, const std::vector<GeomToken>& _g, const MATRIX& _m,
					   int32_t _x, int32_t _y, int32_t _w, int32_t _h, float _s, float _a)
		: CairoRenderer(_o,_t,_m,_x,_y,_w,_h,_s,_a), tokens(_g) {}
	/*
	   Hit testing helper. Uses cairo to find if a point in inside the shape

	   @param tokens The tokens of the shape being tested
	   @param scaleFactor The scale factor to be applied
	   @param x The X in local coordinates
	   @param y The Y in local coordinates
	*/
	static bool hitTest(const std::vector<GeomToken>& tokens, float scaleFactor, number_t x, number_t y);
	/*
	   Opacity test helper. Uses cairo to render a single pixel and see if it's opaque or not

	   @param tokens The tokens of the shape being tested
	   @param scaleFactor The scale factor to be applied
	   @param x The X in local coordinates
	   @param y The Y in local coordinates
	*/
	static bool isOpaque(const std::vector<GeomToken>& tokens, float scaleFactor, number_t x, number_t y);
};

class TextData
{
public:
	/* the default values are from the spec for flash.text.TextField and flash.text.TextFormat */
	TextData() : width(100), height(100), textWidth(0), textHeight(0), background(false), backgroundColor(0xFFFFFF),
		border(false), borderColor(0x000000), multiline(false), textColor(0x000000),
		wordWrap(false), autoSize(AS_NONE), fontSize(12), font("Times New Roman") {}
	uint32_t width;
	uint32_t height;
	uint32_t textWidth;
	uint32_t textHeight;
	tiny_string text;
	bool background;
	RGB backgroundColor;
	bool border;
	RGB borderColor;
	bool multiline;
	RGB textColor;
	bool wordWrap;
	enum AUTO_SIZE {AS_NONE = 0, AS_LEFT, AS_RIGHT, AS_CENTER };
	AUTO_SIZE autoSize;
	uint32_t fontSize;
	tiny_string font;
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
public:
	CairoPangoRenderer(ASObject* _o, CachedSurface& _t, const TextData& _textData, const MATRIX& _m,
			int32_t _x, int32_t _y, int32_t _w, int32_t _h, float _s, float _a)
		: CairoRenderer(_o,_t,_m,_x,_y,_w,_h,_s,_a), textData(_textData) {}
	/**
		Helper. Uses Pango to find the size of the textdata
		@param _texttData The textData being tested
		@param w,h,tw,th are the (text)width and (text)height of the textData.
	*/
	static bool getBounds(const TextData& _textData, uint32_t& w, uint32_t& h, uint32_t& tw, uint32_t& th);
};

};
#endif
