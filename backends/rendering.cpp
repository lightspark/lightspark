/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <sstream>
//#include "swf.h"

#include <SDL.h>
#include <FTGL/ftgl.h>

#include <GL/glew.h>
#ifndef WIN32
#include <GL/glx.h>
#include <fontconfig/fontconfig.h>
#endif

using namespace lightspark;
using namespace std;
extern TLSDATA lightspark::SystemState* sys DLL_PUBLIC;
extern TLSDATA RenderThread* rt;

void RenderThread::wait()
{
	if(terminated)
		return;
	terminated=true;
	//Signal potentially blocking semaphore
	sem_post(&render);
	int ret=pthread_join(t,NULL);
	assert_and_throw(ret==0);
}

RenderThread::RenderThread(SystemState* s,ENGINE e,void* params):m_sys(s),terminated(false),inputNeeded(false),inputDisabled(false),
	resizeNeeded(false),newWidth(0),newHeight(0),scaleX(1),scaleY(1),offsetX(0),offsetY(0),interactive_buffer(NULL),tempBufferAcquired(false),
	frameCount(0),secsCount(0),mutexResources("GLResource Mutex"),dataTex(false),mainTex(false),tempTex(false),inputTex(false),
	hasNPOTTextures(false),selectedDebug(NULL),currentId(0),materialOverride(false)
{
	LOG(LOG_NO_INFO,"RenderThread this=" << this);
	m_sys=s;
	sem_init(&render,0,0);
	sem_init(&inputDone,0,0);

#ifdef WIN32
	fontPath = "TimesNewRoman.ttf";
#else
	FcPattern *pat, *match;
	FcResult result = FcResultMatch;
	char *font = NULL;

	pat = FcPatternCreate();
	FcPatternAddString(pat, FC_FAMILY, (const FcChar8 *)"Serif");
	FcConfigSubstitute(NULL, pat, FcMatchPattern);
	FcDefaultSubstitute(pat);
	match = FcFontMatch(NULL, pat, &result);
	FcPatternDestroy(pat);

	if (result != FcResultMatch)
	{
		LOG(LOG_ERROR,"Unable to find suitable Serif font");
		throw RunTimeException("Unable to find Serif font");
	}

	FcPatternGetString(match, FC_FILE, 0, (FcChar8 **) &font);
	fontPath = font;
	FcPatternDestroy(match);
	LOG(LOG_NO_INFO, "Font File is " << fontPath);
#endif

	if(e==SDL)
		pthread_create(&t,NULL,(thread_worker)sdl_worker,this);
#ifdef COMPILE_PLUGIN
	else if(e==GTKPLUG)
	{
		npapi_params=(NPAPI_params*)params;
		pthread_create(&t,NULL,(thread_worker)gtkplug_worker,this);
	}
#endif
	time_s = compat_get_current_time_ms();
}

RenderThread::~RenderThread()
{
	wait();
	sem_destroy(&render);
	sem_destroy(&inputDone);
	delete[] interactive_buffer;
	LOG(LOG_NO_INFO,"~RenderThread this=" << this);
}

void RenderThread::addResource(GLResource* res)
{
	managedResources.insert(res);
}

void RenderThread::removeResource(GLResource* res)
{
	managedResources.erase(res);
}

void RenderThread::acquireResourceMutex()
{
	mutexResources.lock();
}

void RenderThread::releaseResourceMutex()
{
	mutexResources.unlock();
}

void RenderThread::requestInput()
{
	inputNeeded=true;
	sem_post(&render);
	sem_wait(&inputDone);
}

void RenderThread::glAcquireTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax)
{
	assert(tempBufferAcquired==false);
	tempBufferAcquired=true;

	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	materialOverride=false;
	
	glDisable(GL_BLEND);
	glColor4f(0,0,0,0); //No output is fairly ok to clear
	glBegin(GL_QUADS);
		glVertex2f(xmin,ymin);
		glVertex2f(xmax,ymin);
		glVertex2f(xmax,ymax);
		glVertex2f(xmin,ymax);
	glEnd();
}

void RenderThread::glBlitTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax)
{
	assert(tempBufferAcquired==true);
	tempBufferAcquired=false;

	//Use the blittler program to blit only the used buffer
	glUseProgram(blitter_program);
	glEnable(GL_BLEND);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	rt->tempTex.bind();
	glBegin(GL_QUADS);
		glVertex2f(xmin,ymin);
		glVertex2f(xmax,ymin);
		glVertex2f(xmax,ymax);
		glVertex2f(xmin,ymax);
	glEnd();
	glUseProgram(gpu_program);
}

#ifdef COMPILE_PLUGIN
void* RenderThread::gtkplug_worker(RenderThread* th)
{
	sys=th->m_sys;
	rt=th;
	NPAPI_params* p=th->npapi_params;

	th->windowWidth=p->width;
	th->windowHeight=p->height;
	
	Display* d=XOpenDisplay(NULL);

	int a,b;
	Bool glx_present=glXQueryVersion(d,&a,&b);
	if(!glx_present)
	{
		LOG(LOG_ERROR,"glX not present");
		return NULL;
	}
	int attrib[10]={GLX_BUFFER_SIZE,24,GLX_DOUBLEBUFFER,True,None};
	GLXFBConfig* fb=glXChooseFBConfig(d, 0, attrib, &a);
	if(!fb)
	{
		attrib[2]=None;
		fb=glXChooseFBConfig(d, 0, attrib, &a);
		LOG(LOG_ERROR,"Falling back to no double buffering");
	}
	if(!fb)
	{
		LOG(LOG_ERROR,"Could not find any GLX configuration");
		::abort();
	}
	int i;
	for(i=0;i<a;i++)
	{
		int id;
		glXGetFBConfigAttrib(d,fb[i],GLX_VISUAL_ID,&id);
		if(id==(int)p->visual)
			break;
	}
	if(i==a)
	{
		//No suitable id found
		LOG(LOG_ERROR,"No suitable graphics configuration available");
		return NULL;
	}
	th->mFBConfig=fb[i];
	cout << "Chosen config " << hex << fb[i] << dec << endl;
	XFree(fb);

	th->mContext = glXCreateNewContext(d,th->mFBConfig,GLX_RGBA_TYPE ,NULL,1);
	GLXWindow glxWin=p->window;
	glXMakeCurrent(d, glxWin,th->mContext);
	if(!glXIsDirect(d,th->mContext))
		printf("Indirect!!\n");

	th->commonGLInit(th->windowWidth, th->windowHeight);
	th->commonGLResize(th->windowWidth, th->windowHeight);
	
	ThreadProfile* profile=sys->allocateProfiler(RGB(200,0,0));
	profile->setTag("Render");
	FTTextureFont font(th->fontPath.c_str());
	if(font.Error())
	{
		LOG(LOG_ERROR,"Unable to load serif font");
		throw RunTimeException("Unable to load font");
	}
	
	font.FaceSize(20);

	glEnable(GL_TEXTURE_2D);
	try
	{
		while(1)
		{
			sem_wait(&th->render);
			Chronometer chronometer;
			
			if(th->resizeNeeded)
			{
				if(th->windowWidth!=th->newWidth ||
					th->windowHeight!=th->newHeight)
				{
					th->windowWidth=th->newWidth;
					th->windowHeight=th->newHeight;
					LOG(LOG_ERROR,"Window resize not supported in plugin");
				}
				th->newWidth=0;
				th->newHeight=0;
				th->resizeNeeded=false;
				th->commonGLResize(th->windowWidth, th->windowHeight);
			}

			if(th->inputNeeded)
			{
				th->inputTex.bind();
				glGetTexImage(GL_TEXTURE_2D,0,GL_BGRA,GL_UNSIGNED_BYTE,th->interactive_buffer);
				th->inputNeeded=false;
				sem_post(&th->inputDone);
			}

			//Before starting rendering, cleanup all the request arrived in the meantime
			int fakeRenderCount=0;
			while(sem_trywait(&th->render)==0)
			{
				if(th->m_sys->isShuttingDown())
					break;
				fakeRenderCount++;
			}
			
			if(fakeRenderCount)
				LOG(LOG_NO_INFO,"Faking " << fakeRenderCount << " renderings");
			if(th->m_sys->isShuttingDown())
				break;

			if(th->m_sys->isOnError())
			{
				glUseProgram(0);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDrawBuffer(GL_BACK);
				glLoadIdentity();

				glClearColor(0,0,0,1);
				glClear(GL_COLOR_BUFFER_BIT);
				glColor3f(0.8,0.8,0.8);
					    
				font.Render("We're sorry, Lightspark encountered a yet unsupported Flash file",
					    -1,FTPoint(0,th->windowHeight/2));

				stringstream errorMsg;
				errorMsg << "SWF file: " << th->m_sys->getOrigin();
				font.Render(errorMsg.str().c_str(),
					    -1,FTPoint(0,th->windowHeight/2-20));
					    
				errorMsg.str("");
				errorMsg << "Cause: " << th->m_sys->errorCause;
				font.Render(errorMsg.str().c_str(),
					    -1,FTPoint(0,th->windowHeight/2-40));
				
				glFlush();
				glXSwapBuffers(d,glxWin);
			}
			else
			{
				glXSwapBuffers(d,glxWin);

				glBindFramebuffer(GL_FRAMEBUFFER, th->fboId);
				glDrawBuffer(GL_COLOR_ATTACHMENT0);

				RGB bg=sys->getBackground();
				glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,0);
				glClear(GL_COLOR_BUFFER_BIT);
				glLoadIdentity();
				glTranslatef(th->offsetX,th->offsetY,0);
				glScalef(th->scaleX,th->scaleY,1);
				
				sys->Render();

				glFlush();

				//Now draw the input layer
				if(!th->inputDisabled)
				{
					glDrawBuffer(GL_COLOR_ATTACHMENT2);
					glClearColor(0,0,0,0);
					glClear(GL_COLOR_BUFFER_BIT);

					th->materialOverride=true;
					th->m_sys->inputRender();
					th->materialOverride=false;
				}

				glLoadIdentity();

				//Now blit everything
				glLoadIdentity();
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDrawBuffer(GL_BACK);

				glClearColor(0,0,0,1);
				glClear(GL_COLOR_BUFFER_BIT);

				TextureBuffer* curBuf=((th->m_sys->showInteractiveMap)?&th->inputTex:&th->mainTex);
				curBuf->bind();
				curBuf->setTexScale(th->fragmentTexScaleUniform);
				glColor4f(0,0,1,0);
				glBegin(GL_QUADS);
					glTexCoord2f(0,1);
					glVertex2i(0,0);
					glTexCoord2f(1,1);
					glVertex2i(th->windowWidth,0);
					glTexCoord2f(1,0);
					glVertex2i(th->windowWidth,th->windowHeight);
					glTexCoord2f(0,0);
					glVertex2i(0,th->windowHeight);
				glEnd();

				if(sys->showProfilingData)
				{
					glUseProgram(0);
					glDisable(GL_TEXTURE_2D);

					//Draw bars
					glColor4f(0.7,0.7,0.7,0.7);
					glBegin(GL_LINES);
					for(int i=1;i<10;i++)
					{
						glVertex2i(0,(i*th->windowHeight/10));
						glVertex2i(th->windowWidth,(i*th->windowHeight/10));
					}
					glEnd();
				
					list<ThreadProfile>::iterator it=sys->profilingData.begin();
					for(;it!=sys->profilingData.end();it++)
						it->plot(1000000/sys->getFrameRate(),&font);

					glEnable(GL_TEXTURE_2D);
					glUseProgram(th->gpu_program);
				}
				//Call glFlush to offload work on the GPU
				glFlush();
			}
			profile->accountTime(chronometer.checkpoint());
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Exception in RenderThread " << e.what());
		sys->setError(e.cause);
	}
	glDisable(GL_TEXTURE_2D);
	//Before destroying the context shutdown all the GLResources
	set<GLResource*>::const_iterator it=th->managedResources.begin();
	for(;it!=th->managedResources.end();it++)
		(*it)->shutdown();
	th->commonGLDeinit();
	glXMakeCurrent(d,None,NULL);
	glXDestroyContext(d,th->mContext);
	XCloseDisplay(d);
	return NULL;
}
#endif

bool RenderThread::loadShaderPrograms()
{
	//Create render program
	assert(glCreateShader);
	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	
	const char *fs = NULL;
	fs = dataFileRead("lightspark.frag");
	if(fs==NULL)
	{
		LOG(LOG_ERROR,"Shader lightspark.frag not found");
		throw RunTimeException("Fragment shader code not found");
	}
	assert(glShaderSource);
	glShaderSource(f, 1, &fs,NULL);
	free((void*)fs);

	bool ret=true;
	char str[1024];
	int a;
	assert(glCompileShader);
	glCompileShader(f);
	assert(glGetShaderInfoLog);
	glGetShaderInfoLog(f,1024,&a,str);
	LOG(LOG_NO_INFO,"Fragment shader compilation " << str);

	assert(glCreateProgram);
	gpu_program = glCreateProgram();
	assert(glAttachShader);
	glAttachShader(gpu_program,f);

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

	fs = dataFileRead("lightspark.vert");
	if(fs==NULL)
	{
		LOG(LOG_ERROR,"Shader lightspark.vert not found");
		throw RunTimeException("Vertex shader code not found");
	}
	glShaderSource(v, 1, &fs,NULL);
	free((void*)fs);

	glCompileShader(v);
	glGetShaderInfoLog(v,1024,&a,str);
	LOG(LOG_NO_INFO,"Vertex shader compilation " << str);

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

float RenderThread::getIdAt(int x, int y)
{
	//TODO: use floating point textures
	uint32_t allocWidth=inputTex.getAllocWidth();
	return (interactive_buffer[y*allocWidth+x]&0xff)/255.0f;
}

void RenderThread::commonGLDeinit()
{
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	glDeleteFramebuffers(1,&rt->fboId);
	dataTex.shutdown();
	mainTex.shutdown();
	tempTex.shutdown();
	inputTex.shutdown();
}

void RenderThread::commonGLInit(int width, int height)
{
	//Now we can initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		LOG(LOG_ERROR,"Cannot initialize GLEW");
		cout << glewGetErrorString(err) << endl;
		::abort();
	}
	if(!GLEW_VERSION_2_0)
	{
		LOG(LOG_ERROR,"Video card does not support OpenGL 2.0... Aborting");
		::abort();
	}
	if(GLEW_ARB_texture_non_power_of_two)
		hasNPOTTextures=true;

	//Load shaders
	loadShaderPrograms();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glActiveTexture(GL_TEXTURE0);
	//Viewport setup and interactive_buffer allocation is left for GLResize	

	dataTex.init();

	mainTex.init(width, height, GL_NEAREST);

	tempTex.init(width, height, GL_NEAREST);

	inputTex.init(width, height, GL_NEAREST);

	//Set uniforms
	cleanGLErrors();
	glUseProgram(blitter_program);
	int texScale=glGetUniformLocation(blitter_program,"texScale");
	mainTex.setTexScale(texScale);
	cleanGLErrors();

	glUseProgram(gpu_program);
	cleanGLErrors();
	int tex=glGetUniformLocation(gpu_program,"g_tex1");
	glUniform1i(tex,0);
	fragmentTexScaleUniform=glGetUniformLocation(gpu_program,"texScale");
	glUniform2f(fragmentTexScaleUniform,1,1);
	cleanGLErrors();

	//Default to replace
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	// create a framebuffer object
	glGenFramebuffers(1, &fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, mainTex.getId(), 0);
	//Verify if we have more than an attachment available (1 is guaranteed)
	GLint numberOfAttachments=0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &numberOfAttachments);
	if(numberOfAttachments>=3)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D, tempTex.getId(), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,GL_TEXTURE_2D, inputTex.getId(), 0);
	}
	else
	{
		LOG(LOG_ERROR,"Non enough color attachments available, input disabled");
		inputDisabled=true;
	}
	
	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
	{
		LOG(LOG_ERROR,"Incomplete FBO status " << status << "... Aborting");
		while(err!=GL_NO_ERROR)
		{
			LOG(LOG_ERROR,"GL errors during initialization: " << err);
			err=glGetError();
		}
		::abort();
	}
}

void RenderThread::commonGLResize(int w, int h)
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,windowWidth,0,windowHeight,-100,0);

	glMatrixMode(GL_MODELVIEW);

	mainTex.resize(windowWidth, windowHeight);

	tempTex.resize(windowWidth, windowHeight);

	inputTex.resize(windowWidth, windowHeight);
	//Rellocate buffer for texture readback
	delete[] interactive_buffer;
	interactive_buffer=new uint32_t[inputTex.getAllocWidth()*inputTex.getAllocHeight()];
}

void RenderThread::requestResize(uint32_t w, uint32_t h)
{
	newWidth=w;
	newHeight=h;
	resizeNeeded=true;
	sem_post(&render);
}

void* RenderThread::sdl_worker(RenderThread* th)
{
	sys=th->m_sys;
	rt=th;
	RECT size=sys->getFrameSize();
	int initialWidth=size.Xmax/20;
	int initialHeight=size.Ymax/20;
	th->windowWidth=initialWidth;
	th->windowHeight=initialHeight;
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 0 );
	SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1); 
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	SDL_SetVideoMode(th->windowWidth, th->windowHeight, 24, SDL_OPENGL|SDL_RESIZABLE);
	th->commonGLInit(th->windowWidth, th->windowHeight);
	th->commonGLResize(th->windowWidth, th->windowHeight);

	ThreadProfile* profile=sys->allocateProfiler(RGB(200,0,0));
	profile->setTag("Render");
	FTTextureFont font(th->fontPath.c_str());
	if(font.Error())
		throw RunTimeException("Unable to load font");
	
	font.FaceSize(20);
	try
	{
		//Texturing must be enabled otherwise no tex coord will be sent to the shader
		glEnable(GL_TEXTURE_2D);
		Chronometer chronometer;
		while(1)
		{
			sem_wait(&th->render);
			chronometer.checkpoint();

			SDL_GL_SwapBuffers( );
			if(th->resizeNeeded)
			{
				if(th->windowWidth!=th->newWidth ||
					th->windowHeight!=th->newHeight)
				{
					th->windowWidth=th->newWidth;
					th->windowHeight=th->newHeight;
					SDL_SetVideoMode(th->windowWidth, th->windowHeight, 24, SDL_OPENGL|SDL_RESIZABLE);
				}
				th->newWidth=0;
				th->newHeight=0;
				th->resizeNeeded=false;
				th->commonGLResize(th->windowWidth, th->windowHeight);
			}

			if(th->inputNeeded)
			{
				th->inputTex.bind();
				glGetTexImage(GL_TEXTURE_2D,0,GL_BGRA,GL_UNSIGNED_BYTE,th->interactive_buffer);
				th->inputNeeded=false;
				sem_post(&th->inputDone);
			}

			//Before starting rendering, cleanup all the request arrived in the meantime
			int fakeRenderCount=0;
			while(sem_trywait(&th->render)==0)
			{
				if(th->m_sys->isShuttingDown())
					break;
				fakeRenderCount++;
			}

			if(fakeRenderCount)
				LOG(LOG_NO_INFO,"Faking " << fakeRenderCount << " renderings");

			if(th->m_sys->isShuttingDown())
				break;
			SDL_PumpEvents();

			if(th->m_sys->isOnError())
			{
				glUseProgram(0);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDrawBuffer(GL_BACK);
				glLoadIdentity();

				glClearColor(0,0,0,1);
				glClear(GL_COLOR_BUFFER_BIT);
				glColor3f(0.8,0.8,0.8);
					    
				font.Render("We're sorry, Lightspark encountered a yet unsupported Flash file",
						-1,FTPoint(0,th->windowHeight/2));

				stringstream errorMsg;
				errorMsg << "SWF file: " << th->m_sys->getOrigin();
				font.Render(errorMsg.str().c_str(),
						-1,FTPoint(0,th->windowHeight/2-20));
					    
				errorMsg.str("");
				errorMsg << "Cause: " << th->m_sys->errorCause;
				font.Render(errorMsg.str().c_str(),
						-1,FTPoint(0,th->windowHeight/2-40));

				font.Render("Press 'Q' to exit",-1,FTPoint(0,th->windowHeight/2-60));
				
				glFlush();
				SDL_GL_SwapBuffers( );
			}
			else
			{
				glBindFramebuffer(GL_FRAMEBUFFER, th->fboId);
				
				//Clear the back buffer
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				RGB bg=sys->getBackground();
				glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,1);
				glClear(GL_COLOR_BUFFER_BIT);
				
				glLoadIdentity();
				glTranslatef(th->offsetX,th->offsetY,0);
				glScalef(th->scaleX,th->scaleY,1);
				glTranslatef(th->m_sys->xOffset,th->m_sys->yOffset,0);
				
				th->m_sys->Render();

				glFlush();

				//Now draw the input layer
				if(!th->inputDisabled)
				{
					glDrawBuffer(GL_COLOR_ATTACHMENT2);
					glClearColor(0,0,0,0);
					glClear(GL_COLOR_BUFFER_BIT);
				
					th->materialOverride=true;
					th->m_sys->inputRender();
					th->materialOverride=false;
				}

				glFlush();
				glLoadIdentity();
				//Now blit everything
				glLoadIdentity();
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDrawBuffer(GL_BACK);
				glDisable(GL_BLEND);

				TextureBuffer* curBuf=((th->m_sys->showInteractiveMap)?&th->inputTex:&th->mainTex);
				curBuf->bind();
				curBuf->setTexScale(th->fragmentTexScaleUniform);
				glColor4f(0,0,1,0);
				glBegin(GL_QUADS);
					glTexCoord2f(0,1);
					glVertex2i(0,0);
					glTexCoord2f(1,1);
					glVertex2i(th->windowWidth,0);
					glTexCoord2f(1,0);
					glVertex2i(th->windowWidth,th->windowHeight);
					glTexCoord2f(0,0);
					glVertex2i(0,th->windowHeight);
				glEnd();
				
				if(th->m_sys->showDebug)
				{
					glUseProgram(0);
					glDisable(GL_TEXTURE_2D);
					if(th->selectedDebug)
						th->selectedDebug->debugRender(&font, true);
					else
						th->m_sys->debugRender(&font, true);
					glEnable(GL_TEXTURE_2D);
				}

				if(th->m_sys->showProfilingData)
				{
					glUseProgram(0);
					glColor3f(0,0,0);
					char frameBuf[20];
					snprintf(frameBuf,20,"Frame %u",th->m_sys->state.FP);
					font.Render(frameBuf,-1,FTPoint(0,0));

					//Draw bars
					glColor4f(0.7,0.7,0.7,0.7);
					glBegin(GL_LINES);
					for(int i=1;i<10;i++)
					{
						glVertex2i(0,(i*th->windowHeight/10));
						glVertex2i(th->windowWidth,(i*th->windowHeight/10));
					}
					glEnd();
					
					list<ThreadProfile>::iterator it=th->m_sys->profilingData.begin();
					for(;it!=th->m_sys->profilingData.end();it++)
						it->plot(1000000/sys->getFrameRate(),&font);
				}
				//Call glFlush to offload work on the GPU
				glFlush();
				glUseProgram(th->gpu_program);
				glEnable(GL_BLEND);
			}
			profile->accountTime(chronometer.checkpoint());
		}
		glDisable(GL_TEXTURE_2D);
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Exception in RenderThread " << e.cause);
		sys->setError(e.cause);
	}
	th->commonGLDeinit();
	return NULL;
}

void RenderThread::draw()
{
	sem_post(&render);
	time_d = compat_get_current_time_ms();
	uint64_t diff=time_d-time_s;
	if(diff>1000)
	{
		time_s=time_d;
		LOG(LOG_NO_INFO,"FPS: " << dec << frameCount);
		frameCount=0;
		secsCount++;
	}
	else
		frameCount++;
}

void RenderThread::tick()
{
	draw();
}

