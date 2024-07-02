/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2017 Ludger Krämer <dbluelle@onlinehome.de>

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

#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/flash/display/Stage3D.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/geom/Matrix3D.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "backends/rendering.h"
#include "backends/rendering_context.h"
#include "platforms/engineutils.h"
#include "scripting/flash/display3d/agalconverter.h"
#include "scripting/abc.h"

SamplerRegister SamplerRegister::parse (uint64_t v, bool isVertexProgram)
{
	SamplerRegister sr;
	sr.isVertexProgram = isVertexProgram;
	sr.f = ((v >> 60) & 0xF); // filter
	sr.m = ((v >> 56) & 0xF); // mipmap
	sr.w = ((v >> 52) & 0xF) ? 3 : 0; // wrap (AGAL doesn't seem to differentiate between WRAP_S and WRAP_T, so set flag for both)
	sr.s = ((v >> 48) & 0xF); // special
	sr.d = ((v >> 44) & 0xF); // dimension
	sr.t = ((v >> 40) & 0xF); // texture
	sr.type = (RegisterType) ((v >> 32) & 0xF); // type
	sr.b = ((v >> 16) & 0xFF); // TODO: should this be .low?
	sr.n = (v & 0xFFFF); // number
	sr.program_sampler_id = UINT32_MAX;
	return sr;
}
lightspark::tiny_string SamplerRegister::toGLSL ()
{
	char buf[100];
	sprintf(buf,"%d",n);
	tiny_string str = prefixFromType (type, isVertexProgram) + buf;
	return str;
}

void Context3D::handleRenderAction(EngineData* engineData, renderaction& action)
{
	switch (action.action)
	{
		case RENDER_CLEAR:
			// udata1 = stencil
			// udata2 = mask
			// fdata[0] = red
			// fdata[1] = green
			// fdata[2] = blue
			// fdata[3] = alpha
			// fdata[4] = depth
			
			if ((action.udata2 & CLEARMASK::COLOR) != 0)
				engineData->exec_glClearColor(action.fdata[0],action.fdata[1],action.fdata[2],action.fdata[3]);
			if (enableDepthAndStencilTextureBuffer)
			{
				if ((action.udata2 & CLEARMASK::DEPTH) != 0)
					engineData->exec_glClearDepthf(action.fdata[4]);
				if ((action.udata2 & CLEARMASK::STENCIL) != 0)
					engineData->exec_glClearStencil (action.udata1);
				engineData->exec_glClear((CLEARMASK)action.udata2);
			}
			else
				engineData->exec_glClear((CLEARMASK)(action.udata2 & CLEARMASK::COLOR));
			break;
		case RENDER_CONFIGUREBACKBUFFER:
			//action.udata1 = enableDepthAndStencil
			//action.udata2 = backBufferWidth
			//action.udata3 = backBufferHeight
			configureBackBufferIntern(action.udata1,action.udata2,action.udata3,0);
			configureBackBufferIntern(action.udata1,action.udata2,action.udata3,1);
			break;
		case RENDER_SETPROGRAM:
		{
			//action.dataobject = Program3D
			Program3D* p = action.dataobject->as<Program3D>();
			currentprogram = p;
			engineData->exec_glUseProgram(p->gpu_program);
			break;
		}
		case RENDER_UPLOADPROGRAM:
		{
			//action.dataobject = Program3D
			bool needslink=false;
			char str[1024];
			int a;
			int stat;
			Program3D* p = action.dataobject->as<Program3D>();
			//LOG(LOG_INFO,"uploadProgram:"<<p<<" "<<p->gpu_program);
			uint32_t f= UINT32_MAX;
			uint32_t g= UINT32_MAX;
			if (p->gpu_program == UINT32_MAX)
			{
				needslink=true;
				p->gpu_program = engineData->exec_glCreateProgram();
			}
			if (!p->vertexprogram.empty())
			{
				needslink= true;
				g = engineData->exec_glCreateShader_GL_VERTEX_SHADER();
				const char* buf = p->vertexprogram.raw_buf();
				engineData->exec_glShaderSource(g, 1, &buf,nullptr);
				engineData->exec_glCompileShader(g);
				engineData->exec_glGetShaderInfoLog(g,1024,&a,str);
				engineData->exec_glGetShaderiv_GL_COMPILE_STATUS(g, &stat);
				if (!stat)
				{
					LOG(LOG_ERROR,"Vertex shader:\n" << p->vertexprogram);
					LOG(LOG_ERROR,"Vertex shader compilation:" << str);
					throw RunTimeException("Could not compile vertex shader");
				}
			}
			if (!p->fragmentprogram.empty())
			{
				needslink=true;
				f = engineData->exec_glCreateShader_GL_FRAGMENT_SHADER();
				const char* buf = p->fragmentprogram.raw_buf();
				engineData->exec_glShaderSource(f, 1, &buf,nullptr);
				engineData->exec_glCompileShader(f);
				engineData->exec_glGetShaderInfoLog(f,1024,&a,str);
				engineData->exec_glGetShaderiv_GL_COMPILE_STATUS(f, &stat);
				if (!stat)
				{
					LOG(LOG_ERROR,"Fragment shader:\n" << p->fragmentprogram);
					LOG(LOG_ERROR,"Fragment shader compilation:" << str);
					throw RunTimeException("Could not compile fragment shader");
				}
			}
			if (!p->vertexprogram.empty())
				engineData->exec_glAttachShader(p->gpu_program,g);
			if (!p->fragmentprogram.empty())
				engineData->exec_glAttachShader(p->gpu_program,f);
			
			if (needslink)
				engineData->exec_glLinkProgram(p->gpu_program);
			if (!p->vertexprogram.empty())
				engineData->exec_glDeleteShader(g);
			if (!p->fragmentprogram.empty())
				engineData->exec_glDeleteShader(f);
			if (needslink)
			{
				engineData->exec_glGetProgramInfoLog(p->gpu_program,1024,&a,str);
				engineData->exec_glGetProgramiv_GL_LINK_STATUS(p->gpu_program,&stat);
				if(!stat)
				{
					LOG(LOG_ERROR,"Vertex shader:\n" << p->vertexprogram);
					LOG(LOG_ERROR,"Fragment shader:\n" << p->fragmentprogram);
					LOG(LOG_INFO,"program link " << str);
					throw RunTimeException("Could not link program");
				}
				for (auto it = p->samplerState.begin();it != p->samplerState.end(); it++)
					it->program_sampler_id = UINT32_MAX;
				for (auto it = p->vertexregistermap.begin();it != p->vertexregistermap.end(); it++)
					it->program_register_id = UINT32_MAX;
				for (auto it = p->fragmentregistermap.begin();it != p->fragmentregistermap.end(); it++)
					it->program_register_id = UINT32_MAX;
				for (auto it = p->vertexattributes.begin();it != p->vertexattributes.end(); it++)
					it->program_register_id = UINT32_MAX;
				for (auto it = p->fragmentattributes.begin();it != p->fragmentattributes.end(); it++)
					it->program_register_id = UINT32_MAX;
			}
			p->vertexprogram = "";
			p->fragmentprogram = "";
			setPositionScale(engineData);
			break;
		}
		case RENDER_RENDERTOBACKBUFFER:
			if (renderingToTexture)
			{
				if (textureframebuffer != UINT32_MAX)
				{
					engineData->exec_glDeleteFramebuffers(1,&textureframebuffer);
					if (depthRenderBuffer != UINT32_MAX)
						engineData->exec_glDeleteRenderbuffers(1,&depthRenderBuffer);
					if (stencilRenderBuffer != UINT32_MAX)
						engineData->exec_glDeleteRenderbuffers(1,&stencilRenderBuffer);
					depthRenderBuffer = UINT32_MAX;
					textureframebuffer = UINT32_MAX;
					stencilRenderBuffer = UINT32_MAX;
				}
				engineData->exec_glBindTexture_GL_TEXTURE_2D(backframebufferIDcurrent);
				engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(backframebuffer[currentactionvector]);
				engineData->exec_glViewport(0,0,this->backBufferWidth,this->backBufferHeight);
				if (enableDepthAndStencilBackbuffer)
				{
					engineData->exec_glEnable_GL_DEPTH_TEST();
					engineData->exec_glEnable_GL_STENCIL_TEST();
				}
				else
				{
					engineData->exec_glDisable_GL_DEPTH_TEST();
					engineData->exec_glDisable_GL_STENCIL_TEST();
				}
			}
			vcposdata[1] = 1.0;
			setPositionScale(engineData);
			renderingToTexture = false;
			break;
		case RENDER_TOTEXTURE:
		{
			//action.dataobject = Texture
			//action.udata1 = enableDepthAndStencil
			//action.udata2 = width
			//action.udata3 = height
			TextureBase* tex = action.dataobject->as<TextureBase>();
			if (tex->textureID == UINT32_MAX)
			{
				if (tex->is<CubeTexture>())
					loadCubeTexture(tex->as<CubeTexture>(),UINT32_MAX,UINT32_MAX);
				else
					loadTexture(tex,UINT32_MAX);
			}
			if (textureframebuffer != UINT32_MAX)
			{
				engineData->exec_glDeleteFramebuffers(1,&textureframebuffer);
				if (depthRenderBuffer != UINT32_MAX)
					engineData->exec_glDeleteRenderbuffers(1,&depthRenderBuffer);
				if (stencilRenderBuffer != UINT32_MAX)
					engineData->exec_glDeleteRenderbuffers(1,&stencilRenderBuffer);
				depthRenderBuffer = UINT32_MAX;
				textureframebuffer = UINT32_MAX;
				stencilRenderBuffer = UINT32_MAX;
			}
			else
				textureframebuffer = engineData->exec_glGenFramebuffer();
			engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(textureframebuffer);
			uint32_t textureframebufferID = tex->textureID;
			engineData->exec_glBindTexture_GL_TEXTURE_2D(textureframebufferID);
			engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_NEAREST();
			engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_NEAREST();
			engineData->exec_glFramebufferTexture2D_GL_FRAMEBUFFER(textureframebufferID);
			engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, action.udata2, action.udata3, 0, nullptr,true);
			engineData->exec_glBindTexture_GL_TEXTURE_2D(0);
			enableDepthAndStencilTextureBuffer = action.udata1;
			if (enableDepthAndStencilTextureBuffer)
			{
				depthRenderBuffer = engineData->exec_glGenRenderbuffer();
				
				if (engineData->supportPackedDepthStencil)
				{
					engineData->exec_glBindRenderbuffer(depthRenderBuffer);
					engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_STENCIL(action.udata2,action.udata3);
					engineData->exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_STENCIL_ATTACHMENT(depthRenderBuffer);
				}
				else
				{
					stencilRenderBuffer = engineData->exec_glGenRenderbuffer();
					engineData->exec_glBindRenderbuffer(depthRenderBuffer);
					engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_COMPONENT16(action.udata2,action.udata3);
					engineData->exec_glBindRenderbuffer(stencilRenderBuffer);
					engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_STENCIL_INDEX8(action.udata2,action.udata3);
					engineData->exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_ATTACHMENT(depthRenderBuffer);
					engineData->exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_STENCIL_ATTACHMENT(stencilRenderBuffer);
					engineData->exec_glBindRenderbuffer(0);
				}
				engineData->exec_glEnable_GL_DEPTH_TEST();
				engineData->exec_glEnable_GL_STENCIL_TEST();
			}
			else
			{
				engineData->exec_glDisable_GL_DEPTH_TEST();
				engineData->exec_glDisable_GL_STENCIL_TEST();
			}
			engineData->exec_glViewport(0,0,action.udata2,action.udata3);
			engineData->exec_glBindTexture_GL_TEXTURE_2D(0);
			vcposdata[1] = -1.0;
			setPositionScale(engineData);
			engineData->exec_glFrontFace(true);
			renderingToTexture = true;
			break;
		}
		case RENDER_DELETEPROGRAM:
		{
			//action.dataobject = Program3D
			Program3D* p = action.dataobject->as<Program3D>();
			engineData->exec_glUseProgram(0);
			engineData->exec_glDeleteProgram(p->gpu_program);
			p->gpu_program = UINT32_MAX;
			break;
		}
		case RENDER_SETVERTEXBUFFER:
		{
			//action.udata1 = format | (index << 4) | (data32PerVertex<<8)
			//action.udata2 = bufferID
			//action.udata3 = offset
			if (action.udata2 == UINT32_MAX)
			{
				VertexBuffer3D* buffer = action.dataobject->as<VertexBuffer3D>();
				engineData->exec_glGenBuffers(1,&(buffer->bufferID));
				action.udata2 = buffer->bufferID;
				engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(buffer->bufferID);
				if (buffer->bufferUsage == "dynamicDraw")
					engineData->exec_glBufferData_GL_ARRAY_BUFFER_GL_DYNAMIC_DRAW(buffer->numVertices*buffer->data32PerVertex*sizeof(float),buffer->data.data());
				else
					engineData->exec_glBufferData_GL_ARRAY_BUFFER_GL_STATIC_DRAW(buffer->numVertices*buffer->data32PerVertex*sizeof(float),buffer->data.data());
				engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(0);
			}
			attribs[action.udata1>>4 &0x7].bufferID = action.udata2;
			attribs[action.udata1>>4 &0x7].data32PerVertex = action.udata1>>8;
			attribs[action.udata1>>4 &0x7].offset = action.udata3;
			attribs[action.udata1>>4 &0x7].format = VERTEXBUFFER_FORMAT(action.udata1&0x7);
			break;
		}
		case RENDER_DRAWTRIANGLES:
		{
			//action.dataobject = IndexBuffer3D
			//action.udata1 = firstIndex
			//action.udata2 = numTriangles
			//action.udata3 = bufferID
			if (action.udata3 == UINT32_MAX)
			{
				IndexBuffer3D* buffer = action.dataobject->as<IndexBuffer3D>();
				action.udata3 = buffer->bufferID;
				if (action.udata3 == UINT32_MAX)
				{
					engineData->exec_glGenBuffers(1,&buffer->bufferID);
					action.udata3 = buffer->bufferID;
					if (!buffer->data.empty())
					{
						engineData->exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(buffer->bufferID);
						if (buffer->bufferUsage == "dynamicDraw")
							engineData->exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_DYNAMIC_DRAW(buffer->data.size()*sizeof(uint16_t),buffer->data.data());
						else
							engineData->exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_STATIC_DRAW(buffer->data.size()*sizeof(uint16_t),buffer->data.data());
					}
				}
			}
			if (currentprogram)
			{
				setPositionScale(engineData);
				setSamplers(engineData);
				setRegisters(engineData,currentprogram->vertexregistermap,vertexConstants,true);
				setRegisters(engineData,currentprogram->fragmentregistermap,fragmentConstants,false);
				setAttribs(engineData,currentprogram->vertexattributes);
				setAttribs(engineData,currentprogram->fragmentattributes);
			}
			engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(0);

			uint32_t count = action.udata2;
			engineData->exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(action.udata3);
			engineData->exec_glDrawElements_GL_TRIANGLES_GL_UNSIGNED_SHORT(count,(void*)(action.udata1*sizeof(uint16_t)));
			if (currentprogram)
			{
				resetAttribs(engineData,currentprogram->vertexattributes);
				resetAttribs(engineData,currentprogram->fragmentattributes);
			}
			break;
		}
		case RENDER_CREATEINDEXBUFFER:
		{
			//action.dataobject = IndexBuffer3D
			IndexBuffer3D* buffer = action.dataobject->as<IndexBuffer3D>();
			if (buffer && buffer->bufferID == UINT32_MAX)
				engineData->exec_glGenBuffers(1,&(buffer->bufferID));
			break;
		}
		case RENDER_UPLOADINDEXBUFFER:
		{
			//action.dataobject = IndexBuffer3D
			IndexBuffer3D* buffer = action.dataobject->as<IndexBuffer3D>();
			if (buffer->bufferID == UINT32_MAX)
				engineData->exec_glGenBuffers(1,&(buffer->bufferID));
			engineData->exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(buffer->bufferID);
			if (buffer->bufferUsage == "dynamicDraw")
				engineData->exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_DYNAMIC_DRAW(buffer->data.size()*sizeof(uint16_t),buffer->data.data());
			else
				engineData->exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_STATIC_DRAW(buffer->data.size()*sizeof(uint16_t),buffer->data.data());
			break;
		}
		case RENDER_DELETEINDEXBUFFER:
		{
			//action.dataobject = VertexBuffer3D
			IndexBuffer3D* buffer = action.dataobject->as<IndexBuffer3D>();
			if (buffer && buffer->bufferID != UINT32_MAX)
				engineData->exec_glDeleteBuffers(1,&buffer->bufferID);
			break;
		}
		case RENDER_DELETEBUFFER:
			//action.udata1 = bufferID
			engineData->exec_glDeleteBuffers(1,&action.udata1);
			break;
		case RENDER_SETPROGRAMCONSTANTS_FROM_MATRIX:
		{
			//action.udata1 = firstRegister
			//action.udata2 = 1, if vertex constants, 0 if fragment constants
			//action.udata3 = 1, if transposed
			//action.fdata = matrix (4*4)
			for (uint32_t i = 0; i < 4 && i < CONTEXT3D_PROGRAM_REGISTERS-action.udata1; i++ )
			{
				float* data = action.udata2 ? vertexConstants[i+action.udata1].data : fragmentConstants[i+action.udata1].data;
				if (action.udata3)
				{
					data[0] = action.fdata[i];
					data[1] = action.fdata[i+4];
					data[2] = action.fdata[i+8];
					data[3] = action.fdata[i+12];
				}
				else
				{
					data[0] = action.fdata[i*4];
					data[1] = action.fdata[i*4+1];
					data[2] = action.fdata[i*4+2];
					data[3] = action.fdata[i*4+3];
				}
			}
			break;
		}
		case RENDER_SETPROGRAMCONSTANTS_FROM_VECTOR:
		{
			//action.udata1 = firstRegister
			//action.udata2 = 1, if vertex constants, 0 if fragment constants
			//action.udata3 = numRegisters
			//action.fdata = vector list (4*numRegisters)
			for (uint32_t i = 0; i < action.udata3 && i < CONTEXT3D_PROGRAM_REGISTERS-action.udata1; i++ )
			{
				float* data = action.udata2 ? vertexConstants[i+action.udata1].data : fragmentConstants[i+action.udata1].data;
				data[0] = action.fdata[i*4];
				data[1] = action.fdata[i*4+1];
				data[2] = action.fdata[i*4+2];
				data[3] = action.fdata[i*4+3];
			}
			break;
		}
		case RENDER_SETTEXTUREAT:
		{
			//action.udata1 = sampler
			//action.udata2 = textureID
			//action.udata3 = removetexture
			if (action.udata2==UINT32_MAX)
				action.udata2=currenttextureid;
			if (action.udata3)
			{
				//LOG(LOG_INFO,"RENDER_SETTEXTUREAT remove:"<<action.udata1<<" "<<currentprogram->gpu_program);
				engineData->exec_glActiveTexture_GL_TEXTURE0(action.udata1);
				engineData->exec_glBindTexture_GL_TEXTURE_2D(0);
			}
			else
			{
				//LOG(LOG_INFO,"RENDER_SETTEXTUREAT:"<<action.udata2<<" "<<action.udata1<<" "<<currentprogram->gpu_program);
				samplers[action.udata1] = action.udata2;
			}
			break;
		}
		case RENDER_SETBLENDFACTORS:
		{
			//action.udata1 = sourcefactor
			//action.udata2 = destinationfactor
			engineData->exec_glBlendFunc((BLEND_FACTOR)action.udata1,(BLEND_FACTOR)action.udata2);
			break;
		}
		case RENDER_SETDEPTHTEST:
		{
			//action.udata1 = depthMask ? 1:0
			//action.udata2 = passCompareMode
			engineData->exec_glDepthMask(action.udata1);
			engineData->exec_glDepthFunc((DEPTHSTENCIL_FUNCTION)action.udata2);
			currentdepthfunction = (DEPTHSTENCIL_FUNCTION)action.udata2;
			break;
		}
		case RENDER_SETCULLING:
		{
			//action.udata1 = mode
			engineData->exec_glCullFace((TRIANGLE_FACE)action.udata1);
			currentcullface=(TRIANGLE_FACE)action.udata1;
			break;
		}
		case RENDER_GENERATETEXTURE:
			//action.dataobject = TextureBase
			//action.udata1 = set as current texture for rendering
			if (!action.dataobject.isNull())
			{
				TextureBase* tex = action.dataobject->as<TextureBase>();
				if (tex->textureID == UINT32_MAX)
				{
					if (tex->is<CubeTexture>())
						loadCubeTexture(tex->as<CubeTexture>(),UINT32_MAX,UINT32_MAX);
					else
						loadTexture(tex,UINT32_MAX);
				}
				currenttextureid=tex->textureID;
			}
			break;
		case RENDER_LOADTEXTURE:
			//action.dataobject = TextureBase
			//action.udata1 = miplevel
			loadTexture(action.dataobject->as<TextureBase>(),action.udata1);
			break;
		case RENDER_LOADCUBETEXTURE:
			//action.dataobject = CubeTexture
			//action.udata1 = miplevel
			//action.udata2 = side
			loadCubeTexture(action.dataobject->as<CubeTexture>(),action.udata1,action.udata2);
			break;
		case RENDER_SETSCISSORRECTANGLE:
			// action.udata1 = 0 => scissoring is disabled
			// action.fdata = x,y,width,height
			if (action.udata1)
				engineData->exec_glScissor(action.fdata[0],action.fdata[1],action.fdata[2],action.fdata[3]);
			else
				engineData->exec_glDisable_GL_SCISSOR_TEST();
			break;
		case RENDER_SETCOLORMASK:
			// action.udata1 = red | green | blue | alpha
			engineData->exec_glColorMask(action.udata1&0x01,action.udata1&0x02,action.udata1&0x04,action.udata1&0x08);
			break;
		case RENDER_SETSAMPLERSTATE:
			//action.udata1 = samplerid
			//action.udata2 = wrap + filter + mipfilter
			if (currentprogram && action.udata1 < currentprogram->samplerState.size())
			{
				currentprogram->samplerState[action.udata1].w = (action.udata2 & 0xf); //wrap
				currentprogram->samplerState[action.udata1].f = ((action.udata2 & 0xf0) >>4); //filter
				currentprogram->samplerState[action.udata1].m = ((action.udata2 & 0xf00) >>8) ; //mipfilter
			}
			break;
		case RENDER_DELETETEXTURE:
			//action.udata1 = textureid
			if (action.udata1 != UINT32_MAX)
				engineData->exec_glDeleteTextures(1, &action.udata1);
			break;
		case RENDER_CREATEVERTEXBUFFER:
		{
			//action.dataobject = VertexBuffer3D
			VertexBuffer3D* buffer = action.dataobject->as<VertexBuffer3D>();
			if (buffer && buffer->bufferID == UINT32_MAX)
				engineData->exec_glGenBuffers(1,&(buffer->bufferID));
			break;
		}
		case RENDER_UPLOADVERTEXBUFFER:
		{
			//action.dataobject = VertexBuffer3D
			VertexBuffer3D* buffer = action.dataobject->as<VertexBuffer3D>();
			if (buffer && buffer->bufferID != UINT32_MAX)
			{
				engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(buffer->bufferID);
				if (buffer->bufferUsage == "dynamicDraw")
					engineData->exec_glBufferData_GL_ARRAY_BUFFER_GL_DYNAMIC_DRAW(buffer->numVertices*buffer->data32PerVertex*sizeof(float),buffer->data.data());
				else
					engineData->exec_glBufferData_GL_ARRAY_BUFFER_GL_STATIC_DRAW(buffer->numVertices*buffer->data32PerVertex*sizeof(float),buffer->data.data());
				engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(0);
			}
			break;
		}
		case RENDER_DELETEVERTEXBUFFER:
		{
			//action.dataobject = VertexBuffer3D
			VertexBuffer3D* buffer = action.dataobject->as<VertexBuffer3D>();
			if (buffer && buffer->bufferID != UINT32_MAX)
				engineData->exec_glDeleteBuffers(1,&buffer->bufferID);
			break;
		}
		case RENDER_SETSTENCILREFERENCEVALUE:
			//action.udata1 = referenceValue | readMask | writeMask;
			//action.udata2 = currentstencilfunction;
			engineData->exec_glStencilMask(action.udata1&0xff);
			engineData->exec_glStencilFunc(DEPTHSTENCIL_FUNCTION(action.udata2),(action.udata1>>16)&0xff,(action.udata1>>8)&0xff);
			break;
		case RENDER_SETSTENCILACTIONS:
			//action.udata1 = face;
			//action.udata2 = func;
			//action.udata3 = (pass<<16)|(depthfail<<8)|depthpassstencilfail;
			engineData->exec_glStencilOpSeparate(TRIANGLE_FACE(action.udata1), DEPTHSTENCIL_OP(action.udata3&0xff), DEPTHSTENCIL_OP((action.udata3>>8)&0xff), DEPTHSTENCIL_OP((action.udata3>>16)&0xff));
			break;
	}
}
void Context3D::setRegisters(EngineData* engineData,std::vector<RegisterMapEntry>& registermap,constantregister* constants, bool isVertex)
{
	auto it = registermap.begin();
	while (it != registermap.end())
	{
		switch (it->usage)
		{
			case RegisterUsage::VECTOR_4:
			{
				float* data = constants[it->number].data;
				if (it->program_register_id == UINT32_MAX)
					it->program_register_id = engineData->exec_glGetUniformLocation(currentprogram->gpu_program,it->name.raw_buf());
				if (it->program_register_id != UINT32_MAX)
					engineData->exec_glUniform4fv(it->program_register_id,1, data);
				break;
			}
			case RegisterUsage::MATRIX_4_4:
			{
				if (it->program_register_id == UINT32_MAX)
					it->program_register_id = engineData->exec_glGetUniformLocation(currentprogram->gpu_program,it->name.raw_buf());
				float data2[4*4];
				for (uint32_t i =0; i < 4; i++)
				{
					float* data = constants[it->number+i].data;
					data2[i*4] = data[0];
					data2[i*4+1] = data[1];
					data2[i*4+2] = data[2];
					data2[i*4+3] = data[3];
				}
				if (it->program_register_id != UINT32_MAX)
					engineData->exec_glUniformMatrix4fv(it->program_register_id,1,false, data2);
				break;
			}
			case RegisterUsage::VECTOR_4_ARRAY:
			{
				if (it->program_register_id == UINT32_MAX)
					it->program_register_id = engineData->exec_glGetUniformLocation(currentprogram->gpu_program,it->name.raw_buf());
				// TODO currently the array is filled with every constant from the current number to the end of the constant list
				// find a way to determine the size of the array
				uint32_t regstart = it->number;
				uint32_t regcount = CONTEXT3D_PROGRAM_REGISTERS-regstart;
				float* data2 = new float[4*regcount];
				for (uint32_t i =0; i < regcount; i++)
				{
					float* data = constants[regstart+i].data;
					data2[i*4] = data[0];
					data2[i*4+1] = data[1];
					data2[i*4+2] = data[2];
					data2[i*4+3] = data[3];
				}
				if (it->program_register_id != UINT32_MAX)
					engineData->exec_glUniform4fv(it->program_register_id,regcount, data2);
				delete[] data2;
				break;
			}
			default:
				LOG(LOG_NOT_IMPLEMENTED,"Context3D.setRegisters: RegisterUsage:"<<(uint32_t)it->usage<<" "<<it->number<<" "<<isVertex<<" "<<currentprogram->gpu_program);
				break;
		}
		it++;
	}
}

void Context3D::setAttribs(EngineData *engineData,std::vector<RegisterMapEntry>& attributes)
{
	auto it = attributes.begin();
	while (it != attributes.end())
	{
		if (attribs[it->number].bufferID != UINT32_MAX)
		{
			if (it->program_register_id == UINT32_MAX)
			{
				it->program_register_id = engineData->exec_glGetAttribLocation(currentprogram->gpu_program,it->name.raw_buf());
			}
			if (it->program_register_id != UINT32_MAX)
			{
				engineData->exec_glEnableVertexAttribArray(it->program_register_id);
				engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(attribs[it->number].bufferID);
				engineData->exec_glVertexAttribPointer(it->program_register_id, attribs[it->number].data32PerVertex*sizeof(float), (const void*)(size_t)(attribs[it->number].offset*4),attribs[it->number].format);
			}
		}
		it++;
	}
}
void Context3D::resetAttribs(EngineData *engineData,std::vector<RegisterMapEntry>& attributes)
{
	auto it = attributes.begin();
	while (it != attributes.end())
	{
		if (attribs[it->number].bufferID != UINT32_MAX && it->program_register_id != UINT32_MAX)
			engineData->exec_glDisableVertexAttribArray(it->program_register_id);
		it++;
	}
}

void Context3D::setSamplers(EngineData *engineData)
{
	for (uint32_t i = 0; i < currentprogram->samplerState.size();i++)
	{
		uint32_t sampid = currentprogram->samplerState[i].program_sampler_id;
		if (sampid == UINT32_MAX)
		{
			char buf[100];
			sprintf(buf,"sampler%d",currentprogram->samplerState[i].n);
			sampid = engineData->exec_glGetUniformLocation(currentprogram->gpu_program,buf);
		}
		if (sampid != UINT32_MAX && samplers[currentprogram->samplerState[i].n] != UINT32_MAX)
		{
			currentprogram->samplerState[i].program_sampler_id = sampid;
			engineData->exec_glActiveTexture_GL_TEXTURE0(currentprogram->samplerState[i].n);
			if (currentprogram->samplerState[i].d)
				engineData->exec_glBindTexture_GL_TEXTURE_CUBE_MAP(samplers[currentprogram->samplerState[i].n]);
			else
				engineData->exec_glBindTexture_GL_TEXTURE_2D(samplers[currentprogram->samplerState[i].n]);
			engineData->exec_glSetTexParameters(currentprogram->samplerState[i].b,currentprogram->samplerState[i].d,currentprogram->samplerState[i].f,currentprogram->samplerState[i].m,currentprogram->samplerState[i].w);
			engineData->exec_glUniform1i(sampid, currentprogram->samplerState[i].n);
		}
		else
			LOG(LOG_ERROR,"sampler not found in program:"<<currentprogram->samplerState[i].n<<" "<<currentprogram->gpu_program<<" "<<sampid<<" "<<currentprogram);
	}
}

void Context3D::setPositionScale(EngineData *engineData)
{
	if (currentprogram)
	{
		if (currentprogram->vcPositionScale == UINT32_MAX)
			currentprogram->vcPositionScale = engineData->exec_glGetUniformLocation(currentprogram->gpu_program,"vcPositionScale");
		engineData->exec_glUniform4fv(currentprogram->vcPositionScale,1, vcposdata);
	}
}

void Context3D::disposeintern()
{
	Locker l(rendermutex);
	RenderThread* r = getSystemState()->getRenderThread();
	if (r)
	{
		if (this->backframebufferID[0] != UINT32_MAX)
			r->addDeletedTexture(this->backframebufferID[0]);
		if (this->backframebufferID[1] != UINT32_MAX)
			r->addDeletedTexture(this->backframebufferID[1]);
	}
	this->backframebufferID[0] = UINT32_MAX;
	this->backframebufferID[1] = UINT32_MAX;
	
	while (!programlist.empty())
	{
		auto it = programlist.begin();
		(*it)->removeStoredMember();
		programlist.erase(it);
	}
	while (!texturelist.empty())
	{
		auto it = texturelist.begin();
		(*it)->removeStoredMember();
		texturelist.erase(it);
	}
	while (!indexbufferlist.empty())
	{
		auto it = indexbufferlist.begin();
		(*it)->removeStoredMember();
		indexbufferlist.erase(it);
	}
	while (!vectorbufferlist.empty())
	{
		auto it = vectorbufferlist.begin();
		(*it)->removeStoredMember();
		vectorbufferlist.erase(it);
	}
}

bool Context3D::renderImpl(RenderContext &ctxt)
{
	Locker l(rendermutex);
	auto it = texturestoupload.begin();
	while (it != texturestoupload.end())
	{
		TextureBase* tex = (*it++).getPtr();
		if (tex->is<CubeTexture>())
			loadCubeTexture(tex->as<CubeTexture>(),UINT32_MAX,UINT32_MAX);
		else
			loadTexture(tex,UINT32_MAX);
		tex->incRef();
		getVm(tex->getSystemState())->addEvent(_MR(tex), _MR(Class<Event>::getInstanceS(tex->getInstanceWorker(),"textureReady")));
	}
	texturestoupload.clear();
	if (!swapbuffers || actions[1-currentactionvector].size() == 0)
	{
		swapbuffers = false;
		return false;
	}
	EngineData* engineData = getSystemState()->getEngineData();

	// set state to default values
	if (this->backBufferWidth && this->backBufferHeight)
	{
		engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(backframebuffer[currentactionvector]);
		engineData->exec_glFramebufferTexture2D_GL_FRAMEBUFFER(backframebufferID[currentactionvector]);
	 	engineData->exec_glViewport(0,0,this->backBufferWidth,this->backBufferHeight);
	}
	if (currentprogram)
		engineData->exec_glUseProgram(currentprogram->gpu_program);
	if (enableDepthAndStencilBackbuffer)
	{
		engineData->exec_glEnable_GL_DEPTH_TEST();
		engineData->exec_glEnable_GL_STENCIL_TEST();
	}
	else
	{
		engineData->exec_glDisable_GL_DEPTH_TEST();
		engineData->exec_glDisable_GL_STENCIL_TEST();
	}
	engineData->exec_glDepthMask(true);
	engineData->exec_glDepthFunc(currentdepthfunction);
	engineData->exec_glStencilFunc(currentstencilfunction, currentstencilref, currentstencilmask);
	engineData->exec_glCullFace(currentcullface);
	engineData->exec_glBlendFunc(BLEND_ONE,BLEND_ZERO);
	engineData->exec_glColorMask(true,true,true,true);

	// execute rendering actions
	for (uint32_t i = 0; i < actions[1-currentactionvector].size(); i++)
	{
		renderaction& action = actions[1-currentactionvector][i];
		handleRenderAction(engineData,action);
	}

	// cleanup for stage rendering
	engineData->exec_glClearDepthf(1.0);
	engineData->exec_glClearStencil(0);
	engineData->exec_glStencilMask(0xff);
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(0);
	engineData->exec_glDisable_GL_DEPTH_TEST();
	engineData->exec_glCullFace(FACE_NONE);
	engineData->exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(0);
	engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(0);
	engineData->exec_glUseProgram(0);
	if (renderingToTexture)
	{
		if (textureframebuffer != UINT32_MAX)
		{
			engineData->exec_glDeleteFramebuffers(1,&textureframebuffer);
			if (depthRenderBuffer != UINT32_MAX)
				engineData->exec_glDeleteRenderbuffers(1,&depthRenderBuffer);
			if (stencilRenderBuffer != UINT32_MAX)
				engineData->exec_glDeleteRenderbuffers(1,&stencilRenderBuffer);
			depthRenderBuffer = UINT32_MAX;
			textureframebuffer = UINT32_MAX;
			stencilRenderBuffer = UINT32_MAX;
		}
		engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(0);
		engineData->exec_glFrontFace(false);
		engineData->exec_glDrawBuffer_GL_BACK();
		engineData->exec_glDisable_GL_DEPTH_TEST();
		engineData->exec_glDisable_GL_STENCIL_TEST();
		renderingToTexture = false;
	}
	actions[1-currentactionvector].clear();
	backframebufferIDcurrent=backframebufferID[currentactionvector];
	swapbuffers = false;
	return true;
}

void Context3D::loadTexture(TextureBase *tex, uint32_t level)
{
	EngineData* engineData = getSystemState()->getEngineData();
	bool newtex = tex->textureID == UINT32_MAX;
	if (newtex)
		engineData->exec_glGenTextures(1, &(tex->textureID));
	engineData->exec_glBindTexture_GL_TEXTURE_2D(tex->textureID);
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
	if (newtex && tex->bitmaparray.size() == 0)
		engineData->exec_glTexImage2D_GL_TEXTURE_2D(0, tex->width, tex->height, 0, nullptr,tex->format,tex->compressedformat,0,false);
	else if (level == UINT32_MAX)
	{
		for (uint32_t i = 0; i <= tex->maxmiplevel && i < tex->bitmaparray.size(); i++)
		{
			if (tex->bitmaparray[i].size() > 0)
				engineData->exec_glTexImage2D_GL_TEXTURE_2D(i, max(tex->width>>i,1U), max(tex->height>>i,1U), 0, tex->bitmaparray[i].data(),tex->format,tex->compressedformat,tex->bitmaparray[i].size(),false);
		}
		for (uint32_t i = 0; i < tex->bitmaparray.size(); i++)
			tex->bitmaparray[i].clear();
	}
	else 
	{
		if (tex->maxmiplevel >= level && tex->bitmaparray.size() > level && tex->bitmaparray[level].size() > 0)
		{
			engineData->exec_glTexImage2D_GL_TEXTURE_2D(level, tex->width>>level, tex->height>>level, 0, tex->bitmaparray[level].data(),tex->format,tex->compressedformat,tex->bitmaparray[level].size(),false);
			tex->bitmaparray[level].clear();
		}
	}
}
void Context3D::loadCubeTexture(CubeTexture *tex, uint32_t miplevel, uint32_t side)
{
	EngineData* engineData = getSystemState()->getEngineData();
	if (tex->textureID == UINT32_MAX)
		engineData->exec_glGenTextures(1, &(tex->textureID));
	engineData->exec_glBindTexture_GL_TEXTURE_CUBE_MAP(tex->textureID);
	engineData->exec_glTexParameteri_GL_TEXTURE_CUBE_MAP_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
	engineData->exec_glTexParameteri_GL_TEXTURE_CUBE_MAP_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
	if (tex->bitmaparray.size() == 0)
	{
		engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(0,0, tex->width, tex->height, 0, nullptr,tex->format,tex->compressedformat,0);
		engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(1,0, tex->width, tex->height, 0, nullptr,tex->format,tex->compressedformat,0);
		engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(2,0, tex->width, tex->height, 0, nullptr,tex->format,tex->compressedformat,0);
		engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(3,0, tex->width, tex->height, 0, nullptr,tex->format,tex->compressedformat,0);
		engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(4,0, tex->width, tex->height, 0, nullptr,tex->format,tex->compressedformat,0);
		engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(5,0, tex->width, tex->height, 0, nullptr,tex->format,tex->compressedformat,0);
	}
	else
	{
		if (side == UINT32_MAX)
		{
			assert(miplevel==UINT32_MAX);
			// fill all sides and miplevels
			uint32_t side = 0;
			for (uint32_t i = 0; i < tex->bitmaparray.size(); i++)
			{
				side=(i/tex->max_miplevel)%6;
				uint32_t level = i%tex->max_miplevel;
				if (tex->bitmaparray[i].size() > 0)
					engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(side,level, tex->width>>level, tex->height>>level, 0, tex->bitmaparray[i].data(),tex->format,tex->compressedformat,tex->bitmaparray[i].size());
				else
					engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(side,level, tex->width>>level, tex->height>>level, 0, nullptr,tex->format,tex->compressedformat,0);
			}
		}
		else
		{
			uint32_t level = miplevel;
			uint32_t i = tex->max_miplevel*side+miplevel;
			if (tex->bitmaparray[i].size() > 0)
				engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(side,level, tex->width>>level, tex->height>>level, 0, tex->bitmaparray[i].data(),tex->format,tex->compressedformat,tex->bitmaparray[i].size());
			else
				engineData->exec_glTexImage2D_GL_TEXTURE_CUBE_MAP_POSITIVE_X_GL_UNSIGNED_BYTE(side,level, tex->width>>level, tex->height>>level, 0, nullptr,tex->format,tex->compressedformat,0);
		}
	}
	engineData->exec_glBindTexture_GL_TEXTURE_2D(0);
}

void Context3D::init(Stage3D* s)
{
	stage3D = s;
	EngineData* engineData = getSystemState()->getEngineData();
	driverInfo = engineData->driverInfoString;
	maxBackBufferWidth = engineData->maxTextureSize;;
	maxBackBufferHeight = engineData->maxTextureSize;;
	
	stage3D->incRef();
	getVm(getSystemState())->addEvent(_MR(stage3D),_MR(Class<Event>::getInstanceS(getInstanceWorker(),"context3DCreate")));
}

void Context3D::configureBackBufferIntern(bool enableDepthAndStencil, uint32_t width, uint32_t height, int index)
{
	backBufferWidth=width;
	backBufferHeight=height;
	EngineData* engineData = getSystemState()->getEngineData();
	if (backframebuffer[index] == UINT32_MAX)
		backframebuffer[index] = engineData->exec_glGenFramebuffer();
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(backframebuffer[index]);
	if (backframebufferID[index] == UINT32_MAX)
		engineData->exec_glGenTextures(1,&backframebufferID[index]);

	engineData->exec_glBindTexture_GL_TEXTURE_2D(backframebufferID[index]);
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_NEAREST();
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_NEAREST();
	engineData->exec_glFramebufferTexture2D_GL_FRAMEBUFFER(backframebufferID[index]);
	engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, width, height, 0, nullptr,true);
	engineData->exec_glBindTexture_GL_TEXTURE_2D(0);
	if (enableDepthAndStencil)
	{
		if (backDepthRenderBuffer[index] == UINT32_MAX)
			backDepthRenderBuffer[index] = engineData->exec_glGenRenderbuffer();
		
		if (engineData->supportPackedDepthStencil)
		{
			engineData->exec_glBindRenderbuffer(backDepthRenderBuffer[index]);
			engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_STENCIL(width,height);
			engineData->exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_STENCIL_ATTACHMENT(backDepthRenderBuffer[index]);
			engineData->exec_glBindRenderbuffer(0);
		}
		else
		{
			engineData->exec_glBindRenderbuffer(backDepthRenderBuffer[index]);
			engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_COMPONENT16(width,height);
			engineData->exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_ATTACHMENT(backDepthRenderBuffer[index]);
			engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_STENCIL_INDEX8(width,height);
			engineData->exec_glBindRenderbuffer(0);
			engineData->exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_STENCIL_ATTACHMENT(backStencilRenderBuffer[index]);
		}
		engineData->exec_glEnable_GL_DEPTH_TEST();
		engineData->exec_glEnable_GL_STENCIL_TEST();
	}
	else
	{
		engineData->exec_glDisable_GL_DEPTH_TEST();
		engineData->exec_glDisable_GL_STENCIL_TEST();
	}
	engineData->exec_glViewport(0,0,width,height);
	engineData->exec_glBindTexture_GL_TEXTURE_2D(0);
}

Context3D::Context3D(ASWorker* wrk, Class_base *c):EventDispatcher(wrk,c),samplers{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX},currentactionvector(0)
  ,textureframebuffer(UINT32_MAX),depthRenderBuffer(UINT32_MAX),stencilRenderBuffer(UINT32_MAX),currentprogram(nullptr),currenttextureid(UINT32_MAX)
  ,renderingToTexture(false),enableDepthAndStencilBackbuffer(true),enableDepthAndStencilTextureBuffer(true),swapbuffers(false)
  ,currentcullface(TRIANGLE_FACE::FACE_NONE),currentdepthfunction(DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_LESS)
  ,currentstencilfunction(DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_ALWAYS), currentstencilref(0xff), currentstencilmask(0xff)
  ,stage3D(nullptr),backBufferHeight(0),backBufferWidth(0),enableErrorChecking(false)
  ,maxBackBufferHeight(16384),maxBackBufferWidth(16384)
{
	subtype = SUBTYPE_CONTEXT3D;
	memset(vertexConstants,0,4 * CONTEXT3D_PROGRAM_REGISTERS * sizeof(float));
	memset(fragmentConstants,0,4 * CONTEXT3D_PROGRAM_REGISTERS * sizeof(float));
	driverInfo = "Disposed";
	backframebufferID[0]=UINT32_MAX;
	backframebufferID[1]=UINT32_MAX;
	backframebuffer[0]=UINT32_MAX;
	backframebuffer[1]=UINT32_MAX;
	backDepthRenderBuffer[0]=UINT32_MAX;
	backDepthRenderBuffer[1]=UINT32_MAX;
	backStencilRenderBuffer[0]=UINT32_MAX;
	backStencilRenderBuffer[1]=UINT32_MAX;
}

void Context3D::addAction(RENDER_ACTION type, ASObject *dataobject)
{
	if (!getSystemState()->getRenderThread()->isStarted())
		return;
	renderaction action;
	action.action = type;
	if (dataobject)
	{
		dataobject->incRef();
		action.dataobject = _MR(dataobject);
	}
	actions[currentactionvector].push_back(action);
}

void Context3D::addAction(renderaction action)
{
	if (!getSystemState()->getRenderThread() || !getSystemState()->getRenderThread()->isStarted())
		return;
	actions[currentactionvector].push_back(action);
}

void Context3D::sinit(lightspark::Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED|CLASS_FINAL);

	REGISTER_GETTER_RESULTTYPE(c,backBufferHeight,Integer);
	REGISTER_GETTER_RESULTTYPE(c,backBufferWidth,Integer);
	REGISTER_GETTER_RESULTTYPE(c,driverInfo,ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,enableErrorChecking,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,maxBackBufferHeight,Integer);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,maxBackBufferWidth,Integer);
	c->setDeclaredMethodByQName("profile","",c->getSystemState()->getBuiltinFunction(getProfile,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("supportsVideoTexture","",c->getSystemState()->getBuiltinFunction(supportsVideoTexture,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("dispose","",c->getSystemState()->getBuiltinFunction(dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("configureBackBuffer","",c->getSystemState()->getBuiltinFunction(configureBackBuffer),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createIndexBuffer","",c->getSystemState()->getBuiltinFunction(createIndexBuffer,1,Class<IndexBuffer3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createProgram","",c->getSystemState()->getBuiltinFunction(createProgram,0,Class<Program3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createCubeTexture","",c->getSystemState()->getBuiltinFunction(createCubeTexture,3,Class<CubeTexture>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createTexture","",c->getSystemState()->getBuiltinFunction(createTexture,4,Class<Texture>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createRectangleTexture","",c->getSystemState()->getBuiltinFunction(createRectangleTexture,4,Class<RectangleTexture>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createVertexBuffer","",c->getSystemState()->getBuiltinFunction(createVertexBuffer,2,Class<VertexBuffer3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createVideoTexture","",c->getSystemState()->getBuiltinFunction(createVideoTexture,0,Class<VideoTexture>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawToBitmapData","",c->getSystemState()->getBuiltinFunction(drawToBitmapData),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawTriangles","",c->getSystemState()->getBuiltinFunction(drawTriangles),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setBlendFactors","",c->getSystemState()->getBuiltinFunction(setBlendFactors),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setColorMask","",c->getSystemState()->getBuiltinFunction(setColorMask),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setCulling","",c->getSystemState()->getBuiltinFunction(setCulling),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setDepthTest","",c->getSystemState()->getBuiltinFunction(setDepthTest),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setProgram","",c->getSystemState()->getBuiltinFunction(setProgram),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setProgramConstantsFromByteArray","",c->getSystemState()->getBuiltinFunction(setProgramConstantsFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setProgramConstantsFromMatrix","",c->getSystemState()->getBuiltinFunction(setProgramConstantsFromMatrix),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setProgramConstantsFromVector","",c->getSystemState()->getBuiltinFunction(setProgramConstantsFromVector),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setRenderToBackBuffer","",c->getSystemState()->getBuiltinFunction(setRenderToBackBuffer),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setRenderToTexture","",c->getSystemState()->getBuiltinFunction(setRenderToTexture),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSamplerStateAt","",c->getSystemState()->getBuiltinFunction(setSamplerStateAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setScissorRectangle","",c->getSystemState()->getBuiltinFunction(setScissorRectangle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("present","",c->getSystemState()->getBuiltinFunction(present),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setStencilActions","",c->getSystemState()->getBuiltinFunction(setStencilActions),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setStencilReferenceValue","",c->getSystemState()->getBuiltinFunction(setStencilReferenceValue),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTextureAt","",c->getSystemState()->getBuiltinFunction(setTextureAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setVertexBufferAt","",c->getSystemState()->getBuiltinFunction(setVertexBufferAt),NORMAL_METHOD,true);
}

bool Context3D::destruct()
{
	disposeintern();
	return EventDispatcher::destruct();
}

void Context3D::finalize()
{
	disposeintern();
	EventDispatcher::finalize();
}

bool Context3D::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = EventDispatcher::countCylicMemberReferences(gcstate);
	for (auto it = programlist.begin(); it != programlist.end(); it++)
		ret = (*it)->countAllCylicMemberReferences(gcstate) || ret;
	for (auto it = texturelist.begin(); it != texturelist.end(); it++)
		ret = (*it)->countAllCylicMemberReferences(gcstate) || ret;
	for (auto it = indexbufferlist.begin(); it != indexbufferlist.end(); it++)
		ret = (*it)->countAllCylicMemberReferences(gcstate) || ret;
	for (auto it = vectorbufferlist.begin(); it != vectorbufferlist.end(); it++)
		ret = (*it)->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void Context3D::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
	for (auto it = programlist.begin(); it != programlist.end();)
	{
		ASObject* o = (*it);
		it = programlist.erase(it);
		o->prepareShutdown();
		o->removeStoredMember();
	}
	for (auto it = texturelist.begin(); it != texturelist.end();)
	{
		ASObject* o = (*it);
		it = texturelist.erase(it);
		o->prepareShutdown();
		o->removeStoredMember();
	}
	for (auto it = indexbufferlist.begin(); it != indexbufferlist.end();)
	{
		ASObject* o = (*it);
		it = indexbufferlist.erase(it);
		o->prepareShutdown();
		o->removeStoredMember();
	}
	for (auto it = vectorbufferlist.begin(); it != vectorbufferlist.end();)
	{
		ASObject* o = (*it);
		it = vectorbufferlist.erase(it);
		o->prepareShutdown();
		o->removeStoredMember();
	}
}


ASFUNCTIONBODY_GETTER(Context3D,backBufferHeight)
ASFUNCTIONBODY_GETTER(Context3D,backBufferWidth)
ASFUNCTIONBODY_GETTER(Context3D,driverInfo)
ASFUNCTIONBODY_GETTER_SETTER(Context3D,enableErrorChecking)
ASFUNCTIONBODY_GETTER_SETTER(Context3D,maxBackBufferHeight)
ASFUNCTIONBODY_GETTER_SETTER(Context3D,maxBackBufferWidth)

ASFUNCTIONBODY_ATOM(Context3D,getProfile)
{
	ret = asAtomHandler::fromStringID(wrk->getSystemState()->getEngineData()->context3dProfile);
}
ASFUNCTIONBODY_ATOM(Context3D,supportsVideoTexture)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.supportsVideoTexture");
	asAtomHandler::setBool(ret,false);
}
ASFUNCTIONBODY_ATOM(Context3D,dispose)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	th->disposeintern();
	bool recreate;
	ARG_CHECK(ARG_UNPACK(recreate,true));
	th->driverInfo = "Disposed";
}
ASFUNCTIONBODY_ATOM(Context3D,configureBackBuffer)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	int antiAlias;
	bool wantsBestResolution;
	bool wantsBestResolutionOnBrowserZoom;
	uint32_t w;
	uint32_t h;
	ARG_CHECK(ARG_UNPACK(w)(h)(antiAlias)(th->enableDepthAndStencilBackbuffer, true)(wantsBestResolution,false)(wantsBestResolutionOnBrowserZoom,false));
	if (antiAlias || wantsBestResolution || wantsBestResolutionOnBrowserZoom)
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.configureBackBuffer does not use all parameters:"<<antiAlias<<" "<<wantsBestResolution<<" "<<wantsBestResolutionOnBrowserZoom);
	renderaction action;
	action.action = RENDER_ACTION::RENDER_CONFIGUREBACKBUFFER;
	action.udata1 = th->enableDepthAndStencilBackbuffer ? 1:0;
	action.udata2 = w;
	action.udata3 = h;
	th->addAction(action);
}
ASFUNCTIONBODY_ATOM(Context3D,createCubeTexture)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	tiny_string format;
	bool optimizeForRenderToTexture;
	int32_t streamingLevels;
	CubeTexture* res = Class<CubeTexture>::getInstanceS(wrk,th);
	res->incRef();
	res->addStoredMember();
	th->texturelist.insert(res);
	uint32_t size;
	ARG_CHECK(ARG_UNPACK(size)(format)(optimizeForRenderToTexture)(streamingLevels,0));
	uint32_t i = size;
	while (i)
	{
		res->max_miplevel++;
		i>>=1;
	}
	res->width = res->height = size;
	res->bitmaparray.resize(res->max_miplevel*6); // reserve space for 6 bitmaps * no. of mipmaps
	res->setFormat(format);
	if (optimizeForRenderToTexture || streamingLevels != 0)
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.createCubeTexture ignores parameters optimizeForRenderToTexture,streamingLevels:"<<optimizeForRenderToTexture<<" "<<streamingLevels<<" "<<res);
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Context3D,createRectangleTexture)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	tiny_string format;
	bool optimizeForRenderToTexture;
	int32_t streamingLevels;
	RectangleTexture* res = Class<RectangleTexture>::getInstanceS(wrk,th);
	res->incRef();
	res->addStoredMember();
	th->texturelist.insert(res);
	ARG_CHECK(ARG_UNPACK(res->width)(res->height)(format)(optimizeForRenderToTexture)(streamingLevels, 0));
	res->setFormat(format);
	if (optimizeForRenderToTexture || streamingLevels != 0)
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.createRectangleTexture ignores parameters optimizeForRenderToTexture,streamingLevels:"<<optimizeForRenderToTexture<<" "<<streamingLevels<<" "<<res);
	ret = asAtomHandler::fromObjectNoPrimitive(res);
}
ASFUNCTIONBODY_ATOM(Context3D,createTexture)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	tiny_string format;
	bool optimizeForRenderToTexture;
	int32_t streamingLevels;
	Texture* res = Class<Texture>::getInstanceS(wrk,th);
	res->incRef();
	res->addStoredMember();
	th->texturelist.insert(res);
	ARG_CHECK(ARG_UNPACK(res->width)(res->height)(format)(optimizeForRenderToTexture)(streamingLevels, 0));
	res->setFormat(format);
	if (optimizeForRenderToTexture || streamingLevels != 0)
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.createTexture ignores parameters optimizeForRenderToTexture,streamingLevels:"<<optimizeForRenderToTexture<<" "<<streamingLevels<<" "<<res);
	ret = asAtomHandler::fromObjectNoPrimitive(res);
}
ASFUNCTIONBODY_ATOM(Context3D,createVideoTexture)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.createVideoTexture does nothing");
	VideoTexture* res = Class<VideoTexture>::getInstanceS(wrk,th);
	res->incRef();
	res->addStoredMember();
	th->texturelist.insert(res);
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Context3D,createProgram)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	Program3D* p = Class<Program3D>::getInstanceS(wrk,th);
	p->incRef();
	p->addStoredMember();
	th->programlist.insert(p);
	ret = asAtomHandler::fromObject(p);
}
ASFUNCTIONBODY_ATOM(Context3D,createVertexBuffer)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	int32_t numVertices;
	int32_t data32PerVertex;
	tiny_string bufferUsage;
	ARG_CHECK(ARG_UNPACK(numVertices)(data32PerVertex)(bufferUsage,"staticDraw"));
	if (data32PerVertex > 64 || numVertices > 0x10000)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Buffer Too Big");
		return;
	}
	if (data32PerVertex <= 0 || numVertices <= 0)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Buffer Has Zero Size");
		return;
	}
	VertexBuffer3D* buffer = Class<VertexBuffer3D>::getInstanceS(wrk,th, numVertices,data32PerVertex,bufferUsage);
	th->addAction(RENDER_ACTION::RENDER_CREATEVERTEXBUFFER,buffer);
	ret = asAtomHandler::fromObject(buffer);
}
ASFUNCTIONBODY_ATOM(Context3D,createIndexBuffer)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	int32_t numVertices;
	tiny_string bufferUsage;
	ARG_CHECK(ARG_UNPACK(numVertices)(bufferUsage,"staticDraw"));
	if (numVertices >= 0xf0000)
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Buffer Too Big");
		return;
	}
	IndexBuffer3D* buffer = Class<IndexBuffer3D>::getInstanceS(wrk,th,numVertices,bufferUsage);
	th->addAction(RENDER_ACTION::RENDER_CREATEINDEXBUFFER,buffer);
	ret = asAtomHandler::fromObject(buffer);
}

ASFUNCTIONBODY_ATOM(Context3D,clear)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	number_t red,green,blue,alpha,depth;
	renderaction action;
	action.action = RENDER_ACTION::RENDER_CLEAR;
	ARG_CHECK(ARG_UNPACK(red,0.0)(green,0.0)(blue,0.0)(alpha,1.0)(depth,1.0)(action.udata1,0)(action.udata2,0xffffffff));
	action.fdata[0] = (float)red;
	action.fdata[1] = (float)green;
	action.fdata[2] = (float)blue;
	action.fdata[3] = (float)alpha;
	action.fdata[4] = (float)depth;
	th->addAction(action);
	if (th->enableErrorChecking)
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.clear with errorchecking");
}

ASFUNCTIONBODY_ATOM(Context3D,drawToBitmapData)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.drawToBitmapData does nothing");
	_NR<BitmapData> destination;
	ARG_CHECK(ARG_UNPACK(destination));
}

ASFUNCTIONBODY_ATOM(Context3D,drawTriangles)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	_NR<IndexBuffer3D> indexBuffer;
	int32_t firstIndex;
	int32_t numTriangles;
	ARG_CHECK(ARG_UNPACK(indexBuffer)(firstIndex,0)(numTriangles,-1));
	if (indexBuffer.isNull())
	{
		LOG(LOG_ERROR,"Context3d.drawTriangles without indexBuffer");
		return;
	}
	renderaction action;
	action.action = RENDER_ACTION::RENDER_DRAWTRIANGLES;
	action.udata1 = firstIndex < 0 ? 0 : firstIndex;
	action.udata2 = numTriangles == -1 ? indexBuffer->data.size() : numTriangles*3;
	action.udata3 = indexBuffer->bufferID;
	if (indexBuffer->bufferID==UINT32_MAX)
	{
		// IndexBuffer3D was created during this loop, so it doesn't have a bufferID yet
		th->rendermutex.lock();
		action.dataobject = indexBuffer;
		th->actions[th->currentactionvector].push_back(action);
		th->rendermutex.unlock();
	}
	else
		th->actions[th->currentactionvector].push_back(action);
	if (th->enableErrorChecking)
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.drawTriangles with errorchecking");
}

ASFUNCTIONBODY_ATOM(Context3D,setBlendFactors)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	tiny_string sourceFactor;
	tiny_string destinationFactor;
	ARG_CHECK(ARG_UNPACK(sourceFactor)(destinationFactor));
	BLEND_FACTOR src = BLEND_ONE, dst = BLEND_ONE;
	if (sourceFactor == "destinationAlpha")
		src = BLEND_DST_ALPHA;
	else if (sourceFactor == "destinationColor")
		src = BLEND_DST_COLOR;
	else if (sourceFactor == "one")
		src = BLEND_ONE;
	else if (sourceFactor == "oneMinusDestinationAlpha")
		src = BLEND_ONE_MINUS_DST_ALPHA;
	else if (sourceFactor == "oneMinusDestinationColor")
		src = BLEND_ONE_MINUS_DST_COLOR;
	else if (sourceFactor == "oneMinusSourceAlpha")
		src = BLEND_ONE_MINUS_SRC_ALPHA;
	else if (sourceFactor == "oneMinusSourceColor")
		src = BLEND_ONE_MINUS_SRC_COLOR;
	else if (sourceFactor == "sourceAlpha")
		src = BLEND_SRC_ALPHA;
	else if (sourceFactor == "sourceColor")
		src = BLEND_SRC_COLOR;
	else if (sourceFactor == "zero")
		src = BLEND_ZERO;
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError);
		return;
	}
	if (destinationFactor == "destinationAlpha")
		dst = BLEND_DST_ALPHA;
	else if (destinationFactor == "destinationColor")
		dst = BLEND_DST_COLOR;
	else if (destinationFactor == "one")
		dst = BLEND_ONE;
	else if (destinationFactor == "oneMinusDestinationAlpha")
		dst = BLEND_ONE_MINUS_DST_ALPHA;
	else if (destinationFactor == "oneMinusDestinationColor")
		dst = BLEND_ONE_MINUS_DST_COLOR;
	else if (destinationFactor == "oneMinusSourceAlpha")
		dst = BLEND_ONE_MINUS_SRC_ALPHA;
	else if (destinationFactor == "oneMinusSourceColor")
		dst = BLEND_ONE_MINUS_SRC_COLOR;
	else if (destinationFactor == "sourceAlpha")
		dst = BLEND_SRC_ALPHA;
	else if (destinationFactor == "sourceColor")
		dst = BLEND_SRC_COLOR;
	else if (destinationFactor == "zero")
		dst = BLEND_ZERO;
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError);
		return;
	}
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETBLENDFACTORS;
	action.udata1 = src;
	action.udata2 = dst;
	th->actions[th->currentactionvector].push_back(action);
}
ASFUNCTIONBODY_ATOM(Context3D,setColorMask)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	bool red;
	bool green;
	bool blue;
	bool alpha;
	ARG_CHECK(ARG_UNPACK(red)(green)(blue)(alpha));
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETCOLORMASK;
	action.udata1 = 0;
	if (red) action.udata1 |= 0x01;
	if (green) action.udata1 |= 0x02;
	if (blue) action.udata1 |= 0x04;
	if (alpha) action.udata1 |= 0x08;
	th->actions[th->currentactionvector].push_back(action);
}
ASFUNCTIONBODY_ATOM(Context3D,setCulling)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	tiny_string triangleFaceToCull;
	ARG_CHECK(ARG_UNPACK(triangleFaceToCull));
	
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETCULLING;
	action.udata1 = FACE_NONE;
	if (triangleFaceToCull == "none")
		action.udata1 = FACE_NONE;
	else if (triangleFaceToCull == "front")
		action.udata1 = FACE_FRONT;
	else if (triangleFaceToCull == "back")
		action.udata1 = FACE_BACK;
	else if (triangleFaceToCull == "frontAndBack")
		action.udata1 = FACE_FRONT_AND_BACK;
	th->addAction(action);
}
ASFUNCTIONBODY_ATOM(Context3D,setDepthTest)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	bool depthMask;
	tiny_string passCompareMode;
	ARG_CHECK(ARG_UNPACK(depthMask)(passCompareMode));
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETDEPTHTEST;
	action.udata1= depthMask ? 1:0;
	if (passCompareMode =="always")
		action.udata2 = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_ALWAYS;
	else if (passCompareMode =="equal")
		action.udata2 = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_EQUAL;
	else if (passCompareMode =="greater")
		action.udata2 = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_GREATER;
	else if (passCompareMode =="greaterEqual")
		action.udata2 = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_GREATER_EQUAL;
	else if (passCompareMode =="less")
		action.udata2 = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_LESS;
	else if (passCompareMode =="lessEqual")
		action.udata2 = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_LESS_EQUAL;
	else if (passCompareMode =="never")
		action.udata2 = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_NEVER;
	else if (passCompareMode =="notEqual")
		action.udata2 = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_NOT_EQUAL;
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"passCompareMode");
		return;
	}
	th->addAction(action);
}
ASFUNCTIONBODY_ATOM(Context3D,setProgram)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	_NR<Program3D> program;
	ARG_CHECK(ARG_UNPACK(program));
	if (!program.isNull())
	{
		th->rendermutex.lock();
		if (program->gpu_program == UINT32_MAX)
			th->addAction(RENDER_ACTION::RENDER_UPLOADPROGRAM,program.getPtr());
		th->addAction(RENDER_ACTION::RENDER_SETPROGRAM,program.getPtr());
		th->rendermutex.unlock();
	}
}
ASFUNCTIONBODY_ATOM(Context3D,setProgramConstantsFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.setProgramConstantsFromByteArray does nothing");
	tiny_string programType;
	int32_t firstRegister;
	int32_t numRegisters;
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	ARG_CHECK(ARG_UNPACK(programType)(firstRegister)(numRegisters)(data)(byteArrayOffset));
}
ASFUNCTIONBODY_ATOM(Context3D,setProgramConstantsFromMatrix)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	tiny_string programType;
	uint32_t firstRegister;
	_NR<Matrix3D> matrix;
	bool transposedMatrix;
	ARG_CHECK(ARG_UNPACK(programType)(firstRegister)(matrix)(transposedMatrix, false));
	if (matrix.isNull())
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setProgramConstantsFromMatrix with empty matrix");
	else
	{
		renderaction action;
		action.action = RENDER_ACTION::RENDER_SETPROGRAMCONSTANTS_FROM_MATRIX;
		matrix->getRawDataAsFloat(action.fdata);
		action.udata1= firstRegister;
		action.udata2= programType == "vertex" ? 1 : 0;
		action.udata3= transposedMatrix ? 1 : 0;
		th->addAction(action);
	}
}
ASFUNCTIONBODY_ATOM(Context3D,setProgramConstantsFromVector)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	tiny_string programType;
	int32_t firstRegister;
	_NR<Vector> data;
	int32_t numRegisters;
	ARG_CHECK(ARG_UNPACK(programType)(firstRegister)(data)(numRegisters,-1));
	if (data.isNull())
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setProgramConstantsFromVector with empty Vector");
	else
	{
		renderaction action;
		action.action = RENDER_ACTION::RENDER_SETPROGRAMCONSTANTS_FROM_VECTOR;
		action.udata1= firstRegister;
		action.udata2= programType == "vertex" ? 1 : 0;
		action.udata3= numRegisters < 0 ? data->size()/4 : min ((uint32_t)numRegisters,data->size()/4);
		if (action.udata3)
		{
			if (action.udata3 > CONTEXT3D_PROGRAM_REGISTERS-action.udata1)
			{
				createError<RangeError>(wrk,kOutOfRangeError,"Constant Register Out Of Bounds");
				return;
			}
			for (uint32_t i = 0; i < action.udata3*4; i++)
			{
				asAtom a = data->at(i);
				action.fdata[i] = asAtomHandler::toNumber(a);
			}
			th->addAction(action);
		}
	}
}


ASFUNCTIONBODY_ATOM(Context3D,setScissorRectangle)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	_NR<Rectangle> rectangle;
	ARG_CHECK(ARG_UNPACK(rectangle));
	if (!rectangle.isNull())
	{
		if (th->actions[th->currentactionvector].back().action==RENDER_ACTION::RENDER_SETSCISSORRECTANGLE)
		{
			th->actions[th->currentactionvector].back().udata1=1;
			th->actions[th->currentactionvector].back().fdata[0] = rectangle->x;
			th->actions[th->currentactionvector].back().fdata[1] = rectangle->y;
			th->actions[th->currentactionvector].back().fdata[2] = rectangle->width;
			th->actions[th->currentactionvector].back().fdata[3] = rectangle->height;
		}
		else
		{
			renderaction action;
			action.action = RENDER_ACTION::RENDER_SETSCISSORRECTANGLE;
			action.udata1=1;
			action.fdata[0] = rectangle->x;
			action.fdata[1] = rectangle->y;
			action.fdata[2] = rectangle->width;
			action.fdata[3] = rectangle->height;
			th->addAction(action);
		}
	}
	else
	{
		renderaction action;
		action.action = RENDER_ACTION::RENDER_SETSCISSORRECTANGLE;
		action.udata1=0;
		th->addAction(action);
	}
}
ASFUNCTIONBODY_ATOM(Context3D,setRenderToBackBuffer)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	renderaction action;
	action.action = RENDER_ACTION::RENDER_RENDERTOBACKBUFFER;
	th->addAction(action);
}
ASFUNCTIONBODY_ATOM(Context3D,setRenderToTexture)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	bool enableDepthAndStencil;
	int32_t antiAlias;
	int32_t surfaceSelector;
	int32_t colorOutputIndex;
	_NR<TextureBase> tex;
	ARG_CHECK(ARG_UNPACK(tex)(enableDepthAndStencil,false)(antiAlias,0)(surfaceSelector, 0)(colorOutputIndex, 0));
	if (antiAlias!=0 || surfaceSelector!=0 || colorOutputIndex!=0)
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setRenderToTexture ignores parameters antiAlias, surfaceSelector, colorOutput");
	th->rendermutex.lock();
	renderaction action;
	action.action = RENDER_ACTION::RENDER_TOTEXTURE;
	action.dataobject = tex;
	action.udata1 = enableDepthAndStencil ? 1 : 0;
	action.udata2 = tex->width;
	action.udata3 = tex->height;
	th->addAction(action);
	th->rendermutex.unlock();
}
ASFUNCTIONBODY_ATOM(Context3D,setSamplerStateAt)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	int32_t sampler;
	tiny_string wrap;
	tiny_string filter;
	tiny_string mipfilter;
	ARG_CHECK(ARG_UNPACK(sampler)(wrap)(filter)(mipfilter));
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETSAMPLERSTATE;
	action.udata1 = uint32_t(sampler);
	action.udata2 = 0;
	if (wrap == "clamp")
		action.udata2 |= 0x0;
	else if (wrap == "repeat")
		action.udata2 |= 0x3;
	else if (wrap == "clamp_u_repeat_v")
		action.udata2 |= 0x1;
	else if (wrap == "repeat_u_clamp_v")
		action.udata2 |= 0x2;

	if (filter == "anisotropic16x")
	{
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setSamplerStateAt for filter "<<filter);
		action.udata2 |= 0x60;
	}
	else if (filter == "anisotropic2x")
	{
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setSamplerStateAt for filter "<<filter);
		action.udata2 |= 0x50;
	}
	else if (filter == "anisotropic4x")
	{
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setSamplerStateAt for filter "<<filter);
		action.udata2 |= 0x40;
	}
	else if (filter == "anisotropic8x")
	{
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setSamplerStateAt for filter "<<filter);
		action.udata2 |= 0x30;
	}
	else if (filter == "linear")
		action.udata2 |= 0x10;
	else if (filter == "nearest")
		action.udata2 |= 0x00;

	if (mipfilter == "miplinear")
		action.udata2 |= 0x200;
	else if (mipfilter == "mipnearest")
		action.udata2 |= 0x100;
	else if (mipfilter == "mipnone")
		action.udata2 |= 0x000;

	th->addAction(action);
}
ASFUNCTIONBODY_ATOM(Context3D,present)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	Locker l(th->rendermutex);
	if (th->swapbuffers)
	{
		if (wrk->getSystemState()->getRenderThread()->isStarted() && !th->actions[th->currentactionvector].empty())
			LOG(LOG_ERROR,"last frame has not been rendered yet, skipping frame:"<<th->actions[1-th->currentactionvector].size());
		th->actions[th->currentactionvector].clear();
	}
	else
	{
		th->swapbuffers = true;
		th->currentactionvector=1-th->currentactionvector;
		if (wrk->getSystemState()->getRenderThread()->isStarted())
			wrk->getSystemState()->getRenderThread()->draw(false);
	}
}
ASFUNCTIONBODY_ATOM(Context3D,setStencilActions)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	tiny_string triangleFace;
	tiny_string compareMode;
	tiny_string actionOnBothPass;
	tiny_string actionOnDepthFail;
	tiny_string actionOnDepthPassStencilFail;
	ARG_CHECK(ARG_UNPACK(triangleFace,"frontAndBack")(compareMode,"always")(actionOnBothPass,"keep")(actionOnDepthFail,"keep")(actionOnDepthPassStencilFail,"keep"));
	TRIANGLE_FACE face;
	if (triangleFace =="frontAndBack")
		face = TRIANGLE_FACE::FACE_FRONT_AND_BACK;
	else if (triangleFace =="back")
		face = TRIANGLE_FACE::FACE_BACK;
	else if (triangleFace =="front")
		face = TRIANGLE_FACE::FACE_FRONT;
	else if (triangleFace =="none")
		face = TRIANGLE_FACE::FACE_NONE;
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"triangleFace");
		return;
	}
	DEPTHSTENCIL_FUNCTION func;
	if (compareMode =="always")
		func = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_ALWAYS;
	else if (compareMode =="equal")
		func = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_EQUAL;
	else if (compareMode =="greater")
		func = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_GREATER;
	else if (compareMode =="greaterEqual")
		func = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_GREATER_EQUAL;
	else if (compareMode =="less")
		func = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_LESS;
	else if (compareMode =="lessEqual")
		func = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_LESS_EQUAL;
	else if (compareMode =="never")
		func = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_NEVER;
	else if (compareMode =="notEqual")
		func = DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_NOT_EQUAL;
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"compareMode");
		return;
	}
	DEPTHSTENCIL_OP pass;
	if (actionOnBothPass =="keep")
		pass = DEPTHSTENCIL_OP::DEPTHSTENCIL_KEEP;
	else if (actionOnBothPass =="decrementSaturate")
		pass = DEPTHSTENCIL_OP::DEPTHSTENCIL_DECR;
	else if (actionOnBothPass =="decrementWrap")
		pass = DEPTHSTENCIL_OP::DEPTHSTENCIL_DECR_WRAP;
	else if (actionOnBothPass =="incrementSaturate")
		pass = DEPTHSTENCIL_OP::DEPTHSTENCIL_INCR;
	else if (actionOnBothPass =="incrementWrap")
		pass = DEPTHSTENCIL_OP::DEPTHSTENCIL_INCR_WRAP;
	else if (actionOnBothPass =="invert")
		pass = DEPTHSTENCIL_OP::DEPTHSTENCIL_INVERT;
	else if (actionOnBothPass =="set")
		pass = DEPTHSTENCIL_OP::DEPTHSTENCIL_REPLACE;
	else if (actionOnBothPass =="zero")
		pass = DEPTHSTENCIL_OP::DEPTHSTENCIL_ZERO;
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"actionOnBothPass");
		return;
	}
	DEPTHSTENCIL_OP depthfail;
	if (actionOnDepthFail =="keep")
		depthfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_KEEP;
	else if (actionOnDepthFail =="decrementSaturate")
		depthfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_DECR;
	else if (actionOnDepthFail =="decrementWrap")
		depthfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_DECR_WRAP;
	else if (actionOnDepthFail =="incrementSaturate")
		depthfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_INCR;
	else if (actionOnDepthFail =="incrementWrap")
		depthfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_INCR_WRAP;
	else if (actionOnDepthFail =="invert")
		depthfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_INVERT;
	else if (actionOnDepthFail =="set")
		depthfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_REPLACE;
	else if (actionOnDepthFail =="zero")
		depthfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_ZERO;
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"actionOnDepthFail");
		return;
	}
	DEPTHSTENCIL_OP depthpassstencilfail;
	if (actionOnDepthPassStencilFail =="keep")
		depthpassstencilfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_KEEP;
	else if (actionOnDepthPassStencilFail =="decrementSaturate")
		depthpassstencilfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_DECR;
	else if (actionOnDepthPassStencilFail =="decrementWrap")
		depthpassstencilfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_DECR_WRAP;
	else if (actionOnDepthPassStencilFail =="incrementSaturate")
		depthpassstencilfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_INCR;
	else if (actionOnDepthPassStencilFail =="incrementWrap")
		depthpassstencilfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_INCR_WRAP;
	else if (actionOnDepthPassStencilFail =="invert")
		depthpassstencilfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_INVERT;
	else if (actionOnDepthPassStencilFail =="set")
		depthpassstencilfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_REPLACE;
	else if (actionOnDepthPassStencilFail =="zero")
		depthpassstencilfail = DEPTHSTENCIL_OP::DEPTHSTENCIL_ZERO;
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"actionOnDepthPassStencilFail");
		return;
	}
	
	th->rendermutex.lock();
	th->currentstencilfunction=func;
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETSTENCILACTIONS;
	action.udata1 = face;
	action.udata2 = func;
	action.udata3 = (pass<<16)|(depthfail<<8)|depthpassstencilfail;
	th->actions[th->currentactionvector].push_back(action);
	th->rendermutex.unlock();
	
}
ASFUNCTIONBODY_ATOM(Context3D,setStencilReferenceValue)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	uint32_t referenceValue;
	uint32_t readMask;
	uint32_t writeMask;
	ARG_CHECK(ARG_UNPACK(referenceValue)(readMask,255)(writeMask,255));
	renderaction action;
	th->rendermutex.lock();
	th->currentstencilref = referenceValue&0xff;
	th->currentstencilmask = readMask&0xff;
	action.action = RENDER_ACTION::RENDER_SETSTENCILREFERENCEVALUE;
	action.udata1 = (referenceValue&0xff<<16)|(readMask&0xff<<8)|(writeMask&0xff);
	action.udata2 = th->currentstencilfunction;
	th->actions[th->currentactionvector].push_back(action);
	th->rendermutex.unlock();
}

ASFUNCTIONBODY_ATOM(Context3D,setTextureAt)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	uint32_t sampler;
	_NR<TextureBase> texture;
	ARG_CHECK(ARG_UNPACK(sampler)(texture));
	if (sampler >= CONTEXT3D_SAMPLER_COUNT)
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	th->rendermutex.lock();
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETTEXTUREAT;
	action.udata1 = sampler;
	if (!texture.isNull())
	{
		if (texture->textureID == UINT32_MAX)
		{
			th->addAction(RENDER_ACTION::RENDER_GENERATETEXTURE,texture.getPtr());
		}
		action.udata2 = texture->textureID;
	}
	else
		action.udata2 = UINT32_MAX;
	action.udata3 = texture.isNull() ? 1 : 0;
	th->actions[th->currentactionvector].push_back(action);
	th->rendermutex.unlock();
}

ASFUNCTIONBODY_ATOM(Context3D,setVertexBufferAt)
{
	Context3D* th = asAtomHandler::as<Context3D>(obj);
	uint32_t index;
	_NR<VertexBuffer3D> buffer;
	uint32_t bufferOffset;
	tiny_string format;
	ARG_CHECK(ARG_UNPACK(index)(buffer)(bufferOffset,0)(format,"float4"));
	if (index > 7)
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	if (buffer.isNull())
		return;
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETVERTEXBUFFER;
	action.udata1 = index<<4 | buffer->data32PerVertex<<8;
	action.udata2 = buffer->bufferID;
	action.udata3 = bufferOffset;
	if (format == "bytes4")
		action.udata1 |= VERTEXBUFFER_FORMAT::BYTES_4;
	else if (format == "float1")
		action.udata1 |= VERTEXBUFFER_FORMAT::FLOAT_1;
	else if (format == "float2")
		action.udata1 |= VERTEXBUFFER_FORMAT::FLOAT_2;
	else if (format == "float3")
		action.udata1 |= VERTEXBUFFER_FORMAT::FLOAT_3;
	else if (format == "float4")
		action.udata1 |= VERTEXBUFFER_FORMAT::FLOAT_4;
	else
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setVertexBufferAt with format "<<format);
	if (buffer->bufferID==UINT32_MAX)
	{
		// VertexBuffer was created during this loop, so it doesn't have a bufferID yet
		th->rendermutex.lock();
		action.dataobject = buffer;
		th->actions[th->currentactionvector].push_back(action);
		th->rendermutex.unlock();
	}
	else
		th->actions[th->currentactionvector].push_back(action);
}


void Context3DBlendFactor::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("DESTINATION_ALPHA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"destinationAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DESTINATION_COLOR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"destinationColor"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"one"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE_MINUS_DESTINATION_ALPHA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"oneMinusDestinationAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE_MINUS_DESTINATION_COLOR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"oneMinusDestinationColor"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE_MINUS_SOURCE_ALPHA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"oneMinusSourceAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE_MINUS_SOURCE_COLOR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"oneMinusSourceColor"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SOURCE_ALPHA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"sourceAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SOURCE_COLOR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"sourceColor"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ZERO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"zero"),CONSTANT_TRAIT);
}

void Context3DBufferUsage::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("DYNAMIC_DRAW",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"dynamicDraw"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STATIC_DRAW",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"staticDraw"),CONSTANT_TRAIT);
}

void Context3DClearMask::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALL",nsNameAndKind(),asAtomHandler::fromUInt(CLEARMASK::COLOR | CLEARMASK::DEPTH | CLEARMASK::STENCIL),CONSTANT_TRAIT);
	c->setVariableAtomByQName("COLOR",nsNameAndKind(),asAtomHandler::fromUInt(CLEARMASK::COLOR),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DEPTH",nsNameAndKind(),asAtomHandler::fromUInt(CLEARMASK::DEPTH),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STENCIL",nsNameAndKind(),asAtomHandler::fromUInt(CLEARMASK::STENCIL),CONSTANT_TRAIT);
}

void Context3DCompareMode::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALWAYS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"always"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EQUAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"equal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GREATER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"greater"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GREATER_EQUAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"greaterEqual"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LESS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"less"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LESS_EQUAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"lessEqual"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NEVER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"never"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NOT_EQUAL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"notEqual"),CONSTANT_TRAIT);
}

void Context3DMipFilter::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("MIPLINEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"miplinear"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MIPNEAREST",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mipnearest"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MIPNONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mipnone"),CONSTANT_TRAIT);
}

void Context3DProfile::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BASELINE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"baseline"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BASELINE_CONSTRAINED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"baselineConstrained"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BASELINE_EXTENDED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"baselineExtended"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standard"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD_CONSTRAINED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardConstrained"),CONSTANT_TRAIT);
}

void Context3DProgramType::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("FRAGMENT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fragment"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VERTEX",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"vertex"),CONSTANT_TRAIT);
}

void Context3DRenderMode::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SOFTWARE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"software"),CONSTANT_TRAIT);
}

void Context3DStencilAction::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("DECREMENT_SATURATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"decrementSaturate"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DECREMENT_WRAP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"decrementWrap"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INCREMENT_SATURATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"incrementSaturate"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INCREMENT_WRAP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"incrementWrap"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVERT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invert"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEEP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"keep"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SET",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"set"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ZERO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"zero"),CONSTANT_TRAIT);
}

void Context3DTextureFilter::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ANISOTROPIC16X",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"anisotropic16x"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ANISOTROPIC2X",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"anisotropic2x"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ANISOTROPIC4X",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"anisotropic4x"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ANISOTROPIC8X",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"anisotropic8x"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"linear"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NEAREST",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"nearest"),CONSTANT_TRAIT);
}

void Context3DTextureFormat::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BGRA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bgra"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BGRA_PACKED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bgraPacked4444"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BGR_PACKED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bgrPacked565"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("COMPRESSED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"compressed"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("COMPRESSED_ALPHA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"compressedAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RGBA_HALF_FLOAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rgbaHalfFloat"),CONSTANT_TRAIT);
}

void Context3DTriangleFace::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BACK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"back"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FRONT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"front"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FRONT_AND_BACK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"frontAndBack"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
}

void Context3DVertexBufferFormat::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BYTES_4",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bytes4"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT_1",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"float1"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT_2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"float2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT_3",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"float3"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT_4",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"float4"),CONSTANT_TRAIT);
}

void Context3DWrapMode::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("CLAMP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"clamp"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CLAMP_U_REPEAT_V",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"clamp_u_repeat_v"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REPEAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"repeat"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REPEAT_U_CLAMP_V",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"repeat_u_clamp_v"),CONSTANT_TRAIT);
}

IndexBuffer3D::IndexBuffer3D(ASWorker* wrk,Class_base* c, Context3D* ctx,int numVertices, tiny_string _bufferUsage)
	:ASObject(wrk,c,T_OBJECT,SUBTYPE_INDEXBUFFER3D),context(ctx),bufferID(UINT32_MAX),bufferUsage(_bufferUsage),disposed(false)
{
	data.resize(numVertices);
}

bool IndexBuffer3D::destruct()
{
	if (context && bufferID != UINT32_MAX)
	{
		renderaction action;
		action.action =RENDER_ACTION::RENDER_DELETEBUFFER;
		action.udata1 = bufferID;
		context->addAction(action);
	}
	return ASObject::destruct();
}

void IndexBuffer3D::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("dispose","",c->getSystemState()->getBuiltinFunction(dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",c->getSystemState()->getBuiltinFunction(uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromVector","",c->getSystemState()->getBuiltinFunction(uploadFromVector),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(IndexBuffer3D,dispose)
{
	IndexBuffer3D* th = asAtomHandler::as<IndexBuffer3D>(obj);
	th->disposed=true;
	if (th->context && th->bufferID != UINT32_MAX)
		th->context->addAction(RENDER_ACTION::RENDER_DELETEINDEXBUFFER,th);
}
ASFUNCTIONBODY_ATOM(IndexBuffer3D,uploadFromByteArray)
{
	IndexBuffer3D* th = asAtomHandler::as<IndexBuffer3D>(obj);
	if (th->disposed)
	{
		LOG(LOG_ERROR,"IndexBuffer3D.uploadFromByteArray from disposed buffer");
		return;
	}
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	uint32_t startOffset;
	uint32_t count;
	ARG_CHECK(ARG_UNPACK(data)(byteArrayOffset)(startOffset)(count));
	if (data.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError);
		return;
	}
	if (data->getLength() < byteArrayOffset+count*2)
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	th->context->rendermutex.lock();
	if (th->data.size() < count+startOffset)
		th->data.resize(count+startOffset);
	uint32_t origpos = data->getPosition();
	data->setPosition(byteArrayOffset);
	for (uint32_t i = 0; i< count; i++)
	{
		uint16_t d;
		if (data->readShort(d))
			th->data[startOffset+i] = d;
	}
	th->context->addAction(RENDER_ACTION::RENDER_UPLOADINDEXBUFFER,th);
	th->context->rendermutex.unlock();
	data->setPosition(origpos);
}
ASFUNCTIONBODY_ATOM(IndexBuffer3D,uploadFromVector)
{
	IndexBuffer3D* th = asAtomHandler::as<IndexBuffer3D>(obj);
	if (th->disposed)
	{
		LOG(LOG_ERROR,"IndexBuffer3D.uploadFromVector from disposed buffer");
		return;
	}
	_NR<Vector> data;
	uint32_t startOffset;
	uint32_t count;
	ARG_CHECK(ARG_UNPACK(data)(startOffset)(count));
	th->context->rendermutex.lock();
	if (th->data.size() < count+startOffset)
		th->data.resize(count+startOffset);
	for (uint32_t i = 0; i< count; i++)
	{
		asAtom a = data->at(i);
		th->data[startOffset+i] = asAtomHandler::toUInt(a);
	}
	th->context->addAction(RENDER_ACTION::RENDER_UPLOADINDEXBUFFER,th);
	th->context->rendermutex.unlock();
}

void Program3D::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("dispose","",c->getSystemState()->getBuiltinFunction(dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("upload","",c->getSystemState()->getBuiltinFunction(upload),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(Program3D,dispose)
{
	Program3D* th = asAtomHandler::as<Program3D>(obj);
	th->context->addAction(RENDER_ACTION::RENDER_DELETEPROGRAM,th);
	th->disposed=true;
}
ASFUNCTIONBODY_ATOM(Program3D,upload)
{
	Program3D* th = asAtomHandler::as<Program3D>(obj);
	if (th->disposed)
	{
		LOG(LOG_ERROR,"Program3D.upload from disposed program");
		return;
	}
	_NR<ByteArray> vertexProgram;
	_NR<ByteArray> fragmentProgram;
	ARG_CHECK(ARG_UNPACK(vertexProgram)(fragmentProgram));
	th->context->rendermutex.lock();
	th->samplerState.clear();
	RegisterMap vertexregistermap; // this is needed to keep track of the registers when creating the fragment program
	if (!vertexProgram.isNull())
	{
		th->vertexprogram = AGALtoGLSL(vertexProgram.getPtr(),true,th->samplerState,th->vertexregistermap,th->vertexattributes,vertexregistermap);
		// LOG(LOG_INFO,"vertex shader:"<<th<<"\n"<<th->vertexprogram);
	}
	if (!fragmentProgram.isNull())
	{
		th->fragmentprogram = AGALtoGLSL(fragmentProgram.getPtr(),false,th->samplerState,th->fragmentregistermap,th->fragmentattributes,vertexregistermap);
		// LOG(LOG_INFO,"fragment shader:"<<th<<"\n"<<th->fragmentprogram);
	}
	th->context->addAction(RENDER_ACTION::RENDER_UPLOADPROGRAM,th);
	th->context->rendermutex.unlock();
}

VertexBuffer3D::VertexBuffer3D(ASWorker* wrk,Class_base *c, Context3D *ctx, int _numVertices, int32_t _data32PerVertex, tiny_string _bufferUsage)
	:ASObject(wrk,c,T_OBJECT,SUBTYPE_VERTEXBUFFER3D),context(ctx),numVertices(_numVertices),data32PerVertex(_data32PerVertex),bufferUsage(_bufferUsage),disposed(false),bufferID(UINT32_MAX)
{
	data.resize(numVertices*data32PerVertex);
}

bool VertexBuffer3D::destruct()
{
	if (context && bufferID != UINT32_MAX)
	{
		renderaction action;
		action.action =RENDER_ACTION::RENDER_DELETEBUFFER;
		action.udata1 = bufferID;
		context->addAction(action);
	}
	return ASObject::destruct();
}

void VertexBuffer3D::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("dispose","",c->getSystemState()->getBuiltinFunction(dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",c->getSystemState()->getBuiltinFunction(uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromVector","",c->getSystemState()->getBuiltinFunction(uploadFromVector),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(VertexBuffer3D,dispose)
{
	VertexBuffer3D* th = asAtomHandler::as<VertexBuffer3D>(obj);
	th->data.clear();
	th->disposed=true;
	if (th->context && th->bufferID != UINT32_MAX)
		th->context->addAction(RENDER_ACTION::RENDER_DELETEVERTEXBUFFER,th);
}
ASFUNCTIONBODY_ATOM(VertexBuffer3D,uploadFromByteArray)
{
	VertexBuffer3D* th = asAtomHandler::as<VertexBuffer3D>(obj);
	if (th->disposed)
	{
		LOG(LOG_ERROR,"VertexBuffer3D.uploadFromByteArray from disposed buffer");
		return;
	}
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	uint32_t startVertex;
	uint32_t numVertices;
	ARG_CHECK(ARG_UNPACK(data)(byteArrayOffset)(startVertex)(numVertices));
	if (data.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError);
		return;
	}
	if (data->getLength() < (byteArrayOffset+numVertices*th->data32PerVertex*4))
	{
		createError<RangeError>(wrk,kParamRangeError);
		return;
	}
	uint32_t origpos = data->getPosition();
	data->setPosition(byteArrayOffset);
	th->context->rendermutex.lock();
	if (th->data.size() < (numVertices+startVertex)* th->data32PerVertex)
		th->data.resize((numVertices+startVertex)* th->data32PerVertex);
	for (uint32_t i = 0; i< numVertices* th->data32PerVertex; i++)
	{
		union {
			float f;
			uint32_t u;
		} d;
		if (data->readUnsignedInt(d.u))
			th->data[startVertex*th->data32PerVertex+i] = d.f;
	}
	th->context->addAction(RENDER_ACTION::RENDER_UPLOADVERTEXBUFFER,th);
	th->context->rendermutex.unlock();
	data->setPosition(origpos);
}
ASFUNCTIONBODY_ATOM(VertexBuffer3D,uploadFromVector)
{
	VertexBuffer3D* th = asAtomHandler::as<VertexBuffer3D>(obj);
	if (th->disposed)
	{
		LOG(LOG_ERROR,"VertexBuffer3D.uploadFromVector from disposed buffer");
		return;
	}
	_NR<Vector> data;
	uint32_t startVertex;
	uint32_t numVertices;
	ARG_CHECK(ARG_UNPACK(data)(startVertex)(numVertices));
	th->context->rendermutex.lock();
	if (th->data.size() < (numVertices+startVertex)* th->data32PerVertex)
		th->data.resize((numVertices+startVertex)* th->data32PerVertex);
	for (uint32_t i = 0; i< numVertices* th->data32PerVertex; i++)
	{
		asAtom a = data->at(i);
		th->data[startVertex*th->data32PerVertex+i] = asAtomHandler::toNumber(a);
	}
	th->context->addAction(RENDER_ACTION::RENDER_UPLOADVERTEXBUFFER,th);
	th->context->rendermutex.unlock();
}
