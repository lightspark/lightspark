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
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "backends/rendering.h"
#include "backends/rendering_context.h"
#include "scripting/flash/display3d/agalconverter.h"

namespace lightspark
{
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
			if ((action.udata2 & CLEARMASK::DEPTH) != 0)
				engineData->exec_glClearDepthf(action.fdata[4]);
			if ((action.udata2 & CLEARMASK::STENCIL) != 0)
				engineData->exec_glClearStencil (action.udata1);
			if (renderingToTexture && !enableDepthAndStencil)
				engineData->exec_glClear(CLEARMASK::COLOR);
			else
				engineData->exec_glClear((CLEARMASK)action.udata2);
			engineData->exec_glBlendFunc(BLEND_ONE,BLEND_ZERO);
			delete[] action.fdata;
			break;
		case RENDER_CONFIGUREBACKBUFFER:
			//action.udata1 = enableDepthAndStencil
			//LOG(LOG_INFO,"RENDER_CONFIGUREBACKBUFFER:"<<action.udata1);
			if (action.udata1)
			{
				engineData->exec_glEnable_GL_STENCIL_TEST();
				engineData->exec_glEnable_GL_DEPTH_TEST();
			}
			else
			{
				engineData->exec_glDisable_GL_STENCIL_TEST();
				engineData->exec_glDisable_GL_DEPTH_TEST();
			}
			break;
		case RENDER_SETPROGRAM:
		{
			bool needslink=false;
			char str[1024];
			int a;
			int stat;
			Program3D* p = action.dataobject->as<Program3D>();
			//LOG(LOG_INFO,"setProgram:"<<p<<" "<<p->gpu_program);
			uint32_t f= UINT32_MAX;
			uint32_t g= UINT32_MAX;
			if (!p->vertexprogram.empty())
			{
				needslink= true;
				g = engineData->exec_glCreateShader_GL_VERTEX_SHADER();
				const char* buf = p->vertexprogram.raw_buf();
				engineData->exec_glShaderSource(g, 1, &buf,NULL);
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
				engineData->exec_glShaderSource(f, 1, &buf,NULL);
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
			if (p->gpu_program == UINT32_MAX)
			{
				needslink=true;
				p->gpu_program = engineData->exec_glCreateProgram();
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
			engineData->exec_glUseProgram(p->gpu_program);
			currentprogram = p;
			if (needslink)
			{
				engineData->exec_glGetProgramInfoLog(p->gpu_program,1024,&a,str);
				engineData->exec_glGetProgramiv_GL_LINK_STATUS(p->gpu_program,&stat);
				if(!stat)
				{
					LOG(LOG_INFO,_("program link ") << str);
					throw RunTimeException("Could not link program");
				}
			}
			p->vertexprogram = "";
			p->fragmentprogram = "";
			break;
		}
		case RENDER_RENDERTOBACKBUFFER:
			engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(0);
			engineData->exec_glDrawBuffer_GL_BACK();
			engineData->exec_glViewport(0,0,this->backBufferWidth,this->backBufferHeight);
			if (enableDepthAndStencil)
			{
				engineData->exec_glEnable_GL_DEPTH_TEST();
				engineData->exec_glEnable_GL_STENCIL_TEST();
			}
			else
			{
				engineData->exec_glDisable_GL_DEPTH_TEST();
				engineData->exec_glDisable_GL_STENCIL_TEST();
			}
			renderingToTexture = false;
			break;
		case RENDER_TOTEXTURE:
		{
			//action.dataobject = TextureBase
			//action.udata1 = enableDepthAndStencil
			if (textureframebuffer == UINT32_MAX)
				textureframebuffer = engineData->exec_glGenFramebuffer();
			engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(textureframebuffer);
			TextureBase* tex = action.dataobject->as<TextureBase>();
			if (tex->textureID == UINT32_MAX || tex->needrefresh)
			{
				engineData->exec_glEnable_GL_TEXTURE_2D();
				loadTexture(tex);
			}
			engineData->exec_glBindTexture_GL_TEXTURE_2D(tex->textureID);
			engineData->exec_glFramebufferTexture2D_GL_FRAMEBUFFER(tex->textureID);
			if (action.udata1)//enableDepthAndStencil
			{
				if (depthRenderBuffer == UINT32_MAX)
					depthRenderBuffer = engineData->exec_glGenRenderbuffer();
				
				if (engineData->supportPackedDepthStencil)
				{
					engineData->exec_glBindRenderbuffer(depthRenderBuffer);
					engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_STENCIL(tex->width,tex->height);
					engineData->exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_DEPTH_STENCIL_ATTACHMENT(depthRenderBuffer);
				}
				else
				{
					if (stencilRenderBuffer == UINT32_MAX)
						stencilRenderBuffer = engineData->exec_glGenRenderbuffer();
					engineData->exec_glBindRenderbuffer(depthRenderBuffer);
					engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_DEPTH_COMPONENT16(tex->width,tex->height);
					engineData->exec_glBindRenderbuffer(stencilRenderBuffer);
					engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_STENCIL_INDEX8(tex->width,tex->height);
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
			engineData->exec_glViewport(0,0,tex->width,tex->height);
			renderingToTexture = true;
			break;
		}
		case RENDER_DELETEPROGRAM:
		{
			Program3D* p = action.dataobject->as<Program3D>();
			engineData->exec_glUseProgram(0);
			engineData->exec_glDeleteProgram(p->gpu_program);
			p->gpu_program = UINT32_MAX;
			break;
		}
		case RENDER_SETVERTEXBUFFER:
		{
			//action.dataobject = VertexBuffer3D
			//action.udata1 = index
			//action.udata2 = bufferOffset
			//action.udata3 = format
			if (!action.dataobject.isNull())
			{
				VertexBuffer3D* buffer = action.dataobject->as<VertexBuffer3D>();
				assert_and_throw(buffer->numVertices*buffer->data32PerVertex <= buffer->data.size());
				if (buffer->bufferID == UINT32_MAX)
					engineData->exec_glGenBuffers(1,&(buffer->bufferID));
				if (buffer->upload_needed)
				{
					engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(buffer->bufferID);
					if (buffer->bufferUsage == "dynamicDraw")
						engineData->exec_glBufferData_GL_ARRAY_BUFFER_GL_DYNAMIC_DRAW(buffer->numVertices*buffer->data32PerVertex*sizeof(float),buffer->data.data());
					else
						engineData->exec_glBufferData_GL_ARRAY_BUFFER_GL_STATIC_DRAW(buffer->numVertices*buffer->data32PerVertex*sizeof(float),buffer->data.data());
					engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(0);
					buffer->upload_needed=false;
				}
				attribs[action.udata1].bufferID = buffer->bufferID;
				attribs[action.udata1].data32PerVertex = buffer->data32PerVertex;
				attribs[action.udata1].offset = action.udata2;
				attribs[action.udata1].format = (VERTEXBUFFER_FORMAT)action.udata3;
			}
			break;
		}
		case RENDER_DRAWTRIANGLES:
		{
			//action.dataobject = IndexBuffer3D
			//action.udata1 = firstIndex
			//action.udata2 = numTriangles
			if (currentprogram)
			{
				setSamplers(engineData);
				setRegisters(engineData,currentprogram->vertexregistermap,vertexConstants,true);
				setRegisters(engineData,currentprogram->fragmentregistermap,fragmentConstants,false);
				setAttribs(engineData);
			}

			IndexBuffer3D* buffer = action.dataobject->as<IndexBuffer3D>();
			uint32_t count = (action.udata2 == UINT32_MAX) ? buffer->data.size() : (action.udata2 * 3);
			assert_and_throw(count+ action.udata1 <= buffer->data.size());
			if (buffer->bufferID == UINT32_MAX)
			{
				engineData->exec_glGenBuffers(1,&(buffer->bufferID));
				engineData->exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(buffer->bufferID);
				if (buffer->bufferUsage == "dynamicDraw")
					engineData->exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_DYNAMIC_DRAW(buffer->data.size()*sizeof(uint16_t),buffer->data.data());
				else
					engineData->exec_glBufferData_GL_ELEMENT_ARRAY_BUFFER_GL_STATIC_DRAW(buffer->data.size()*sizeof(uint16_t),buffer->data.data());
			}
			else
				engineData->exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(buffer->bufferID);
			engineData->exec_glDrawElements_GL_TRIANGLES_GL_UNSIGNED_SHORT(count,(void*)(action.udata1*sizeof(uint16_t)));
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
			delete[] action.fdata;
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
			delete[] action.fdata;
			break;
		}
		case RENDER_SETTEXTUREAT:
		{
			//action.dataobject = TextureBase
			//action.udata1 = sampler
			if (action.dataobject.isNull())
			{
				//LOG(LOG_INFO,"RENDER_SETTEXTUREAT remove:"<<action.udata1);
//					engineData->exec_glActiveTexture_GL_TEXTURE0(action.udata1);
//					engineData->exec_glBindTexture_GL_TEXTURE_2D(0);
			}
			else
			{
				TextureBase* tex = action.dataobject->as<TextureBase>();
				//LOG(LOG_INFO,"RENDER_SETTEXTUREAT:"<<tex->textureID<<" "<<action.udata1<<" "<<tex->needrefresh);
				if (tex->textureID == UINT32_MAX || tex->needrefresh)
				{
					engineData->exec_glEnable_GL_TEXTURE_2D();
					engineData->exec_glActiveTexture_GL_TEXTURE0(action.udata1);
					loadTexture(tex);
				}
				samplers[action.udata1] = tex->textureID;
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
			engineData->exec_glDepthFunc((DEPTH_FUNCTION)action.udata2);
			break;
		}
		case RENDER_SETCULLING:
		{
			//action.udata1 = mode
			engineData->exec_glCullFace((TRIANGLE_FACE)action.udata1);
			break;
		}
		case RENDER_LOADTEXTURE:
			loadTexture(action.dataobject->as<TextureBase>());
			break;
	}
}

void Context3D::setRegisters(EngineData* engineData,std::map<uint32_t,uint32_t>& registermap,constantregister* constants, bool isVertex)
{
	auto it = registermap.begin();
	while (it != registermap.end())
	{
		RegisterUsage usage = (RegisterUsage)it->second;
		switch (usage)
		{
			case RegisterUsage::VECTOR_4:
			{
				float* data = constants[it->first].data;
				char buf[100];
				sprintf(buf,"%cc%d",isVertex?'v':'f',it->first);
				uint32_t loc = engineData->exec_glGetUniformLocation(currentprogram->gpu_program,buf);
				if (loc != UINT32_MAX)
					engineData->exec_glUniform4fv(loc,1, data);
				else
					LOG(LOG_ERROR,"setRegisters vector no location:"<<buf);
				break;
			}
			case RegisterUsage::MATRIX_4_4:
			{
				char buf[100];
				sprintf(buf,"%cc%d",isVertex?'v':'f',it->first);
				uint32_t loc = engineData->exec_glGetUniformLocation(currentprogram->gpu_program,buf);
				float data2[4*4];
				for (uint32_t i =0; i < 4; i++)
				{
					float* data = constants[it->first+i].data;
					data2[i*4] = data[0];
					data2[i*4+1] = data[1];
					data2[i*4+2] = data[2];
					data2[i*4+3] = data[3];
				}
				if (loc != UINT32_MAX)
					engineData->exec_glUniformMatrix4fv(loc,1,false, data2);
				else
					LOG(LOG_ERROR,"setRegisters no location:"<<buf);
				break;
			}
			default:
				LOG(LOG_NOT_IMPLEMENTED,"Context3D.setRegisters: RegisterUsage:"<<(uint32_t)usage<<" "<<it->first<<" "<<it->second);
				break;
		}
		it++;
	}
}

void Context3D::setAttribs(EngineData *engineData)
{
	for (uint32_t i = 0; i < CONTEXT3D_ATTRIBUTE_COUNT; i++)
	{
		if (attribs[i].bufferID != UINT32_MAX)
		{
			char buf[100];
			sprintf(buf,"va%d",i);
			uint32_t pos = engineData->exec_glGetAttribLocation(currentprogram->gpu_program,buf);
			if (pos != UINT32_MAX)
			{
				engineData->exec_glEnableVertexAttribArray(pos);
				engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(attribs[i].bufferID);
				engineData->exec_glVertexAttribPointer(pos, attribs[i].data32PerVertex*sizeof(float), (const void*)(size_t)(attribs[i].offset*4),attribs[i].format);
			}
		}
	}
}

void Context3D::setSamplers(EngineData *engineData)
{
	for (uint32_t i = 0; i < currentprogram->samplerState.size();i++)
	{
		char buf[100];
		sprintf(buf,"sampler%d",currentprogram->samplerState[i]);
		uint32_t sampid = engineData->exec_glGetUniformLocation(currentprogram->gpu_program,buf);
		if (sampid != UINT32_MAX && samplers[currentprogram->samplerState[i]] != UINT32_MAX)
		{
			engineData->exec_glActiveTexture_GL_TEXTURE0(currentprogram->samplerState[i]);
			engineData->exec_glBindTexture_GL_TEXTURE_2D(samplers[currentprogram->samplerState[i]]);
			engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
			engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
			engineData->exec_glUniform1i(sampid, currentprogram->samplerState[i]);
		}
		else
			LOG(LOG_ERROR,"sampler not found in program:"<<buf);
	}
}
bool Context3D::renderImpl(RenderContext &ctxt)
{
	Locker l(rendermutex);
	if (actions[1-currentactionvector].size() == 0)
		return false;
	EngineData* engineData = getSystemState()->getEngineData();
	if (currentprogram)
		engineData->exec_glUseProgram(currentprogram->gpu_program);
	
	for (uint32_t i = 0; i < actions[1-currentactionvector].size(); i++)
	{
		renderaction& action = actions[1-currentactionvector][i];
		handleRenderAction(engineData,action);
	}
	engineData->exec_glDisable_GL_DEPTH_TEST();
	engineData->exec_glBindBuffer_GL_ELEMENT_ARRAY_BUFFER(0);
	engineData->exec_glBindBuffer_GL_ARRAY_BUFFER(0);
	engineData->exec_glUseProgram(0);
	
	if (renderingToTexture)
	{
		engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(0);
		engineData->exec_glDrawBuffer_GL_BACK();
		engineData->exec_glViewport(0,0,this->backBufferWidth,this->backBufferHeight);
		if (enableDepthAndStencil)
		{
			engineData->exec_glEnable_GL_DEPTH_TEST();
			engineData->exec_glEnable_GL_STENCIL_TEST();
		}
		else
		{
			engineData->exec_glDisable_GL_DEPTH_TEST();
			engineData->exec_glDisable_GL_STENCIL_TEST();
		}
		renderingToTexture = false;
	}
	((GLRenderContext&)ctxt).handleGLErrors();
	actions[1-currentactionvector].clear();
	return true;
}

void Context3D::loadTexture(TextureBase *tex)
{
	EngineData* engineData = getSystemState()->getEngineData();
	if (tex->textureID == UINT32_MAX || tex->needrefresh)
	{
		if (tex->textureID == UINT32_MAX)
			engineData->exec_glGenTextures(1, &(tex->textureID));
		engineData->exec_glBindTexture_GL_TEXTURE_2D(tex->textureID);
		engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
		engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
		if (tex->bitmaparray.size() == 0)
			engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, tex->width, tex->height, 0, NULL);
		else
		{
			for (uint32_t i = 0; i < tex->bitmaparray.size(); i++)
			{
				if (tex->bitmaparray[i].size() > 0)
					engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(i, tex->width>>i, tex->height>>i, 0, tex->bitmaparray[i].data());
				else
					engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(i, tex->width>>i, tex->height>>i, 0, NULL);
			}
		}
		tex->needrefresh = false;
	}
}

Context3D::Context3D(Class_base *c):EventDispatcher(c),samplers{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX},currentactionvector(0)
  ,textureframebuffer(UINT32_MAX),depthRenderBuffer(UINT32_MAX),stencilRenderBuffer(UINT32_MAX),currentprogram(NULL)
  ,depthMask(true),renderingToTexture(false),enableDepthAndStencil(true),backBufferHeight(0),backBufferWidth(0),enableErrorChecking(false)
  ,maxBackBufferHeight(16384),maxBackBufferWidth(16384)
{
	subtype = SUBTYPE_CONTEXT3D;
	memset(vertexConstants,0,4 * CONTEXT3D_PROGRAM_REGISTERS * sizeof(float));
	memset(fragmentConstants,0,4 * CONTEXT3D_PROGRAM_REGISTERS * sizeof(float));
	driverInfo = getSystemState()->getEngineData()->driverInfoString;
}

void Context3D::addAction(RENDER_ACTION type, ASObject *dataobject)
{
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
	actions[currentactionvector].push_back(action);
}

void Context3D::sinit(lightspark::Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, EventDispatcher, CLASS_SEALED|CLASS_FINAL);

	REGISTER_GETTER(c,backBufferHeight);
	REGISTER_GETTER(c,backBufferWidth);
	REGISTER_GETTER(c,driverInfo);
	REGISTER_GETTER_SETTER(c,enableErrorChecking);
	REGISTER_GETTER_SETTER(c,maxBackBufferHeight);
	REGISTER_GETTER_SETTER(c,maxBackBufferWidth);
	REGISTER_GETTER(c,profile);
	c->setDeclaredMethodByQName("supportsVideoTexture","",Class<IFunction>::getFunction(c->getSystemState(),supportsVideoTexture),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("configureBackBuffer","",Class<IFunction>::getFunction(c->getSystemState(),configureBackBuffer),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createIndexBuffer","",Class<IFunction>::getFunction(c->getSystemState(),createIndexBuffer),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createProgram","",Class<IFunction>::getFunction(c->getSystemState(),createProgram),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createCubeTexture","",Class<IFunction>::getFunction(c->getSystemState(),createCubeTexture),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createTexture","",Class<IFunction>::getFunction(c->getSystemState(),createTexture),NORMAL_METHOD,true);
	if(c->getSystemState()->flashMode==SystemState::AIR)
		c->setDeclaredMethodByQName("createRectangleTexture","",Class<IFunction>::getFunction(c->getSystemState(),createRectangleTexture),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createVertexBuffer","",Class<IFunction>::getFunction(c->getSystemState(),createVertexBuffer),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createVideoTexture","",Class<IFunction>::getFunction(c->getSystemState(),createVideoTexture),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(c->getSystemState(),clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawToBitmapData","",Class<IFunction>::getFunction(c->getSystemState(),drawToBitmapData),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("drawTriangles","",Class<IFunction>::getFunction(c->getSystemState(),drawTriangles),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setBlendFactors","",Class<IFunction>::getFunction(c->getSystemState(),setBlendFactors),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setColorMask","",Class<IFunction>::getFunction(c->getSystemState(),setColorMask),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setCulling","",Class<IFunction>::getFunction(c->getSystemState(),setCulling),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setDepthTest","",Class<IFunction>::getFunction(c->getSystemState(),setDepthTest),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setProgram","",Class<IFunction>::getFunction(c->getSystemState(),setProgram),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setProgramConstantsFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),setProgramConstantsFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setProgramConstantsFromMatrix","",Class<IFunction>::getFunction(c->getSystemState(),setProgramConstantsFromMatrix),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setProgramConstantsFromVector","",Class<IFunction>::getFunction(c->getSystemState(),setProgramConstantsFromVector),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setRenderToBackBuffer","",Class<IFunction>::getFunction(c->getSystemState(),setRenderToBackBuffer),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setRenderToTexture","",Class<IFunction>::getFunction(c->getSystemState(),setRenderToTexture),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSamplerStateAt","",Class<IFunction>::getFunction(c->getSystemState(),setSamplerStateAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setScissorRectangle","",Class<IFunction>::getFunction(c->getSystemState(),setScissorRectangle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("present","",Class<IFunction>::getFunction(c->getSystemState(),present),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setStencilActions","",Class<IFunction>::getFunction(c->getSystemState(),setStencilActions),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setStencilReferenceValue","",Class<IFunction>::getFunction(c->getSystemState(),setStencilReferenceValue),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTextureAt","",Class<IFunction>::getFunction(c->getSystemState(),setTextureAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setVertexBufferAt","",Class<IFunction>::getFunction(c->getSystemState(),setVertexBufferAt),NORMAL_METHOD,true);
}



ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Context3D,backBufferHeight);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Context3D,backBufferWidth);
ASFUNCTIONBODY_GETTER(Context3D,driverInfo);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Context3D,enableErrorChecking);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Context3D,maxBackBufferHeight);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Context3D,maxBackBufferWidth);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(Context3D,profile);

ASFUNCTIONBODY_ATOM(Context3D,supportsVideoTexture)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.supportsVideoTexture");
	return asAtom::falseAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,dispose)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.dispose does nothing");
	bool recreate;
	ARG_UNPACK_ATOM(recreate,true);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,configureBackBuffer)
{
	Context3D* th = obj.as<Context3D>();
	int antiAlias;
	bool wantsBestResolution;
	bool wantsBestResolutionOnBrowserZoom;
	ARG_UNPACK_ATOM(th->backBufferWidth)(th->backBufferHeight)(antiAlias)(th->enableDepthAndStencil, true)(wantsBestResolution,false)(wantsBestResolutionOnBrowserZoom,false);
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.configureBackBuffer does not use all parameters:"<<antiAlias);
	renderaction action;
	action.action = RENDER_ACTION::RENDER_CONFIGUREBACKBUFFER;
	action.udata1 = th->enableDepthAndStencil ? 1:0;
	th->addAction(action);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,createCubeTexture)
{
	Context3D* th = obj.as<Context3D>();
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.createCubeTexture does nothing");
	int32_t size;
	tiny_string format;
	bool optimizeForRenderToTexture;
	int32_t streamingLevels;
	ARG_UNPACK_ATOM(size)(format)(optimizeForRenderToTexture)(streamingLevels,0);
	return asAtom::fromObject(Class<CubeTexture>::getInstanceS(sys,th));
}
ASFUNCTIONBODY_ATOM(Context3D,createRectangleTexture)
{
	Context3D* th = obj.as<Context3D>();
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.createRectangleTexture does nothing");
	int width;
	int height;
	tiny_string format;
	bool optimizeForRenderToTexture;
	int32_t streamingLevels;
	ARG_UNPACK_ATOM(width)(height)(format)(optimizeForRenderToTexture)(streamingLevels, 0);
	return asAtom::fromObject(Class<RectangleTexture>::getInstanceS(sys,th));
}
ASFUNCTIONBODY_ATOM(Context3D,createTexture)
{
	Context3D* th = obj.as<Context3D>();
	tiny_string format;
	bool optimizeForRenderToTexture;
	int32_t streamingLevels;
	Texture* res = Class<Texture>::getInstanceS(sys,th);
	ARG_UNPACK_ATOM(res->width)(res->height)(format)(optimizeForRenderToTexture)(streamingLevels, 0);
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.createTexture ignores parameters format,optimizeForRenderToTexture,streamingLevels:"<<format<<" "<<optimizeForRenderToTexture<<" "<<streamingLevels<<" "<<res);
	return asAtom::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Context3D,createVideoTexture)
{
	Context3D* th = obj.as<Context3D>();
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.createVideoTexture does nothing");
	return asAtom::fromObject(Class<VideoTexture>::getInstanceS(sys,th));
}
ASFUNCTIONBODY_ATOM(Context3D,createProgram)
{
	Context3D* th = obj.as<Context3D>();
	th->incRef();
	return asAtom::fromObject(Class<Program3D>::getInstanceS(sys,_MR(th)));
}
ASFUNCTIONBODY_ATOM(Context3D,createVertexBuffer)
{
	Context3D* th = obj.as<Context3D>();
	int32_t numVertices;
	int32_t data32PerVertex;
	tiny_string bufferUsage;
	ARG_UNPACK_ATOM(numVertices)(data32PerVertex)(bufferUsage,"staticDraw");
	return asAtom::fromObject(Class<VertexBuffer3D>::getInstanceS(sys,th, numVertices,data32PerVertex,bufferUsage));
}
ASFUNCTIONBODY_ATOM(Context3D,createIndexBuffer)
{
	Context3D* th = obj.as<Context3D>();
	int32_t numVertices;
	tiny_string bufferUsage;
	ARG_UNPACK_ATOM(numVertices)(bufferUsage,"staticDraw");
	return asAtom::fromObject(Class<IndexBuffer3D>::getInstanceS(sys,th,numVertices,bufferUsage));
}

ASFUNCTIONBODY_ATOM(Context3D,clear)
{
	Context3D* th = obj.as<Context3D>();
	number_t red,green,blue,alpha,depth;
	renderaction action;
	action.action = RENDER_ACTION::RENDER_CLEAR;
	ARG_UNPACK_ATOM(red,0.0)(green,0.0)(blue,0.0)(alpha,1.0)(depth,1.0)(action.udata1,0)(action.udata2,0xffffffff);
	action.fdata =new float[5] { (float)red, (float)green, (float)blue, (float)alpha, (float)depth};
	th->addAction(action);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(Context3D,drawToBitmapData)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.drawToBitmapData does nothing");
	_NR<BitmapData> destination;
	ARG_UNPACK_ATOM(destination);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(Context3D,drawTriangles)
{
	Context3D* th = obj.as<Context3D>();
	_NR<IndexBuffer3D> indexBuffer;
	uint32_t firstIndex;
	int32_t numTriangles;
	ARG_UNPACK_ATOM(indexBuffer)(firstIndex,0)(numTriangles,-1);
	renderaction action;
	action.action = RENDER_ACTION::RENDER_DRAWTRIANGLES;
	action.dataobject = indexBuffer;
	action.udata1 = firstIndex;
	action.udata2 = (numTriangles == -1 ? UINT32_MAX : numTriangles);
	th->actions[th->currentactionvector].push_back(action);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(Context3D,setBlendFactors)
{
	Context3D* th = obj.as<Context3D>();
	tiny_string sourceFactor;
	tiny_string destinationFactor;
	ARG_UNPACK_ATOM(sourceFactor)(destinationFactor);
	BLEND_FACTOR src, dst;
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
		throwError<ArgumentError>(kInvalidArgumentError);
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
		throwError<ArgumentError>(kInvalidArgumentError);
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETBLENDFACTORS;
	action.udata1 = src;
	action.udata2 = dst;
	th->actions[th->currentactionvector].push_back(action);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setColorMask)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.setColorMask does nothing");
	bool red;
	bool green;
	bool blue;
	bool alpha;
	ARG_UNPACK_ATOM(red)(green)(blue)(alpha);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setCulling)
{
	Context3D* th = obj.as<Context3D>();
	tiny_string triangleFaceToCull;
	ARG_UNPACK_ATOM(triangleFaceToCull);
	
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETCULLING;
	if (triangleFaceToCull == "none")
		action.udata1 = FACE_NONE;
	else if (triangleFaceToCull == "front")
		action.udata1 = FACE_FRONT;
	else if (triangleFaceToCull == "back")
		action.udata1 = FACE_BACK;
	else if (triangleFaceToCull == "frontAndBack")
		action.udata1 = FACE_FRONT_AND_BACK;
	th->addAction(action);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setDepthTest)
{
	Context3D* th = obj.as<Context3D>();
	bool depthMask;
	tiny_string passCompareMode;
	ARG_UNPACK_ATOM(depthMask)(passCompareMode);
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETDEPTHTEST;
	action.udata1= depthMask ? 1:0;
	if (passCompareMode =="always")
		action.udata2 = DEPTH_FUNCTION::ALWAYS;
	else if (passCompareMode =="equal")
		action.udata2 = DEPTH_FUNCTION::EQUAL;
	else if (passCompareMode =="greater")
		action.udata2 = DEPTH_FUNCTION::GREATER;
	else if (passCompareMode =="greaterEqual")
		action.udata2 = DEPTH_FUNCTION::GREATER_EQUAL;
	else if (passCompareMode =="less")
		action.udata2 = DEPTH_FUNCTION::LESS;
	else if (passCompareMode =="lessEqual")
		action.udata2 = DEPTH_FUNCTION::LESS_EQUAL;
	else if (passCompareMode =="never")
		action.udata2 = DEPTH_FUNCTION::NEVER;
	else if (passCompareMode =="notEqual")
		action.udata2 = DEPTH_FUNCTION::NOT_EQUAL;
	else
		throwError<ArgumentError>(kInvalidArgumentError,"passCompareMode");
	th->addAction(action);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setProgram)
{
	Context3D* th = obj.as<Context3D>();
	_NR<Program3D> program;
	ARG_UNPACK_ATOM(program);
	if (!program.isNull())
		th->addAction(RENDER_SETPROGRAM,program.getPtr());
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setProgramConstantsFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.setProgramConstantsFromByteArray does nothing");
	tiny_string programType;
	int32_t firstRegister;
	int32_t numRegisters;
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	ARG_UNPACK_ATOM(programType)(firstRegister)(numRegisters)(data)(byteArrayOffset);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setProgramConstantsFromMatrix)
{
	Context3D* th = obj.as<Context3D>();
	tiny_string programType;
	uint32_t firstRegister;
	_NR<Matrix3D> matrix;
	bool transposedMatrix;
	ARG_UNPACK_ATOM(programType)(firstRegister)(matrix)(transposedMatrix, false);
	if (matrix.isNull())
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setProgramConstantsFromMatrix with empty matrix");
	else
	{
		renderaction action;
		action.action = RENDER_ACTION::RENDER_SETPROGRAMCONSTANTS_FROM_MATRIX;
		action.fdata = new float[16];
		matrix->getRawDataAsFloat(action.fdata);
		action.udata1= firstRegister;
		action.udata2= programType == "vertex" ? 1 : 0;
		action.udata3= transposedMatrix ? 1 : 0;
		th->addAction(action);
	}
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setProgramConstantsFromVector)
{
	Context3D* th = obj.as<Context3D>();
	tiny_string programType;
	int32_t firstRegister;
	_NR<Vector> data;
	int32_t numRegisters;
	ARG_UNPACK_ATOM(programType)(firstRegister)(data)(numRegisters,-1);
	if (data.isNull())
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setProgramConstantsFromVector with empty Vector");
	else
	{
		renderaction action;
		action.action = RENDER_ACTION::RENDER_SETPROGRAMCONSTANTS_FROM_VECTOR;
		action.udata1= firstRegister;
		action.udata2= programType == "vertex" ? 1 : 0;
		action.udata3= numRegisters < 0 ? data->size()/4 : min ((uint32_t)numRegisters,data->size()/4);
		action.fdata= new float[action.udata3*4];
		for (uint32_t i = 0; i < action.udata3*4; i++)
		{
			action.fdata[i] = data->at(i).toNumber();
		}
		th->addAction(action);
	}
	return asAtom::invalidAtom;
}


ASFUNCTIONBODY_ATOM(Context3D,setScissorRectangle)
{
	_NR<Rectangle> rectangle;
	ARG_UNPACK_ATOM(rectangle);
	if (!rectangle.isNull())
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setScissorRectangle does nothing:"<<rectangle->toDebugString());
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setRenderToBackBuffer)
{
	Context3D* th = obj.as<Context3D>();
	renderaction action;
	action.action = RENDER_ACTION::RENDER_RENDERTOBACKBUFFER;
	th->addAction(action);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setRenderToTexture)
{
	Context3D* th = obj.as<Context3D>();
	bool enableDepthAndStencil;
	int32_t antiAlias;
	int32_t surfaceSelector;
	int32_t colorOutputIndex;
	renderaction action;
	ARG_UNPACK_ATOM(action.dataobject)(enableDepthAndStencil,false)(antiAlias,0)(surfaceSelector, 0)(colorOutputIndex, 0);
	if (action.dataobject.isNull())
		throwError<ArgumentError>(kCheckTypeFailedError,
								  "null",
								  Class<TextureBase>::getClass(sys)->getQualifiedClassName());
	if (!action.dataobject->is<TextureBase>())
		throwError<ArgumentError>(kCheckTypeFailedError,
								  action.dataobject->getClassName(),
								  Class<TextureBase>::getClass(sys)->getQualifiedClassName());
	if (antiAlias!=0 || surfaceSelector!=0 || colorOutputIndex!=0)
		LOG(LOG_NOT_IMPLEMENTED,"Context3D.setRenderToTexture ignores parameters antiAlias, surfaceSelector, colorOutput");
	action.action = RENDER_ACTION::RENDER_TOTEXTURE;
	action.udata1 = enableDepthAndStencil ? 1 : 0;
	th->addAction(action);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setSamplerStateAt)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.setSamplerStateAt does nothing");
	int32_t sampler;
	tiny_string wrap;
	tiny_string filter;
	tiny_string mipfilter;
	ARG_UNPACK_ATOM(sampler)(wrap)(filter)(mipfilter);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,present)
{
	Context3D* th = obj.as<Context3D>();
	Locker l(th->rendermutex);
	th->currentactionvector=1-th->currentactionvector;
	sys->stage->requestInvalidation(sys);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setStencilActions)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.setStencilActions does nothing");
	tiny_string triangleFace;
	tiny_string compareMode;
	tiny_string actionOnBothPass;
	tiny_string actionOnDepthFail;
	tiny_string actionOnDepthPassStencilFail;
	ARG_UNPACK_ATOM(triangleFace,"frontAndBack")(compareMode,"always")(actionOnBothPass,"keep")(actionOnDepthFail,"keep")(actionOnDepthPassStencilFail,"keep");
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Context3D,setStencilReferenceValue)
{
	LOG(LOG_NOT_IMPLEMENTED,"Context3D.setStencilReferenceValue does nothing");
	uint32_t referenceValue;
	uint32_t readMask;
	uint32_t writeMask;
	ARG_UNPACK_ATOM(referenceValue)(readMask,255)(writeMask,255);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(Context3D,setTextureAt)
{
	Context3D* th = obj.as<Context3D>();
	uint32_t sampler;
	_NR<TextureBase> texture;
	ARG_UNPACK_ATOM(sampler)(texture);
	if (sampler >= CONTEXT3D_SAMPLER_COUNT)
		throwError<RangeError>(kParamRangeError);
	if (!texture.isNull())
		texture->incRef();
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETTEXTUREAT;
	action.dataobject = texture;
	action.udata1 = sampler;
	th->actions[th->currentactionvector].push_back(action);
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY_ATOM(Context3D,setVertexBufferAt)
{
	Context3D* th = obj.as<Context3D>();
	uint32_t index;
	_NR<VertexBuffer3D> buffer;
	uint32_t bufferOffset;
	tiny_string format;
	ARG_UNPACK_ATOM(index)(buffer)(bufferOffset,0)(format,"float4");
	if (index > 7)
		throwError<RangeError>(kParamRangeError);
	renderaction action;
	action.action = RENDER_ACTION::RENDER_SETVERTEXBUFFER;
	action.dataobject = buffer;
	action.udata1 = index;
	action.udata2 = bufferOffset;
	if (format == "bytes4")
		action.udata3 = VERTEXBUFFER_FORMAT::BYTES_4;
	else if (format == "float1")
		action.udata3 = VERTEXBUFFER_FORMAT::FLOAT_1;
	else if (format == "float2")
		action.udata3 = VERTEXBUFFER_FORMAT::FLOAT_2;
	else if (format == "float3")
		action.udata3 = VERTEXBUFFER_FORMAT::FLOAT_3;
	else if (format == "float4")
		action.udata3 = VERTEXBUFFER_FORMAT::FLOAT_4;
	th->actions[th->currentactionvector].push_back(action);
	return asAtom::invalidAtom;
}


void Context3DBlendFactor::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("DESTINATION_ALPHA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"destinationAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DESTINATION_COLOR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"destinationColor"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"one"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE_MINUS_DESTINATION_ALPHA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"oneMinusDestinationAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE_MINUS_DESTINATION_COLOR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"oneMinusDestinationColor"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE_MINUS_SOURCE_ALPHA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"oneMinusSourceAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ONE_MINUS_SOURCE_COLOR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"oneMinusSourceColor"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SOURCE_ALPHA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"sourceAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SOURCE_COLOR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"sourceColor"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ZERO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"zero"),CONSTANT_TRAIT);
}

void Context3DBufferUsage::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("DYNAMIC_DRAW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"dynamicDraw"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STATIC_DRAW",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"staticDraw"),CONSTANT_TRAIT);
}

void Context3DClearMask::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALL",nsNameAndKind(),asAtom(CLEARMASK::COLOR | CLEARMASK::DEPTH | CLEARMASK::STENCIL),CONSTANT_TRAIT);
	c->setVariableAtomByQName("COLOR",nsNameAndKind(),asAtom(CLEARMASK::COLOR),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DEPTH",nsNameAndKind(),asAtom(CLEARMASK::DEPTH),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STENCIL",nsNameAndKind(),asAtom(CLEARMASK::STENCIL),CONSTANT_TRAIT);
}

void Context3DCompareMode::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALWAYS",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"always"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("EQUAL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"equal"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GREATER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"greater"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("GREATER_EQUAL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"greaterEqual"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LESS",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"less"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LESS_EQUAL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"lessEqual"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NEVER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"never"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NOT_EQUAL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"notEqual"),CONSTANT_TRAIT);
}

void Context3DMipFilter::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("MIPLINEAR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"miplinear"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MIPNEAREST",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mipnearest"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MIPNONE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mipnone"),CONSTANT_TRAIT);
}

void Context3DProfile::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BASELINE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"baseline"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BASELINE_CONSTRAINED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"baselineConstrained"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BASELINE_EXTENDED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"baselineExtended"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standard"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD_CONSTRAINED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardConstrained"),CONSTANT_TRAIT);
}

void Context3DProgramType::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("FRAGMENT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"fragment"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VERTEX",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"vertex"),CONSTANT_TRAIT);
}

void Context3DRenderMode::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SOFTWARE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"software"),CONSTANT_TRAIT);
}

void Context3DStencilAction::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("DECREMENT_SATURATE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"decrementSaturate"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DECREMENT_WRAP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"decrementWrap"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INCREMENT_SATURATE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"incrementSaturate"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INCREMENT_WRAP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"incrementWrap"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVERT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"invert"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KEEP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"keep"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SET",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"set"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ZERO",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"zero"),CONSTANT_TRAIT);
}

void Context3DTextureFilter::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ANISOTROPIC16X",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"anisotropic16x"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ANISOTROPIC2X",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"anisotropic2x"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ANISOTROPIC4X",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"anisotropic4x"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ANISOTROPIC8X",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"anisotropic8x"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LINEAR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"linear"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NEAREST",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"nearest"),CONSTANT_TRAIT);
}

void Context3DTextureFormat::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BGRA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"bgra"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BGRA_PACKED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"bgraPacked4444"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BGR_PACKED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"bgrPacked565"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("COMPRESSED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"compressed"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("COMPRESSED_ALPHA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"compressedAlpha"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RGBA_HALF_FLOAT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"rgbaHalfFloat"),CONSTANT_TRAIT);
}

void Context3DTriangleFace::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BACK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"back"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FRONT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"front"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FRONT_AND_BACK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"frontAndBack"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
}

void Context3DVertexBufferFormat::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BYTES_4",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"bytes4"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT_1",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"float1"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT_2",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"float2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT_3",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"float3"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT_4",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"float4"),CONSTANT_TRAIT);
}

void Context3DWrapMode::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("CLAMP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"clamp"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CLAMP_U_REPEAT_V",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"clamp_u_repeat_v"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REPEAT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"repeat"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REPEAT_U_CLAMP_V",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"repeat_u_clamp_v"),CONSTANT_TRAIT);
}

IndexBuffer3D::IndexBuffer3D(Class_base *c, Context3D* ctx,int numVertices, tiny_string _bufferUsage)
	:ASObject(c,T_OBJECT,SUBTYPE_INDEXBUFFER3D),context(ctx),bufferID(UINT32_MAX),bufferUsage(_bufferUsage)
{
	data.reserve(numVertices);
}

IndexBuffer3D::~IndexBuffer3D()
{
	if (context && bufferID != UINT32_MAX)
	{
		renderaction action;
		action.action =RENDER_ACTION::RENDER_DELETEBUFFER;
		action.udata1 = this->bufferID;
		context->addAction(action);
		this->bufferID = UINT32_MAX;
	}
}

void IndexBuffer3D::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromVector","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromVector),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(IndexBuffer3D,dispose)
{
	IndexBuffer3D* th = obj.as<IndexBuffer3D>();
	if (th->context && th->bufferID != UINT32_MAX)
	{
		renderaction action;
		action.action =RENDER_ACTION::RENDER_DELETEBUFFER;
		action.udata1 = th->bufferID;
		th->context->addAction(action);
		th->bufferID = UINT32_MAX;
	}
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(IndexBuffer3D,uploadFromByteArray)
{
	IndexBuffer3D* th = obj.as<IndexBuffer3D>();
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	uint32_t startOffset;
	uint32_t count;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(startOffset)(count);
	if (data.isNull())
		throwError<TypeError>(kNullPointerError);
	if (data->getLength() < byteArrayOffset+count*2)
		throwError<RangeError>(kParamRangeError);
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
	data->setPosition(origpos);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(IndexBuffer3D,uploadFromVector)
{
	IndexBuffer3D* th = obj.as<IndexBuffer3D>();
	_NR<Vector> data;
	uint32_t startOffset;
	uint32_t count;
	ARG_UNPACK_ATOM(data)(startOffset)(count);
	if (th->data.size() < count+startOffset)
		th->data.resize(count+startOffset);
	for (uint32_t i = 0; i< count; i++)
	{
		th->data[startOffset+i] = data->at(i).toUInt();
	}
	return asAtom::invalidAtom;
}

void Program3D::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("upload","",Class<IFunction>::getFunction(c->getSystemState(),upload),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(Program3D,dispose)
{
	Program3D* th = obj.as<Program3D>();
	th->context3D->addAction(RENDER_DELETEPROGRAM,th);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(Program3D,upload)
{
	Program3D* th = obj.as<Program3D>();
	_NR<ByteArray> vertexProgram;
	_NR<ByteArray> fragmentProgram;
	ARG_UNPACK_ATOM(vertexProgram)(fragmentProgram);
	th->samplerState.clear();
	if (!vertexProgram.isNull())
		th->vertexprogram = AGALtoGLSL(vertexProgram.getPtr(),true,th->samplerState,th->vertexregistermap,th->vertexattributes);
	if (!fragmentProgram.isNull())
		th->fragmentprogram = AGALtoGLSL(fragmentProgram.getPtr(),false,th->samplerState,th->fragmentregistermap,th->fragmentattributes);
	return asAtom::invalidAtom;
}

VertexBuffer3D::VertexBuffer3D(Class_base *c, Context3D *ctx, int _numVertices, int32_t _data32PerVertex, tiny_string _bufferUsage)
	:ASObject(c,T_OBJECT,SUBTYPE_VERTEXBUFFER3D),context(ctx),bufferID(UINT32_MAX),numVertices(_numVertices),data32PerVertex(_data32PerVertex),bufferUsage(_bufferUsage),upload_needed(false)
{
	data.resize(numVertices*data32PerVertex);
}

VertexBuffer3D::~VertexBuffer3D()
{
	if (context && bufferID != UINT32_MAX)
	{
		renderaction action;
		action.action =RENDER_ACTION::RENDER_DELETEBUFFER;
		action.udata1 = this->bufferID;
		context->addAction(action);
		this->bufferID = UINT32_MAX;
	}
}

void VertexBuffer3D::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromVector","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromVector),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(VertexBuffer3D,dispose)
{
	VertexBuffer3D* th = obj.as<VertexBuffer3D>();
	if (th->context && th->bufferID != UINT32_MAX)
	{
		renderaction action;
		action.action =RENDER_ACTION::RENDER_DELETEBUFFER;
		action.udata1 = th->bufferID;
		th->context->addAction(action);
		th->bufferID = UINT32_MAX;
	}
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(VertexBuffer3D,uploadFromByteArray)
{
	VertexBuffer3D* th = obj.as<VertexBuffer3D>();
	_NR<ByteArray> data;
	uint32_t byteArrayOffset;
	uint32_t startVertex;
	uint32_t numVertices;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(startVertex)(numVertices);
	if (data.isNull())
		throwError<TypeError>(kNullPointerError);
	if (data->getLength() < byteArrayOffset+numVertices*th->data32PerVertex*4)
		throwError<RangeError>(kParamRangeError);
	th->upload_needed = true;
	if (th->data.size() < (numVertices+startVertex)* th->data32PerVertex)
		th->data.resize((numVertices+startVertex)* th->data32PerVertex);
	uint32_t origpos = data->getPosition();
	data->setPosition(byteArrayOffset);
	for (uint32_t i = 0; i< numVertices* th->data32PerVertex; i++)
	{
		union {
			float f;
			uint32_t u;
		} d;
		if (data->readUnsignedInt(d.u))
			th->data[startVertex*th->data32PerVertex+i] = d.f;
	}
	data->setPosition(origpos);
	return asAtom::invalidAtom;
}
ASFUNCTIONBODY_ATOM(VertexBuffer3D,uploadFromVector)
{
	VertexBuffer3D* th = obj.as<VertexBuffer3D>();
	_NR<Vector> data;
	uint32_t startVertex;
	uint32_t numVertices;
	ARG_UNPACK_ATOM(data)(startVertex)(numVertices);
	th->upload_needed = true;
	if (th->data.size() < (numVertices+startVertex)* th->data32PerVertex)
		th->data.resize((numVertices+startVertex)* th->data32PerVertex);
	for (uint32_t i = 0; i< numVertices* th->data32PerVertex; i++)
	{
		th->data[startVertex*th->data32PerVertex+i] = data->at(i).toNumber();
	}
	return asAtom::invalidAtom;
}

}
