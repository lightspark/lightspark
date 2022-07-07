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

#ifndef BACKENDS_RENDERING_H
#define BACKENDS_RENDERING_H 1

#include "backends/rendering_context.h"
#include "timer.h"
#include <SDL2/SDL.h>
#include <sys/time.h>
#ifdef _WIN32
#	include <windef.h>
#endif

namespace lightspark
{
class ThreadProfile;

class DLL_PUBLIC RenderThread: public ITickJob, public GLRenderContext
{
friend class DisplayObject;
friend class Context3D;
private:
	SystemState* m_sys;
	SDL_Thread* t;
	enum STATUS { CREATED=0, STARTED, TERMINATED };
	volatile STATUS status;

	static int worker(void* d);

	void commonGLInit(int width, int height);
	void commonGLResize();
	void commonGLDeinit();
	ITextureUploadable* prevUploadJob;
	uint32_t allocateNewGLTexture() const;
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
	volatile uint32_t newWidth;
	volatile uint32_t newHeight;
	float scaleX;
	float scaleY;
	int offsetX;
	int offsetY;

	struct timeval time_s, time_d;

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
		returns true if at least one of the displayobjects on the stage couldn't be rendered becaus of an AsyncDrawJob not done yet
	*/
	bool coreRendering();
	void plotProfilingData();
	Semaphore initialized;
	volatile bool refreshNeeded;
	Mutex mutexRefreshSurfaces;
	struct refreshableSurface
	{
		IDrawable* drawable;
		_NR<DisplayObject> displayobject;
	};
	std::list<refreshableSurface> surfacesToRefresh;
public:
	Mutex mutexRendering;
	volatile bool screenshotneeded;
	volatile bool inSettings;
	volatile bool canrender;
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

	void init();
	void deinit();
	bool doRender(ThreadProfile *profile=nullptr, Chronometer *chronometer=nullptr);
	void generateScreenshot();
	bool isStarted() const { return status == STARTED; }
	/**
	 * @brief updates the arguments of a cachedSurface without recreating the texture
	 * @param d IDrawable containing the new values
	 * this will be deleted in this method
	 * @param o target containing the cachedSurface to be updated
	 */
	void addRefreshableSurface(IDrawable* d,_NR<DisplayObject> o)
	{
		Locker l(mutexRefreshSurfaces);
		refreshableSurface s;
		s.displayobject = o;
		s.drawable = d;
		surfacesToRefresh.push_back(s);
	}
	void signalSurfaceRefresh()
	{
		Locker l(mutexRefreshSurfaces);
		if (!surfacesToRefresh.empty())
		{
			refreshNeeded=true;
			event.signal();
		}
	}
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
	volatile uint32_t windowWidth;
	volatile uint32_t windowHeight;

	void renderErrorPage(RenderThread *rt, bool standalone);
	void renderSettingsPage();
	cairo_t *cairoTextureContextSettings;
	cairo_surface_t *cairoTextureSurfaceSettings;
	uint8_t *cairoTextureDataSettings;
	uint32_t cairoTextureIDSettings;
	cairo_t* getCairoContextSettings(int w, int h);

	cairo_t *cairoTextureContext;
	cairo_surface_t *cairoTextureSurface;
	uint8_t *cairoTextureData;
	uint32_t cairoTextureID;
	cairo_t* getCairoContext(int w, int h);
	void mapCairoTexture(int w, int h, bool forsettings=false);
	void renderText(cairo_t *cr, const char *text, int x, int y);
	void waitRendering();
};

RenderThread* getRenderThread();

}
#endif /* BACKENDS_RENDERING_H */
