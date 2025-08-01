/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#define SWF_H 1

#include "asobject.h"
#include "caseless_string.h"
#include "forwards/events.h"
#include "forwards/scripting/flash/text/flashtext.h"
#include "interfaces/logger.h"
#include "interfaces/threading.h"
#include "interfaces/timer.h"
#include "utils/optional.h"
#include "utils/timespec.h"
#include "backends/graphics.h"
#include "backends/urlutils.h"
#include "timer.h"
#include "threading.h"
#include "compat.h"
#include <fstream>
#include <list>
#include <queue>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <tsl/ordered_set.h>
#include "swftypes.h"
#include "memory_support.h"
#include "scripting/abcutils.h"

using namespace std;

class uncompressing_filter;

namespace lightspark
{

class AudioManager;
class Config;
class ControlTag;
class DisplayListTag;
class DictionaryTag;
class DefineScalingGridTag;
class EventLoop;
class ExtScriptObject;
class InputThread;
class IntervalManager;
class ParseThread;
class PluginManager;
class RenderThread;
class SecurityManager;
class LocaleManager;
class CurrencyManager;
class DownloadManager;
class Tag;
class Class_inherit;
class FontTag;
class SoundTransform;
class ASFile;
class EngineData;
class NativeApplication;
class RootMovieClip;
class Null;
class Undefined;
class Boolean;
class Class_base;
class ThreadPool;
class TimerThread;
class LocalConnectionEvent;
class ABCVm;
class Function;

enum class FramePhase
{
	NO_PHASE,
	ADVANCE_FRAME,
	INIT_FRAME,
	EXECUTE_FRAMESCRIPT,
	EXIT_FRAME,
	IDLE
};

class ThreadProfile
{
private:
	/* ThreadProfile cannot be copied because Mutex cannot */
	ThreadProfile(const ThreadProfile&) { assert(false); }
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
	EngineData* engineData;
public:
	ThreadProfile(const RGB& c,uint32_t l,EngineData* _engineData):color(c),len(l),tickCount(0),engineData(_engineData){}
	void accountTime(uint32_t time);
	void setTag(const std::string& tag);
	void tick();
	void plot(uint32_t max, cairo_t *cr);
};

class DLL_PUBLIC SystemState: public ITickJob, public InvalidateQueue
{
private:
	ThreadPool* threadPool;
	ThreadPool* downloadThreadPool;
	LSTimers timers;
	Optional<TimeSpec> _timeUntilNextTick;
	TimeSpec frameAccumulator;
	std::list<TimeSpec> recentFrameTimings;
	TimerThread* timerThread;
	TimerThread* frameTimerThread;
	EventLoop* eventLoop;
	ITime* time;
	Optional<ILogger&> logger;
	Semaphore terminated;
	float renderRate;
	bool error;
	bool shutdown;
	bool firsttick;
	bool localstorageallowed;
	bool influshing;
	bool inMouseEvent;
	bool inWindowMove;
	bool hasExitCode;
	int innerGotoCount;
	int exitCode;
	RenderThread* renderThread;
	InputThread* inputThread;
	EngineData* engineData;
	void startRenderTicks();
	Mutex rootMutex;
	/**
		Create the rendering and input engines

		@pre engine and useAVM2 are known
	*/
	void createEngines();

	void launchGnash();
	/**
	  	Destroys all the engines used in lightspark: timer, thread pool, vm...
	*/
	void stopEngines();

	static void delayedCreation(SystemState* sys);
	void delayedStopping();

	/* dumpedSWFPathAvailable is signaled after dumpedSWFPath has been set */
	Semaphore dumpedSWFPathAvailable;
	tiny_string dumpedSWFPath;

	//Data for handling Gnash fallback
	enum VMVERSION { VMNONE=0, AVM1, AVM2 };
	VMVERSION vmVersion;
#ifdef _WIN32
	HANDLE childPid;
#else
	int childPid;
#endif

	//shared null, undefined, true and false instances
	Null* null;
	Undefined* undefined;
	Boolean* trueRef;
	Boolean* falseRef;
	Class_base* objClassRef;

	//Parameters/FlashVars
	_NR<ASObject> parameters;
	void setParameters(_R<ASObject> p);
	/*
	   	Used to keep a copy of the FlashVars, it's useful when gnash fallback is used
	*/
	std::string rawParameters;

	//Cookies for Gnash fallback
	std::string rawCookies;
	char* cookiesFileName;

	URLInfo url;
	Mutex profileDataSpinlock;

	Mutex mutexFrameListeners;
	std::list<DisplayObject*> frameListeners;
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
	Mutex invalidateQueueLock;
	
	Mutex drawjobLock;
	std::unordered_set<AsyncDrawJob*> drawJobsNew;
	std::unordered_set<AsyncDrawJob*> drawJobsPending;
#ifdef PROFILING_SUPPORT
	/*
	   Output file for the profiling data
	*/
	tiny_string profOut;
#endif
#ifdef MEMORY_USAGE_PROFILING
	mutable Mutex memoryAccountsMutex;
	std::list<MemoryAccount> memoryAccounts;
#endif
	mutable Mutex fontListMutex;
	// TODO: Make this a map, once we support proper HTML formatting.
	// A global list of all fonts registered via the AS3 API
	// `Font.registerFont()`.
	std::vector<ASFont*> globalEmbeddedFontList;

	/*
	 * Pooling support
	 */
	mutable Mutex poolMutex;
	tsl::ordered_set<tiny_string, CaselessHash, std::equal_to<>> uniqueStringMap;
	map<nsNameAndKindImpl, uint32_t> uniqueNamespaceImplMap;
	unordered_map<uint32_t,nsNameAndKindImpl> uniqueNamespaceIDMap;
	//This needs to be atomic because it's decremented without the mutex held
	ATOMIC_INT32(lastUsedNamespaceId);
	
	Mutex mainsignalMutex;
	Cond mainsignalCond;
	void systemFinalize();
	std::map<tiny_string, Class_base *> classnamemap;
	unordered_set<DisplayObject*> listResetParent;
	Mutex mutexLocalConnection;
	std::map<uint32_t, _NR<ASObject>> localconnection_client_map;
	ACQUIRE_RELEASE_VARIABLE(FramePhase, framePhase);
	ATOMIC_INT32(instanceCounter); // used to create unique instanceX names for AVM1
public:
	void setURL(const tiny_string& url) DLL_PUBLIC;
	tiny_string getDumpedSWFPath() const { return dumpedSWFPath;}
	void waitThreadpool();

	//Interative analysis flags
	bool showProfilingData;
	bool standalone;
	bool allowFullscreen;
	bool allowFullscreenInteractive;
	//Flash for execution mode
	enum FLASH_MODE { FLASH=0, AIR };
	const FLASH_MODE flashMode;
	uint32_t swffilesize;
	asAtom nanAtom;
	uint32_t getNextInstanceName();
	// the global object for AVM1
	Global* avm1global;
	void setupAVM1();
	// Error types used to decide when to exit, extend as a bitmap
	enum ERROR_TYPE { ERROR_NONE    = 0x0000,
			  ERROR_PARSING = 0x0001,
			  ERROR_OTHER   = 0x8000,
			  ERROR_ANY     = 0xFFFF };
	std::string errorCause;
	void setError(const std::string& c, ERROR_TYPE type=ERROR_OTHER);
	bool hasError() { return error; }
	std::string& getErrorCause() { return errorCause; }
	bool shouldTerminate() const;
	bool isShuttingDown() const DLL_PUBLIC;
	bool isOnError() const DLL_PUBLIC;
	void setShutdownFlag() DLL_PUBLIC;
	void setExitCode(int exitcode) DLL_PUBLIC;
	int getExitCode() DLL_PUBLIC;
	void setInMouseEvent(bool inmouseevent);
	bool getInMouseEvent() const;
	void setWindowMoveMode(bool startmove);
	bool getInWindowMoveMode() const;
	void signalTerminated();
	std::map<tiny_string, _R<SharedObject> > sharedobjectmap;
	bool localStorageAllowed() const { return localstorageallowed; }
	void setLocalStorageAllowed(bool allowed);
	bool inInnerGoto() const { return innerGotoCount;}
	void runInnerGotoFrame(DisplayObject* innerClip, const std::vector<_R<DisplayObject>>& removedFrameScripts = {});

	void trace(const tiny_string& str);
	Optional<ILogger&> getLogger() { return logger; }

	uint64_t getCurrentTime_ms() const { return time->getCurrentTime_ms(); }
	uint64_t getCurrentTime_us() const { return time->getCurrentTime_us(); }
	uint64_t getCurrentTime_ns() const { return time->getCurrentTime_ns(); }
	TimeSpec now() const { return time->now(); }
	void sleep_ms(uint32_t ms) { return time->sleep_ms(ms); }
	void sleep_us(uint32_t us) { return time->sleep_us(us); }
	void sleep_ns(uint64_t ns) { return time->sleep_ns(ns); }

	size_t maxFramesPerTick() const;
	void addFrameTiming(const TimeSpec& elapsed);
	void runTick(const TimeSpec& delta);
	TimeSpec timeUntilNextFrame() const;
	void tick() override;
	void tickFence() override;

	RenderThread* getRenderThread() const { return renderThread; }
	InputThread* getInputThread() const { return inputThread; }
	void setParamsAndEngine(EngineData* e, bool s) DLL_PUBLIC;
	void setDownloadedPath(const tiny_string& p) DLL_PUBLIC;
	void needsAVM2(bool n);
	void stageCoordinateMapping(uint32_t windowWidth, uint32_t windowHeight, int& offsetX, int& offsetY, float& scaleX, float& scaleY);
	void stageCoordinateMapping(const Vector2& windowSize, Vector2& offset, Vector2f& scale);
	Vector2f windowToStagePoint(const Vector2f& windowPos);
	void windowToStageCoordinates(int windowX, int windowY, int& stageX, int& stageY);
	void setLocalConnectionClient(uint32_t nameID,_NR<ASObject> client)
	{
		Locker l(mutexLocalConnection);
		localconnection_client_map[nameID]=client;
	}
	void removeLocalConnectionClient(uint32_t nameID)
	{
		Locker l(mutexLocalConnection);
		localconnection_client_map.erase(nameID);
	}
	void handleLocalConnectionEvent(LocalConnectionEvent* ev);
	FramePhase getFramePhase() const { return ACQUIRE_READ(framePhase); }
	void setFramePhase(FramePhase phase) { RELEASE_WRITE(framePhase, phase); }

	/**
	 * Be careful, SystemState constructor does some global initialization that must be done
	 * before any other thread gets started
	 * \param fileSize The size of the SWF being parsed, if known
	 * \param mode FLASH or AIR
	 */
	SystemState
	(
		uint32_t fileSize,
		FLASH_MODE mode,
		EventLoop* _eventLoop = nullptr,
		ITime* _time = nullptr,
		Optional<ILogger&> _logger = {},
		bool _runSingleThreaded = false,
		size_t threads = SIZE_MAX
	) DLL_PUBLIC;
	~SystemState();
	/* Stop engines, threads and free classes and objects.
	 * This call will decRef this object in the end,
	 * thus destroy() may cause a 'delete this'.
	 */
	void destroy() DLL_PUBLIC;
	
	//Performance profiling
	ThreadProfile* allocateProfiler(const RGB& color);
	std::list<ThreadProfile*> profilingData;
	
	inline Null* getNullRef() const
	{
		return null;
	}
	
	inline Undefined* getUndefinedRef() const
	{
		return undefined;
	}
	
	inline Boolean* getTrueRef() const
	{
		return trueRef;
	}
	
	inline Boolean* getFalseRef() const
	{
		return falseRef;
	}

	inline Class_base* getObjectClassRef() const
	{
		return objClassRef;
	}

	RootMovieClip* mainClip;
	Stage* stage;
	ABCVm* currentVm;

	AudioManager* audioManager;

	//Application starting time in milliseconds
	uint64_t startTime;

	//This is an array of fixed size, we can avoid using std::vector
	Class_base** builtinClasses;

	//Flags for command line options
	bool useInterpreter;
	bool useFastInterpreter;
	bool useJit;
	bool ignoreUnhandledExceptions;
	bool runSingleThreaded;
	ERROR_TYPE exitOnError;

	//Parameters/FlashVars
	void parseParametersFromFile(const char* f) DLL_PUBLIC;
	void parseParametersFromFlashvars(const char* vars) DLL_PUBLIC;
	void parseParametersFromURL(const URLInfo& url) DLL_PUBLIC;
	static void parseParametersFromURLIntoObject(const URLInfo& url, _R<ASObject> outParams);
	_NR<ASObject> getParameters() const;

	//Cookies management (HTTP downloads and Gnash fallback)
	void setCookies(const char* c) DLL_PUBLIC;
	const std::string& getCookies();

	//Interfaces to the internal thread pool and timer thread
	void addJob(IThreadJob* j) DLL_PUBLIC;
	// downloaders may be executed from inside a job from the main threadpool,
	// so we use a second threadpool for them, to avoid deadlocks
	void addDownloadJob(IThreadJob* j) DLL_PUBLIC;
	void addTick(uint32_t tickTime, ITickJob* job);
	void addFrameTick(ITickJob* job);
	void addFrameTick(uint32_t tickTime, ITickJob* job);
	void addWait(uint32_t waitTime, ITickJob* job);
	void removeJob(ITickJob* job);
	void pushEvent(const LSEvent& event);
	void updateTimers(const TimeSpec& delta, bool allowFrameTimers = true);
	TimeSpec getFakeCurrentTime() const { return timers.getFakeCurrentTime(); }
	const LSTimer& getCurrentTimer() { return timers.getCurrentTimer(); }
	Optional<const TimeSpec&> timeUntilNextTick() const { return _timeUntilNextTick.asRef(); }

	void setRenderRate(float rate);
	float getRenderRate();

	/*
	 * The application domain for the system
	 */
	ApplicationDomain* systemDomain;

	ASWorker* worker;
	WorkerDomain* workerDomain;
	bool singleworker;
	Mutex workerMutex;
	void addWorker(ASWorker* w);
	void removeWorker(ASWorker* w);
	void addEventToBackgroundWorkers(_NR<EventDispatcher> obj, _R<Event> ev);

	//Stuff to be done once for process and not for plugin instance
	static void staticInit() DLL_PUBLIC;
	static void staticDeinit() DLL_PUBLIC;

	DownloadManager* downloadManager;
	IntervalManager* intervalManager;
	SecurityManager* securityManager;
	LocaleManager* localeManager;
	CurrencyManager* currencyManager;
	ExtScriptObject* extScriptObject;

	enum SCALE_MODE { EXACT_FIT=0, NO_BORDER=1, NO_SCALE=2, SHOW_ALL=3 };
	SCALE_MODE scaleMode;
	
	//Static AS class properties
	//TODO: Those should be different for each security domain
	//NAMING: static$CLASSNAME$$PROPERTYNAME$
	//	NetConnection
	OBJECT_ENCODING staticNetConnectionDefaultObjectEncoding;
	OBJECT_ENCODING staticByteArrayDefaultObjectEncoding;
	OBJECT_ENCODING staticSharedObjectDefaultObjectEncoding;
	bool staticSharedObjectPreventBackup;

	LINESTYLE2 avm1FocusRectLinestyle;

	
	//broadcast event management
	void registerFrameListener(DisplayObject* clip);
	void unregisterFrameListener(DisplayObject* clip);
	void addBroadcastEvent(const tiny_string& event);
	void handleBroadcastEvent(const tiny_string& event);

	//Invalidation queue management
	void addToInvalidateQueue(_R<DisplayObject> d) override;
	void flushInvalidationQueue();
	void AsyncDrawJobCompleted(AsyncDrawJob* j);
	void swapAsyncDrawJobQueue();

	//Resize support
	void resizeCompleted();

#ifdef PROFILING_SUPPORT
	void setProfilingOutput(const tiny_string& t) DLL_PUBLIC;
	const tiny_string& getProfilingOutput() const;
	std::vector<ABCContext*> contextes;
	void saveProfilingInformation();
#endif
	MemoryAccount* allocateMemoryAccount(const tiny_string& name) DLL_PUBLIC;
	MemoryAccount* unaccountedMemory;
	MemoryAccount* tagsMemory;
	MemoryAccount* stringMemory;
	MemoryAccount* textTokenMemory;
	MemoryAccount* shapeTokenMemory;
	MemoryAccount* morphShapeTokenMemory;
	MemoryAccount* bitmapTokenMemory;
	MemoryAccount* spriteTokenMemory;
#ifdef MEMORY_USAGE_PROFILING
	void saveMemoryUsageInformation(std::ofstream& out, int snapshotCount) const;
#endif
	using FontListCallback = std::function<void(ASFont*)>;
	void forEachEmbeddedFont(FontListCallback callback) const;
	void registerGlobalFont(ASFont* font);

	/*
	 * Pooling support
	 */
	uint32_t getUniqueStringId(const tiny_string& s);
	uint32_t getUniqueStringId(const tiny_string& s, bool caseSensitive);
	const tiny_string& getStringFromUniqueId(uint32_t id) const;
	/*
	 * Looks for the given nsNameAndKindImpl in the map.
	 * If not present it will be created with hintedId as it's id.
	 * The namespace id and the baseId are returned by reference.
	 */
	void getUniqueNamespaceId(const nsNameAndKindImpl& s, uint32_t hintedId, uint32_t& nsId, uint32_t& baseId);
	/*
	 * Looks for the given nsNameAndKindImpl in the map.
	 * If not present it will be create with an id chosen internally.
	 * The namespace id and the baseId are returned by reference.
	 */
	void getUniqueNamespaceId(const nsNameAndKindImpl& s, uint32_t& nsId, uint32_t& baseId);
	const nsNameAndKindImpl& getNamespaceFromUniqueId(uint32_t id) const;

	//Opening web pages
	void openPageInBrowser(const tiny_string& url, const tiny_string& window);

	void showMouseCursor(bool visible);
	void setMouseHandCursor(bool sethand);
	void waitRendering() DLL_PUBLIC;
	EngineData* getEngineData() { return engineData;}
	uint32_t getSwfVersion();

	// these methods ensure that externalcallevents are executed as soon and as fast as possible
	// the ppapi plugin needs this because external call events are blocking the main plugin thread 
	// so we need to make sure external call events are executed even if another plugin method is currently running in another thread
	void checkExternalCallEvent() DLL_PUBLIC;
	void waitMainSignal() DLL_PUBLIC;
	void sendMainSignal() DLL_PUBLIC;

	// Declare static class properties with `ASPROPERTY_STATIC(type, class, name)`.
	ASPROPERTY_STATIC(_NR<SoundTransform>, SoundMixer, soundTransform);
	ASPROPERTY_STATIC(int, SoundMixer, bufferTime);
	ASPROPERTY_STATIC(_NR<ASObject>, ObjectEncoding, dynamicPropertyWriter);
	ASPROPERTY_STATIC(tiny_string, Multitouch, inputMode);
	ASPROPERTY_STATIC(_NR<ASFile>, ASFile, applicationDirectory);
	ASPROPERTY_STATIC(_NR<ASFile>, ASFile, applicationStorageDirectory);
	ASPROPERTY_STATIC(_NR<NativeApplication>, NativeApplication, nativeApplication);
	ASPROPERTY_STATIC(bool, XML, ignoreComments);
	ASPROPERTY_STATIC(bool, XML, ignoreProcessingInstructions);
	ASPROPERTY_STATIC(bool, XML, ignoreWhitespace);
	ASPROPERTY_STATIC(int, XML, prettyIndent);
	ASPROPERTY_STATIC(bool, XML, prettyPrinting);
	ASPROPERTY_STATIC(bool, AVM1XMLDocument, ignoreWhite);

	ACQUIRE_RELEASE_FLAG(isinitialized);
	bool use_testrunner_date; // ruffle tests use a specific date as "current date"
	Mutex initializedMutex;
	Cond initializedCond;
	void waitInitialized();
	void getClassInstanceByName(ASWorker* wrk, asAtom &ret, const tiny_string& clsname);
	Mutex resetParentMutex;
	void addDisplayObjectToResetParentList(DisplayObject* child);
	void resetParentList();
	bool isInResetParentList(DisplayObject* d);
	void removeFromResetParentList(DisplayObject* d);
	ASObject* getBuiltinFunction(as_atom_function v, int len = 0, Class_base* returnType=nullptr, Class_base* returnTypeAllArgsInt=nullptr);
};

class DLL_PUBLIC ParseThread: public IThreadJob
{
public:
	int version;
	uint32_t uncompressedsize;
	// Parse an object from stream. The type is detected
	// automatically. After parsing the new object is available
	// from LoaderInfo::getParsedObject().
	ParseThread(std::istream& in, _R<ApplicationDomain> appDomain, _R<SecurityDomain> secDomain, Loader *loader, tiny_string url) DLL_PUBLIC;
	// Parse a clip from stream into root. The stream must be an
	// SWF file.
	ParseThread(std::istream& in, RootMovieClip *root) DLL_PUBLIC;
	~ParseThread();
	FILE_TYPE getFileType() const { return fileType; }
	static FILE_TYPE recognizeFile(uint8_t c1, uint8_t c2, uint8_t c3, uint8_t c4);
	void execute() override;
	void getSWFByteArray(ByteArray* ba);
	void addExtensions(std::vector<tiny_string>& ext) { extensions = ext; }
	void setForBackgroundWorker(uint32_t filelength) { backgroundWorkerFileLength=filelength; }
	ApplicationDomain* getApplicationDomain() const;
private:
	SystemState* sys;
	std::vector<tiny_string> extensions;
	std::istream& f;
	uncompressing_filter* uncompressingFilter;
	std::streambuf* backend;
	Loader *loader;
	DisplayObject* parsedObject;
	Mutex objectSpinlock;
	tiny_string url;
	FILE_TYPE fileType;
	uint32_t backgroundWorkerFileLength;
	void threadAbort() override;
	void jobFence() override {}
	void parseSWFHeader(RootMovieClip *root, UI8 ver);
	void parseSWF(UI8 ver);
	void parseBitmap();
	void parseExtensions(RootMovieClip* root);
};

// Returns true if we're currently running in the main thread.
bool isMainThread() DLL_PUBLIC;

/* Returns the thread-specific SystemState */
SystemState* getSys() DLL_PUBLIC;
/* Set thread-specific SystemState to be returned by getSys() */
void setTLSSys(SystemState* sys) DLL_PUBLIC;

ParseThread* getParseThread();
/* Returns the thread-specific SystemState */
ASWorker* getWorker() DLL_PUBLIC;
void setTLSWorker(ASWorker* worker) DLL_PUBLIC;

}
#endif /* SWF_H */
