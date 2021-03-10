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

#include <cstdlib>
#include <cstring>
#include <stack>
#include "backends/rendering_context.h"
#include "logger.h"
#include "scripting/flash/display/flashdisplay.h"

using namespace std;
using namespace lightspark;

#define LSGL_MATRIX_SIZE (16*sizeof(float))

const float RenderContext::lsIdentityMatrix[16] = {
								1, 0, 0, 0,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1
								};

const CachedSurface CairoRenderContext::invalidSurface;

RenderContext::RenderContext(CONTEXT_TYPE t):contextType(t)
{
	lsglLoadIdentity();
}

void RenderContext::lsglLoadMatrixf(const float *m)
{
	memcpy(lsMVPMatrix, m, LSGL_MATRIX_SIZE);
}

void RenderContext::lsglLoadIdentity()
{
	lsglLoadMatrixf(lsIdentityMatrix);
}

void RenderContext::lsglMultMatrixf(const float *m)
{
	float tmp[16];
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			float sum=0;
			for (int k=0;k<4;k++)
			{
				sum += lsMVPMatrix[i+k*4]*m[j*4+k];
			}
			tmp[i+j*4] = sum;
		}
	}
	memcpy(lsMVPMatrix, tmp, LSGL_MATRIX_SIZE);
}

void RenderContext::lsglScalef(float scaleX, float scaleY, float scaleZ)
{
	static float scale[16];

	memcpy(scale, lsIdentityMatrix, LSGL_MATRIX_SIZE);
	scale[0] = scaleX;
	scale[5] = scaleY;
	scale[10] = scaleZ;
	lsglMultMatrixf(scale);
}

void RenderContext::lsglTranslatef(float translateX, float translateY, float translateZ)
{
	static float trans[16];

	memcpy(trans, lsIdentityMatrix, LSGL_MATRIX_SIZE);
	trans[12] = translateX;
	trans[13] = translateY;
	trans[14] = translateZ;
	lsglMultMatrixf(trans);
}

void GLRenderContext::lsglOrtho(float l, float r, float b, float t, float n, float f)
{
	float ortho[16];
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

const CachedSurface& GLRenderContext::getCachedSurface(const DisplayObject* d) const
{
	return d->cachedSurface;
}

void GLRenderContext::setProperties(AS_BLENDMODE blendmode)
{
	// TODO handle other blend modes ,maybe with shaders ? (see https://github.com/jamieowen/glsl-blend)
	switch (blendmode)
	{
		case BLENDMODE_NORMAL:
			engineData->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
			break;
		case BLENDMODE_MULTIPLY:
			engineData->exec_glBlendFunc(BLEND_DST_COLOR,BLEND_ONE_MINUS_SRC_ALPHA);
			break;
		case BLENDMODE_ADD:
			engineData->exec_glBlendFunc(BLEND_ONE,BLEND_ONE);
			break;
		case BLENDMODE_SCREEN:
			engineData->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_COLOR);
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"renderTextured of blend mode "<<(int)blendmode);
			break;
	}
}
void GLRenderContext::renderTextured(const TextureChunk& chunk, int32_t x, int32_t y, uint32_t w, uint32_t h,
			float alpha, COLOR_MODE colorMode, float rotate, int32_t xtransformed, int32_t ytransformed, int32_t widthtransformed, int32_t heighttransformed, float xscale, float yscale,
									 float redMultiplier,float greenMultiplier,float blueMultiplier,float alphaMultiplier,
									 float redOffset,float greenOffset,float blueOffset,float alphaOffset,
									 bool isMask, bool hasMask)
{
	if (isMask)
	{
		engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(maskframebuffer);
		engineData->exec_glClearColor(0,0,0,0);
		engineData->exec_glClear_GL_COLOR_BUFFER_BIT();
		engineData->exec_glUniform1f(maskUniform, 0);
	}
	else
	{
		engineData->exec_glUniform1f(maskUniform, hasMask ? 1 : 0);
	}
	//Set color mode
	engineData->exec_glUniform1f(yuvUniform, (colorMode==YUV_MODE)?1:0);
	//Set alpha
	engineData->exec_glUniform1f(alphaUniform, alpha);
	// set rotation, scaling and translation
	// TODO there may be a more elegant way to handle this, but I'm new to shader programming...
	engineData->exec_glUniform1f(rotateUniform, rotate*M_PI/180.0);
	engineData->exec_glUniform2f(beforeRotateUniform, float(w)/2.0,float(h)/2.0);
	engineData->exec_glUniform2f(afterRotateUniform, float(widthtransformed)/2.0,float(heighttransformed)/2.0);
	engineData->exec_glUniform2f(startPositionUniform, xtransformed,ytransformed);
	engineData->exec_glUniform2f(scaleUniform, xscale,yscale);
	engineData->exec_glUniform4f(colortransMultiplyUniform, redMultiplier,greenMultiplier,blueMultiplier,alphaMultiplier);
	engineData->exec_glUniform4f(colortransAddUniform, redOffset/255.0,greenOffset/255.0,blueOffset/255.0,alphaOffset/255.0);
	//Set matrix
	setMatrixUniform(LSGL_MODELVIEW);

	engineData->exec_glBindTexture_GL_TEXTURE_2D(largeTextures[chunk.texId].id);
	const uint32_t blocksPerSide=largeTextureSize/CHUNKSIZE;
	float startX, startY, endX, endY;
	assert(chunk.getNumberOfChunks()==((chunk.width+CHUNKSIZE-1)/CHUNKSIZE)*((chunk.height+CHUNKSIZE-1)/CHUNKSIZE));

	uint32_t curChunk=0;
	//The 4 corners of each texture are specified as the vertices of 2 triangles,
	//so there are 6 vertices per quad, two of them duplicated (the diagonal)
	//Allocate the data on the stack to reduce heap fragmentation
	float *vertex_coords = g_newa(float,chunk.getNumberOfChunks()*12);
	float *texture_coords = g_newa(float,chunk.getNumberOfChunks()*12);
	for(uint32_t i=0, k=0;i<chunk.height;i+=CHUNKSIZE)
	{
		startY=float(h*i)/float(chunk.height);
		endY=min(float(h*(i+CHUNKSIZE))/float(chunk.height),float(h));
		for(uint32_t j=0;j<chunk.width;j+=CHUNKSIZE)
		{
			startX=float(w*j)/float(chunk.width);
			endX=min(float(w*(j+CHUNKSIZE))/float(chunk.width),float(w));
			const uint32_t curChunkId=chunk.chunks[curChunk];
			const uint32_t blockX=((curChunkId%blocksPerSide)*CHUNKSIZE);
			const uint32_t blockY=((curChunkId/blocksPerSide)*CHUNKSIZE);
			const uint32_t availX=min(int(chunk.width-j),CHUNKSIZE);
			const uint32_t availY=min(int(chunk.height-i),CHUNKSIZE);
			float startU=blockX + 1;
			startU/=float(largeTextureSize);
			float startV=blockY + 1;
			startV/=float(largeTextureSize);
			float endU=blockX+availX - 1;
			endU/=float(largeTextureSize);
			float endV=blockY+availY - 1;
			endV/=float(largeTextureSize);

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

	engineData->exec_glVertexAttribPointer(VERTEX_ATTRIB, 0, vertex_coords,FLOAT_2);
	engineData->exec_glVertexAttribPointer(TEXCOORD_ATTRIB, 0, texture_coords,FLOAT_2);
	engineData->exec_glEnableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glEnableVertexAttribArray(TEXCOORD_ATTRIB);
	engineData->exec_glDrawArrays_GL_TRIANGLES( 0, curChunk*6);
	engineData->exec_glDisableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glDisableVertexAttribArray(TEXCOORD_ATTRIB);
	if (isMask)
		engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(0);
}

int GLRenderContext::errorCount = 0;
bool GLRenderContext::handleGLErrors() const
{
	uint32_t err;
	while(1)
	{
		if(engineData && engineData->getGLError(err))
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
	int uni = (m == LSGL_MODELVIEW) ? modelviewMatrixUniform:projectionMatrixUniform;

	engineData->exec_glUniformMatrix4fv(uni, 1, false, lsMVPMatrix);
}

CairoRenderContext::CairoRenderContext(uint8_t* buf, uint32_t width, uint32_t height, bool smoothing):RenderContext(CAIRO)
{
	cairo_surface_t* cairoSurface=getCairoSurfaceForData(buf, width, height);
	cr=cairo_create(cairoSurface);
	cairo_surface_destroy(cairoSurface); /* cr has an reference to it */
	cairo_set_antialias(cr,smoothing ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
}

CairoRenderContext::~CairoRenderContext()
{
	cairo_destroy(cr);
}

cairo_surface_t* CairoRenderContext::getCairoSurfaceForData(uint8_t* buf, uint32_t width, uint32_t height)
{
	uint32_t cairoWidthStride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	assert(cairoWidthStride==width*4);
	return cairo_image_surface_create_for_data(buf, CAIRO_FORMAT_ARGB32, width, height, cairoWidthStride);
}

void CairoRenderContext::simpleBlit(int32_t destX, int32_t destY, uint8_t* sourceBuf, uint32_t sourceTotalWidth, uint32_t sourceTotalHeight,
		int32_t sourceX, int32_t sourceY, uint32_t sourceWidth, uint32_t sourceHeight)
{
	cairo_surface_t* sourceSurface = getCairoSurfaceForData(sourceBuf, sourceTotalWidth, sourceTotalHeight);
	cairo_pattern_t* sourcePattern = cairo_pattern_create_for_surface(sourceSurface);
	cairo_surface_destroy(sourceSurface);
	cairo_pattern_set_filter(sourcePattern, CAIRO_FILTER_NEAREST);
	cairo_pattern_set_extend(sourcePattern, CAIRO_EXTEND_NONE);
	cairo_matrix_t matrix;
	cairo_matrix_init_translate(&matrix, sourceX-destX, sourceY-destY);
	cairo_pattern_set_matrix(sourcePattern, &matrix);
	cairo_set_source(cr, sourcePattern);
	cairo_pattern_destroy(sourcePattern);
	cairo_rectangle(cr, destX, destY, sourceWidth, sourceHeight);
	cairo_fill(cr);
}

void CairoRenderContext::transformedBlit(const MATRIX& m, uint8_t* sourceBuf, uint32_t sourceTotalWidth, uint32_t sourceTotalHeight,
		FILTER_MODE filterMode)
{
	cairo_surface_t* sourceSurface = getCairoSurfaceForData(sourceBuf, sourceTotalWidth, sourceTotalHeight);
	cairo_pattern_t* sourcePattern = cairo_pattern_create_for_surface(sourceSurface);
	cairo_surface_destroy(sourceSurface);
	cairo_pattern_set_filter(sourcePattern, (filterMode==FILTER_SMOOTH)?CAIRO_FILTER_BILINEAR:CAIRO_FILTER_NEAREST);
	cairo_pattern_set_extend(sourcePattern, CAIRO_EXTEND_NONE);
	cairo_set_matrix(cr, &m);
	cairo_set_source(cr, sourcePattern);
	cairo_pattern_destroy(sourcePattern);
	cairo_rectangle(cr, 0, 0, sourceTotalWidth, sourceTotalHeight);
	cairo_fill(cr);
}

void CairoRenderContext::renderTextured(const TextureChunk& chunk, int32_t x, int32_t y, uint32_t w, uint32_t h,
			float alpha, COLOR_MODE colorMode, float rotate, int32_t xtransformed, int32_t ytransformed, int32_t widthtransformed, int32_t heighttransformed, float xscale, float yscale,
			float redMultiplier, float greenMultiplier, float blueMultiplier, float alphaMultiplier,
			float redOffset, float greenOffset, float blueOffset, float alphaOffset,
			bool isMask, bool hasMask)
{
	if (alpha != 1.0)
		LOG(LOG_NOT_IMPLEMENTED,"CairoRenderContext.renderTextured alpha not implemented:"<<alpha);
	if (colorMode != RGB_MODE)
		LOG(LOG_NOT_IMPLEMENTED,"CairoRenderContext.renderTextured colorMode not implemented:"<<(int)colorMode);
	uint8_t* buf=(uint8_t*)chunk.chunks;
	cairo_surface_t* chunkSurface = getCairoSurfaceForData(buf, chunk.width, chunk.height);
	cairo_pattern_t* chunkPattern = cairo_pattern_create_for_surface(chunkSurface);
	cairo_surface_destroy(chunkSurface);
	cairo_pattern_set_filter(chunkPattern, CAIRO_FILTER_BILINEAR);
	cairo_pattern_set_extend(chunkPattern, CAIRO_EXTEND_NONE);
	cairo_save(cr);
	cairo_matrix_t matrix;
	cairo_matrix_init_translate(&matrix,xtransformed,ytransformed);
	cairo_matrix_scale(&matrix, xscale, yscale);
	cairo_matrix_rotate(&matrix,rotate*M_PI/180.0);
	cairo_set_matrix(cr,&matrix);

	cairo_set_source(cr, chunkPattern);
	cairo_pattern_destroy(chunkPattern);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_fill(cr);
	cairo_restore(cr);
}

const CachedSurface& CairoRenderContext::getCachedSurface(const DisplayObject* d) const
{
	auto ret=customSurfaces.find(d);
	if(ret==customSurfaces.end())
	{
		//No surface is stored, return an invalid one
		return invalidSurface;
	}
	return ret->second;
}

void CairoRenderContext::setProperties(AS_BLENDMODE blendmode)
{
	switch (blendmode)
	{
		case BLENDMODE_NORMAL:
			break;
		case BLENDMODE_MULTIPLY:
			cairo_set_operator(cr,CAIRO_OPERATOR_MULTIPLY);
			break;
		case BLENDMODE_ADD:
			cairo_set_operator(cr,CAIRO_OPERATOR_ADD);
			break;
		case BLENDMODE_SCREEN:
			cairo_set_operator(cr,CAIRO_OPERATOR_SCREEN);
			break;
		case BLENDMODE_LAYER:
			cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
			break;
		case BLENDMODE_DARKEN:
			cairo_set_operator(cr,CAIRO_OPERATOR_DARKEN);
			break;
		case BLENDMODE_DIFFERENCE:
			cairo_set_operator(cr,CAIRO_OPERATOR_DIFFERENCE);
			break;
		case BLENDMODE_HARDLIGHT:
			cairo_set_operator(cr,CAIRO_OPERATOR_HARD_LIGHT);
			break;
		case BLENDMODE_LIGHTEN:
			cairo_set_operator(cr,CAIRO_OPERATOR_LIGHTEN);
			break;
		case BLENDMODE_OVERLAY:
			cairo_set_operator(cr,CAIRO_OPERATOR_OVERLAY);
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"renderTextured of blend mode "<<(int)blendmode);
			break;
	}
}

CachedSurface& CairoRenderContext::allocateCustomSurface(const DisplayObject* d, uint8_t* texBuf, bool isBufferOwner)
{
	auto ret=customSurfaces.insert(make_pair(d, CachedSurface()));
//	assert(ret.second);
	CachedSurface& surface=ret.first->second;
	if (surface.tex==nullptr)
		surface.tex=new TextureChunk();
	surface.tex->chunks=(uint32_t*)texBuf;
	surface.isChunkOwner=isBufferOwner;
	return surface;
}
