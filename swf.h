/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "compat.h"
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <semaphore.h>
#include <FTGL/ftgl.h>
#include "swftypes.h"
#include "frame.h"
#include "vm.h"
#include "flashdisplay.h"
#include "timer.h"

#include <GL/glew.h>
#ifndef WIN32
#include <X11/Xlib.h>
#include <GL/glx.h>
namespace X11Intrinsic
{
#include <X11/Intrinsic.h>
};
#else
//#include <windows.h>
#endif

#ifdef COMPILE_PLUGIN
#include <gtk/gtk.h>
#endif

namespace lightspark
{

class DownloadManager;
class DisplayListTag;
class DictionaryTag;
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
	bool valid;
	SWF_HEADER(std::istream& in);
	const RECT& getFrameSize(){ return FrameSize; }
};

//RootMovieClip is used as a ThreadJob for timed rendering purpose
class RootMovieClip: public MovieClip, public ITickJob
{
friend class ParseThread;
protected:
	sem_t mutex;
	bool initialized;
	tiny_string origin;
private:
	//Semaphore to wait for new frames to be available
	sem_t new_frame;
	bool parsingIsFailed;
	RGB Background;
	std::list < DictionaryTag* > dictionary;
	//frameSize and frameRate are valid only after the header has been parsed
	RECT frameSize;
	float frameRate;
	mutable sem_t sem_valid_size;
	mutable sem_t sem_valid_rate;
	//Frames mutex (shared with drawing thread)
	sem_t sem_frames;
	bool toBind;
	tiny_string bindName;
	void tick();
public:
	RootMovieClip(LoaderInfo* li, bool isSys=false);
	~RootMovieClip();
	unsigned int version;
	unsigned int fileLenght;
	RGB getBackground();
	void setBackground(const RGB& bg);
	void setFrameSize(const RECT& f);
	RECT getFrameSize() const;
	float getFrameRate() const;
	void setFrameRate(float f);
	void setFrameCount(int f);
	void addToDictionary(DictionaryTag* r);
	DictionaryTag* dictionaryLookup(int id);
	void addToFrame(DisplayListTag* t);
	void addToFrame(ControlTag* t);
	void commitFrame(bool another);
	void revertFrame();
	void Render();
	void parsingFailed();
	bool getBounds(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void bindToName(const tiny_string& n);
	void initialize();
	void setOrigin(const tiny_string& o){origin=o;}
	const tiny_string& getOrigin(){return origin;}
/*	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o);
	void setVariableByMultiname(multiname& name, ASObject* o);
	void setVariableByString(const std::string& s, ASObject* o);*/
};

class ThreadProfile
{
private:
	Mutex mutex;
	class ProfilingData
	{
	public:
		uint32_t index;
		uint32_t timing;
		std::string tag;
 		ProfilingData(uint32_t i, uint32_t t):index(i),timing(t){}
	};
	std::deque<ProfilingData> data;
	RGB color;
	int32_t len;
	uint32_t tickCount;
public:
	ThreadProfile(const RGB& c,uint32_t l):mutex("ThreadProfile"),color(c),len(l),tickCount(0){}
	void accountTime(uint32_t time);
	void setTag(const std::string& tag);
	void tick();
	void plot(uint32_t max, FTFont* font);
};

class SystemState: public RootMovieClip
{
private:
	ThreadPool* threadPool;
	TimerThread* timerThread;
	sem_t terminated;
	float renderRate;
	bool error;
	bool shutdown;
public:
	void setUrl(const tiny_string& url);

	//Interative analysis flags
	bool showProfilingData;
	bool showInteractiveMap;
	bool showDebug;
	int xOffset;
	int yOffset;
	
	std::string errorCause;
	void setError(std::string& c);
	bool shouldTerminate() const;
	bool isShuttingDown() const;
	bool isOnError() const;
	void setShutdownFlag();
	void tick();
	void wait();

	//Be careful, SystemState constructor does some global initialization that must be done
	//before any other thread gets started
	SystemState();
	~SystemState();
	
	//Performance profiling
	ThreadProfile* allocateProfiler(const RGB& color);
	std::list<ThreadProfile> profilingData;
	
	Stage* stage;
	ABCVm* currentVm;
	InputThread* inputThread;
	RenderThread* renderThread;
	//Application starting time in milliseconds
	uint64_t startTime;

	//Class map
	std::map<tiny_string, Class_base*> classes;

	//DEBUG
	std::vector<tiny_string> events_name;
	void dumpEvents()
	{
		for(unsigned int i=0;i<events_name.size();i++)
			std::cout << events_name[i] << std::endl;
	}

	//Flags for command line options
	bool useInterpreter;
	bool useJit;

	void parseParameters(std::istream& i);
	void setParameters(ASObject* p);
	void addJob(IThreadJob* j);
	void addTick(uint32_t tickTime, ITickJob* job);
	void addWait(uint32_t waitTime, ITickJob* job);
	bool removeJob(ITickJob* job);
	void setRenderRate(float rate);
	float getRenderRate();

	DownloadManager* downloadManager;
};

class ParseThread: public IThreadJob
{
private:
	std::istream& f;
	sem_t ended;
	void execute();
	void threadAbort();
public:
	RootMovieClip* root;
	int version;
	ParseThread(RootMovieClip* r,std::istream& in);
	~ParseThread();
	void wait();
};

enum ENGINE { SDL=0, NPAPI, GLX, GTKPLUG};
#ifdef COMPILE_PLUGIN
struct NPAPI_params
{
	Display* display;
	GtkWidget* container;
	VisualID visual;
	Window window;
	int width;
	int height;
};
#else
struct NPAPI_params
{
};
#endif

class InputThread
{
private:
	SystemState* m_sys;
	pthread_t t;
	bool terminated;
	static void* sdl_worker(InputThread*);
	#ifdef COMPILE_PLUGIN
	NPAPI_params* npapi_params;
	static void npapi_worker(X11Intrinsic::Widget xt_w, InputThread* th, XEvent* xevent, X11Intrinsic::Boolean* b);
	static gboolean gtkplug_worker(GtkWidget *widget, GdkEvent *event, InputThread* th);
	#endif

	std::vector<InteractiveObject* > listeners;
	Mutex mutexListeners;
	Mutex mutexDragged;

	Sprite* curDragged;
	RECT dragLimit;
public:
	InputThread(SystemState* s,ENGINE e, void* param=NULL);
	~InputThread();
	void wait();
	void addListener(InteractiveObject* ob);
	void removeListener(InteractiveObject* ob);
	void broadcastEvent(const tiny_string& type);
	void enableDrag(Sprite* s, const RECT& limit);
	void disableDrag();
};

class RenderThread: public ITickJob
{
private:
	SystemState* m_sys;
	pthread_t t;
	bool terminated;
	static void* sdl_worker(RenderThread*);
	static void* glx_worker(RenderThread*);
	#ifdef COMPILE_PLUGIN
	NPAPI_params* npapi_params;
	static void* npapi_worker(RenderThread*);
	static void* gtkplug_worker(RenderThread*);
	#endif
	void commonGLInit(int width, int height, unsigned int t2[3]);
	void commonGLDeinit(unsigned int t2[3]);
	sem_t render;
	sem_t inputDone;
	bool inputNeeded;

#ifndef WIN32
	Display* mDisplay;
	GLXFBConfig mFBConfig;
	GLXContext mContext;
	GLXPbuffer mPbuffer;
	Window mWindow;
	GC mGC;

	timespec ts,td;
#endif
	bool loadShaderPrograms();
	uint32_t* interactive_buffer;
	bool fbAcquired;
	void tick();
	int frameCount;
	int secsCount;
	std::vector<float> idStack;
	
public:
	RenderThread(SystemState* s,ENGINE e, void* param=NULL);
	~RenderThread();
	void wait();
	void draw();
	float getIdAt(int x, int y);
	//The calling context MUST call this function with the transformation matrix ready
	void glAcquireFramebuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax);
	void glBlitFramebuffer(number_t xmin, number_t xmax, number_t ymin, number_t ymax);

	void requestInput();
	void glClearIdBuffer();
	bool glAcquireIdBuffer();
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
	GLuint spare_tex;
	GLuint data_tex;
	int width;
	int height;
	
	InteractiveObject* selectedDebug;
	float currentId;
	bool materialOverride;
};

};
#endif
