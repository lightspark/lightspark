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

#include "interfaces/timer.h"
#include "backends/rendering_context.h"
#include "timer.h"
#include <SDL.h>
#include <sys/time.h>
#include <unordered_map>
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

	void commonGLInit();
	void commonGLResize();
	void commonGLDeinit();
	ITextureUploadable* prevUploadJob;
	uint32_t allocateNewGLTexture() const;
	LargeTexture& allocateNewTexture(bool direct);
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
	void coreRendering();
	void plotProfilingData();
	Semaphore initialized;
	volatile bool refreshNeeded;
	Mutex mutexRefreshSurfaces;
	std::list<RefreshableSurface> surfacesToRefresh;

	volatile bool renderToBitmapContainerNeeded;
	Mutex mutexRenderToBitmapContainer;
	std::list<RenderDisplayObjectToBitmapContainer> displayobjectsToRender;

	std::list<uint32_t> texturesToDelete;

	struct DebugRect
	{
		DisplayObject* obj;
		MATRIX matrix;
		Vector2f pos;
		Vector2f size;
		bool onlyTranslate;
	};
	std::list<DebugRect> debugRects;
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
	void addRefreshableSurface(IDrawable* d,_NR<DisplayObject> o);
	void signalSurfaceRefresh();

	void renderDisplayObjectToBimapContainer(_NR<DisplayObject> o, const MATRIX& initialMatrix,bool smoothing, AS_BLENDMODE blendMode, ColorTransformBase* ct,_NR<BitmapContainer> bm);
	/**
		Allocates a chunk from the shared texture
		if direct is true, the openGL texture is generated directly. this can only be used inside the render thread
	*/
	TextureChunk allocateTexture(uint32_t w, uint32_t h, bool compact, bool direct=false);
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
	void requestResize(const Vector2f& size, bool force);
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
	uint32_t currentframebufferWidth;
	uint32_t currentframebufferHeight;

	Vector2 getOffset() const { return Vector2(offsetX, offsetY); }
	Vector2f getScale() const { return Vector2f(scaleX, scaleY); }
	void renderErrorPage(RenderThread *rt, bool standalone);
	void renderSettingsPage();
	void drawDebugPoint(const Vector2f& pos);
	void drawDebugLine(const Vector2f &a, const Vector2f &b);
	void drawDebugRect(float x, float y, float width, float height, const MATRIX &matrix, bool onlyTranslate = false);
	void drawDebugText(const tiny_string& str, const Vector2f& pos);
	void addDebugRect(DisplayObject* obj, const MATRIX& matrix, bool scaleDown = false, const Vector2f& pos = Vector2f(), const Vector2f& size = Vector2f(), bool onlyTranslate = false);
	void removeDebugRect();
	void setViewPort(uint32_t w, uint32_t h, bool flip);
	void resetViewPort();
	void setModelView(const MATRIX& matrix);
	void renderTextureToFrameBuffer(uint32_t filterTextureID, uint32_t w, uint32_t h, float* filterdata, float* gradientcolors, bool isFirstFilter, bool flippedvertical, bool clearstate=true, bool renderstage3d=false);
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
	void addDeletedTexture(uint32_t textureID)
	{
		Locker l(mutexRendering);
		texturesToDelete.push_back(textureID);
	}
};

RenderThread* getRenderThread();

}
#endif /* BACKENDS_RENDERING_H */
