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

#ifndef RENDERING_H
#define RENDERING_H

#include "timer.h"

namespace lightspark
{

class RenderThread: public ITickJob
{
private:
	SystemState* m_sys;
	pthread_t t;
	bool terminated;
	static void* sdl_worker(RenderThread*);
#ifdef COMPILE_PLUGIN
	NPAPI_params* npapi_params;
	static void* gtkplug_worker(RenderThread*);
#endif
	void commonGLInit(int width, int height);
	void commonGLResize(int width, int height);
	void commonGLDeinit();
	sem_t render;
	sem_t inputDone;
	bool inputNeeded;
	bool inputDisabled;
	std::string fontPath;
	bool resizeNeeded;
	uint32_t newWidth;
	uint32_t newHeight;
	float scaleX;
	float scaleY;
	int offsetX;
	int offsetY;

#ifndef WIN32
	Display* mDisplay;
	GLXFBConfig mFBConfig;
	GLXContext mContext;
	Window mWindow;
#endif
	uint64_t time_s, time_d;

	bool loadShaderPrograms();
	uint32_t* interactive_buffer;
	bool tempBufferAcquired;
	void tick();
	int frameCount;
	int secsCount;
	std::vector<float> idStack;
	Mutex mutexResources;
	std::set<GLResource*> managedResources;
public:
	RenderThread(SystemState* s,ENGINE e, void* param=NULL);
	~RenderThread();
	void wait();
	void draw();
	float getIdAt(int x, int y);
	//The calling context MUST call this function with the transformation matrix ready
	void glAcquireTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax);
	void glBlitTempBuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax);
	/**
		Add a GLResource to the managed pool
		@param res The GLResource to be manged
		@pre running inside the renderthread OR the resourceMutex is acquired
	*/
	void addResource(GLResource* res);
	/**
		Remove a GLResource from the managed pool
		@param res The GLResource to stop managing
		@pre running inside the renderthread OR the resourceMutex is acquired
	*/
	void removeResource(GLResource* res);
	/**
		Acquire the resource mutex to do resource cleanup
	*/
	void acquireResourceMutex();
	/**
		Release the resource mutex
	*/
	void releaseResourceMutex();

	void requestInput();
	void requestResize(uint32_t w, uint32_t h);
	void pushId()
	{
		idStack.push_back(currentId);
	}
	void popId()
	{
		currentId=idStack.back();
		idStack.pop_back();
	}

	//OpenGL programs
	int gpu_program;
	int blitter_program;
	GLuint fboId;
	TextureBuffer dataTex;
	TextureBuffer mainTex;
	TextureBuffer tempTex;
	TextureBuffer inputTex;
	uint32_t windowWidth;
	uint32_t windowHeight;
	bool hasNPOTTextures;
	GLuint fragmentTexScaleUniform;
	
	InteractiveObject* selectedDebug;
	float currentId;
	bool materialOverride;
};

};
#endif
