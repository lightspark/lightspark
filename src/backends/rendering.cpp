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
#include "scripting/class.h"
#include "parsing/textfile.h"
#include "backends/rendering.h"
#include "backends/input.h"
#include "compat.h"
#include <sstream>
#include <unistd.h>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif

using namespace lightspark;
using namespace std;


DEFINE_AND_INITIALIZE_TLS(renderThread);
RenderThread* lightspark::getRenderThread()
{
	RenderThread* ret = (RenderThread*)tls_get(renderThread);
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
		SDL_WaitThread(t,nullptr);
	}
}

RenderThread::RenderThread(SystemState* s):GLRenderContext(),
	m_sys(s),status(CREATED),
	prevUploadJob(nullptr),
	renderNeeded(false),uploadNeeded(false),resizeNeeded(false),newTextureNeeded(false),event(0),newWidth(0),newHeight(0),scaleX(1),scaleY(1),
	offsetX(0),offsetY(0),tempBufferAcquired(false),frameCount(0),secsCount(0),initialized(0),refreshNeeded(false),screenshotneeded(false),inSettings(false),canrender(false),
	cairoTextureContextSettings(nullptr),cairoTextureContext(nullptr)
{
	LOG(LOG_INFO,"RenderThread this=" << this);
#ifdef _WIN32
	fontPath = "TimesNewRoman.ttf";
#else
	fontPath = "Serif";
#endif
	gettimeofday(&time_s, nullptr);
}

void RenderThread::start(EngineData* data)
{
	status=STARTED;
	engineData=data;
	t = SDL_CreateThread(RenderThread::worker,"RenderThread",this);
}

void RenderThread::stop()
{
	initialized.signal();
}

RenderThread::~RenderThread()
{
	wait();
	LOG(LOG_INFO,"~RenderThread this=" << this);
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
	TextureChunk& tex=u->getTexture();
	u->contentScale(tex.xContentScale, tex.yContentScale);
	u->contentOffset(tex.xOffset, tex.yOffset);
	loadChunkBGRA(tex, w, h, u->upload(false));
	u->uploadFence();
	prevUploadJob=nullptr;
}

void RenderThread::handleUpload()
{
	ITextureUploadable* u=getUploadJob();
	assert(u);
	uint32_t w,h;
	u->sizeNeeded(w,h);
	
	//force creation of buffer if neccessary
	u->upload(true);
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

int RenderThread::worker(void* d)
{
	RenderThread *th = (RenderThread *)d;
	setTLSSys(th->m_sys);
	setTLSWorker(th->m_sys->worker);
	/* set TLS variable for getRenderThread() */
	tls_set(renderThread, th);

	ThreadProfile* profile=th->m_sys->allocateProfiler(RGB(200,0,0));
	profile->setTag("Render");
	try
	{
		th->init();

		ThreadProfile* profile=th->m_sys->allocateProfiler(RGB(200,0,0));
		profile->setTag("Render");

		th->engineData->exec_glEnable_GL_TEXTURE_2D();

		Chronometer chronometer;
		while(1)
		{
			if (!th->doRender(profile,&chronometer))
				break;
		}

		th->deinit();
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Exception in RenderThread, stopping rendering: " << e.what());
		//TODO: add a comandline switch to disable rendering. Then add that commandline switch
		//to the test runner script and uncomment the next line
		//m_sys->setError(e.cause);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Exception in RenderThread, stopping rendering",e.what(),th->engineData->widget);
	}

	/* cleanup */
	//Keep addUploadJob from enqueueing
	th->status=TERMINATED;
	//Fence existing jobs
	th->mutexUploadJobs.lock();
	if(th->prevUploadJob)
		th->prevUploadJob->uploadFence();
	for(auto i=th->uploadJobs.begin(); i != th->uploadJobs.end(); ++i)
		(*i)->uploadFence();
	th->mutexUploadJobs.unlock();
	return 0;
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
		LOG(LOG_INFO,"Window resized to " << windowWidth << 'x' << windowHeight);
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
	if (refreshNeeded)
	{
		Locker l(mutexRefreshSurfaces);
		auto it = surfacesToRefresh.begin();
		while (it != surfacesToRefresh.end())
		{
			it->displayobject->updateCachedSurface(it->drawable);
			delete it->drawable;
			it = surfacesToRefresh.erase(it);
		}
		refreshNeeded=false;
		renderNeeded=true;
	}

	if(uploadNeeded)
	{
		handleUpload();
		if (profile && chronometer)
			profile->accountTime(chronometer->checkpoint());
		return true;
	}

	if(USUALLY_FALSE(m_sys->isOnError()))
	{
		renderErrorPage(this, m_sys->standalone);
	}
	else
	{
		if (m_sys->stage->renderStage3D())
		{
			// stage3d rendering is always needed, so we ignore canrender
			coreRendering();
			if (inSettings)
				renderSettingsPage();
			engineData->exec_glFlush();
			if (screenshotneeded)
				generateScreenshot();
			engineData->DoSwapBuffers();
			if (profile && chronometer)
				profile->accountTime(chronometer->checkpoint());
			renderNeeded=false;
			return true;
		}
		else
		{
			if(!canrender)
			{
				// there are still drawjobs pending, so we don't render the stage
				if (inSettings)
				{
					renderSettingsPage();
					engineData->DoSwapBuffers();
				}
				if (screenshotneeded)
					generateScreenshot();
				renderNeeded=false;
				return true;
			}
			if(!m_sys->isOnError())
			{
				coreRendering();
				//Call glFlush to offload work on the GPU
				engineData->exec_glFlush();
			}
		}
	}
	if (inSettings)
		renderSettingsPage();
	if (screenshotneeded)
		generateScreenshot();
	engineData->DoSwapBuffers();
	if (profile && chronometer)
		profile->accountTime(chronometer->checkpoint());
	canrender=false;
	renderNeeded=false;
	return true;
}
void RenderThread::renderSettingsPage()
{
	lsglLoadIdentity();
	lsglScalef(1.0f,-1.0f,1);
	lsglTranslatef(-offsetX,(windowHeight-offsetY)*(-1.0f),0);

	setMatrixUniform(LSGL_MODELVIEW);

	float bordercolor = 0.3;
	float backgroundcolor = 0.7;
	float buttonbackgroundcolor = 0.9;
	float selectedbackgroundcolor = 0.5;
	float textcolor = 0.0;

	int width=210;
	int height=136;
	Vector2 mousepos = m_sys->getInputThread()->getMousePos();
	int startposx = (windowWidth-width)/2;
	int startposy = (windowHeight-height)/2;
	cairo_t *cr = getCairoContextSettings(width, height);

	cairo_set_source_rgb (cr, backgroundcolor, backgroundcolor,backgroundcolor);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_antialias(cr,CAIRO_ANTIALIAS_DEFAULT);
	cairo_set_source_rgb (cr, bordercolor, bordercolor, bordercolor);
	cairo_set_line_width(cr, 2);
	cairo_rectangle(cr, 1, 1, width-2, height-2);
	cairo_stroke(cr);

	// close button
	int buttonwidth=50;
	int buttonheight=20;
	cairo_set_source_rgb (cr, bordercolor, bordercolor, bordercolor);
	cairo_set_line_width(cr, 2);
	cairo_rectangle(cr, width-buttonwidth-10, 15, buttonwidth, buttonheight);
	cairo_stroke(cr);
	if (mousepos.x > startposx + (width-buttonwidth-10) && mousepos.x < startposx +(width-10) &&
		mousepos.y > startposy + (height-buttonheight-15) && mousepos.y < startposy +(height-15))
	{
		if (m_sys->getInputThread()->getLeftButtonPressed())
		{
			inSettings=false;
		}
		cairo_set_source_rgb (cr, selectedbackgroundcolor, selectedbackgroundcolor, selectedbackgroundcolor);
	}
	else
		cairo_set_source_rgb (cr, buttonbackgroundcolor, buttonbackgroundcolor,buttonbackgroundcolor);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, width-buttonwidth-10+1, 15+1, buttonwidth-2, buttonheight-2);
	cairo_fill(cr);

	cairo_set_source_rgb (cr, textcolor, textcolor,textcolor);
	renderText(cr, "close",width-50,20);

	// local storage checkbox
	int checkboxwidth=20;
	int checkboxheight=20;
	if (mousepos.x > startposx + (width-checkboxwidth-10) && mousepos.x < startposx +(width-10) &&
		mousepos.y > startposy + (10) && mousepos.y < startposy +(checkboxheight+10))
	{
		if (m_sys->getInputThread()->getLeftButtonPressed())
		{
			m_sys->setLocalStorageAllowed(!m_sys->localStorageAllowed());
		}
	}
	cairo_set_source_rgb (cr, bordercolor, bordercolor, bordercolor);
	cairo_set_line_width(cr, 2);
	cairo_rectangle(cr, width-checkboxwidth-10, height-checkboxheight-10, checkboxwidth, checkboxheight);
	cairo_stroke(cr);
	if (m_sys->localStorageAllowed())
	{
		cairo_set_source_rgb (cr, 0, 1.0, 0);
		cairo_set_line_width(cr, 5);
		cairo_move_to(cr, width-checkboxwidth-12, height-checkboxheight/2-10);
		cairo_line_to(cr, width-checkboxwidth/2-10, height-checkboxheight-10);
		cairo_line_to(cr, width-10, height-8);
		cairo_stroke(cr);
	}
	cairo_set_source_rgb (cr, textcolor, textcolor,textcolor);
	renderText(cr, "allow local storage",10,height-25);

	engineData->exec_glUniform1f(alphaUniform, 1);
	engineData->exec_glUniform4f(colortransMultiplyUniform, 1.0,1.0,1.0,1.0);
	engineData->exec_glUniform4f(colortransAddUniform, 0.0,0.0,0.0,0.0);
	mapCairoTexture(width, height,true);
	engineData->exec_glFlush();
}
void RenderThread::generateScreenshot()
{
	char* buf = new char[windowWidth*windowHeight*3];
	if (!buf)
	{
		LOG(LOG_ERROR,"generating screenshot memory failed");
		return;
	}
	engineData->exec_glReadPixels(windowWidth, windowHeight, buf);
	
	char* name_used=nullptr;
	int fd = g_file_open_tmp("lightsparkXXXXXX.bmp",&name_used,nullptr);
	if(fd == -1)
	{
		LOG(LOG_ERROR,"generating screenshot file failed");
		return;
	}
	
	unsigned char bmp_file_header[14] = { 'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, };
	unsigned char bmp_info_header[40] = { 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0, };
	size_t size = 54 + windowWidth * windowHeight * 3;

	bmp_file_header[2] = (size)&0xff;
	bmp_file_header[3] = (size >> 8);
	bmp_file_header[4] = (size >> 16);
	bmp_file_header[5] = (size >> 24);

	bmp_info_header[4] = (windowWidth)&0xff;
	bmp_info_header[5] = (windowWidth >> 8)&0xff;
	bmp_info_header[6] = (windowWidth >> 16)&0xff;
	bmp_info_header[7] = (windowWidth >> 24)&0xff;

	bmp_info_header[8] = (windowHeight)&0xff;
	bmp_info_header[9] = (windowHeight >> 8)&0xff;
	bmp_info_header[10] = (windowHeight >> 16)&0xff;
	bmp_info_header[11] = (windowHeight >> 24)&0xff;
		
	if (write(fd,bmp_file_header,14)<0)
		LOG(LOG_INFO,"screenshot header write error");
	if (write(fd,bmp_info_header,40)<0)
		LOG(LOG_INFO,"screenshot header write error");
	if (write(fd,buf,windowWidth * windowHeight * 3)<0)
		LOG(LOG_INFO,"screenshot write error");
	close(fd);
	delete[] buf;
	LOG(LOG_INFO,"screenshot generated:"<<name_used);
	g_free(name_used);
	screenshotneeded=false;
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
	engineData->exec_glShaderSource(f, 1, &fs,nullptr);
	uint32_t g = engineData->exec_glCreateShader_GL_VERTEX_SHADER();
	
	bool ret=true;
	char str[1024];
	int a;
	int stat;
	engineData->exec_glCompileShader(f);
	engineData->exec_glGetShaderInfoLog(f,1024,&a,str);
	LOG(LOG_INFO,"Fragment shader compilation " << str);
	engineData->exec_glGetShaderiv_GL_COMPILE_STATUS(f, &stat);
	if (!stat)
	{
		throw RunTimeException("Could not compile fragment shader");
	}

	// directly include shader source to avoid filesystem access
	const char *fs2 = 
#include "lightspark.vert"
;
	engineData->exec_glShaderSource(g, 1, &fs2,nullptr);

	engineData->exec_glGetShaderInfoLog(g,1024,&a,str);
	LOG(LOG_INFO,"Vertex shader compilation " << str);

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
	engineData->exec_glFrontFace(false);
	for(uint32_t i=0;i<largeTextures.size();i++)
	{
		engineData->exec_glDeleteTextures(1,&largeTextures[i].id);
		delete[] largeTextures[i].bitmap;
	}
	engineData->exec_glDeleteTextures(1, &cairoTextureID);
	engineData->exec_glDeleteTextures(1, &cairoTextureIDSettings);
	engineData->exec_glDeleteTextures(1, &maskTextureID);
}

void RenderThread::commonGLInit(int width, int height)
{
	//Load shaders
	loadShaderPrograms();
	engineData->driverInfoString = engineData->getGLDriverInfo();
	engineData->getGlCompressedTextureFormats();

	engineData->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
	engineData->exec_glEnable_GL_BLEND();

	engineData->exec_glActiveTexture_GL_TEXTURE0(0);
	//Viewport setup is left for GLResize

	//Get the maximum allowed texture size, up to 8192
	int maxTexSize;
	engineData->exec_glGetIntegerv_GL_MAX_TEXTURE_SIZE(&maxTexSize);
	assert(maxTexSize>0);
	largeTextureSize=min(maxTexSize,8192);

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
	//The uniform that indicates if the object to be rendered has a mask (1) or not (0)
	maskUniform =engineData->exec_glGetUniformLocation(gpu_program,"mask");
	//The uniform that contains the coordinate matrix
	projectionMatrixUniform =engineData->exec_glGetUniformLocation(gpu_program,"ls_ProjectionMatrix");
	modelviewMatrixUniform =engineData->exec_glGetUniformLocation(gpu_program,"ls_ModelViewMatrix");

	colortransMultiplyUniform=engineData->exec_glGetUniformLocation(gpu_program,"colorTransformMultiply");
	colortransAddUniform=engineData->exec_glGetUniformLocation(gpu_program,"colorTransformAdd");
	directColorUniform=engineData->exec_glGetUniformLocation(gpu_program,"directColor");

	//Texturing must be enabled otherwise no tex coord will be sent to the shaders
	engineData->exec_glEnable_GL_TEXTURE_2D();

	engineData->exec_glGenTextures(1, &cairoTextureID);
	engineData->exec_glGenTextures(1, &cairoTextureIDSettings);

	// create framebuffer for masks
	maskframebuffer = engineData->exec_glGenFramebuffer();
	engineData->exec_glGenTextures(1, &maskTextureID);

	if(handleGLErrors())
	{
		LOG(LOG_ERROR,"GL errors during initialization");
	}
}

void RenderThread::commonGLResize()
{
	m_sys->stageCoordinateMapping(windowWidth, windowHeight, offsetX, offsetY, scaleX, scaleY);
	engineData->exec_glViewport(0,0,windowWidth,windowHeight);
	if (cairoTextureContext)
	{
		cairo_destroy(cairoTextureContext);
		cairoTextureContext=nullptr;
	}
	if (cairoTextureContextSettings)
	{
		cairo_destroy(cairoTextureContextSettings);
		cairoTextureContextSettings=nullptr;
	}
	lsglLoadIdentity();
	lsglOrtho(0,windowWidth,0,windowHeight,-100,0);
	//scaleY is negated to adapt the flash and gl coordinates system
	//An additional translation is added for the same reason
	lsglTranslatef(offsetX,windowHeight-offsetY,0);
	lsglScalef(1.0,-1.0,1);
	setMatrixUniform(LSGL_PROJECTION);

	// setup mask framebuffer
	engineData->exec_glActiveTexture_GL_TEXTURE0(1);
	engineData->exec_glBindTexture_GL_TEXTURE_2D(maskTextureID);
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(maskframebuffer);
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_NEAREST();
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_NEAREST();
	engineData->exec_glFramebufferTexture2D_GL_FRAMEBUFFER(maskTextureID);
	engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, windowWidth,windowHeight, 0, nullptr,true);
	engineData->exec_glViewport(0,0,windowWidth,windowHeight);
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(0);
	engineData->exec_glActiveTexture_GL_TEXTURE0(0);
	engineData->exec_glBindTexture_GL_TEXTURE_2D(0);
	engineData->exec_glDisable_GL_DEPTH_TEST();
	engineData->exec_glDisable_GL_STENCIL_TEST();
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
	if (w != UINT32_MAX)
		newWidth=w;
	else if (newWidth == 0)
		newWidth=windowWidth;
	if (h != UINT32_MAX)
		newHeight=h;
	else if (newHeight == 0)
		newHeight=windowHeight;
	resizeNeeded=true;
	m_sys->stage->incRef();
	getVm(m_sys)->addEvent(_MR(m_sys->stage),_MR(Class<Event>::getInstanceS(m_sys->worker,"resize")));
	
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
cairo_t* RenderThread::getCairoContextSettings(int w, int h)
{
	if (!cairoTextureContextSettings) {
		cairoTextureDataSettings = new uint8_t[w*h*4];
		cairoTextureSurfaceSettings = cairo_image_surface_create_for_data(cairoTextureDataSettings, CAIRO_FORMAT_ARGB32, w, h, w*4);
		cairoTextureContextSettings = cairo_create(cairoTextureSurfaceSettings);

		cairo_select_font_face (cairoTextureContextSettings, fontPath.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cairoTextureContextSettings, 11);
	}
	return cairoTextureContextSettings;
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
void RenderThread::mapCairoTexture(int w, int h,bool forsettings)
{
	engineData->exec_glEnable_GL_TEXTURE_2D();
	engineData->exec_glBindTexture_GL_TEXTURE_2D(forsettings ? cairoTextureIDSettings : cairoTextureID);

	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR();
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR();
	engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, w, h, 0, forsettings ? cairoTextureDataSettings : cairoTextureData,true);

	float vertex_coords[] = {0,0, float(w),0, 0,float(h), float(w),float(h)};
	float texture_coords[] = {0,0, 1,0, 0,1, 1,1};
	engineData->exec_glVertexAttribPointer(VERTEX_ATTRIB, 0, vertex_coords,FLOAT_2);
	engineData->exec_glVertexAttribPointer(TEXCOORD_ATTRIB, 0, texture_coords,FLOAT_2);
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

	engineData->exec_glVertexAttribPointer(VERTEX_ATTRIB, 0, vertex_coords,FLOAT_2);
	engineData->exec_glVertexAttribPointer(COLOR_ATTRIB, 0, color_coords,FLOAT_4);
	engineData->exec_glEnableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glEnableVertexAttribArray(COLOR_ATTRIB);
	engineData->exec_glDrawArrays_GL_LINES(0, 20);
	engineData->exec_glDisableVertexAttribArray(VERTEX_ATTRIB);
	engineData->exec_glDisableVertexAttribArray(COLOR_ATTRIB);
 
	list<ThreadProfile*>::iterator it=m_sys->profilingData.begin();
	for(;it!=m_sys->profilingData.end();++it)
		(*it)->plot(1000000/m_sys->mainClip->getFrameRate(),cr);
	engineData->exec_glUniform1f(directUniform, 0);
	engineData->exec_glUniform4f(colortransMultiplyUniform, 1.0,1.0,1.0,1.0);
	engineData->exec_glUniform4f(colortransAddUniform, 0.0,0.0,0.0,0.0);

	mapCairoTexture(windowWidth, windowHeight);

	//clear the surface
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);
	cairo_restore(cr);

}

bool RenderThread::coreRendering()
{
	Locker l(mutexRendering);
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(0);
	engineData->exec_glFrontFace(false);
	engineData->exec_glDrawBuffer_GL_BACK();
	if (!m_sys->stage->renderStage3D()) // no need to clear the backbuffer when using Stage3D
	{
		//Clear the back buffer
		RGB bg=m_sys->mainClip->getBackground();
		engineData->exec_glClearColor(bg.Red/255.0F,bg.Green/255.0F,bg.Blue/255.0F,1);
		engineData->exec_glClear_GL_COLOR_BUFFER_BIT();
	}
	engineData->exec_glUseProgram(gpu_program);
	lsglLoadIdentity();
	setMatrixUniform(LSGL_MODELVIEW);

	bool ret = m_sys->stage->Render(*this);

	if(m_sys->showProfilingData)
		plotProfilingData();

	handleGLErrors();
	return ret;
}

//Renders the error message which caused the VM to stop.
void RenderThread::renderErrorPage(RenderThread *th, bool standalone)
{
	lsglLoadIdentity();
	lsglScalef(1.0f,-1.0f,1);
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

	engineData->exec_glUniform1f(directUniform, 0);
	engineData->exec_glUniform1f(alphaUniform, 1);
	engineData->exec_glUniform4f(colortransMultiplyUniform, 1.0,1.0,1.0,1.0);
	engineData->exec_glUniform4f(colortransAddUniform, 0.0,0.0,0.0,0.0);
	mapCairoTexture(windowWidth, windowHeight);
	engineData->exec_glFlush();
}

void RenderThread::addUploadJob(ITextureUploadable* u)
{
	mutexUploadJobs.lock();
	if (u->getQueued())
	{
		mutexUploadJobs.unlock();
		return;
	}
	if(m_sys->isShuttingDown() || status!=STARTED)
	{
		u->uploadFence();
		mutexUploadJobs.unlock();
		return;
	}
	u->setQueued();
	uploadJobs.push_back(u);
	uploadNeeded=true;
	mutexUploadJobs.unlock();
	event.signal();
}

ITextureUploadable* RenderThread::getUploadJob()
{
	mutexUploadJobs.lock();
	assert(!uploadJobs.empty());
	ITextureUploadable* ret=uploadJobs.front();
	uploadJobs.pop_front();
	if(uploadJobs.empty())
		uploadNeeded=false;
	mutexUploadJobs.unlock();
	return ret;
}

void RenderThread::draw(bool force)
{
	if(renderNeeded && !force) //A rendering is already queued
		return;
	renderNeeded=true;
	event.signal();

	gettimeofday(&time_d, nullptr);
	int diff = time_d.tv_sec-time_s.tv_sec;
	if(diff>0) /* is one seconds elapsed? */
	{
		time_s=time_d;
		LOG(LOG_INFO,"FPS: " << dec << frameCount<<" "<<(getVm(m_sys) ? getVm(m_sys)->getEventQueueSize() : 0));
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
		LOG(LOG_ERROR,"Can't allocate large texture... Aborting");
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
	TextureChunk ret(w, h);
	uint32_t blocksW=(ret.width+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
	uint32_t blocksH=(ret.height+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
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
	if(chunk.chunks==nullptr || data == nullptr)
		return;
	engineData->exec_glBindTexture_GL_TEXTURE_2D(largeTextures[chunk.texId].id);
	//TODO: Detect continuos
	//The size is ok if doesn't grow over the allocated size
	//this allows some alignment freedom
	const uint32_t numberOfChunks=chunk.getNumberOfChunks();
	const uint32_t blocksPerSide=largeTextureSize/CHUNKSIZE;
	const uint32_t blocksW=((w+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL);
	uint8_t data_clamp[4*CHUNKSIZE*CHUNKSIZE];
	for(uint32_t i=0;i<numberOfChunks;i++)
	{
		uint32_t curX=(i%blocksW)*CHUNKSIZE_REAL;
		uint32_t curY=(i/blocksW)*CHUNKSIZE_REAL;
		if (curX > w || curY > h)
			break;
		uint32_t sizeX=min(int(w-curX),CHUNKSIZE_REAL)+2;
		uint32_t sizeY=min(int(h-curY),CHUNKSIZE_REAL)+2;
		const uint32_t blockX=((chunk.chunks[i]%blocksPerSide)*CHUNKSIZE);
		const uint32_t blockY=((chunk.chunks[i]/blocksPerSide)*CHUNKSIZE);

		// copy chunk data
		for(uint32_t j=1;j<sizeY-1;j++) {
			memcpy(data_clamp+4*j*sizeX+4, data+4*w*((j-1)+curY)+4*curX, (sizeX-2)*4);
		}
		for (uint32_t j = 0; j < sizeY; j++)
		{
			// clamp left border to edge
			data_clamp[j*sizeX*4  ] = data_clamp[j*sizeX*4+4  ];
			data_clamp[j*sizeX*4+1] = data_clamp[j*sizeX*4+4+1];
			data_clamp[j*sizeX*4+2] = data_clamp[j*sizeX*4+4+2];
			data_clamp[j*sizeX*4+3] = data_clamp[j*sizeX*4+4+3];
	
			// clamp right border to edge
			data_clamp[j*sizeX*4 + (sizeX-1)*4  ] = data_clamp[j*sizeX*4 + (sizeX-2)*4  ];
			data_clamp[j*sizeX*4 + (sizeX-1)*4+1] = data_clamp[j*sizeX*4 + (sizeX-2)*4+1];
			data_clamp[j*sizeX*4 + (sizeX-1)*4+2] = data_clamp[j*sizeX*4 + (sizeX-2)*4+2];
			data_clamp[j*sizeX*4 + (sizeX-1)*4+3] = data_clamp[j*sizeX*4 + (sizeX-2)*4+3];
		}
		// clamp top border to edge
		memcpy(data_clamp, data_clamp+4*sizeX, sizeX*4);
		// clamp bottom border to edge
		memcpy(data_clamp+(sizeY-1)*sizeX*4, data_clamp+(sizeY-2)*sizeX*4, sizeX*4);
		engineData->exec_glTexSubImage2D_GL_TEXTURE_2D(0, blockX, blockY, sizeX, sizeY, data_clamp);
	}
}
