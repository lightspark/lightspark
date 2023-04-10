/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef BACKENDS_RENDERING_CONTEXT_H
#define BACKENDS_RENDERING_CONTEXT_H 1

#include <stack>
#include "backends/graphics.h"
#include "platforms/engineutils.h"

namespace lightspark
{

enum VertexAttrib { VERTEX_ATTRIB=0, COLOR_ATTRIB, TEXCOORD_ATTRIB};

class Rectangle;
/*
 * The RenderContext contains all (public) functions that are needed by DisplayObjects to draw themselves.
 */
class RenderContext
{
protected:
	/* Modelview matrix manipulation */
	static const float lsIdentityMatrix[16];
	float lsMVPMatrix[16];
	std::stack<float*> lsglMatrixStack;
	~RenderContext(){}
	void lsglMultMatrixf(const float *m);
public:
	enum CONTEXT_TYPE { CAIRO=0, GL };
	RenderContext(CONTEXT_TYPE t);
	CONTEXT_TYPE contextType;
	const DisplayObject* currentMask;

	/* Modelview matrix manipulation */
	void lsglLoadIdentity();
	void lsglLoadMatrixf(const float *m);
	void lsglScalef(float scaleX, float scaleY, float scaleZ);
	void lsglTranslatef(float translateX, float translateY, float translateZ);
	void lsglRotatef(float angle);

	enum COLOR_MODE { RGB_MODE=0, YUV_MODE };
	enum MASK_MODE { NO_MASK = 0, ENABLE_MASK };
	/* Textures */
	/**
		Render a quad of given size using the given chunk
	*/
	virtual void renderTextured(const TextureChunk& chunk, float alpha, COLOR_MODE colorMode,
			float redMultiplier, float greenMultiplier, float blueMultiplier, float alphaMultiplier,
			float redOffset, float greenOffset, float blueOffset, float alphaOffset,
			bool isMask, bool hasMask, float directMode, RGB directColor,SMOOTH_MODE smooth, const MATRIX& matrix,
			Rectangle* scalingGrid=nullptr, AS_BLENDMODE blendmode=BLENDMODE_NORMAL)=0;
	/**
	 * Get the right CachedSurface from an object
	 */
	virtual const CachedSurface& getCachedSurface(const DisplayObject* obj) const=0;
};

class GLRenderContext: public RenderContext
{
private:
	static int errorCount;
protected:
	EngineData* engineData;
	int projectionMatrixUniform;
	int modelviewMatrixUniform;

	int yuvUniform;
	int alphaUniform;
	int maskUniform;
	int colortransMultiplyUniform;
	int colortransAddUniform;
	int directUniform;
	int directColorUniform;
	uint32_t maskframebuffer;
	uint32_t maskTextureID;

	/* Textures */
	Mutex mutexLargeTexture;
	uint32_t largeTextureSize;
	class LargeTexture
	{
	public:
		uint32_t id;
		uint8_t* bitmap;
		LargeTexture(uint8_t* b):id(-1),bitmap(b){}
		~LargeTexture(){/*delete[] bitmap;*/}
	};
	std::vector<LargeTexture> largeTextures;

	~GLRenderContext(){}
	void renderpart(const MATRIX& matrix, const TextureChunk& chunk, float cropleft, float croptop, float cropwidth, float cropheight, float tx, float ty);
public:
	enum LSGL_MATRIX {LSGL_PROJECTION=0, LSGL_MODELVIEW};
	/*
	 * Uploads the current matrix as the specified type.
	 */
	void setMatrixUniform(LSGL_MATRIX m) const;
	GLRenderContext() : RenderContext(GL),engineData(nullptr), largeTextureSize(0)
	{
	}
	void SetEngineData(EngineData* data) { engineData = data;}
	void lsglOrtho(float l, float r, float b, float t, float n, float f);

	void renderTextured(const TextureChunk& chunk, float alpha, COLOR_MODE colorMode,
			float redMultiplier, float greenMultiplier, float blueMultiplier, float alphaMultiplier,
			float redOffset, float greenOffset, float blueOffset, float alphaOffset,
			bool isMask, bool hasMask, float directMode, RGB directColor, SMOOTH_MODE smooth, const MATRIX& matrix,
			Rectangle* scalingGrid=nullptr, AS_BLENDMODE blendmode=BLENDMODE_NORMAL) override;
	/**
	 * Get the right CachedSurface from an object
	 * In the OpenGL case we just get the CachedSurface inside the object itself
	 */
	const CachedSurface& getCachedSurface(const DisplayObject* obj) const override;

	/* Utility */
	bool handleGLErrors() const;
};

class CairoRenderContext: public RenderContext
{
private:
	std::map<const DisplayObject*, CachedSurface> customSurfaces;
	cairo_t* cr;
	std::list<std::pair<cairo_surface_t*,MATRIX>> masksurfaces;
	static cairo_surface_t* getCairoSurfaceForData(uint8_t* buf, uint32_t width, uint32_t height, uint32_t stride);
	/*
	 * An invalid surface to be returned for objects with no content
	 */
	static const CachedSurface invalidSurface;
public:
	CairoRenderContext(uint8_t* buf, uint32_t width, uint32_t height,bool smoothing);
	virtual ~CairoRenderContext();
	void renderTextured(const TextureChunk& chunk, float alpha, COLOR_MODE colorMode,
			float redMultiplier, float greenMultiplier, float blueMultiplier, float alphaMultiplier,
			float redOffset, float greenOffset, float blueOffset, float alphaOffset,
			bool isMask, bool hasMask, float directMode, RGB directColor, SMOOTH_MODE smooth, const MATRIX& matrix,
			Rectangle* scalingGrid=nullptr, AS_BLENDMODE blendmode=BLENDMODE_NORMAL) override;
	/**
	 * Get the right CachedSurface from an object
	 * In the Cairo case we get the right CachedSurface out of the map
	 */
	const CachedSurface& getCachedSurface(const DisplayObject* obj) const override;

	/**
	 * The CairoRenderContext acquires the ownership of the buffer
	 * it will be freed on destruction
	 */
	CachedSurface& allocateCustomSurface(const DisplayObject* d, uint8_t* texBuf, bool isBufferOwner);
	/**
	 * Do a fast non filtered, non scaled blit of ARGB data
	 */
	void simpleBlit(int32_t destX, int32_t destY, uint8_t* sourceBuf, uint32_t sourceTotalWidth, uint32_t sourceTotalHeight,
			int32_t sourceX, int32_t sourceY, uint32_t sourceWidth, uint32_t sourceHeight);
	/**
	 * Do an optionally filtered blit with transformation
	 */
	enum FILTER_MODE { FILTER_NONE = 0, FILTER_SMOOTH };
	void transformedBlit(const MATRIX& m, uint8_t* sourceBuf, uint32_t sourceTotalWidth, uint32_t sourceTotalHeight,
			FILTER_MODE filterMode);
};

}
#endif /* BACKENDS_RENDERING_CONTEXT_H */
