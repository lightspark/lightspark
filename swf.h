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

#ifndef SWF_H
#define SWF_H

#include "compat.h"
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <semaphore.h>
#include <string>
#include "swftypes.h"
#include "frame.h"
#include "scripting/flashdisplay.h"
#include "timer.h"
#include "backends/graphics.h"
#include "backends/sound.h"

#include "platforms/pluginutils.h"

#include <GL/glew.h>
#ifndef WIN32
#include <GL/glx.h>
#else
//#include <windows.h>
#endif

namespace lightspark
{

class DownloadManager;
class DisplayListTag;
class DictionaryTag;
class ABCVm;
class InputThread;
class RenderThread;
class ParseThread;
class Tag;

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
	void tick();
private:
	//Semaphore to wait for new frames to be available
	sem_t new_frame;
	bool parsingIsFailed;
	RGB Background;
	std::list < DictionaryTag* > dictionary;
	//frameSize and frameRate are valid only after the header has been parsed
	RECT frameSize;
	float frameRate;
	//Frames mutex (shared with drawing thread)
	Mutex mutexFrames;
	bool toBind;
	tiny_string bindName;
	Mutex mutexChildrenClips;
	std::set<MovieClip*> childrenClips;
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
	void labelCurrentFrame(const STRING& name);
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
	void registerChildClip(MovieClip* clip);
	void unregisterChildClip(MovieClip* clip);
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
	class EngineCreator: public IThreadJob
	{
	public:
		EngineCreator()
		{
			destroyMe=true;
		}
		void execute();
		void threadAbort();
	};
	friend class SystemState::EngineCreator;
	ThreadPool* threadPool;
	TimerThread* timerThread;
	ParseThread* parseThread;
	sem_t terminated;
	float renderRate;
	bool error;
	bool shutdown;
	RenderThread* renderThread;
	InputThread* inputThread;
	NPAPI_params npapiParams;
	ENGINE engine;
	void startRenderTicks();
	/**
		Create the rendering and input engines

		@pre engine and useAVM2 are known
	*/
	void createEngines();
	/**
	  	Destroys all the engines used in lightspark: timer, thread pool, vm...
	*/
#ifdef COMPILE_PLUGIN
	static void delayedCreation(SystemState* th);
#endif
	void stopEngines();
	//Useful to wait for complete download of the SWF
	Semaphore fileDumpAvailable;
	tiny_string dumpedSWFPath;
	bool waitingForDump;
	//Data for handling Gnash fallback
	enum VMVERSION { VMNONE=0, AVM1, AVM2 };
	VMVERSION vmVersion;
	pid_t childPid;
	bool useGnashFallback;
	void setParameters(ASObject* p);
	/*
	   	Used to keep a copy of the FlashVars, it's useful when gnash fallback is used
	*/
	std::string rawParameters;
	std::string rawCookies;
	static int hexToInt(char c);
	char cookiesFileName[32]; // "/tmp/lightsparkcookiesXXXXXX"
public:
	void setUrl(const tiny_string& url) DLL_PUBLIC;

	//Interative analysis flags
	bool showProfilingData;
	bool showInteractiveMap;
	bool showDebug;
	int xOffset;
	int yOffset;
	
	std::string errorCause;
	void setError(const std::string& c);
	bool shouldTerminate() const;
	bool isShuttingDown() const DLL_PUBLIC;
	bool isOnError() const;
	void setShutdownFlag() DLL_PUBLIC;
	void tick();
	void wait() DLL_PUBLIC;
	RenderThread* getRenderThread() const { return renderThread; }
	InputThread* getInputThread() const { return inputThread; }
	void setParamsAndEngine(ENGINE e, NPAPI_params* p) DLL_PUBLIC;
	void setDownloadedPath(const tiny_string& p) DLL_PUBLIC;
	void enableGnashFallback() DLL_PUBLIC;
	void needsAVM2(bool n);

	//Be careful, SystemState constructor does some global initialization that must be done
	//before any other thread gets started
	SystemState(ParseThread* p) DLL_PUBLIC;
	~SystemState();
	
	//Performance profiling
	ThreadProfile* allocateProfiler(const RGB& color);
	std::list<ThreadProfile> profilingData;
	
	Stage* stage;
	ABCVm* currentVm;
#ifdef ENABLE_SOUND
	SoundManager* soundManager;
#endif
	//Application starting time in milliseconds
	uint64_t startTime;

	//Class map
	std::map<tiny_string, Class_base*> classes;
	bool finalizingDestruction;
	std::vector<Tag*> tagsStorage;

	//Flags for command line options
	bool useInterpreter;
	bool useJit;

	void parseParametersFromFile(const char* f) DLL_PUBLIC;
	void parseParametersFromFlashvars(const char* vars) DLL_PUBLIC;
	void setCookies(const char* c) DLL_PUBLIC;
	void addJob(IThreadJob* j) DLL_PUBLIC;
	void addTick(uint32_t tickTime, ITickJob* job);
	void addWait(uint32_t waitTime, ITickJob* job);
	bool removeJob(ITickJob* job);
	void setRenderRate(float rate);
	float getRenderRate();

	//Stuff to be done once for process and not for plugin instance
	static void staticInit() DLL_PUBLIC;
	static void staticDeinit() DLL_PUBLIC;

	DownloadManager* downloadManager;

	enum SCALE_MODE { EXACT_FIT=0, NO_BORDER=1, NO_SCALE=2, SHOW_ALL=3 };
	SCALE_MODE scaleMode;
};

class ParseThread: public IThreadJob
{
private:
	std::istream& f;
	sem_t ended;
	bool isEnded;
	void execute();
	void threadAbort();
public:
	RootMovieClip* root;
	int version;
	bool useAVM2;
	ParseThread(RootMovieClip* r,std::istream& in) DLL_PUBLIC;
	~ParseThread();
	void wait() DLL_PUBLIC;
};

};
#endif
