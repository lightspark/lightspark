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
#include "timer.h"

namespace lightspark
{

enum VertexAttrib { VERTEX_ATTRIB=0, COLOR_ATTRIB, TEXCOORD_ATTRIB};
enum LSGL_MATRIX {LSGL_PROJECTION=0, LSGL_MODELVIEW};

class RenderThread: public ITickJob
{
friend class DisplayObject;
private:
	SystemState* m_sys;
	pthread_t t;
	enum STATUS { CREATED=0, STARTED, TERMINATED };
	STATUS status;

	const EngineData* engineData;
	static void* worker(RenderThread*);

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
	Mutex mutexLargeTexture;
	uint32_t largeTextureSize;
	class LargeTexture
	{
	public:
		GLuint id;
		uint8_t* bitmap;
		LargeTexture(uint8_t* b):id(-1),bitmap(b){}
		~LargeTexture(){/*delete[] bitmap;*/}
	};
	std::vector<LargeTexture> largeTextures;
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
	sem_t event;
	std::string fontPath;
	uint32_t newWidth;
	uint32_t newHeight;
	float scaleX;
	float scaleY;
	int offsetX;
	int offsetY;

#ifndef WIN32
	Display* mDisplay;
#ifndef ENABLE_GLES2
	GLXFBConfig mFBConfig;
	GLXContext mContext;
#else
	EGLContext mEGLContext;
	EGLConfig mEGLConfig;
#endif
	Window mWindow;
#endif
	uint64_t time_s, time_d;

	bool loadShaderPrograms();
	bool tempBufferAcquired;
	void tick();
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
	class MaskData
	{
	public:
		DisplayObject* d;
		MATRIX m;
		MaskData(DisplayObject* _d, const MATRIX& _m):d(_d),m(_m){}
	};
	std::vector<MaskData> maskStack;

	static void SizeAllocateCallback(GtkWidget* widget, GdkRectangle* allocation, gpointer data);
public:
	RenderThread(SystemState* s);
	~RenderThread();
	/**
	   The EngineData object must survive for the whole life of this RenderThread
	*/
	void start(const EngineData* data);
	/*
	   The stop function should be call on exit even if the thread is not started
	*/
	void stop();
	void wait();
	void draw(bool force);

	//The calling context MUST call this function with the transformation matrix ready
	//void acquireTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax);
	//void blitTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax);

	/**
		Allocates a chunk from the shared texture
	*/
	TextureChunk allocateTexture(uint32_t w, uint32_t h, bool compact);
	/**
		Release texture
	*/
	void releaseTexture(const TextureChunk& chunk);
	/**
		Render a quad of given size using the given chunk
	*/
	void renderTextured(const TextureChunk& chunk, int32_t x, int32_t y, uint32_t w, uint32_t h);
	/**
		Load the given data in the given texture chunk
	*/
	void loadChunkBGRA(const TextureChunk& chunk, uint32_t w, uint32_t h, uint8_t* data);
	/**
		Enqueue something to be uploaded to texture
	*/
	void addUploadJob(ITextureUploadable* u);
	/**
	  	Add a mask to the stack mask
		@param d The DisplayObject used as a mask
		@param m The total matrix from the parent of the object to stage
		\pre A reference is not acquired, we assume the object life is protected until the corresponding pop
	*/
	void pushMask(DisplayObject* d, const MATRIX& m)
	{
		maskStack.push_back(MaskData(d,m));
	}
	/**
	  	Remove the last pushed mask
	*/
	void popMask()
	{
		maskStack.pop_back();
	}
	bool isMaskPresent()
	{
		return !maskStack.empty();
	}
	void renderMaskToTmpBuffer() const;
	void requestResize(uint32_t w, uint32_t h);
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
	int blitter_program;
	GLuint fboId;
	TextureBuffer tempTex;
	uint32_t windowWidth;
	uint32_t windowHeight;
	bool hasNPOTTextures;
	GLint fragmentTexScaleUniform;
	GLint yuvUniform;
	GLint maskUniform;
	GLint alphaUniform;
	GLint directUniform;
	GLint projectionMatrixUniform;
	GLint modelviewMatrixUniform;

	void renderErrorPage(RenderThread *rt, bool standalone);

	cairo_t *cairoTextureContext;
	cairo_surface_t *cairoTextureSurface;
	uint8_t *cairoTextureData;
	GLuint cairoTextureID;
	cairo_t* getCairoContext(int w, int h);
	void mapCairoTexture(int w, int h);
	void renderText(cairo_t *cr, const char *text, int x, int y);
	void setMatrixUniform(LSGL_MATRIX m) const;
};

};
extern TLSDATA lightspark::RenderThread* rt DLL_PUBLIC;
#endif
