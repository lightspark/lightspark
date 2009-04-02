/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef SWF_H
#define SWF_H

#include <iostream>
#include <fstream>
#include <list>
#include <semaphore.h>
#include "swftypes.h"
#include "frame.h"

#include <X11/Xlib.h>
#include <GL/glx.h>

class DisplayListTag;
class RenderTag;
class IActiveObject;

class SWF_HEADER
{
private:
	UI8 Signature[3];
	UI8 Version;
	UI32 FileLenght;
	RECT FrameSize;
	UI16 FrameRate;
	UI16 FrameCount;
public:
	SWF_HEADER(std::istream& in);
	const RECT& getFrameSize(){ return FrameSize; }
};

class RunState
{
public:
	int FP;
	int next_FP;
	bool stop_FP;
	RunState();
};

class MovieClip
{
public:
	std::list < DisplayListTag* > displayList;
	//Frames mutex (shared with drawing thread)
	sem_t sem_frames;
	std::vector<Frame> frames;
	RunState state;

	MovieClip();
};

class SystemState
{
//friend int main();
friend class ParseThread;
public:
	MovieClip clip;
	RECT frame_size;

public:
	//Semaphore to wait for new frames to be available
	sem_t new_frame;

	sem_t sem_dict;
	std::vector < RenderTag* > dictionary;

	RGBA Background;

	SystemState();
	sem_t sem_run;

	std::list < DisplayListTag* >* currentDisplayList;
	RunState* currentState;

	bool update_request;
};

class ParseThread
{
private:
	static pthread_t t;
	static void* worker(void*);
public:
	ParseThread(std::istream& in);
	void wait();
};

enum ENGINE { SDL=0, NPAPI};

class InputThread
{
private:
	static pthread_t t;
	static void* worker(void*);
	static std::list< IActiveObject* > listeners;
	static sem_t sem_listeners;

public:
	InputThread();
	void wait();
	static void addListener(IActiveObject* tag);
};

struct NPAPI_params
{
	VisualID visual;
	Window window;
	int width;
	int height;
};

class RenderThread
{
private:
	static pthread_t t;
	static void* sdl_worker(void*);
	static void* npapi_worker(void*);
	static sem_t mutex;
	static sem_t render;
	static sem_t end_render;
	static Frame* cur_frame;
	static Frame bak_frame;
	static int bak;

	static GLXFBConfig mFBConfig;
	static GLXContext mContext;
public:
	RenderThread(ENGINE e, void* param=NULL);
	static void draw(Frame* f);
	void wait();
};
#endif
