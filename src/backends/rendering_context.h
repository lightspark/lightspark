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
	/* Modelview matrix manipulation */
	static const GLfloat lsIdentityMatrix[16];
	GLfloat lsMVPMatrix[16];
	std::stack<GLfloat*> lsglMatrixStack;
	GLint projectionMatrixUniform;
	GLint modelviewMatrixUniform;

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
public:
	RenderContext() : largeTextureSize(0)
	{
		lsglLoadIdentity();
	}
	/* Modelview matrix manipulation */
	void lsglLoadMatrixf(const GLfloat *m);
	void lsglLoadIdentity();
	void lsglPushMatrix();
	void lsglPopMatrix();
	void lsglMultMatrixf(const GLfloat *m);
	void lsglScalef(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ);
	void lsglTranslatef(GLfloat translateX, GLfloat translateY, GLfloat translateZ);
	void lsglOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	/*
	 * Uploads the current matrix as the specified type.
	 */
	void setMatrixUniform(LSGL_MATRIX m) const;

	/* Textures */
	/**
		Render a quad of given size using the given chunk
	*/
	void renderTextured(const TextureChunk& chunk, int32_t x, int32_t y, uint32_t w, uint32_t h);

	static bool handleGLErrors();
};

}
#endif
