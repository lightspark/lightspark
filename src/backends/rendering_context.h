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

#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

#include <stack>
#include "lsopengl.h"
#include "graphics.h"

namespace lightspark
{

enum VertexAttrib { VERTEX_ATTRIB=0, COLOR_ATTRIB, TEXCOORD_ATTRIB};
enum LSGL_MATRIX {LSGL_PROJECTION=0, LSGL_MODELVIEW};

/*
 * The RenderContext contains all (public) functions that are needed by DisplayObjects to draw themselves.
 */
class RenderContext
{
protected:
	/* Masks */
	class MaskData
	{
	public:
		DisplayObject* d;
		MATRIX m;
		MaskData(DisplayObject* _d, const MATRIX& _m):d(_d),m(_m){}
	};
	std::vector<MaskData> maskStack;
	/* Modelview matrix manipulation */
	static const GLfloat lsIdentityMatrix[16];
	GLfloat lsMVPMatrix[16];
	std::stack<GLfloat*> lsglMatrixStack;
	~RenderContext(){}
public:
	enum CONTEXT_TYPE { SOFTWARE=0, GL };
	RenderContext(CONTEXT_TYPE t);
	CONTEXT_TYPE contextType;
	/* Masks */
	/**
		Add a mask to the stack mask
		@param d The DisplayObject used as a mask
		@param m The total matrix from the parent of the object to stage
		\pre A reference is not acquired, we assume the object life is protected until the corresponding pop
	*/
	void pushMask(DisplayObject* d, const MATRIX& m)
	{
		maskStack.push_back(MaskData(d,m));
	}
	/**
		Remove the last pushed mask
	*/
	void popMask()
	{
		maskStack.pop_back();
	}
	bool isMaskPresent()
	{
		return !maskStack.empty();
	}

	/* Modelview matrix manipulation */
	void lsglLoadIdentity();
	void lsglPushMatrix();
	void lsglPopMatrix();
	void lsglLoadMatrixf(const GLfloat *m);
	void lsglMultMatrixf(const GLfloat *m);
	void lsglScalef(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ);
	void lsglTranslatef(GLfloat translateX, GLfloat translateY, GLfloat translateZ);

	/*
	 * Uploads the current matrix as the specified type.
	 */
	virtual void setMatrixUniform(LSGL_MATRIX m) const=0;

	virtual void setYUVtoRGBConversion(bool b)=0;

	virtual void setMask(bool b)=0;
	virtual void setAlpha(float a)=0;

	/* Textures */
	/**
		Render a quad of given size using the given chunk
	*/
	virtual void renderTextured(const TextureChunk& chunk, int32_t x, int32_t y, uint32_t w, uint32_t h)=0;
};

class GLRenderContext: public RenderContext
{
protected:
	GLuint fboId;

	GLint projectionMatrixUniform;
	GLint modelviewMatrixUniform;

	GLint yuvUniform;
	GLint maskUniform;
	GLint alphaUniform;

	bool useMask;

	/* Textures */
	Mutex mutexLargeTexture;
	uint32_t largeTextureSize;
	class LargeTexture
	{
	public:
		GLuint id;
		uint8_t* bitmap;
		LargeTexture(uint8_t* b):id(-1),bitmap(b){}
		~LargeTexture(){/*delete[] bitmap;*/}
	};
	std::vector<LargeTexture> largeTextures;

	void renderMaskToTmpBuffer();
	~GLRenderContext(){}
public:
	GLRenderContext() : RenderContext(GL), useMask(false), largeTextureSize(0)
	{
	}
	void lsglOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	/*
	 * Uploads the current matrix as the specified type.
	 */
	void setMatrixUniform(LSGL_MATRIX m) const;

	void renderTextured(const TextureChunk& chunk, int32_t x, int32_t y, uint32_t w, uint32_t h);

	/* Utility */
	static bool handleGLErrors();

	void setYUVtoRGBConversion(bool b);
	void setMask(bool b);
	void setAlpha(float a);
};

}
#endif
