/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "backends/rendering.h"
#include "compat.h"
#include <sstream>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif
//None is #defined by X11/X.h, but because it conflicts with libxml++
//headers Lightspark undefines it elsewhere except in this file.
#ifndef None
#define None 0L
#endif

using namespace lightspark;
using namespace std;

/* calculate FPS every second */
const Glib::TimeVal RenderThread::FPS_time(/*seconds*/1,/*microseconds*/0);

DEFINE_AND_INITIALIZE_TLS(renderThread);
RenderThread* lightspark::getRenderThread()
{
	RenderThread* ret = (RenderThread*)tls_get(&renderThread);
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
	}
}

RenderThread::RenderThread(SystemState* s):GLRenderContext(),
	m_sys(s),status(CREATED),
	prevUploadJob(NULL),
	renderNeeded(false),uploadNeeded(false),resizeNeeded(false),newTextureNeeded(false),event(0),newWidth(0),newHeight(0),scaleX(1),scaleY(1),
	offsetX(0),offsetY(0),tempBufferAcquired(false),frameCount(0),secsCount(0),initialized(0),
	cairoTextureContext(NULL)
{
	LOG(LOG_INFO,_("RenderThread this=") << this);
#ifdef _WIN32
	fontPath = "TimesNewRoman.ttf";
#else
	fontPath = "Serif";
#endif
	time_s.assign_current_time();
}

void RenderThread::start(EngineData* data)
{
	status=STARTED;
	engineData=data;
#ifdef HAVE_NEW_GLIBMM_THREAD_API
	t = Thread::create(sigc::mem_fun(this,&RenderThread::worker));
#else
	t = Thread::create(sigc::mem_fun(this,&RenderThread::worker),true);
#endif
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

void RenderThread::handleNewTexture()
{
	//Find if any largeTexture is not initialized
	Locker l(mutexLargeTexture);
	for(uint32_t i=0;i<largeTextures.size();i++)
	{
		if(largeTextures[i].id==(uint32_t)-1)
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
	engineData->bindCurrentBuffer();
	loadChunkBGRA(tex, w, h, engineData->getCurrentPixBuf());
	engineData->exec_glBindBuffer_GL_PIXEL_UNPACK_BUFFER(0);
	u->uploadFence();
	prevUploadJob=NULL;
}

void RenderThread::handleUpload()
{
	ITextureUploadable* u=getUploadJob();
	assert(u);
	uint32_t w,h;
	u->sizeNeeded(w,h);
	engineData->resizePixelBuffers(w,h);
	
	uint8_t* buf= engineData->switchCurrentPixBuf(w,h);
	if (!buf)
	{
		handleGLErrors();
		return;
	}
	u->upload(buf, w, h);
	
	engineData->exec_glUnmapBuffer_GL_PIXEL_UNPACK_BUFFER();
	engineData->exec_glBindBuffer_GL_PIXEL_UNPACK_BUFFER(0);

	
	//Get the texture to be sure it's allocated when the upload comes
	u->getTexture();
	prevUploadJob=u;
}

/*
 * Create an OpenGL context, load shaders and setup FBO
 */
void RenderThread::init()
{
	/* This will call initialized.signal() when lighter goes out of scope */
	SemaphoreLighter lighter(initialized);

	windowWidth=engineData->width;
	windowHeight=engineData->height;

	engineData->InitOpenGL();
	commonGLInit(windowWidth, windowHeight);
	commonGLResize();
	
}

void RenderThread::worker()
{
	setTLSSys(m_sys);
	/* set TLS variable for getRenderThread() */
	tls_set(&renderThread, this);

	ThreadProfile* profile=m_sys->allocateProfiler(RGB(200,0,0));
	profile->setTag("Render");
	try
	{
		init();

		ThreadProfile* profile=m_sys->allocateProfiler(RGB(200,0,0));
		profile->setTag("Render");

		engineData->exec_glEnable_GL_TEXTURE_2D();

		Chronometer chronometer;
		while(1)
		{
			if (!doRender(profile,&chronometer))
				break;
		}

		deinit();
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,_("Exception in RenderThread, stopping rendering: ") << e.what());
		//TODO: add a comandline switch to disable rendering. Then add that commandline switch
		//to the test runner script and uncomment the next line
		//m_sys->setError(e.cause);
	}

	/* cleanup */
	//Keep addUploadJob from enqueueing
	status=TERMINATED;
	//Fence existing jobs
	Locker l(mutexUploadJobs);
	if(prevUploadJob)
		prevUploadJob->uploadFence();
	for(auto i=uploadJobs.begin(); i != uploadJobs.end(); ++i)
		(*i)->uploadFence();
}
bool RenderThread::doRender(ThreadProfile* profile,Chronometer* chronometer)
{
	event.wait();
	if(m_sys->isShuttingDown())
		return false;
	if (chronometer)
		chronometer->checkpoint();

	if(resizeNeeded)
	{
		//Order of the operations here matters for requestResize
		windowWidth=newWidth;
		windowHeight=newHeight;
		resizeNeeded=false;
		newWidth=0;
		newHeight=0;
		//End of order critical part
		LOG(LOG_INFO,_("Window resized to ") << windowWidth << 'x' << windowHeight);
		commonGLResize();
		m_sys->resizeCompleted();
		if (profile && chronometer)
			profile->accountTime(chronometer->checkpoint());
		return true;
	}
	if(newTextureNeeded)
		handleNewTexture();

	if(prevUploadJob)
		finalizeUpload();

	if(uploadNeeded)
	{
		handleUpload();
		if (profile && chronometer)
			profile->accountTime(chronometer->checkpoint());
		return true;
	}

	if(m_sys->isOnError())
	{
		renderErrorPage(this, m_sys->standalone);
	}
	if(!m_sys->isOnError())
	{
		coreRendering();
		//Call glFlush to offload work on the GPU
		engineData->exec_glFlush();
	}
	engineData->DoSwapBuffers();
	if (profile && chronometer)
		profile->accountTime(chronometer->checkpoint());
	renderNeeded=false;
	
	return true;
}
void RenderThread::deinit()
{
	engineData->exec_glDisable_GL_TEXTURE_2D();
	commonGLDeinit();
	engineData->DeinitOpenGL();
}

bool RenderThread::loadShaderPrograms()
{
	//Create render program
	uint32_t f = engineData->exec_glCreateShader_GL_FRAGMENT_SHADER();
	
	// directly include shader source to avoid filesystem access
	const char *fs = 
#include "lightspark.frag"
;
	engineData->exec_glShaderSource(f, 1, &fs,NULL);
	uint32_t g = engineData->exec_glCreateShader_GL_VERTEX_SHADER();
	
	bool ret=true;
	char str[1024];
	int a;
	int stat;
	engineData->exec_glCompileShader(f);
	engineData->exec_glGetShaderInfoLog(f,1024,&a,str);
	LOG(LOG_INFO,_("Fragment shader compilation ") << str);
	engineData->exec_glGetShaderiv_GL_COMPILE_STATUS(f, &stat);
	if (!stat)
	{
		throw RunTimeException("Could not compile fragment shader");
	}

	// directly include shader source to avoid filesystem access
	const char *fs2 = 
#include "lightspark.vert"
;
	engineData->exec_glShaderSource(g, 1, &fs2,NULL);

	engineData->exec_glGetShaderInfoLog(g,1024,&a,str);
	LOG(LOG_INFO,_("Vertex shader compilation ") << str);

	engineData->exec_glCompileShader(g);
	engineData->exec_glGetShaderiv_GL_COMPILE_STATUS(g, &stat);
	if (!stat)
	{
		throw RunTimeException("Could not compile vertex shader");
	}

	gpu_program = engineData->exec_glCreateProgram();
	engineData->exec_glBindAttribLocation(gpu_program, VERTEX_ATTRIB, "ls_Vertex");
	engineData->exec_glBindAttribLocation(gpu_program, COLOR_ATTRIB, "ls_Color");
	engineData->exec_glBindAttribLocation(gpu_program, TEXCOORD_ATTRIB, "ls_TexCoord");
	engineData->exec_glAttachShader(gpu_program,f);
	engineData->exec_glAttachShader(gpu_program,g);

	engineData->exec_glLinkProgram(gpu_program);

	engineData->exec_glGetProgramiv_GL_LINK_STATUS(gpu_program,&a);
	if(!a)
	{
		ret=false;
		return ret;
	}
	
	assert(ret);
	return true;
}

void RenderThread::commonGLDeinit()
{
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(0);
	for(uint32_t i=0;i<largeTextures.size();i++)
	{
		engineData->exec_glDeleteTextures(1,&largeTextures[i].id);
		delete[] largeTextures[i].bitmap;
	}
	engineData->exec_glDeleteBuffers();
	engineData->exec_glDeleteTextures(1, &cairoTextureID);
}

void RenderThread::commonGLInit(int width, int height)
{
	//Load shaders
	loadShaderPrograms();

	engineData->exec_glBlendFunc_GL_ONE_GL_ONE_MINUS_SRC_ALPHA();
	engineData->exec_glEnable_GL_BLEND();

	engineData->exec_glActiveTexture_GL_TEXTURE0();
	//Viewport setup is left for GLResize	

	//Get the maximum allowed texture size, up to 1024
	int maxTexSize;
	engineData->exec_glGetIntegerv_GL_MAX_TEXTURE_SIZE(&maxTexSize);
	assert(maxTexSize>0);
	largeTextureSize=min(maxTexSize,1024);

	//Create the PBOs
	engineData->exec_glGenBuffers();

	//Set uniforms
	engineData->exec_glUseProgram(gpu_program);
	int tex=engineData->exec_glGetUniformLocation(gpu_program,"g_tex1");
	if(tex!=-1)
		engineData->exec_glUniform1i(tex,0);
	tex=engineData->exec_glGetUniformLocation(gpu_program,"g_tex2");
	if(tex!=-1)
		engineData->exec_glUniform1i(tex,1);

	//The uniform that enables YUV->RGB transform on the texels (needed for video)
	yuvUniform =engineData->exec_glGetUniformLocation(gpu_program,"yuv");
	//The uniform that tells the alpha value multiplied to the alpha of every pixel
	alphaUniform =engineData->exec_glGetUniformLocation(gpu_program,"alpha");
	//The uniform that tells to draw directly using the selected color
	directUniform =engineData->exec_glGetUniformLocation(gpu_program,"direct");
	//The uniform that contains the coordinate matrix
	projectionMatrixUniform =engineData->exec_glGetUniformLocation(gpu_program,"ls_ProjectionMatrix");
	modelviewMatrixUniform =engineData->exec_glGetUniformLocation(gpu_program,"ls_ModelViewMatrix");

	fragmentTexScaleUniform=engineData->exec_glGetUniformLocation(gpu_program,"texScale");

	//Texturing must be enabled otherwise no tex coord will be sent to the shaders
	engineData->exec_glEnable_GL_TEXTURE_2D();

	engineData->exec_glGenTextures(1, &cairoTextureID);

	if(handleGLErrors())
	{
		LOG(LOG_ERROR,_("GL errors during initialization"));
	}
}

void RenderThread::commonGLResize()
{
	m_sys->stageCoordinateMapping(windowWidth, windowHeight, offsetX, offsetY, scaleX, scaleY);
	engineData->exec_glViewport(0,0,windowWidth,windowHeight);
	lsglLoadIdentity();
	lsglOrtho(0,windowWidth,0,windowHeight,-100,0);
	//scaleY is negated to adapt the flash and gl coordinates system
	//An additional translation is added for the same reason
	lsglTranslatef(offsetX,windowHeight-offsetY,0);
	lsglScalef(scaleX,-scaleY,1);
	setMatrixUniform(LSGL_PROJECTION);
}

void RenderThread::requestResize(uint32_t w, uint32_t h, bool force)
{
	//We can skip the resize if the current size is correct
	//and there is no pending resize or if there is already a
	//pending resize with the correct size

	//This test is correct only if the order of operation where
	//the resize is handled does not change!
	if(!force &&
		((windowWidth==w && windowHeight==h && resizeNeeded==false) ||
		 (newWidth==w && newHeight==h)))
	{
		return;
	}
	newWidth=w;
	newHeight=h;
	resizeNeeded=true;
	event.signal();
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

void RenderThread::waitRendering()
{
	Locker l(mutexRendering);
}

//Send the texture drawn by Cairo to the GPU
void RenderThread::mapCairoTexture(int w, int h)
{
	engineData->exec_glEnable_GL_TEXTURE_2D();
	engineData->exec_glBindTexture_GL_TEXTURE_2D(cairoTextureID);

	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
	engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, w, h, 0, cairoTextureData);

	float vertex_coords[] = {0,0, float(w),0, 0,float(h), float(w),float(h)};
	float texture_coords[] = {0,0, 1,0, 0,1, 1,1};
	engineData->exec_glVertexAttribPointer(VERTEX_ATTRIB, 2, 0, vertex_coords);
	engineData->exec_glVertexAttribPointer(TEXCOORD_ATTRIB, 2, 0, texture_coords);
	engineData->exec_glEnableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glEnableVertexAttribArray(TEXCOORD_ATTRIB);
	engineData->exec_glDrawArrays_GL_TRIANGLE_STRIP(0, 4);
	engineData->exec_glDisableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glDisableVertexAttribArray(TEXCOORD_ATTRIB);
}

void RenderThread::plotProfilingData()
{
	lsglLoadIdentity();
	lsglScalef(1.0f/scaleX,-1.0f/scaleY,1);
	lsglTranslatef(-offsetX,(windowHeight-offsetY)*(-1.0f),0);
	setMatrixUniform(LSGL_MODELVIEW);

	cairo_t *cr = getCairoContext(windowWidth, windowHeight);

	engineData->exec_glUniform1f(directUniform, 1);

	char frameBuf[20];
	snprintf(frameBuf,20,"Frame %u",m_sys->mainClip->state.FP);

	float vertex_coords[40];
	float color_coords[80];

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

	engineData->exec_glVertexAttribPointer(VERTEX_ATTRIB, 2, 0, vertex_coords);
	engineData->exec_glVertexAttribPointer(COLOR_ATTRIB, 4, 0, color_coords);
	engineData->exec_glEnableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glEnableVertexAttribArray(COLOR_ATTRIB);
	engineData->exec_glDrawArrays_GL_LINES(0, 20);
	engineData->exec_glDisableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glDisableVertexAttribArray(COLOR_ATTRIB);
 
	list<ThreadProfile*>::iterator it=m_sys->profilingData.begin();
	for(;it!=m_sys->profilingData.end();++it)
		(*it)->plot(1000000/m_sys->mainClip->getFrameRate(),cr);
	engineData->exec_glUniform1f(directUniform, 0);

	mapCairoTexture(windowWidth, windowHeight);

	//clear the surface
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);

}

void RenderThread::coreRendering()
{
	Locker l(mutexRendering);
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(0);
	engineData->exec_glDrawBuffer_GL_BACK();
	//Clear the back buffer
	RGB bg=m_sys->mainClip->getBackground();
	engineData->exec_glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,1);
	engineData->exec_glClear_GL_COLOR_BUFFER_BIT();
	lsglLoadIdentity();
	setMatrixUniform(LSGL_MODELVIEW);

	m_sys->mainClip->getStage()->Render(*this);

	if(m_sys->showProfilingData)
		plotProfilingData();

	handleGLErrors();
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

	int y = th->windowHeight-20;
	renderText(cr, "We're sorry, Lightspark encountered a yet unsupported Flash file",
			0,y);
	y -= 20;

	stringstream errorMsg;
	errorMsg << "SWF file: " << th->m_sys->mainClip->getOrigin().getParsedURL();
	renderText(cr, errorMsg.str().c_str(),0,y);
	y -= 20;

	errorMsg.str("");
	errorMsg << "Cause: " << th->m_sys->errorCause;
	tiny_string s = errorMsg.str();
	std::list<tiny_string> msglist = s.split('\n');
	for (auto it = msglist.begin(); it != msglist.end(); it++)
	{
		renderText(cr, (*it).raw_buf(),0,y);
		y -= 20;
	}

	if (standalone)
	{
		renderText(cr, "Please look at the console output to copy this error",
			0,y);
		y -= 20;

		renderText(cr, "Press 'Ctrl+Q' to exit",0,y);
	}
	else
	{
		renderText(cr, "Press Ctrl+C to copy this error to clipboard",
				0,y);
	}

	engineData->exec_glUniform1f(alphaUniform, 1);
	mapCairoTexture(windowWidth, windowHeight);
	engineData->exec_glFlush();
}

void RenderThread::addUploadJob(ITextureUploadable* u)
{
	Locker l(mutexUploadJobs);
	if(m_sys->isShuttingDown() || status!=STARTED)
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
		LOG(LOG_INFO,_("FPS: ") << dec << frameCount<<" "<<getVm(m_sys)->getEventQueueSize());
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

void RenderThread::tickFence()
{
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

uint32_t RenderThread::allocateNewGLTexture() const
{
	//Set up the huge texture
	uint32_t tmp;
	engineData->exec_glGenTextures(1,&tmp);
	assert(tmp!=0);
	//If the previous call has not failed these should not fail (in specs, we trust)
	engineData->exec_glBindTexture_GL_TEXTURE_2D(tmp);
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
	//Allocate the texture
	engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_INT_8_8_8_8_HOST(0, largeTextureSize, largeTextureSize, 0, 0);
	if(handleGLErrors())
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

void RenderThread::loadChunkBGRA(const TextureChunk& chunk, uint32_t w, uint32_t h, uint8_t* data)
{
	//Fast bailout if the TextureChunk is not valid
	if(chunk.chunks==NULL)
		return;
	engineData->exec_glBindTexture_GL_TEXTURE_2D(largeTextures[chunk.texId].id);
	//TODO: Detect continuos
	//The size is ok if doesn't grow over the allocated size
	//this allows some alignment freedom
	assert(w<=((chunk.width+CHUNKSIZE-1)&0xffffff80));
	assert(h<=((chunk.height+CHUNKSIZE-1)&0xffffff80));
	const uint32_t numberOfChunks=chunk.getNumberOfChunks();
	const uint32_t blocksPerSide=largeTextureSize/CHUNKSIZE;
	const uint32_t blocksW=(w+CHUNKSIZE-1)/CHUNKSIZE;
	engineData->exec_glPixelStorei_GL_UNPACK_ROW_LENGTH(w);
	for(uint32_t i=0;i<numberOfChunks;i++)
	{
		uint32_t curX=(i%blocksW)*CHUNKSIZE;
		uint32_t curY=(i/blocksW)*CHUNKSIZE;
		uint32_t sizeX=min(int(w-curX),CHUNKSIZE);
		uint32_t sizeY=min(int(h-curY),CHUNKSIZE);
		engineData->exec_glPixelStorei_GL_UNPACK_SKIP_PIXELS(curX);
		engineData->exec_glPixelStorei_GL_UNPACK_SKIP_ROWS(curY);
		const uint32_t blockX=((chunk.chunks[i]%blocksPerSide)*CHUNKSIZE);
		const uint32_t blockY=((chunk.chunks[i]/blocksPerSide)*CHUNKSIZE);
		engineData->exec_glTexSubImage2D_GL_TEXTURE_2D(0, blockX, blockY, sizeX, sizeY, data,w,curX,curY);
	}
	engineData->exec_glPixelStorei_GL_UNPACK_SKIP_PIXELS(0);
	engineData->exec_glPixelStorei_GL_UNPACK_SKIP_ROWS(0);
	engineData->exec_glPixelStorei_GL_UNPACK_ROW_LENGTH(0);
}
