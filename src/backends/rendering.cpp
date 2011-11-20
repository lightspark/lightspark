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

//The interpretation of texture data change with the endianness
#if __BYTE_ORDER == __BIG_ENDIAN
#define GL_UNSIGNED_INT_8_8_8_8_HOST GL_UNSIGNED_INT_8_8_8_8_REV
#else
#define GL_UNSIGNED_INT_8_8_8_8_HOST GL_UNSIGNED_BYTE
#endif

using namespace lightspark;
using namespace std;

/* calulcate FPS every second */
Glib::TimeVal RenderThread::FPS_time(/*seconds*/1,/*microseconds*/0);

static GStaticPrivate renderThread = G_STATIC_PRIVATE_INIT; /* TLS */
RenderThread* lightspark::getRenderThread()
{
	RenderThread* ret = (RenderThread*)g_static_private_get(&renderThread);
	/* If this is NULL, then you are not calling from the render thread,
	 * which is disallowed! (OpenGL is not threadsafe)
	 */
	assert(ret);
	return ret;
}

void RenderThread::wait()
{
	if(status==STARTED)
	{
		//Signal potentially blocking semaphore
		event.signal();
		t->join();
		status=TERMINATED;
	}
}

RenderThread::RenderThread(SystemState* s):
	m_sys(s),status(CREATED),currentPixelBuffer(0),currentPixelBufferOffset(0),
	pixelBufferWidth(0),pixelBufferHeight(0),prevUploadJob(NULL),largeTextureSize(0),
	renderNeeded(false),uploadNeeded(false),resizeNeeded(false),newTextureNeeded(false),event(0),newWidth(0),newHeight(0),scaleX(1),scaleY(1),
	offsetX(0),offsetY(0),tempBufferAcquired(false),frameCount(0),secsCount(0),initialized(0),
	tempTex(false),hasNPOTTextures(false),cairoTextureContext(NULL)
{
	LOG(LOG_INFO,_("RenderThread this=") << this);
	
	m_sys=s;

#ifdef _WIN32
	fontPath = "TimesNewRoman.ttf";
#else
	fontPath = "Serif";
#endif
	time_s.assign_current_time();
}

/* this is called in the context of the gtk main thread */
void RenderThread::start(EngineData* data)
{
	status=STARTED;
	engineData=data;
	/* this function must be called in the gtk main thread */
	engineData->setSizeChangeHandler(sigc::mem_fun(this,&RenderThread::requestResize));
	t = Thread::create(sigc::bind<0>(&RenderThread::worker,this), true);
}

void RenderThread::stop()
{
	initialized.signal();
}

RenderThread::~RenderThread()
{
	wait();
	LOG(LOG_INFO,_("~RenderThread this=") << this);
}

/*void RenderThread::acquireTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax)
{
	::abort();
	GLfloat vertex_coords[8];
	static GLfloat color_coords[16];
	assert(tempBufferAcquired==false);
	tempBufferAcquired=true;

	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
	vertex_coords[0] = xmin;vertex_coords[1] = ymin;
	vertex_coords[2] = xmax;vertex_coords[3] = ymin;
	vertex_coords[4] = xmin;vertex_coords[5] = ymax;
	vertex_coords[6] = xmax;vertex_coords[7] = ymax;

	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, vertex_coords);
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
	GLfloat vertex_coords[8];
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

	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, vertex_coords);
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

#ifdef ENABLE_GLES2
uint8_t* pixelBuf = 0;
#endif

void RenderThread::finalizeUpload()
{
	ITextureUploadable* u=prevUploadJob;
	uint32_t w,h;
	u->sizeNeeded(w,h);
	const TextureChunk& tex=u->getTexture();
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffers[currentPixelBuffer]);
#ifndef ENABLE_GLES2
	//Copy content of the pbo to the texture, currentPixelBufferOffset is the offset in the pbo
	loadChunkBGRA(tex, w, h, (uint8_t*)currentPixelBufferOffset);
#else
	loadChunkBGRA(tex, w, h, pixelBuf);
#endif
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
#ifndef ENABLE_GLES2
	unsigned int nextBuffer = (currentPixelBuffer + 1)%2;
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffers[nextBuffer]);
	uint8_t* buf=(uint8_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER,GL_WRITE_ONLY);
	uint8_t* alignedBuf=(uint8_t*)(uintptr_t((buf+15))&(~0xfL));

	u->upload(alignedBuf, w, h);

	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	currentPixelBufferOffset=alignedBuf-buf;
	currentPixelBuffer=nextBuffer;
#else
	//TODO See if a more elegant way of handling the non-PBO case can be found.
	//for now, each frame is uploaded one at a time synchronously to the server
	if(!pixelBuf)
		if(posix_memalign((void **)&pixelBuf, 16, w*h*4)) {
			LOG(LOG_ERROR, "posix_memalign could not allocate memory");
			return;
		}
	u->upload(pixelBuf, w, h);
#endif
	//Get the texture to be sure it's allocated when the upload comes
	u->getTexture();
	prevUploadJob=u;
}

void RenderThread::worker(RenderThread* th)
{
	setTLSSys(th->m_sys);
	/* set TLS variable for getRenderThread() */
	g_static_private_set(&renderThread,th,NULL);

	EngineData* e=th->engineData;
	SemaphoreLighter lighter(th->initialized);

	th->windowWidth=e->width;
	th->windowHeight=e->height;

#if defined(_WIN32)
	HGLRC           hRC=NULL;                           // Permanent Rendering Context
	HDC             hDC=NULL;                           // Private GDI Device Context
	PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
			PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
			32,                        //Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                        //Number of bits for the depthbuffer
			0,                        //Number of bits for the stencilbuffer
			0,                        //Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};
	if(!(hDC = GetDC(e->window)))
	{
		LOG(LOG_ERROR,"GetDC failed");
		return;
	}
	int PixelFormat;
	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))
	{
		LOG(LOG_ERROR,"ChoosePixelFormat failed");
		return;
	}
	if(!SetPixelFormat(hDC,PixelFormat,&pfd))
	{
		LOG(LOG_ERROR,"SetPixelFormat failed");
		return;
	}
	if (!(hRC=wglCreateContext(hDC)))
	{
		LOG(LOG_ERROR,"wglCreateContext failed");
		return;
	}
	if(!wglMakeCurrent(hDC,hRC))
	{
		LOG(LOG_ERROR,"wglMakeCurrent failed");
		return;
	}
#elif !defined(ENABLE_GLES2)
	Display* d=XOpenDisplay(NULL);
	int a,b;
	Bool glx_present=glXQueryVersion(d,&a,&b);
	if(!glx_present)
	{
		LOG(LOG_ERROR,_("glX not present"));
		return;
	}
	int attrib[10]={GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DOUBLEBUFFER, True, None};
	GLXFBConfig* fb=glXChooseFBConfig(d, 0, attrib, &a);
	if(!fb)
	{
		attrib[6]=None;
		LOG(LOG_ERROR,_("Falling back to no double buffering"));
		fb=glXChooseFBConfig(d, 0, attrib, &a);
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
		return;
	}
	th->mFBConfig=fb[i];
	LOG(LOG_INFO, "Chosen config " << hex << fb[i] << dec);
	XFree(fb);

	th->mContext = glXCreateNewContext(d,th->mFBConfig,GLX_RGBA_TYPE ,NULL,1);
	GLXWindow glxWin=e->window;
	glXMakeCurrent(d, glxWin,th->mContext);
	if(!glXIsDirect(d,th->mContext))
		LOG(LOG_INFO, "Indirect!!");
#else //egl
	Display* d=XOpenDisplay(NULL);
	int a;
	eglBindAPI(EGL_OPENGL_ES_API);
	EGLDisplay ed = EGL_NO_DISPLAY;
	ed = eglGetDisplay(d);

	if (ed == EGL_NO_DISPLAY) {
		LOG(LOG_ERROR, _("EGL not present"));
		return;
	}

	EGLint major, minor;
	if (eglInitialize(ed, &major, &minor) == EGL_FALSE) {
		LOG(LOG_ERROR, _("EGL initialization failed"));
		return;
	}

	LOG(LOG_INFO, _("EGL version: ") << eglQueryString(ed, EGL_VERSION));

	EGLint config_attribs[] = {
				EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
				EGL_RED_SIZE, 8,
				EGL_GREEN_SIZE, 8,
				EGL_BLUE_SIZE, 8,
				EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
				EGL_NONE
	};

	EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	if (!eglChooseConfig(ed, config_attribs, 0, 0, &a)) {
		LOG(LOG_ERROR,_("Could not get number of EGL configurations"));
	} else {
	    LOG(LOG_INFO, "Number of EGL configurations: " << a);
	}
	EGLConfig *conf = new EGLConfig[a];
	if (!eglChooseConfig(ed, config_attribs, conf, a, &a))
	{
		LOG(LOG_ERROR,_("Could not find any EGL configuration"));
		::abort();
	}

	int i;
	for(i=0;i<a;i++)
	{
		EGLint id;
		eglGetConfigAttrib(ed, conf[i], EGL_NATIVE_VISUAL_ID, &id);
		if(id==(int)e->visual)
			break;
	}
	if(i==a)
	{
		//No suitable id found
		LOG(LOG_ERROR,_("No suitable graphics configuration available"));
		return;
	}
	th->mEGLConfig=conf[i];
	LOG(LOG_INFO, "Chosen config " << hex << conf[i] << dec);

	th->mEGLContext = eglCreateContext(ed, th->mEGLConfig, EGL_NO_CONTEXT, context_attribs);
	if (th->mEGLContext == EGL_NO_CONTEXT) {
		LOG(LOG_ERROR,_("Could not create EGL context"));
		return;
	}
	EGLSurface win = eglCreateWindowSurface(ed, th->mEGLConfig, e->window, NULL);
	if (win == EGL_NO_SURFACE) {
		LOG(LOG_ERROR,_("Could not create EGL surface"));
		return;
	}
	eglMakeCurrent(ed, win, win, th->mEGLContext);
#endif

	th->commonGLInit(th->windowWidth, th->windowHeight);
	th->commonGLResize();
	lighter.light();
	
	ThreadProfile* profile=th->m_sys->allocateProfiler(RGB(200,0,0));
	profile->setTag("Render");

	glEnable(GL_TEXTURE_2D);
	try
	{
		Chronometer chronometer;
		while(1)
		{
			th->event.wait();
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
				LOG(LOG_INFO,_("Window resized to ") << th->windowWidth << 'x' << th->windowHeight);
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
			}

#if defined(_WIN32)
			SwapBuffers(hDC);
#elif !defined(ENABLE_GLES2)
			glXSwapBuffers(d,glxWin);
#else
			eglSwapBuffers(ed, win);
#endif
			if(!th->m_sys->isOnError())
			{
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
		th->m_sys->setError(e.cause);
	}
	glDisable(GL_TEXTURE_2D);
	th->commonGLDeinit();

#if defined(_WIN32)
	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(hRC);
	/* Do not ReleaseDC(e->window,hDC); as our window does not have CS_OWNDC */
#elif !defined(ENABLE_GLES2)
	glXMakeCurrent(d,None,NULL);
	glXDestroyContext(d,th->mContext);
	XCloseDisplay(d);
#else
	eglMakeCurrent(ed, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(ed, th->mEGLContext);
	XCloseDisplay(d);
#endif
	e->removeSizeChangeHandler();
	return;
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
	GLint stat;
	assert(glCompileShader);
	glCompileShader(f);
	assert(glGetShaderInfoLog);
	glGetShaderInfoLog(f,1024,&a,str);
	LOG(LOG_INFO,_("Fragment shader compilation ") << str);
	glGetShaderiv(f, GL_COMPILE_STATUS, &stat);
	if (!stat)
	{
		throw RunTimeException("Could not compile fragment shader");
	}

	fs = dataFileRead("lightspark.vert");
	if(fs==NULL)
	{
		LOG(LOG_ERROR,_("Shader lightspark.vert not found"));
		throw RunTimeException("Vertex shader code not found");
	}
	glShaderSource(g, 1, &fs,NULL);
	free((void*)fs);

	glGetShaderInfoLog(g,1024,&a,str);
	LOG(LOG_INFO,_("Vertex shader compilation ") << str);

	glCompileShader(g);
	glGetShaderiv(g, GL_COMPILE_STATUS, &stat);
	if (!stat)
	{
		throw RunTimeException("Could not compile vertex shader");
	}

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

	glGetShaderInfoLog(v,1024,&a,str);
	LOG(LOG_INFO,_("Vertex shader compilation ") << str);

	glCompileShader(v);
	glGetShaderiv(v, GL_COMPILE_STATUS, &stat);
	if (!stat)
	{
		throw RunTimeException("Could not compile vertex blitter shader");
	}

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
	GLenum err;
//For now GLEW does not work with GLES2
#ifndef ENABLE_GLES2
	//Now we can initialize GLEW
	glewExperimental = GL_TRUE;
	err = glewInit();
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
	if(!GLEW_ARB_framebuffer_object)
	{
		LOG(LOG_ERROR,"OpenGL does not support framebuffer objects!");
		::abort();
	}
#else
		//Open GLES 2.0 has NPOT textures
		hasNPOTTextures=true;
#endif
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
	//The uniform that tells to draw directly using the selected color
	directUniform =glGetUniformLocation(gpu_program,"direct");
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
		err=glGetError();
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
	event.signal();
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
#ifdef ENABLE_GLES2
	if (pixelBuf) {
		free(pixelBuf);
		pixelBuf = 0;
	}
#endif
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

	GLfloat vertex_coords[] = {0,0, GLfloat(w),0, 0,GLfloat(h), GLfloat(w),GLfloat(h)};
	GLfloat texture_coords[] = {0,0, 1,0, 0,1, 1,1};
	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, vertex_coords);
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

	glUniform1f(directUniform, 1);

	char frameBuf[20];
	snprintf(frameBuf,20,"Frame %u",m_sys->state.FP);

	GLfloat vertex_coords[40];
	GLfloat color_coords[80];

	//Draw bars
	for (int i=0;i<9;i++)
	{
		vertex_coords[i*4] = 0;
		vertex_coords[i*4+1] = (i+1)*windowHeight/10;
		vertex_coords[i*4+2] = windowWidth;
		vertex_coords[i*4+3] = (i+1)*windowHeight/10;
	}
	for (int i=0;i<80;i++)
		color_coords[i] = 0.7;

	glVertexAttribPointer(VERTEX_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, vertex_coords);
	glVertexAttribPointer(COLOR_ATTRIB, 4, GL_FLOAT, GL_FALSE, 0, color_coords);
	glEnableVertexAttribArray(VERTEX_ATTRIB);
	glEnableVertexAttribArray(COLOR_ATTRIB);
	glDrawArrays(GL_LINES, 0, 20);
	glDisableVertexAttribArray(VERTEX_ATTRIB);
	glDisableVertexAttribArray(COLOR_ATTRIB);
 
	list<ThreadProfile*>::iterator it=m_sys->profilingData.begin();
	for(;it!=m_sys->profilingData.end();it++)
		(*it)->plot(1000000/m_sys->getFrameRate(),cr);
	glUniform1f(directUniform, 0);

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
	RGB bg=m_sys->getBackground();
	glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,1);
	glClear(GL_COLOR_BUFFER_BIT);
	lsglLoadIdentity();
	setMatrixUniform(LSGL_MODELVIEW);

	m_sys->getStage()->Render(false);
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

	glUniform1f(alphaUniform, 1);
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
	event.signal();
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
	event.signal();
	time_d.assign_current_time();
	Glib::TimeVal diff=time_d-time_s-FPS_time;
	if(!diff.negative()) /* is one seconds elapsed? */
	{
		time_s=time_d;
		LOG(LOG_INFO,_("FPS: ") << dec << frameCount);
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
	uint32_t blocksW=(chunk.width+CHUNKSIZE-1)/CHUNKSIZE;
	uint32_t blocksH=(chunk.height+CHUNKSIZE-1)/CHUNKSIZE;
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
	//Let's allocate the bitmap for the texture blocks, minumum block size is CHUNKSIZE
	uint32_t bitmapSize=(largeTextureSize/CHUNKSIZE)*(largeTextureSize/CHUNKSIZE)/8;
	uint8_t* bitmap=new uint8_t[bitmapSize];
	memset(bitmap,0,bitmapSize);
	largeTextures.emplace_back(bitmap);
	return largeTextures.back();
}

bool RenderThread::allocateChunkOnTextureCompact(LargeTexture& tex, TextureChunk& ret, uint32_t blocksW, uint32_t blocksH)
{
	//Find a contiguos set of blocks
	uint32_t start;
	uint32_t blockPerSide=largeTextureSize/CHUNKSIZE;
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
	uint32_t blockPerSide=largeTextureSize/CHUNKSIZE;
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
	uint32_t blocksW=(w+CHUNKSIZE-1)/CHUNKSIZE;
	uint32_t blocksH=(h+CHUNKSIZE-1)/CHUNKSIZE;
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
	const uint32_t blocksPerSide=largeTextureSize/CHUNKSIZE;
	uint32_t startX, startY, endX, endY;
	assert(chunk.getNumberOfChunks()==((chunk.width+CHUNKSIZE-1)/CHUNKSIZE)*((chunk.height+CHUNKSIZE-1)/CHUNKSIZE));
	
	uint32_t curChunk=0;
	//The 4 corners of each texture are specified as the vertices of 2 triangles,
	//so there are 6 vertices per quad, two of them duplicated (the diagonal)
	GLfloat *vertex_coords = new GLfloat[chunk.getNumberOfChunks()*12];
	GLfloat *texture_coords = new GLfloat[chunk.getNumberOfChunks()*12];
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
	assert(w<=((chunk.width+CHUNKSIZE-1)&0xffffff80));
	assert(h<=((chunk.height+CHUNKSIZE-1)&0xffffff80));
	const uint32_t numberOfChunks=chunk.getNumberOfChunks();
	const uint32_t blocksPerSide=largeTextureSize/CHUNKSIZE;
	const uint32_t blocksW=(w+CHUNKSIZE-1)/CHUNKSIZE;
	glPixelStorei(GL_UNPACK_ROW_LENGTH,w);
	for(uint32_t i=0;i<numberOfChunks;i++)
	{
		uint32_t curX=(i%blocksW)*CHUNKSIZE;
		uint32_t curY=(i/blocksW)*CHUNKSIZE;
		uint32_t sizeX=min(int(w-curX),CHUNKSIZE);
		uint32_t sizeY=min(int(h-curY),CHUNKSIZE);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS,curX);
		glPixelStorei(GL_UNPACK_SKIP_ROWS,curY);
		const uint32_t blockX=((chunk.chunks[i]%blocksPerSide)*CHUNKSIZE);
		const uint32_t blockY=((chunk.chunks[i]/blocksPerSide)*CHUNKSIZE);
		while(glGetError()!=GL_NO_ERROR);
#ifndef ENABLE_GLES2
		glTexSubImage2D(GL_TEXTURE_2D, 0, blockX, blockY, sizeX, sizeY, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_HOST, data);
#else
		//We need to copy the texture area to a contiguous memory region first,
		//as GLES2 does not support UNPACK state (skip pixels, skip rows, row_lenght).
		uint8_t *gdata = new uint8_t[4*sizeX*sizeY];
		for(unsigned int j=0;j<sizeY;j++) {
			memcpy(gdata+4*j*sizeX, data+4*w*(j+curY)+4*curX, sizeX*4);
		}
		glTexSubImage2D(GL_TEXTURE_2D, 0, blockX, blockY, sizeX, sizeY, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_HOST, gdata);
		delete[] gdata;
#endif
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
