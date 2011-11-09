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

#ifndef SWF_H
#define SWF_H

#include "compat.h"
#include <fstream>
#include <list>
#include <map>
#include <semaphore.h>
#include <string>
#include "swftypes.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/net/flashnet.h"
#include "timer.h"

#include "platforms/engineutils.h"

namespace lightspark
{

class ABCVm;
class AudioManager;
class Config;
class DownloadManager;
class DisplayListTag;
class DictionaryTag;
class ExtScriptObject;
class InputThread;
class ParseThread;
class PluginManager;
class RenderThread;
class SecurityManager;
class Tag;

//RootMovieClip is used as a ThreadJob for timed rendering purpose
class RootMovieClip: public MovieClip
{
friend class ParseThread;
protected:
	Mutex mutex;
	URLInfo origin;
private:
	bool parsingIsFailed;
	RGB Background;
	Spinlock dictSpinlock;
	std::list < _R<DictionaryTag> > dictionary;
	//frameSize and frameRate are valid only after the header has been parsed
	RECT frameSize;
	float frameRate;
	bool toBind;
	tiny_string bindName;
	void constructionComplete();
	/* those are private because you shouldn't call sys->*,
	 * but sys->getStage()->* instead.
	 */
	void initFrame();
	void advanceFrame();
	void setOnStage(bool staged);
	ACQUIRE_RELEASE_FLAG(finishedLoading);
public:
	RootMovieClip(LoaderInfo* li, bool isSys=false);
	bool hasFinishedLoading() { return ACQUIRE_READ(finishedLoading); }
	uint32_t version;
	uint32_t fileLength;
	RGB getBackground();
	void setBackground(const RGB& bg);
	void setFrameSize(const RECT& f);
	RECT getFrameSize() const;
	float getFrameRate() const;
	void setFrameRate(float f);
	void addToDictionary(_R<DictionaryTag> r);
	_R<DictionaryTag> dictionaryLookup(int id);
	void labelCurrentFrame(const STRING& name);
	void commitFrame(bool another);
	void revertFrame();
	void parsingFailed();
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	void bindToName(const tiny_string& n);
	void DLL_PUBLIC setOrigin(const tiny_string& u, const tiny_string& filename="");
	URLInfo& getOrigin() DLL_PUBLIC { return origin; };
/*	ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o);
	void setVariableByMultiname(multiname& name, ASObject* o);
	void setVariableByString(const std::string& s, ASObject* o);*/
	static RootMovieClip* getInstance(LoaderInfo* li);
	//DisplayObject interface
	_NR<RootMovieClip> getRoot();
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
	void plot(uint32_t max, cairo_t *cr);
};

class SystemState: public RootMovieClip, public ITickJob
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
		void jobFence(){}
	};
	friend class SystemState::EngineCreator;
	ThreadPool* threadPool;
	TimerThread* timerThread;
	sem_t terminated;
	float renderRate;
	bool error;
	bool shutdown;
	RenderThread* renderThread;
	InputThread* inputThread;
	EngineData* engineData;
	void startRenderTicks();
	/**
		Create the rendering and input engines

		@pre engine and useAVM2 are known
	*/
	void createEngines();
	/**
	  	Destroys all the engines used in lightspark: timer, thread pool, vm...
	*/
	void stopEngines();

	static void delayedCreation(SystemState* th);
	static void delayedStopping(SystemState* th);
	//Useful to wait for complete download of the SWF
	Semaphore fileDumpAvailable;
	tiny_string dumpedSWFPath;
	bool waitingForDump;
	//Data for handling Gnash fallback
	enum VMVERSION { VMNONE=0, AVM1, AVM2 };
	VMVERSION vmVersion;
	GPid childPid;
	bool useGnashFallback;

	//Parameters/FlashVars
	_NR<ASObject> parameters;
	void setParameters(_R<ASObject> p);
	/*
	   	Used to keep a copy of the FlashVars, it's useful when gnash fallback is used
	*/
	std::string rawParameters;

	//Cookies for Gnash fallback
	std::string rawCookies;
	char cookiesFileName[32]; // "/tmp/lightsparkcookiesXXXXXX"

	URLInfo url;
	Spinlock profileDataSpinlock;

	Mutex mutexFrameListeners;
	std::set<_R<DisplayObject>> frameListeners;
	/*
	   The head of the invalidate queue
	*/
	_NR<DisplayObject> invalidateQueueHead;
	/*
	   The tail of the invalidate queue
	*/
	_NR<DisplayObject> invalidateQueueTail;
	/*
	   The lock for the invalidate queue
	*/
	Spinlock invalidateQueueLock;
#ifdef PROFILING_SUPPORT
	/*
	   Output file for the profiling data
	*/
	tiny_string profOut;
#endif
protected:
	~SystemState();
public:
	void setURL(const tiny_string& url) DLL_PUBLIC;

	//Interative analysis flags
	bool showProfilingData;
	bool standalone;
	
	std::string errorCause;
	void setError(const std::string& c);
	bool hasError() { return error; }
	std::string& getErrorCause() { return errorCause; }
	bool shouldTerminate() const;
	bool isShuttingDown() const DLL_PUBLIC;
	bool isOnError() const;
	void setShutdownFlag() DLL_PUBLIC;
	void tick();
	void wait() DLL_PUBLIC;
	RenderThread* getRenderThread() const { return renderThread; }
	InputThread* getInputThread() const { return inputThread; }
	void setParamsAndEngine(EngineData* e, bool s) DLL_PUBLIC;
	void setDownloadedPath(const tiny_string& p) DLL_PUBLIC;
	void enableGnashFallback() DLL_PUBLIC;
	void needsAVM2(bool n);
	//DisplayObject interface
	_NR<Stage> getStage() const;

	//Be careful, SystemState constructor does some global initialization that must be done
	//before any other thread gets started
	SystemState(ParseThread* p, uint32_t fileSize) DLL_PUBLIC;
	void finalize();
	/* Stop engines, threads and free classes and objects.
	 * This call will decRef this object in the end,
	 * thus destroy() may cause a 'delete this'.
	 */
	void destroy() DLL_PUBLIC;
	
	//Performance profiling
	ThreadProfile* allocateProfiler(const RGB& color);
	std::list<ThreadProfile> profilingData;
	
	Stage* stage;
	ABCVm* currentVm;

	Config* config;
	PluginManager* pluginManager;
	AudioManager* audioManager;

	//Application starting time in milliseconds
	uint64_t startTime;

	//Class/Template map. They own one reference to each class/template
	std::map<QName, Class_base*> classes;
	std::map<QName, Template_base*> templates;

	//Flags for command line options
	bool useInterpreter;
	bool useJit;
	bool exitOnError;

	//Parameters/FlashVars
	void parseParametersFromFile(const char* f) DLL_PUBLIC;
	void parseParametersFromFlashvars(const char* vars) DLL_PUBLIC;
	_NR<ASObject> getParameters() const;

	//Cookies management for Gnash fallback
	void setCookies(const char* c) DLL_PUBLIC;

	//Interfaces to the internal thread pool and timer thread
	void addJob(IThreadJob* j) DLL_PUBLIC;
	void addTick(uint32_t tickTime, ITickJob* job);
	void addWait(uint32_t waitTime, ITickJob* job);
	bool removeJob(ITickJob* job);

	void setRenderRate(float rate);
	float getRenderRate();
	/*
	   This is not supposed to be used in the VM, it's only useful to create the Downloader when plugin is being used
	   So don't create a smart reference
	*/
	LoaderInfo* getLoaderInfo() const { return loaderInfo.getPtr(); }

	//Stuff to be done once for process and not for plugin instance
	static void staticInit() DLL_PUBLIC;
	static void staticDeinit() DLL_PUBLIC;

	DownloadManager* downloadManager;
	IntervalManager* intervalManager;
	SecurityManager* securityManager;
	ExtScriptObject* extScriptObject;

	enum SCALE_MODE { EXACT_FIT=0, NO_BORDER=1, NO_SCALE=2, SHOW_ALL=3 };
	SCALE_MODE scaleMode;
	
	//Static AS class properties
	//NAMING: static$CLASSNAME$$PROPERTYNAME$
	//	NetConnection
	ObjectEncoding::ENCODING staticNetConnectionDefaultObjectEncoding;
	ObjectEncoding::ENCODING staticByteArrayDefaultObjectEncoding;
	
	//enterFrame event management
	void registerFrameListener(_R<DisplayObject> clip);
	void unregisterFrameListener(_R<DisplayObject> clip);

	//tags management
	void registerTag(Tag* t);

	//Invalidation queue management
	void addToInvalidateQueue(_R<DisplayObject> d);
	void flushInvalidationQueue();

	//Resize support
	void resizeCompleted() const;

#ifdef PROFILING_SUPPORT
	void setProfilingOutput(const tiny_string& t) DLL_PUBLIC;
	const tiny_string& getProfilingOutput() const;
	std::vector<ABCContext*> contextes;
	void saveProfilingInformation();
#endif
};

class ParseThread: public IThreadJob
{
public:
	int version;
	bool useAVM2;
	bool useNetwork;
	// Parse an object from stream. The type is detected
	// automatically. After parsing the new object is available
	// from getParsedObject().
	ParseThread(std::istream& in, Loader *loader=NULL, tiny_string url="") DLL_PUBLIC;
	// Parse a clip from stream into root. The stream must be an
	// SWF file.
	ParseThread(std::istream& in, RootMovieClip *root) DLL_PUBLIC;
	~ParseThread();
	FILE_TYPE getFileType() const { return fileType; }
        _NR<DisplayObject> getParsedObject();
	void setRootMovie(RootMovieClip *root);
	RootMovieClip *getRootMovie();
	static FILE_TYPE recognizeFile(uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4);
private:
	std::istream& f;
	std::streambuf* zlibFilter;
	std::streambuf* backend;
	Loader *loader;
	_NR<DisplayObject> parsedObject;
	Spinlock objectSpinlock;
	tiny_string url;
	FILE_TYPE fileType;
	void execute();
	void threadAbort();
	void jobFence() {};
	void parseSWFHeader(RootMovieClip *root, UI8 ver);
	void parseSWF(UI8 ver);
	void parseBitmap();
};

};
extern TLSDATA lightspark::SystemState* sys DLL_PUBLIC;
#endif
