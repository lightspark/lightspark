/**************************************************************************
    Lightspark, a free flash player implementation

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
#include <map>
#include <semaphore.h>
#include "swftypes.h"
#include "frame.h"
#include "vm.h"
#include "asobjects.h"
#include "flashdisplay.h"

#include <X11/Xlib.h>
#include <GL/glx.h>

class DisplayListTag;
class DictionaryTag;
class PlaceObject2Tag;
class EventDispatcher;
class ABCVm;
class InputThread;
class RenderThread;

typedef void* (*thread_worker)(void*);
long timeDiff(timespec& s, timespec& d);

class SWF_HEADER
{
public:
	UI8 Signature[3];
	UI8 Version;
	UI32 FileLength;
	RECT FrameSize;
	UI16 FrameRate;
	UI16 FrameCount;
public:
	SWF_HEADER(std::istream& in);
	const RECT& getFrameSize(){ return FrameSize; }
};

class ExecutionContext;

struct fps_profiling
{
	uint64_t render_time;
	uint64_t action_time;
	uint64_t cache_time;
	uint64_t fps;
	uint64_t event_count;
	uint64_t event_time;
	fps_profiling():render_time(0),action_time(0),cache_time(0),fps(0),event_count(0),event_time(0){}
};

class RootMovieClip: public MovieClip
{
friend class ParseThread;
protected:
	sem_t mutex;
	//Semaphore to wait for new frames to be available
	sem_t new_frame;
	LoaderInfo* loaderInfo;
private:
	RGB Background;
	std::list < DictionaryTag* > dictionary;
	RECT frame_size;
	sem_t sem_valid_frame_size;
	//Frames mutex (shared with drawing thread)
	sem_t sem_frames;

public:
	RootMovieClip();
	float frame_rate;
	RGB getBackground();
	void setBackground(const RGB& bg);
	void setFrameSize(const RECT& f);
	RECT getFrameSize();
	void setFrameCount(int f);
	void addToDictionary(DictionaryTag* r);
	DictionaryTag* dictionaryLookup(int id);
	void addToFrame(DisplayListTag* t);
	void commitFrame();
	void Render();
	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& owner);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o);
	void setVariableByMultiname(multiname& name, ASObject* o);
	void setVariableByString(const std::string& s, ASObject* o);
};

class SystemState:public RootMovieClip
{
public:
	ASFUNCTION(getBounds);
	RootMovieClip* root;

	bool shutdown;
	void setShutdownFlag();

	SystemState();
	~SystemState();
	fps_profiling* fps_prof;
	ABCVm* currentVm;
	InputThread* cur_input_thread;
	RenderThread* cur_render_thread;
	ThreadPool* cur_thread_pool;

	//DEBUG
	std::vector<std::string> events_name;
	void dumpEvents()
	{
		for(int i=0;i<events_name.size();i++)
			std::cout << events_name[i] << std::endl;
	}
};

class ParseThread
{
private:
	std::istream& f;
	pthread_t t;
	static void* worker(ParseThread*);
	static int error;
	SystemState* m_sys;
public:
	RootMovieClip* root;
	int version;
	ParseThread(SystemState* s, RootMovieClip* r,std::istream& in);
	~ParseThread();
	void wait();
	static void setError(){error=1;}

	//Used only in ParseThread context
	std::list < std::pair<PlaceInfo, IDisplayListElem*> >* parsingDisplayList;
	ASObject* parsingTarget;
};

enum ENGINE { SDL=0, NPAPI, GLX};
struct NPAPI_params
{
	Display* display;
	VisualID visual;
	Window window;
	int width;
	int height;
};


class InputThread
{
private:
	SystemState* m_sys;
	NPAPI_params* npapi_params;
	pthread_t t;
	static void* sdl_worker(InputThread*);
	static void* npapi_worker(InputThread*);

	std::multimap< std::string, EventDispatcher* > listeners;
	sem_t sem_listeners;

public:
	InputThread(SystemState* s,ENGINE e, void* param=NULL);
	~InputThread();
	void wait();
	void addListener(const std::string& type, EventDispatcher* tag);
	void broadcastEvent(const std::string& type);
};

class RenderThread
{
private:
	SystemState* m_sys;
	NPAPI_params* npapi_params;
	pthread_t t;
	static void* sdl_worker(RenderThread*);
	static void* npapi_worker(RenderThread*);
	static void* glx_worker(RenderThread*);
	sem_t mutex;
	sem_t render;
	sem_t end_render;
	int bak;
	static int error;

	Display* mDisplay;
	GLXFBConfig mFBConfig;
	GLXContext mContext;
	GLXPbuffer mPbuffer;
	Window mWindow;
	GC mGC;
	static int load_program();
	float* interactive_buffer;
public:
	RenderThread(SystemState* s,ENGINE e, void* param=NULL);
	~RenderThread();
	void draw();
	void wait();
	static int setError(){error=1;}
	float getIdAt(int x, int y);

	//Used only in RenderThread context
	MovieClip* currentClip;
	ExecutionContext* execContext;
	VirtualMachine vm;

	//OpenGL fragment programs
	int gpu_program;
	GLuint fboId;
	GLuint spare_tex;
	GLuint data_tex;
	int width;
	int height;

};
#endif
