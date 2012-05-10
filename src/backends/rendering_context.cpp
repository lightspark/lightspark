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

//This file implements a few helpers that should be drop-in replacements for
//the Open GL coordinate matrix handling API. GLES 2.0 does not provide this
//API, so applications need to handle the coordinate transformations and keep
//the state themselves.
//
//The functions have the same signature as the original gl ones but with a ls
//prefix added to make their purpose more clear. The main difference from a
//usage point of view compared to the GL API is that the operations take effect
//- the projection of modelview matrix uniforms sent to the shader - only when
//explicitly calling setMatrixUniform.

#include <stdlib.h>
#include <string.h>
#include <stack>
#include "rendering_context.h"
#include "../logger.h"
#include "scripting/flash/display/flashdisplay.h"

using namespace std;
using namespace lightspark;

#define LSGL_MATRIX_SIZE (16*sizeof(GLfloat))

const GLfloat RenderContext::lsIdentityMatrix[16] = {
								1, 0, 0, 0,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1
								};

RenderContext::RenderContext(CONTEXT_TYPE t):contextType(t)
{
	lsglLoadIdentity();
}

void RenderContext::lsglLoadMatrixf(const GLfloat *m)
{
	memcpy(lsMVPMatrix, m, LSGL_MATRIX_SIZE);
}

void RenderContext::lsglLoadIdentity()
{
	lsglLoadMatrixf(lsIdentityMatrix);
}

void RenderContext::lsglPushMatrix()
{
	GLfloat *top = new GLfloat[16];
	memcpy(top, lsMVPMatrix, LSGL_MATRIX_SIZE);
	lsglMatrixStack.push(top);
}

void RenderContext::lsglPopMatrix()
{
	assert(lsglMatrixStack.size() > 0);
	memcpy(lsMVPMatrix, lsglMatrixStack.top(), LSGL_MATRIX_SIZE);
	delete[] lsglMatrixStack.top();
	lsglMatrixStack.pop();
}

void RenderContext::lsglMultMatrixf(const GLfloat *m)
{
	GLfloat tmp[16];
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			GLfloat sum=0;
			for (int k=0;k<4;k++)
			{
				sum += lsMVPMatrix[i+k*4]*m[j*4+k];
			}
			tmp[i+j*4] = sum;
		}
	}
	memcpy(lsMVPMatrix, tmp, LSGL_MATRIX_SIZE);
}

void RenderContext::lsglScalef(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ)
{
	static GLfloat scale[16];

	memcpy(scale, lsIdentityMatrix, LSGL_MATRIX_SIZE);
	scale[0] = scaleX;
	scale[5] = scaleY;
	scale[10] = scaleZ;
	lsglMultMatrixf(scale);
}

void RenderContext::lsglTranslatef(GLfloat translateX, GLfloat translateY, GLfloat translateZ)
{
	static GLfloat trans[16];

	memcpy(trans, lsIdentityMatrix, LSGL_MATRIX_SIZE);
	trans[12] = translateX;
	trans[13] = translateY;
	trans[14] = translateZ;
	lsglMultMatrixf(trans);
}

void GLRenderContext::lsglOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
	GLfloat ortho[16];
	memset(ortho, 0, sizeof(ortho));
	ortho[0] = 2/(r-l);
	ortho[5] = 2/(t-b);
	ortho[10] = 2/(n-f);
	ortho[12] = -(r+l)/(r-l);
	ortho[13] = -(t+b)/(t-b);
	ortho[14] = -(f+n)/(f-n);
	ortho[15] = 1;

	lsglMultMatrixf(ortho);
}

void GLRenderContext::renderTextured(const TextureChunk& chunk, int32_t x, int32_t y, uint32_t w, uint32_t h,
			float alpha, COLOR_MODE colorMode, MASK_MODE maskMode)
{
	if(maskMode==ENABLE_MASK)
	{
		glPushMatrix();
		renderMaskToTmpBuffer();
		glPopMatrix();
	}

	//Set color mode
	glUniform1f(yuvUniform, (colorMode==YUV_MODE)?1:0);
	//Set mask mode
	glUniform1f(maskUniform, (maskMode==ENABLE_MASK)?1.0f:0.0f);
	//Set alpha
	glUniform1f(alphaUniform, alpha);
	//Set matrix
	setMatrixUniform(LSGL_MODELVIEW);

	glBindTexture(GL_TEXTURE_2D, largeTextures[chunk.texId].id);
	const uint32_t blocksPerSide=largeTextureSize/CHUNKSIZE;
	uint32_t startX, startY, endX, endY;
	assert(chunk.getNumberOfChunks()==((chunk.width+CHUNKSIZE-1)/CHUNKSIZE)*((chunk.height+CHUNKSIZE-1)/CHUNKSIZE));

	uint32_t curChunk=0;
	//The 4 corners of each texture are specified as the vertices of 2 triangles,
	//so there are 6 vertices per quad, two of them duplicated (the diagonal)
	//Allocate the data on the stack to reduce heap fragmentation
	GLfloat *vertex_coords = g_newa(GLfloat,chunk.getNumberOfChunks()*12);
	GLfloat *texture_coords = g_newa(GLfloat,chunk.getNumberOfChunks()*12);
	for(uint32_t i=0, k=0;i<chunk.height;i+=CHUNKSIZE)
	{
		startY=h*i/chunk.height;
		endY=min(h*(i+CHUNKSIZE)/chunk.height,h);
		//Take yOffset into account
		startY = (y<0)?startY:y+startY;
		endY = (y<0)?endY:y+endY;
		for(uint32_t j=0;j<chunk.width;j+=CHUNKSIZE)
		{
			startX=w*j/chunk.width;
			endX=min(w*(j+CHUNKSIZE)/chunk.width,w);
			//Take xOffset into account
			startX = (x<0)?startX:x+startX;
			endX = (x<0)?endX:x+endX;
			const uint32_t curChunkId=chunk.chunks[curChunk];
			const uint32_t blockX=((curChunkId%blocksPerSide)*CHUNKSIZE);
			const uint32_t blockY=((curChunkId/blocksPerSide)*CHUNKSIZE);
			const uint32_t availX=min(int(chunk.width-j),CHUNKSIZE);
			const uint32_t availY=min(int(chunk.height-i),CHUNKSIZE);
			float startU=blockX;
			startU/=largeTextureSize;
			float startV=blockY;
			startV/=largeTextureSize;
			float endU=blockX+availX;
			endU/=largeTextureSize;
			float endV=blockY+availY;
			endV/=largeTextureSize;

			//Upper-right triangle of the quad
			texture_coords[k] = startU;
			texture_coords[k+1] = startV;
			vertex_coords[k] = startX;
			vertex_coords[k+1] = startY;
			k+=2;
			texture_coords[k] = endU;
			texture_coords[k+1] = startV;
			vertex_coords[k] = endX;
			vertex_coords[k+1] = startY;
			k+=2;
			texture_coords[k] = endU;
			texture_coords[k+1] = endV;
			vertex_coords[k] = endX;
			vertex_coords[k+1] = endY;
			k+=2;

			//Lower-left triangle of the quad
			texture_coords[k] = startU;
			texture_coords[k+1] = startV;
			vertex_coords[k] = startX;
			vertex_coords[k+1] = startY;
			k+=2;
			texture_coords[k] = endU;
			texture_coords[k+1] = endV;
			vertex_coords[k] = endX;
			vertex_coords[k+1] = endY;
			k+=2;
			texture_coords[k] = startU;
			texture_coords[k+1] = endV;
			vertex_coords[k] = startX;
			vertex_coords[k+1] = endY;
			k+=2;

			curChunk++;
		}
	}

	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, vertex_coords);
	glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);
	glEnableVertexAttribArray(VERTEX_ATTRIB);
	glEnableVertexAttribArray(TEXCOORD_ATTRIB);
	glDrawArrays(GL_TRIANGLES, 0, curChunk*6);
	glDisableVertexAttribArray(VERTEX_ATTRIB);
	glDisableVertexAttribArray(TEXCOORD_ATTRIB);
	handleGLErrors();
}

bool GLRenderContext::handleGLErrors()
{
	int errorCount = 0;
	GLenum err;
	while(1)
	{
		err=glGetError();
		if(err!=GL_NO_ERROR)
		{
			errorCount++;
			LOG(LOG_ERROR,_("GL error ")<< err);
		}
		else
			break;
	}

	if(errorCount)
	{
		LOG(LOG_ERROR,_("Ignoring ") << errorCount << _(" openGL errors"));
	}
	return errorCount;
}

void GLRenderContext::setMatrixUniform(LSGL_MATRIX m) const
{
	GLint uni = (m == LSGL_MODELVIEW) ? modelviewMatrixUniform:projectionMatrixUniform;

	glUniformMatrix4fv(uni, 1, GL_FALSE, lsMVPMatrix);
}

void GLRenderContext::renderMaskToTmpBuffer()
{
	assert(!maskStack.empty());
	//Clear the tmp buffer
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	for(uint32_t i=0;i<maskStack.size();i++)
	{
		maskStack[i]->Render(*this, true);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
}

