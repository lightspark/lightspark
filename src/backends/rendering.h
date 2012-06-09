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

#ifndef RENDERING_H
#define RENDERING_H

#include "lsopengl.h"
#include "rendering_context.h"
#include "timer.h"
#include <glibmm/timeval.h>

namespace lightspark
{

class RenderThread: public ITickJob, public GLRenderContext
{
friend class DisplayObject;
private:
	SystemState* m_sys;
	Thread* t;
	enum STATUS { CREATED=0, STARTED, TERMINATED };
	volatile STATUS status;

	EngineData* engineData;
	void worker();
	void init();
	void deinit();

	void commonGLInit(int width, int height);
	void commonGLResize();
	void commonGLDeinit();
	GLuint pixelBuffers[2];
	uint32_t currentPixelBuffer;
	uint32_t currentPixelBufferOffset;
	uint32_t pixelBufferWidth;
	uint32_t pixelBufferHeight;
	void resizePixelBuffers(uint32_t w, uint32_t h);
	ITextureUploadable* prevUploadJob;
	GLuint allocateNewGLTexture() const;
	LargeTexture& allocateNewTexture();
	bool allocateChunkOnTextureCompact(LargeTexture& tex, TextureChunk& ret, uint32_t blocksW, uint32_t blocksH);
	bool allocateChunkOnTextureSparse(LargeTexture& tex, TextureChunk& ret, uint32_t blocksW, uint32_t blocksH);
	//Possible events to be handled
	//TODO: pad to avoid false sharing on the cache lines
	volatile bool renderNeeded;
	volatile bool uploadNeeded;
	volatile bool resizeNeeded;
	volatile bool newTextureNeeded;
	void handleNewTexture();
	void finalizeUpload();
	void handleUpload();
	Semaphore event;
	std::string fontPath;
	uint32_t newWidth;
	uint32_t newHeight;
	float scaleX;
	float scaleY;
	int offsetX;
	int offsetY;

#ifdef _WIN32
	HGLRC mRC;
	HDC mDC;
#else
	Display* mDisplay;
	Window mWindow;
#ifndef ENABLE_GLES2
	GLXFBConfig mFBConfig;
	GLXContext mContext;
#else
	EGLDisplay mEGLDisplay;
	EGLContext mEGLContext;
	EGLConfig mEGLConfig;
	EGLSurface mEGLSurface;
#endif
#endif
	Glib::TimeVal time_s, time_d;
	static const Glib::TimeVal FPS_time;

	bool loadShaderPrograms();
	bool tempBufferAcquired;
	void tick();
	void tickFence();
	int frameCount;
	int secsCount;
	Mutex mutexUploadJobs;
	std::deque<ITextureUploadable*> uploadJobs;
	/*
		Utility to get a job to do
	*/
	ITextureUploadable* getUploadJob();
	/*
		Common code to handle the core of the rendering
	*/
	void coreRendering();
	void plotProfilingData();
	Semaphore initialized;

	static void SizeAllocateCallback(GtkWidget* widget, GdkRectangle* allocation, gpointer data);
public:
	RenderThread(SystemState* s);
	~RenderThread();
	/**
	   The EngineData object must survive for the whole life of this RenderThread
	*/
	void start(EngineData* data);
	/*
	   The stop function should be call on exit even if the thread is not started
	*/
	void stop();
	void wait();
	void draw(bool force);

	/**
		Allocates a chunk from the shared texture
	*/
	TextureChunk allocateTexture(uint32_t w, uint32_t h, bool compact);
	/**
		Release texture
	*/
	void releaseTexture(const TextureChunk& chunk);
	/**
		Load the given data in the given texture chunk
	*/
	void loadChunkBGRA(const TextureChunk& chunk, uint32_t w, uint32_t h, uint8_t* data);
	/**
		Enqueue something to be uploaded to texture
	*/
	void addUploadJob(ITextureUploadable* u);

	void requestResize(uint32_t w, uint32_t h, bool force);
	void waitForInitialization()
	{
		initialized.wait();
	}
	void forceInitialization()
	{
		initialized.signal();
	}

	//OpenGL programs
	int gpu_program;
	TextureBuffer tempTex;
	uint32_t windowWidth;
	uint32_t windowHeight;
	bool hasNPOTTextures;
	GLint fragmentTexScaleUniform;
	GLint directUniform;

	void renderErrorPage(RenderThread *rt, bool standalone);

	cairo_t *cairoTextureContext;
	cairo_surface_t *cairoTextureSurface;
	uint8_t *cairoTextureData;
	GLuint cairoTextureID;
	cairo_t* getCairoContext(int w, int h);
	void mapCairoTexture(int w, int h);
	void renderText(cairo_t *cr, const char *text, int x, int y);
};

RenderThread* getRenderThread();

};
#endif
