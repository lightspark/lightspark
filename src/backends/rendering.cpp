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

#include "scripting/abc.h"
#include "parsing/textfile.h"
#include "rendering.h"
#include "glmatrices.h"
#include "compat.h"
#include <sstream>

#include <GL/glew.h>
#ifndef WIN32
#include <GL/glx.h>
#endif

//The interpretation of texture data change with the endianness
#if __BYTE_ORDER == __BIG_ENDIAN
#define GL_UNSIGNED_INT_8_8_8_8_HOST GL_UNSIGNED_INT_8_8_8_8_REV
#else
#define GL_UNSIGNED_INT_8_8_8_8_HOST GL_UNSIGNED_BYTE
#endif

using namespace lightspark;
using namespace std;

void RenderThread::wait()
{
	if(status==STARTED)
	{
		//Signal potentially blocking semaphore
		sem_post(&event);
		int ret=pthread_join(t,NULL);
		assert_and_throw(ret==0);
		status=TERMINATED;
	}
}

RenderThread::RenderThread(SystemState* s):
	m_sys(s),status(CREATED),currentPixelBuffer(0),currentPixelBufferOffset(0),
	pixelBufferWidth(0),pixelBufferHeight(0),prevUploadJob(NULL),mutexLargeTexture("Large texture"),largeTextureSize(0),
	renderNeeded(false),uploadNeeded(false),resizeNeeded(false),newTextureNeeded(false),newWidth(0),newHeight(0),scaleX(1),scaleY(1),
	offsetX(0),offsetY(0),tempBufferAcquired(false),frameCount(0),secsCount(0),mutexUploadJobs("Upload jobs"),initialized(0),
	tempTex(false),hasNPOTTextures(false),cairoTextureContext(NULL)
{
	LOG(LOG_NO_INFO,_("RenderThread this=") << this);
	
	m_sys=s;
	sem_init(&event,0,0);

#ifdef WIN32
	fontPath = "TimesNewRoman.ttf";
#else
	fontPath = "Serif";
#endif
	time_s = compat_get_current_time_ms();
}

void RenderThread::start(const EngineData* data)
{
	status=STARTED;
	engineData=data;
	pthread_create(&t,NULL,(thread_worker)worker,this);
}

void RenderThread::stop()
{
	initialized.signal();
}

RenderThread::~RenderThread()
{
	wait();
	sem_destroy(&event);
	LOG(LOG_NO_INFO,_("~RenderThread this=") << this);
}

/*void RenderThread::acquireTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax)
{
	::abort();
	GLint vertex_coords[8];
	static GLfloat color_coords[16];
	assert(tempBufferAcquired==false);
	tempBufferAcquired=true;

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
	vertex_coords[0] = xmin;vertex_coords[1] = ymin;
	vertex_coords[2] = xmax;vertex_coords[3] = ymin;
	vertex_coords[4] = xmin;vertex_coords[5] = ymax;
	vertex_coords[6] = xmax;vertex_coords[7] = ymax;

	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_INT, GL_FALSE, 0, vertex_coords);
	glVertexAttribPointer(COLOR_ATTRIB, 4, GL_FLOAT, GL_FALSE, 0, color_coords);
	glEnableVertexAttribArray(VERTEX_ATTRIB);
	glEnableVertexAttribArray(COLOR_ATTRIB);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(VERTEX_ATTRIB);
	glDisableVertexAttribArray(COLOR_ATTRIB);
}

void RenderThread::blitTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax)
{
	assert(tempBufferAcquired==true);
	GLint vertex_coords[8];
	tempBufferAcquired=false;

	//Use the blittler program to blit only the used buffer
	glUseProgram(blitter_program);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);

	rt->tempTex.bind();

	vertex_coords[0] = xmin;vertex_coords[1] = ymin;
	vertex_coords[2] = xmax;vertex_coords[3] = ymin;
	vertex_coords[4] = xmin;vertex_coords[5] = ymax;
	vertex_coords[6] = xmax;vertex_coords[7] = ymax;

	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_INT, GL_FALSE, 0, vertex_coords);
	glEnableVertexAttribArray(VERTEX_ATTRIB);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(VERTEX_ATTRIB);

	glUseProgram(gpu_program);
}*/

void RenderThread::handleNewTexture()
{
	//Find if any largeTexture is not initialized
	Locker l(mutexLargeTexture);
	for(uint32_t i=0;i<largeTextures.size();i++)
	{
		if(largeTextures[i].id==(GLuint)-1)
			largeTextures[i].id=allocateNewGLTexture();
	}
	newTextureNeeded=false;
}

void RenderThread::finalizeUpload()
{
	ITextureUploadable* u=prevUploadJob;
	uint32_t w,h;
	u->sizeNeeded(w,h);
	const TextureChunk& tex=u->getTexture();
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffers[currentPixelBuffer]);
	//Copy content of the pbo to the texture, currentPixelBufferOffset is the offset in the pbo
	loadChunkBGRA(tex, w, h, (uint8_t*)currentPixelBufferOffset);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	u->uploadFence();
	prevUploadJob=NULL;
}

void RenderThread::handleUpload()
{
	ITextureUploadable* u=getUploadJob();
	assert(u);
	uint32_t w,h;
	u->sizeNeeded(w,h);
	if(w>pixelBufferWidth || h>pixelBufferHeight)
		resizePixelBuffers(w,h);
	//Increment and wrap current buffer index
	unsigned int nextBuffer = (currentPixelBuffer + 1)%2;

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffers[nextBuffer]);
	uint8_t* buf=(uint8_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER,GL_WRITE_ONLY);
	uint8_t* alignedBuf=(uint8_t*)(uintptr_t((buf+15))&(~0xfL));

	u->upload(alignedBuf, w, h);

	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	currentPixelBufferOffset=alignedBuf-buf;
	currentPixelBuffer=nextBuffer;

	//Get the texture to be sure it's allocated when the upload comes
	u->getTexture();
	prevUploadJob=u;
}

void RenderThread::SizeAllocateCallback(GtkWidget* widget, GdkRectangle* allocation, gpointer data)
{
	RenderThread* th=static_cast<RenderThread*>(data);
	th->requestResize(allocation->width, allocation->height);
}

void* RenderThread::worker(RenderThread* th)
{
	sys=th->m_sys;
	rt=th;
	const EngineData* e=th->engineData;
	SemaphoreLighter lighter(th->initialized);

	//Get information about changes in the available space
	g_signal_connect(e->container,"size-allocate",G_CALLBACK(SizeAllocateCallback),th);

	th->windowWidth=e->width;
	th->windowHeight=e->height;
	
	Display* d=XOpenDisplay(NULL);

	int a,b;
	Bool glx_present=glXQueryVersion(d,&a,&b);
	if(!glx_present)
	{
		LOG(LOG_ERROR,_("glX not present"));
		return NULL;
	}
	int attrib[10]={GLX_BUFFER_SIZE,24,GLX_DOUBLEBUFFER,True,None};
	GLXFBConfig* fb=glXChooseFBConfig(d, 0, attrib, &a);
	if(!fb)
	{
		attrib[2]=None;
		fb=glXChooseFBConfig(d, 0, attrib, &a);
		LOG(LOG_ERROR,_("Falling back to no double buffering"));
	}
	if(!fb)
	{
		LOG(LOG_ERROR,_("Could not find any GLX configuration"));
		::abort();
	}
	int i;
	for(i=0;i<a;i++)
	{
		int id;
		glXGetFBConfigAttrib(d,fb[i],GLX_VISUAL_ID,&id);
		if(id==(int)e->visual)
			break;
	}
	if(i==a)
	{
		//No suitable id found
		LOG(LOG_ERROR,_("No suitable graphics configuration available"));
		return NULL;
	}
	th->mFBConfig=fb[i];
	cout << "Chosen config " << hex << fb[i] << dec << endl;
	XFree(fb);

	th->mContext = glXCreateNewContext(d,th->mFBConfig,GLX_RGBA_TYPE ,NULL,1);
	GLXWindow glxWin=e->window;
	glXMakeCurrent(d, glxWin,th->mContext);
	if(!glXIsDirect(d,th->mContext))
		cout << "Indirect!!" << endl;

	th->commonGLInit(th->windowWidth, th->windowHeight);
	th->commonGLResize();
	lighter.light();
	
	ThreadProfile* profile=sys->allocateProfiler(RGB(200,0,0));
	profile->setTag("Render");

	glEnable(GL_TEXTURE_2D);
	try
	{
		Chronometer chronometer;
		while(1)
		{
			sem_wait(&th->event);
			if(th->m_sys->isShuttingDown())
				break;
			chronometer.checkpoint();
			
			if(th->resizeNeeded)
			{
				th->windowWidth=th->newWidth;
				th->windowHeight=th->newHeight;
				th->newWidth=0;
				th->newHeight=0;
				th->resizeNeeded=false;
				LOG(LOG_NO_INFO,_("Window resized to ") << th->windowWidth << 'x' << th->windowHeight);
				th->commonGLResize();
				th->m_sys->resizeCompleted();
				profile->accountTime(chronometer.checkpoint());
				continue;
			}

			if(th->newTextureNeeded)
				th->handleNewTexture();

			if(th->prevUploadJob)
				th->finalizeUpload();

			if(th->uploadNeeded)
			{
				th->handleUpload();
				profile->accountTime(chronometer.checkpoint());
				continue;
			}

			if(th->m_sys->isOnError())
			{
				th->renderErrorPage(th, th->m_sys->standalone);
				glXSwapBuffers(d,glxWin);
			}
			else
			{
				glXSwapBuffers(d,glxWin);
				th->coreRendering();
				//Call glFlush to offload work on the GPU
				glFlush();
			}
			profile->accountTime(chronometer.checkpoint());
			th->renderNeeded=false;
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,_("Exception in RenderThread ") << e.what());
		sys->setError(e.cause);
	}
	glDisable(GL_TEXTURE_2D);
	th->commonGLDeinit();
	glXMakeCurrent(d,None,NULL);
	glXDestroyContext(d,th->mContext);
	XCloseDisplay(d);
	return NULL;
}

bool RenderThread::loadShaderPrograms()
{
	//Create render program
	assert(glCreateShader);
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	
	const char *fs = NULL;
	fs = dataFileRead("lightspark.frag");
	if(fs==NULL)
	{
		LOG(LOG_ERROR,_("Shader lightspark.frag not found"));
		throw RunTimeException("Fragment shader code not found");
	}
	assert(glShaderSource);
	glShaderSource(f, 1, &fs,NULL);
	free((void*)fs);
	GLuint g = glCreateShader(GL_VERTEX_SHADER);
	
	bool ret=true;
	char str[1024];
	int a;
	assert(glCompileShader);
	glCompileShader(f);
	assert(glGetShaderInfoLog);
	glGetShaderInfoLog(f,1024,&a,str);
	LOG(LOG_NO_INFO,_("Fragment shader compilation ") << str);

	fs = dataFileRead("lightspark.vert");
	if(fs==NULL)
	{
		LOG(LOG_ERROR,_("Shader lightspark.vert not found"));
		throw RunTimeException("Vertex shader code not found");
	}
	glShaderSource(g, 1, &fs,NULL);
	free((void*)fs);

	glCompileShader(g);
	glGetShaderInfoLog(g,1024,&a,str);
	LOG(LOG_NO_INFO,_("Vertex shader compilation ") << str);

	assert(glCreateProgram);
	gpu_program = glCreateProgram();
	glBindAttribLocation(gpu_program, VERTEX_ATTRIB, "ls_Vertex");
	glBindAttribLocation(gpu_program, COLOR_ATTRIB, "ls_Color");
	glBindAttribLocation(gpu_program, TEXCOORD_ATTRIB, "ls_TexCoord");
	assert(glAttachShader);
	glAttachShader(gpu_program,f);
	glAttachShader(gpu_program,g);

	assert(glLinkProgram);
	glLinkProgram(gpu_program);
	assert(glGetProgramiv);
	glGetProgramiv(gpu_program,GL_LINK_STATUS,&a);
	if(a==GL_FALSE)
	{
		ret=false;
		return ret;
	}
	
	//Create the blitter shader
	GLuint v = glCreateShader(GL_VERTEX_SHADER);

	fs = dataFileRead("lightspark-blitter.vert");
	if(fs==NULL)
	{
		LOG(LOG_ERROR,_("Shader lightspark-blitter.vert not found"));
		throw RunTimeException("Vertex shader code not found");
	}
	glShaderSource(v, 1, &fs,NULL);
	free((void*)fs);

	glCompileShader(v);
	glGetShaderInfoLog(v,1024,&a,str);
	LOG(LOG_NO_INFO,_("Vertex shader compilation ") << str);

	blitter_program = glCreateProgram();
	glAttachShader(blitter_program,v);
	
	glLinkProgram(blitter_program);
	glGetProgramiv(blitter_program,GL_LINK_STATUS,&a);
	if(a==GL_FALSE)
	{
		ret=false;
		return ret;
	}

	assert(ret);
	return true;
}

void RenderThread::commonGLDeinit()
{
	//Fence any object that is still waiting for upload
	Locker l(mutexUploadJobs);
	deque<ITextureUploadable*>::iterator it=uploadJobs.begin();
	if(prevUploadJob)
		prevUploadJob->uploadFence();
	for(;it!=uploadJobs.end();it++)
		(*it)->uploadFence();
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	glDeleteFramebuffers(1,&fboId);
	tempTex.shutdown();
	for(uint32_t i=0;i<largeTextures.size();i++)
	{
		glDeleteTextures(1,&largeTextures[i].id);
		delete[] largeTextures[i].bitmap;
	}
	glDeleteBuffers(2,pixelBuffers);
	glDeleteTextures(1, &cairoTextureID);
}

void RenderThread::commonGLInit(int width, int height)
{
	//Now we can initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		LOG(LOG_ERROR,_("Cannot initialize GLEW: cause ") << glewGetErrorString(err));;
		::abort();
	}
	if(!GLEW_VERSION_2_0)
	{
		LOG(LOG_ERROR,_("Video card does not support OpenGL 2.0... Aborting"));
		::abort();
	}
	if(GLEW_ARB_texture_non_power_of_two)
		hasNPOTTextures=true;

	//Load shaders
	loadShaderPrograms();

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glActiveTexture(GL_TEXTURE0);
	//Viewport setup is left for GLResize	

	tempTex.init(width, height, GL_NEAREST);

	//Get the maximum allowed texture size, up to 1024
	int maxTexSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	assert(maxTexSize>0);
	largeTextureSize=min(maxTexSize,1024);

	//Create the PBOs
	glGenBuffers(2,pixelBuffers);

	//Set uniforms
	cleanGLErrors();
	glUseProgram(blitter_program);
	int texScale=glGetUniformLocation(blitter_program,"texScale");
	tempTex.setTexScale(texScale);
	cleanGLErrors();

	glUseProgram(gpu_program);
	cleanGLErrors();
	int tex=glGetUniformLocation(gpu_program,"g_tex1");
	if(tex!=-1)
		glUniform1i(tex,0);
	tex=glGetUniformLocation(gpu_program,"g_tex2");
	if(tex!=-1)
		glUniform1i(tex,1);

	//The uniform that enables YUV->RGB transform on the texels (needed for video)
	yuvUniform =glGetUniformLocation(gpu_program,"yuv");
	//The uniform that tells the shader if a mask is being rendered
	maskUniform =glGetUniformLocation(gpu_program,"mask");
	//The uniform that tells the alpha value multiplied to the alpha of every pixel
	alphaUniform =glGetUniformLocation(gpu_program,"alpha");
	//The uniform that contains the coordinate matrix
	projectionMatrixUniform =glGetUniformLocation(gpu_program,"ls_ProjectionMatrix");
	modelviewMatrixUniform =glGetUniformLocation(gpu_program,"ls_ModelViewMatrix");

	fragmentTexScaleUniform=glGetUniformLocation(gpu_program,"texScale");
	if(fragmentTexScaleUniform!=-1)
		glUniform2f(fragmentTexScaleUniform,1.0f/width,1.0f/height);
	cleanGLErrors();

	//Texturing must be enabled otherwise no tex coord will be sent to the shaders
	glEnable(GL_TEXTURE_2D);
	//At least two texture unit are guaranteed in OpenGL 1.3
	//The second unit will be used to access the temporary buffer
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tempTex.getId());

	glActiveTexture(GL_TEXTURE0);
	//Create a framebuffer object
	glGenFramebuffers(1, &fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, tempTex.getId(), 0);
	
	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
	{
		LOG(LOG_ERROR,_("Incomplete FBO status ") << status << _("... Aborting"));
		while(err!=GL_NO_ERROR)
		{
			LOG(LOG_ERROR,_("GL errors during initialization: ") << err);
			err=glGetError();
		}
		::abort();
	}
	glGenTextures(1, &cairoTextureID);
}

void RenderThread::commonGLResize()
{
	//Get the size of the content
	RECT r=m_sys->getFrameSize();
	r.Xmax/=20;
	r.Ymax/=20;
	//Now compute the scalings and offsets
	switch(m_sys->scaleMode)
	{
		case SystemState::SHOW_ALL:
			//Compute both scaling
			scaleX=windowWidth;
			scaleX/=r.Xmax;
			scaleY=windowHeight;
			scaleY/=r.Ymax;
			//Enlarge with no distortion
			if(scaleX<scaleY)
			{
				//Uniform scaling for Y
				scaleY=scaleX;
				//Apply the offset
				offsetY=(windowHeight-r.Ymax*scaleY)/2;
				offsetX=0;
			}
			else
			{
				//Uniform scaling for X
				scaleX=scaleY;
				//Apply the offset
				offsetX=(windowWidth-r.Xmax*scaleX)/2;
				offsetY=0;
			}
			break;
		case SystemState::NO_BORDER:
			//Compute both scaling
			scaleX=windowWidth;
			scaleX/=r.Xmax;
			scaleY=windowHeight;
			scaleY/=r.Ymax;
			//Enlarge with no distortion
			if(scaleX>scaleY)
			{
				//Uniform scaling for Y
				scaleY=scaleX;
				//Apply the offset
				offsetY=(windowHeight-r.Ymax*scaleY)/2;
				offsetX=0;
			}
			else
			{
				//Uniform scaling for X
				scaleX=scaleY;
				//Apply the offset
				offsetX=(windowWidth-r.Xmax*scaleX)/2;
				offsetY=0;
			}
			break;
		case SystemState::EXACT_FIT:
			//Compute both scaling
			scaleX=windowWidth;
			scaleX/=r.Xmax;
			scaleY=windowHeight;
			scaleY/=r.Ymax;
			offsetX=0;
			offsetY=0;
			break;
		case SystemState::NO_SCALE:
			scaleX=1;
			scaleY=1;
			offsetX=0;
			offsetY=0;
			break;
	}
	glViewport(0,0,windowWidth,windowHeight);
	lsglLoadIdentity();
	lsglOrtho(0,windowWidth,0,windowHeight,-100,0);
	//scaleY is negated to adapt the flash and gl coordinates system
	//An additional translation is added for the same reason
	lsglTranslatef(offsetX,windowHeight-offsetY,0);
	lsglScalef(scaleX,-scaleY,1);
	setMatrixUniform(LSGL_PROJECTION);
	tempTex.resize(windowWidth, windowHeight);
}

void RenderThread::requestResize(uint32_t w, uint32_t h)
{
	newWidth=w;
	newHeight=h;
	resizeNeeded=true;
	sem_post(&event);
}

void RenderThread::resizePixelBuffers(uint32_t w, uint32_t h)
{
	//Add enough room to realign to 16
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffers[0]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, w*h*4+16, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffers[1]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, w*h*4+16, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	pixelBufferWidth=w;
	pixelBufferHeight=h;
}

void RenderThread::renderMaskToTmpBuffer() const
{
	assert(!maskStack.empty());
	//Clear the tmp buffer
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	for(uint32_t i=0;i<maskStack.size();i++)
	{
		float matrix[16];
		maskStack[i].m.get4DMatrix(matrix);
		lsglLoadMatrixf(matrix);
		setMatrixUniform(LSGL_MODELVIEW);
		maskStack[i].d->Render(true);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
}

cairo_t* RenderThread::getCairoContext(int w, int h)
{
	if (!cairoTextureContext) {
		cairoTextureData = new uint8_t[w*h*4];
		cairoTextureSurface = cairo_image_surface_create_for_data(cairoTextureData, CAIRO_FORMAT_ARGB32, w, h, w*4);
		cairoTextureContext = cairo_create(cairoTextureSurface);

		cairo_select_font_face (cairoTextureContext, fontPath.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cairoTextureContext, 11);
	}
	return cairoTextureContext;
}

//Render strings using Cairo's 'toy' text API
void RenderThread::renderText(cairo_t *cr, const char *text, int x, int y)
{
	cairo_move_to(cr, x, y);
	cairo_save(cr);
	cairo_scale(cr, 1.0, -1.0);
	cairo_show_text(cr, text);
	cairo_restore(cr);
}

//Send the texture drawn by Cairo to the GPU
void RenderThread::mapCairoTexture(int w, int h)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, cairoTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, cairoTextureData);

	GLint vertex_coords[] = {0,0, w,0, 0,h, w,h};
	GLfloat texture_coords[] = {0,0, 1,0, 0,1, 1,1};
	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_INT, GL_FALSE, 0, vertex_coords);
	glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);
	glEnableVertexAttribArray(VERTEX_ATTRIB);
	glEnableVertexAttribArray(TEXCOORD_ATTRIB);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(VERTEX_ATTRIB);
	glDisableVertexAttribArray(TEXCOORD_ATTRIB);

}


void RenderThread::plotProfilingData()
{
	lsglLoadIdentity();
	lsglScalef(1.0f/scaleX,-1.0f/scaleY,1);
	lsglTranslatef(-offsetX,(windowHeight-offsetY)*(-1.0f),0);
	setMatrixUniform(LSGL_MODELVIEW);
	cairo_t *cr = getCairoContext(windowWidth, windowHeight);

	char frameBuf[20];
	snprintf(frameBuf,20,"Frame %u",m_sys->state.FP);
	renderText(cr, frameBuf, 20, 20);

	//Draw bars
	cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.7);

	for (int i=1;i<10;i++)
	{
		cairo_move_to(cr, 0, i*windowHeight/10);
		cairo_line_to(cr, windowWidth, i*windowHeight/10);
	}
	cairo_stroke(cr);

	list<ThreadProfile>::iterator it=m_sys->profilingData.begin();
	for(;it!=m_sys->profilingData.end();it++)
		it->plot(1000000/m_sys->getFrameRate(),cr);
	mapCairoTexture(windowWidth, windowHeight);

	//clear the surface
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);
}

void RenderThread::coreRendering()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	//Clear the back buffer
	RGB bg=sys->getBackground();
	glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,1);
	glClear(GL_COLOR_BUFFER_BIT);
	lsglLoadIdentity();
	setMatrixUniform(LSGL_MODELVIEW);

	m_sys->Render(false);
	assert(maskStack.empty());

	if(m_sys->showProfilingData)
		plotProfilingData();
}

//Renders the error message which caused the VM to stop.
void RenderThread::renderErrorPage(RenderThread *th, bool standalone)
{
	lsglLoadIdentity();
	lsglScalef(1.0f/th->scaleX,-1.0f/th->scaleY,1);
	lsglTranslatef(-th->offsetX,(th->windowHeight-th->offsetY)*(-1.0f),0);

	setMatrixUniform(LSGL_MODELVIEW);

	cairo_t *cr = getCairoContext(windowWidth, windowHeight);

	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_paint(cr);
	cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);

	renderText(cr, "We're sorry, Lightspark encountered a yet unsupported Flash file",
			0,th->windowHeight/2+20);

	stringstream errorMsg;
	errorMsg << "SWF file: " << th->m_sys->getOrigin().getParsedURL();
	renderText(cr, errorMsg.str().c_str(),0,th->windowHeight/2);

	errorMsg.str("");
	errorMsg << "Cause: " << th->m_sys->errorCause;
	renderText(cr, errorMsg.str().c_str(),0,th->windowHeight/2-20);

	if (standalone)
	{
		renderText(cr, "Please look at the console output to copy this error",
			0,th->windowHeight/2-40);

		renderText(cr, "Press 'Q' to exit",0,th->windowHeight/2-60);
	}
	else
	{
		renderText(cr, "Press C to copy this error to clipboard",
				0,th->windowHeight/2-40);
	}

	mapCairoTexture(windowWidth, windowHeight);
	glFlush();
}

void RenderThread::addUploadJob(ITextureUploadable* u)
{
	Locker l(mutexUploadJobs);
	if(m_sys->isShuttingDown() || status==TERMINATED)
	{
		u->uploadFence();
		return;
	}
	uploadJobs.push_back(u);
	uploadNeeded=true;
	sem_post(&event);
}

ITextureUploadable* RenderThread::getUploadJob()
{
	Locker l(mutexUploadJobs);
	assert(!uploadJobs.empty());
	ITextureUploadable* ret=uploadJobs.front();
	uploadJobs.pop_front();
	if(uploadJobs.empty())
		uploadNeeded=false;
	return ret;
}

void RenderThread::draw(bool force)
{
	if(renderNeeded && !force) //A rendering is already queued
		return;
	renderNeeded=true;
	sem_post(&event);
	time_d = compat_get_current_time_ms();
	uint64_t diff=time_d-time_s;
	if(diff>1000)
	{
		time_s=time_d;
		LOG(LOG_NO_INFO,_("FPS: ") << dec << frameCount);
		frameCount=0;
		secsCount++;
	}
	else
		frameCount++;
}

void RenderThread::tick()
{
	draw(false);
}

void RenderThread::releaseTexture(const TextureChunk& chunk)
{
	uint32_t blocksW=(chunk.width+127)/128;
	uint32_t blocksH=(chunk.height+127)/128;
	uint32_t numberOfBlocks=blocksW*blocksH;
	Locker l(mutexLargeTexture);
	LargeTexture& tex=largeTextures[chunk.texId];
	for(uint32_t i=0;i<numberOfBlocks;i++)
	{
		uint32_t bitOffset=chunk.chunks[i];
		assert(tex.bitmap[bitOffset/8]&(1<<(bitOffset%8)));
		tex.bitmap[bitOffset/8]^=(1<<(bitOffset%8));
	}
}

GLuint RenderThread::allocateNewGLTexture() const
{
	//Set up the huge texture
	GLuint tmp;
	glGenTextures(1,&tmp);
	assert(tmp!=0);
	assert(glGetError()!=GL_INVALID_OPERATION);
	//If the previous call has not failed these should not fail (in specs, we trust)
	glBindTexture(GL_TEXTURE_2D,tmp);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	//Allocate the texture
	while(glGetError()!=GL_NO_ERROR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, largeTextureSize, largeTextureSize, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_HOST, 0);
	GLenum err=glGetError();
	if(err)
	{
		LOG(LOG_ERROR,_("Can't allocate large texture... Aborting"));
		::abort();
	}
	return tmp;
}

RenderThread::LargeTexture& RenderThread::allocateNewTexture()
{
	//Signal that a new texture is needed
	newTextureNeeded=true;
	//Let's allocate the bitmap for the texture blocks, minumum block size is 128
	uint32_t bitmapSize=(largeTextureSize/128)*(largeTextureSize/128)/8;
	uint8_t* bitmap=new uint8_t[bitmapSize];
	memset(bitmap,0,bitmapSize);
	largeTextures.emplace_back(bitmap);
	return largeTextures.back();
}

bool RenderThread::allocateChunkOnTextureCompact(LargeTexture& tex, TextureChunk& ret, uint32_t blocksW, uint32_t blocksH)
{
	//Find a contiguos set of blocks
	uint32_t start;
	uint32_t blockPerSide=largeTextureSize/128;
	uint32_t bitmapSize=blockPerSide*blockPerSide;
	for(start=0;start<bitmapSize;start++)
	{
		bool badRect=false;
		//TODO: correct rect lookup
		for(uint32_t i=0;i<blocksH;i++)
		{
			for(uint32_t j=0;j<blocksW;j++)
			{
				uint32_t bitOffset=start+i*blockPerSide+j;
				if(bitOffset>=bitmapSize)
				{
					badRect=true;
					break;
				}
				if(tex.bitmap[bitOffset/8]&(1<<(bitOffset%8)))
				{
					badRect=true;
					break;
				}
			}
			if(badRect)
				break;
		}
		if(!badRect)
			break;
	}
	if(start==bitmapSize)
		return false;
	//Now set all those blocks are used
	for(uint32_t i=0;i<blocksH;i++)
	{
		for(uint32_t j=0;j<blocksW;j++)
		{
			uint32_t bitOffset=start+i*blockPerSide+j;
			tex.bitmap[bitOffset/8]|=1<<(bitOffset%8);
			ret.chunks[i*blocksW+j]=bitOffset;
		}
	}
	return true;
}

bool RenderThread::allocateChunkOnTextureSparse(LargeTexture& tex, TextureChunk& ret, uint32_t blocksW, uint32_t blocksH)
{
	//Allocate a sparse set of texture chunks
	uint32_t found=0;
	uint32_t blockPerSide=largeTextureSize/128;
	uint32_t bitmapSize=blockPerSide*blockPerSide;
	//TODO: use the already allocated array
	uint32_t* tmp=new uint32_t[blocksW*blocksH];
	for(uint32_t i=0;i<bitmapSize;i++)
	{
		if((tex.bitmap[i/8]&(1<<(i%8)))==0)
		{
			tex.bitmap[i/8]|=1<<(i%8);
			tmp[found]=i;
			found++;
			if(found==(blocksW*blocksH))
				break;
		}
	}
	if(found<blocksW*blocksH)
	{
		//This is considered the unfrequent case
		for(uint32_t i=0;i<found;i++)
		{
			uint32_t bitOffset=tmp[i];
			assert(tex.bitmap[bitOffset/8]&(1<<(bitOffset%8)));
			tex.bitmap[bitOffset/8]^=(1<<(bitOffset%8));
		}
		delete[] tmp;
		return false;
	}
	else
	{
		delete[] ret.chunks;
		ret.chunks=tmp;
		return true;
	}
}

TextureChunk RenderThread::allocateTexture(uint32_t w, uint32_t h, bool compact)
{
	assert(w && h);
	Locker l(mutexLargeTexture);
	//Find the number of blocks needed for the given w and h
	uint32_t blocksW=(w+127)/128;
	uint32_t blocksH=(h+127)/128;
	TextureChunk ret(w, h);
	//Try to find a good place in the available textures
	uint32_t index=0;
	for(index=0;index<largeTextures.size();index++)
	{
		if(compact)
		{
			if(allocateChunkOnTextureCompact(largeTextures[index], ret, blocksW, blocksH))
			{
				ret.texId=index;
				return ret;
			}
		}
		else
		{
			if(allocateChunkOnTextureSparse(largeTextures[index], ret, blocksW, blocksH))
			{
				ret.texId=index;
				return ret;
			}
		}
	}
	//No place found, allocate a new one and try on that
	LargeTexture& tex=allocateNewTexture();
	bool done;
	if(compact)
		done=allocateChunkOnTextureCompact(tex, ret, blocksW, blocksH);
	else
		done=allocateChunkOnTextureSparse(tex, ret, blocksW, blocksH);
	if(!done)
	{
		//We were not able to allocate the whole surface on a single page
		LOG(LOG_NOT_IMPLEMENTED,"Support multi page surface allocation");
		ret.makeEmpty();
	}
	else
		ret.texId=index;
	return ret;
}

void RenderThread::renderTextured(const TextureChunk& chunk, int32_t x, int32_t y, uint32_t w, uint32_t h)
{
	glBindTexture(GL_TEXTURE_2D, largeTextures[chunk.texId].id);
	const uint32_t blocksPerSide=largeTextureSize/128;
	uint32_t startX, startY, endX, endY;
	assert(chunk.getNumberOfChunks()==((chunk.width+127)/128)*((chunk.height+127)/128));
	
	uint32_t curChunk=0;
	//The 4 corners of each texture are specified as the vertices of 2 triangles,
	//so there are 6 vertices per quad, two of them duplicated (the diagonal)
	GLint *vertex_coords = new GLint[chunk.getNumberOfChunks()*12];
	GLfloat *texture_coords = new GLfloat[chunk.getNumberOfChunks()*12];
	for(uint32_t i=0, k=0;i<chunk.height;i+=128)
	{
		startY=h*i/chunk.height;
		endY=min(h*(i+128)/chunk.height,h);
		for(uint32_t j=0;j<chunk.width;j+=128)
		{
			startX=w*j/chunk.width;
			endX=min(w*(j+128)/chunk.width,w);
			const uint32_t curChunkId=chunk.chunks[curChunk];
			const uint32_t blockX=((curChunkId%blocksPerSide)*128);
			const uint32_t blockY=((curChunkId/blocksPerSide)*128);
			const uint32_t availX=min(int(chunk.width-j),128);
			const uint32_t availY=min(int(chunk.height-i),128);
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
			vertex_coords[k] = x+startX;
			vertex_coords[k+1] = y+startY;
			k+=2;
			texture_coords[k] = endU;
			texture_coords[k+1] = startV;
			vertex_coords[k] = x+endX;
			vertex_coords[k+1] = y+startY;
			k+=2;
			texture_coords[k] = endU;
			texture_coords[k+1] = endV;
			vertex_coords[k] = x+endX;
			vertex_coords[k+1] = y+endY;
			k+=2;

			//Lower-left triangle of the quad
			texture_coords[k] = startU;
			texture_coords[k+1] = startV;
			vertex_coords[k] = x+startX;
			vertex_coords[k+1] = y+startY;
			k+=2;
			texture_coords[k] = endU;
			texture_coords[k+1] = endV;
			vertex_coords[k] = x+endX;
			vertex_coords[k+1] = y+endY;
			k+=2;
			texture_coords[k] = startU;
			texture_coords[k+1] = endV;
			vertex_coords[k] = x+startX;
			vertex_coords[k+1] = y+endY;
			k+=2;

			curChunk++;
		}
	}

	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_INT, GL_FALSE, 0, vertex_coords);
	glVertexAttribPointer(TEXCOORD_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, texture_coords);
	glEnableVertexAttribArray(VERTEX_ATTRIB);
	glEnableVertexAttribArray(TEXCOORD_ATTRIB);
	glDrawArrays(GL_TRIANGLES, 0, curChunk*6);
	glDisableVertexAttribArray(VERTEX_ATTRIB);
	glDisableVertexAttribArray(TEXCOORD_ATTRIB);
	delete[] vertex_coords;
	delete[] texture_coords;
}

void RenderThread::loadChunkBGRA(const TextureChunk& chunk, uint32_t w, uint32_t h, uint8_t* data)
{
	//Fast bailout if the TextureChunk is not valid
	if(chunk.chunks==NULL)
		return;
	glBindTexture(GL_TEXTURE_2D, largeTextures[chunk.texId].id);
	//TODO: Detect continuos
	//The size is ok if doesn't grow over the allocated size
	//this allows some alignment freedom
	assert(w<=((chunk.width+127)&0xffffff80));
	assert(h<=((chunk.height+127)&0xffffff80));
	const uint32_t numberOfChunks=chunk.getNumberOfChunks();
	const uint32_t blocksPerSide=largeTextureSize/128;
	const uint32_t blocksW=(w+127)/128;
	glPixelStorei(GL_UNPACK_ROW_LENGTH,w);
	for(uint32_t i=0;i<numberOfChunks;i++)
	{
		uint32_t curX=(i%blocksW)*128;
		uint32_t curY=(i/blocksW)*128;
		uint32_t sizeX=min(int(w-curX),128);
		uint32_t sizeY=min(int(h-curY),128);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS,curX);
		glPixelStorei(GL_UNPACK_SKIP_ROWS,curY);
		const uint32_t blockX=((chunk.chunks[i]%blocksPerSide)*128);
		const uint32_t blockY=((chunk.chunks[i]/blocksPerSide)*128);
		while(glGetError()!=GL_NO_ERROR);
		glTexSubImage2D(GL_TEXTURE_2D, 0, blockX, blockY, sizeX, sizeY, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_HOST, data);
		assert(glGetError()!=GL_INVALID_OPERATION);
	}
	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
}

void RenderThread::setMatrixUniform(LSGL_MATRIX m) const
{
	GLint uni = (m == LSGL_MODELVIEW) ? modelviewMatrixUniform:projectionMatrixUniform;

	glUniformMatrix4fv(uni, 1, GL_FALSE, lsMVPMatrix);
}
