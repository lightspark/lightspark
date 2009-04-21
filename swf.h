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
#include "vm.h"

#include <X11/Xlib.h>
#include <GL/glx.h>

class DisplayListTag;
class RenderTag;
class IActiveObject;

typedef void* (*thread_worker)(void*);

class SWF_HEADER
{
private:
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

class RunState
{
public:
	int FP;
	int next_FP;
	bool stop_FP;
public:
	RunState();
	void prepareNextFP();
	void tick()
	{
		if(!stop_FP)
			FP=next_FP;
	}
};

class MovieClip
{
public:
	std::list < DisplayListTag* > displayList;
	//Frames mutex (shared with drawing thread)
	sem_t sem_frames;
	std::list<Frame> frames;
	RunState state;
public:
	MovieClip();
	void addToDisplayList(DisplayListTag* r);
};

class ExecutionContext;

class SystemState:public ISWFObject//,public ISWFClass
{
private:
	MovieClip clip;
	RECT frame_size;

	//Semaphore to wait for new frames to be available
	sem_t new_frame;

	sem_t sem_dict;
	std::list < RenderTag* > dictionary;

	RGB Background;

	sem_t sem_run;

	bool update_request;

	sem_t mutex;

	std::vector<SWFObject> Classes;

	std::vector<SWFObject> Variables;
	void registerVariable(const STRING& name, const SWFObject& o);
	std::vector<SWFObject>& getVariables();
public:
	SWFObject getVariableByName(const STRING& name);
	void setVariableByName(const STRING& name, const SWFObject& o);
	void dumpVariables();
	ISWFObject* getParent() { return NULL; }

	//Initial instances
	void registerClass(const STRING& name, const SWFObject& o);
	SWFObject instantiateClass(const STRING& name);

	bool performance_profiling;
	VirtualMachine vm;
	//Used only in ParseThread context
	std::list < DisplayListTag* >* parsingDisplayList;
	ISWFObject* parsingTarget;

	//Used only in RenderThread context
	RunState* currentState;
	ISWFObject* renderTarget;
	ExecutionContext* execContext;

	SystemState();
	void waitToRun();
	Frame& getFrameAtFP();
	void advanceFP();
	void setFrameSize(const RECT& f);
	RECT getFrameSize();
	void addToDictionary(RenderTag* r);
	void addToDisplayList(DisplayListTag* r);
	void commitFrame();
	RGB getBackground();
	void setBackground(const RGB& bg);
	void setUpdateRequest(bool s);
	RenderTag* dictionaryLookup(UI16 id);
	void reset();
	void _register(){}
	SWFOBJECT_TYPE getObjectType();
};

class ParseThread
{
private:
	SystemState* m_sys;
	std::istream& f;
	pthread_t t;
	static void* worker(ParseThread*);
	static int error;
public:
	ParseThread(SystemState* s, std::istream& in);
	~ParseThread();
	void wait();
	static void setError(){error=1;}
};

enum ENGINE { SDL=0, NPAPI};
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
	std::list< IActiveObject* > listeners;
	sem_t sem_listeners;

public:
	InputThread(SystemState* s,ENGINE e, void* param=NULL);
	~InputThread();
	void wait();
	void addListener(IActiveObject* tag);
};

class RenderThread
{
private:
	SystemState* m_sys;
	NPAPI_params* npapi_params;
	pthread_t t;
	static void* sdl_worker(RenderThread*);
	static void* npapi_worker(RenderThread*);
	sem_t mutex;
	sem_t render;
	sem_t end_render;
	Frame* cur_frame;
	Frame bak_frame;
	int bak;
	static int error;

	GLXFBConfig mFBConfig;
	GLXContext mContext;
	GC mGC;
public:
	RenderThread(SystemState* s,ENGINE e, void* param=NULL);
	~RenderThread();
	void draw(Frame* f);
	void wait();
	static int setError(){error=1;}
};
#endif
