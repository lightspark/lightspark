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
#include "platforms/engineutils.h"
#include "backends/rendering_context.h"
#include "logger.h"
#include "scripting/flash/display/BitmapContainer.h"
#include "scripting/flash/display/Bitmap.h"
#include "scripting/flash/geom/flashgeom.h"
#include "3rdparty/nanovg/src/nanovg.h"

using namespace std;
using namespace lightspark;

#define LSGL_MATRIX_SIZE (16*sizeof(float))

const float RenderContext::lsIdentityMatrix[16] = {
								1, 0, 0, 0,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1
								};

void TransformStack::push(const Transform2D& _transform)
{
	if (!transforms.empty())
	{
		auto matrix = transform().matrix.multiplyMatrix(_transform.matrix);
		auto colorTransform = transform().colorTransform.multiplyTransform(_transform.colorTransform);
		AS_BLENDMODE blendmode = transform().blendmode;
		if (DisplayObject::isShaderBlendMode(blendmode)
			|| blendmode == AS_BLENDMODE::BLENDMODE_LAYER
			|| _transform.blendmode != AS_BLENDMODE::BLENDMODE_NORMAL)
			blendmode = _transform.blendmode;
		transforms.push_back(Transform2D(matrix, colorTransform,blendmode));
	}
	else
		transforms.push_back(_transform);
}

RenderContext::RenderContext():
	inMaskRendering(false),maskActive(false)
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

CachedSurface* GLRenderContext::getCachedSurface(const DisplayObject* d) const
{
	return d->cachedSurface.getPtr();
}

void GLRenderContext::pushMask()
{
	RenderContext::pushMask();
	if (engineData->nvgcontext != nullptr)
		nvgPushClip(engineData->nvgcontext);
	if (!(maskCount++))
	{
		engineData->exec_glClearStencil(0);
		engineData->exec_glClear(CLEARMASK::STENCIL);
	}
}

void GLRenderContext::popMask()
{
	RenderContext::popMask();
	if (engineData->nvgcontext != nullptr)
		nvgPopClip(engineData->nvgcontext);
	if (!(--maskCount))
	{
		engineData->exec_glClearStencil(0);
		engineData->exec_glClear(CLEARMASK::STENCIL);
	}
}

void GLRenderContext::deactivateMask()
{
	RenderContext::deactivateMask();
}

void GLRenderContext::activateMask()
{
	RenderContext::activateMask();
}

void GLRenderContext::resetCurrentFrameBuffer()
{
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(baseFramebuffer);
	engineData->exec_glBindRenderbuffer_GL_RENDERBUFFER(baseRenderbuffer);
}
void GLRenderContext::setupRenderingState(float alpha, const ColorTransformBase& colortransform,SMOOTH_MODE smooth,AS_BLENDMODE blendmode)
{
	engineData->exec_glUniform1f(blendModeUniform, blendmode);
	switch (blendmode)
	{
		case BLENDMODE_NORMAL:
		case BLENDMODE_LAYER: // layer implies rendering to bitmap, so no special blending needed
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
		case BLENDMODE_ERASE:
			engineData->exec_glBlendFunc(BLEND_ZERO,BLEND_ONE_MINUS_SRC_ALPHA);
			break;
		case BLENDMODE_OVERLAY: // handled through blendMode uniform
		case BLENDMODE_HARDLIGHT: // handled through blendMode uniform
			engineData->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"renderTextured of blend mode "<<(int)blendmode);
			break;
	}
	if (smooth == SMOOTH_MODE::SMOOTH_NONE)
	{
		engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_NEAREST();
		engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_NEAREST();
	}
	//Set alpha
	engineData->exec_glUniform1f(alphaUniform, alpha);
	//Set colotransform
	engineData->exec_glUniform4f(colortransMultiplyUniform, colortransform.redMultiplier,colortransform.greenMultiplier,colortransform.blueMultiplier,colortransform.alphaMultiplier);
	engineData->exec_glUniform4f(colortransAddUniform, colortransform.redOffset/255.0,colortransform.greenOffset/255.0,colortransform.blueOffset/255.0,colortransform.alphaOffset/255.0);
	// set mask drawing indicator
	engineData->exec_glUniform1f(maskUniform, isDrawingMask() ? 1 : 0);
}
void GLRenderContext::renderTextured(const TextureChunk& chunk, float alpha, COLOR_MODE colorMode,
									 const ColorTransformBase& colortransform,
									 bool isMask, float directMode, RGB directColor, SMOOTH_MODE smooth, const MATRIX& matrix, const RECT& scalingGrid,
									 AS_BLENDMODE blendmode)
{
	setupRenderingState(alpha,colortransform,smooth,blendmode);
	float empty=0;
	engineData->exec_glUniform1fv(filterdataUniform, 1, &empty);
	engineData->exec_glUniform1f(yuvUniform, colorMode==COLOR_MODE::YUV_MODE?1.0:0.0);
	
	// set mode for direct coloring:
	// 0.0:no coloring
	// 1.0 coloring for profiling/error message (?)
	// 2.0:set color for every non transparent pixel (used for text rendering)
	// 3.0 set color for every pixel (renders a filled rectangle)
	engineData->exec_glUniform1f(directUniform, directMode);
	engineData->exec_glUniform1f(renderStage3DUniform, 0.0);
	engineData->exec_glUniform4f(directColorUniform,float(directColor.Red)/255.0,float(directColor.Green)/255.0,float(directColor.Blue)/255.0,1.0);

	engineData->exec_glBindTexture_GL_TEXTURE_2D(largeTextures[chunk.texId].id);
	assert(chunk.getNumberOfChunks()==((chunk.width+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL)*((chunk.height+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL));
	
	if ((scalingGrid.Xmin!= 0 || scalingGrid.Xmax != 0 || scalingGrid.Ymin !=0 || scalingGrid.Ymax != 0)
		&& (scalingGrid.Xmax-scalingGrid.Xmin)+abs(scalingGrid.Xmin) < chunk.width/chunk.xContentScale && (scalingGrid.Ymax-scalingGrid.Ymin)+abs(scalingGrid.Ymin) < chunk.height/chunk.yContentScale && matrix.getRotation()==0)
	{
		// rendering with scalingGrid

		MATRIX m;
		number_t scalex = chunk.xContentScale;
		number_t scaley = chunk.yContentScale;
		number_t leftborder = abs(scalingGrid.Xmin);
		number_t topborder = abs(scalingGrid.Ymin);
		number_t rightborder = number_t(chunk.width)/scalex-(leftborder+(scalingGrid.Xmax-scalingGrid.Xmin));
		number_t bottomborder = number_t(chunk.height)/scaley-(topborder+(scalingGrid.Ymax-scalingGrid.Ymin));
		number_t scaledleftborder = leftborder*scalex;
		number_t scaledtopborder = topborder*scaley;
		number_t scaledrightborder = number_t(chunk.width)-((leftborder+(scalingGrid.Xmax-scalingGrid.Xmin))*scalex);
		number_t scaledbottomborder = number_t(chunk.height)-((topborder+(scalingGrid.Ymax-scalingGrid.Ymin))*scaley);
		number_t scaledinnerwidth = number_t(chunk.width) - (scaledrightborder+scaledleftborder);
		number_t scaledinnerheight = number_t(chunk.height) - (scaledbottomborder+scaledtopborder);
		number_t innerwidth = number_t(chunk.width) - (scaledrightborder+scaledleftborder);
		number_t innerheight = number_t(chunk.height) - (scaledbottomborder+scaledtopborder);
		number_t innerscalex = innerwidth/(number_t(chunk.width) - (scaledrightborder+scaledleftborder)/scalex);
		number_t innerscaley = innerheight/(number_t(chunk.height) - (scaledbottomborder+scaledtopborder)/scaley);

		// 1) render unscaled upper left corner
		m=MATRIX();
		m.translate(matrix.getTranslateX()+chunk.xOffset,matrix.getTranslateY()+chunk.yOffset);
		renderpart(m,chunk,0,0,scaledleftborder,scaledtopborder,0,0);

		// 2) render unscaled upper right corner
		m=MATRIX();
		m.translate(matrix.getTranslateX()+(number_t(chunk.width)-rightborder)/scalex*matrix.getScaleX()+chunk.xOffset,matrix.getTranslateY()+chunk.yOffset);
		renderpart(m,chunk,(number_t(chunk.width) - scaledrightborder),0,scaledrightborder,scaledtopborder,0,0);

		// 3) render unscaled lower right corner
		m=MATRIX();
		m.translate(matrix.getTranslateX()+(number_t(chunk.width)-rightborder)/scalex*matrix.getScaleX()+chunk.xOffset,matrix.getTranslateY()+(number_t(chunk.height)-bottomborder)/scaley*matrix.getScaleY()+chunk.yOffset);
		renderpart(m,chunk,(number_t(chunk.width) - scaledrightborder),(number_t(chunk.height) - scaledbottomborder),scaledrightborder,scaledbottomborder,0,0);

		// 4) render unscaled lower left corner
		m=MATRIX();
		m.translate(matrix.getTranslateX()+chunk.xOffset,matrix.getTranslateY()+(number_t(chunk.height)-bottomborder)/scaley*matrix.getScaleY()+chunk.yOffset);
		renderpart(m,chunk,0,(number_t(chunk.height) - scaledbottomborder),scaledleftborder,scaledbottomborder,0,0);

		// 5) render x-scaled upper border
		m=MATRIX();
		m.scale(matrix.getScaleX()/innerscalex,1.0);
		m.translate(matrix.getTranslateX()+leftborder+chunk.xOffset,matrix.getTranslateY()+chunk.yOffset);
		renderpart(m,chunk,scaledleftborder,0,number_t(chunk.width) - (scaledrightborder+scaledleftborder),scaledtopborder,0,0);

		// 6) render y-scaled right border
		m=MATRIX();
		m.scale(1.0,matrix.getScaleY()/innerscaley);
		m.translate(matrix.getTranslateX()+(number_t(chunk.width)-rightborder)/scalex*matrix.getScaleX()+chunk.xOffset,matrix.getTranslateY()+topborder+chunk.yOffset);
		renderpart(m,chunk,number_t(chunk.width) - scaledrightborder,scaledtopborder,scaledrightborder,scaledinnerheight,0,0);

		// 7) render x-scaled bottom border
		m=MATRIX();
		m.scale(matrix.getScaleX()/innerscalex,1.0);
		m.translate(matrix.getTranslateX()+leftborder+chunk.xOffset,matrix.getTranslateY()+(number_t(chunk.height)-bottomborder)/scaley*matrix.getScaleY()+chunk.yOffset);
		renderpart(m,chunk,scaledleftborder,(number_t(chunk.height) - scaledbottomborder),scaledinnerwidth,scaledbottomborder,0,0);

		// 8) render y-scaled left border
		m=MATRIX();
		m.scale(1.0,matrix.getScaleY()/innerscaley);
		m.translate(matrix.getTranslateX()+chunk.xOffset,matrix.getTranslateY()+topborder+chunk.yOffset);
		renderpart(m,chunk,0,scaledtopborder,scaledleftborder,scaledinnerheight,0,0);

		// 9) render scaled center
		m=MATRIX();
		m.scale(matrix.getScaleX()/innerscalex,matrix.getScaleY()/innerscaley);
		m.translate(matrix.getTranslateX()+leftborder+chunk.xOffset,matrix.getTranslateY()+topborder+chunk.yOffset);
		renderpart(m,chunk,scaledleftborder,scaledtopborder,scaledinnerwidth,scaledinnerheight,0,0);
	}
	else
		renderpart(matrix,chunk,0,0,chunk.width,chunk.height,chunk.xOffset/chunk.xContentScale,chunk.yOffset/chunk.yContentScale);

	if (smooth != SMOOTH_MODE::SMOOTH_NONE)
	{
		engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
		engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
	}
}
void GLRenderContext::renderpart(const MATRIX& matrix, const TextureChunk& chunk, float cropleft, float croptop, float cropwidth, float cropheight,float tx,float ty)
{
	//Set matrix
	float fmatrix[16];
	matrix.get4DMatrix(fmatrix);
	lsglLoadMatrixf(fmatrix);
	setMatrixUniform(LSGL_MODELVIEW);
	
	uint32_t firstchunkhorizontal = floor(float(cropleft)/float(CHUNKSIZE_REAL));
	uint32_t firstchunkvertical = floor(float(croptop)/float(CHUNKSIZE_REAL));
	uint32_t lastchunkhorizontal = (cropleft+cropwidth+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
	uint32_t lastchunkvertical = (croptop+cropheight+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
	uint32_t horizontalchunks = (chunk.width+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
	uint32_t chunkskiphorizontal = horizontalchunks - lastchunkhorizontal + firstchunkhorizontal;
	int realchunkcount = (lastchunkhorizontal-firstchunkhorizontal)*(lastchunkvertical-firstchunkvertical);
	//The 4 corners of each texture are specified as the vertices of 2 triangles,
	//so there are 6 vertices per quad, two of them duplicated (the diagonal)
	//Allocate the data on the stack to reduce heap fragmentation
	float *vertex_coords = g_newa(float,realchunkcount*12);
	float *texture_coords = g_newa(float,realchunkcount*12);
	
	const uint32_t blocksPerSide=largeTextureSize/CHUNKSIZE;
	float realchunkwidth = cropwidth;
	float realchunkheight = cropheight;
	uint32_t curChunk=firstchunkhorizontal+firstchunkvertical*horizontalchunks;
	uint32_t chunkrendercount=0;
	float startX, startY, endX, endY;
	float leftstart = cropleft-firstchunkhorizontal*CHUNKSIZE_REAL;
	float topstart = croptop-firstchunkvertical*CHUNKSIZE_REAL;

	float startVtop=topstart;
	uint32_t availYForTexture=realchunkheight+topstart;
	startY = ty;
	float heighttoplace=realchunkheight;
	for(uint32_t i=firstchunkvertical, k=0;i<lastchunkvertical;i++)
	{
		float heightconsumed;
		if (startVtop && (realchunkheight + topstart > CHUNKSIZE_REAL))
			heightconsumed = min((float(CHUNKSIZE_REAL) - topstart),float(CHUNKSIZE_REAL));
		else
			heightconsumed = min(heighttoplace ,float(CHUNKSIZE_REAL));
		endY = startY + heightconsumed / chunk.yContentScale;
		heighttoplace-= heightconsumed;
		
		float startULeft=leftstart;
		float widthtoplace=realchunkwidth;
		uint32_t availXForTexture=realchunkwidth+leftstart;
		startX = tx;
		const uint32_t availY=min(availYForTexture,uint32_t(CHUNKSIZE_REAL));
		availYForTexture-=availY;
		for(uint32_t j=firstchunkhorizontal;j<lastchunkhorizontal;j++)
		{
			const uint32_t curChunkId=chunk.chunks[curChunk];
			const uint32_t blockX=((curChunkId%blocksPerSide)*CHUNKSIZE);
			const uint32_t blockY=((curChunkId/blocksPerSide)*CHUNKSIZE);
			const uint32_t availX=min(availXForTexture,uint32_t(CHUNKSIZE_REAL));
			availXForTexture-=availX;
			float startU=blockX + 1 + startULeft;
			startU/=float(largeTextureSize);
			float startV=blockY + 1 + startVtop;
			startV/=float(largeTextureSize);
			float endU=blockX+availX+1;
			endU/=float(largeTextureSize);
			float endV=blockY+availY+1;
			endV/=float(largeTextureSize);

			float widthconsumed;
			if (startULeft && (realchunkwidth + leftstart > CHUNKSIZE_REAL))
				widthconsumed = min((float(CHUNKSIZE_REAL) - leftstart),float(CHUNKSIZE_REAL));
			else
				widthconsumed = min(widthtoplace ,float(CHUNKSIZE_REAL));
			endX = startX + widthconsumed / chunk.xContentScale;
			widthtoplace-= widthconsumed;
			
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
			chunkrendercount++;
			startULeft = 0;
			startX = endX;
		}
		curChunk += chunkskiphorizontal;
		startVtop = 0;
		startY = endY;
	}
	engineData->exec_glVertexAttribPointer(VERTEX_ATTRIB, 0, vertex_coords,FLOAT_2);
	engineData->exec_glVertexAttribPointer(TEXCOORD_ATTRIB, 0, texture_coords,FLOAT_2);
	engineData->exec_glEnableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glEnableVertexAttribArray(TEXCOORD_ATTRIB);
	engineData->exec_glDrawArrays_GL_TRIANGLES( 0, chunkrendercount*6);
	engineData->exec_glDisableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glDisableVertexAttribArray(TEXCOORD_ATTRIB);
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
			LOG(LOG_ERROR,"GL error "<< hex<<err);
		}
		else
			break;
	}

	if(errorCount)
	{
		LOG(LOG_ERROR,"Ignoring " << errorCount << " openGL errors");
	}
	return errorCount;
}

void GLRenderContext::setMatrixUniform(LSGL_MATRIX m) const
{
	int uni = (m == LSGL_MODELVIEW) ? modelviewMatrixUniform:projectionMatrixUniform;

	engineData->exec_glUniformMatrix4fv(uni, 1, false, lsMVPMatrix);
}

CairoRenderContext::CairoRenderContext(uint8_t* buf, uint32_t _width, uint32_t _height, bool smoothing)
{
	cairoSurface=getCairoSurfaceForData(buf, _width, _height,_width);
	width=_width;
	height=_height;
	cr_list.push_back(cairo_create(cairoSurface));
	cairo_set_antialias(cr_list.back(),smoothing ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
}

CairoRenderContext::~CairoRenderContext()
{
	cairo_surface_destroy(cairoSurface);
	while (!cr_list.empty())
	{
		cairo_destroy(cr_list.back());
		cr_list.pop_back();
	}
	
}

cairo_surface_t* CairoRenderContext::getCairoSurfaceForData(uint8_t* buf, uint32_t width, uint32_t height, uint32_t stride)
{
	uint32_t cairoWidthStride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, stride);
	return cairo_image_surface_create_for_data(buf, CAIRO_FORMAT_ARGB32, width, height, cairoWidthStride);
}

void CairoRenderContext::simpleBlit(int32_t destX, int32_t destY, uint8_t* sourceBuf, uint32_t sourceTotalWidth, uint32_t sourceTotalHeight,
		int32_t sourceX, int32_t sourceY, uint32_t sourceWidth, uint32_t sourceHeight)
{
	cairo_t* cr = cr_list.back();
	cairo_surface_t* sourceSurface = getCairoSurfaceForData(sourceBuf, sourceTotalWidth, sourceTotalHeight, sourceTotalWidth);
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

void CairoRenderContext::transformedBlit(const MATRIX& m, BitmapContainer* bc, ColorTransform* ct,
		FILTER_MODE filterMode, number_t x, number_t y, number_t w, number_t h)
{
	cairo_t* cr = cr_list.back();
	uint8_t* bmp = ct ? bc->applyColorTransform(ct) : bc->getData();
	cairo_surface_t* sourceSurface = getCairoSurfaceForData(bmp, bc->getWidth(), bc->getHeight(), bc->getWidth());
	cairo_pattern_t* sourcePattern = cairo_pattern_create_for_surface(sourceSurface);
	cairo_surface_destroy(sourceSurface);
	cairo_pattern_set_filter(sourcePattern, (filterMode==FILTER_SMOOTH)?CAIRO_FILTER_BILINEAR:CAIRO_FILTER_NEAREST);
	cairo_pattern_set_extend(sourcePattern, CAIRO_EXTEND_NONE);
	cairo_set_source(cr, sourcePattern);
	cairo_matrix_t matrix = m.getInverted();
	cairo_pattern_set_matrix(sourcePattern, &matrix);
	cairo_pattern_destroy(sourcePattern);
	cairo_rectangle(cr, x, y, w, h);
	cairo_fill(cr);
}

void CairoRenderContext::setupRenderState(cairo_t* cr,AS_BLENDMODE blendmode,bool isMask,SMOOTH_MODE smooth)
{
	switch (blendmode)
	{
		case BLENDMODE_NORMAL:
			cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
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
		case BLENDMODE_ERASE:
			cairo_set_operator(cr,CAIRO_OPERATOR_DEST_OUT);
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"cairo renderTextured of blend mode "<<(int)blendmode);
			break;
	}
	if (isMask)
		cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
	else
	{
		switch (smooth)
		{
			case SMOOTH_MODE::SMOOTH_NONE:
				cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
				break;
			case SMOOTH_MODE::SMOOTH_SUBPIXEL:
				cairo_set_antialias(cr,CAIRO_ANTIALIAS_SUBPIXEL);
				break;
			case SMOOTH_MODE::SMOOTH_ANTIALIAS:
				cairo_set_antialias(cr,CAIRO_ANTIALIAS_DEFAULT);
				break;
		}
	}
	
}
