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

#ifndef SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H
#define SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H 1

#include "interfaces/threading.h"
#include "compat.h"
#include "asobject.h"
#include "threading.h"
#include "scripting/abcutils.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/toplevel/Error.h"
#include "scripting/flash/events/flashevents.h"
#include <sys/time.h>

#define MIN_DOMAIN_MEMORY_LIMIT 1024
namespace lightspark
{

class SecurityDomain;
struct call_context;

class Capabilities: public ASObject
{
public:
	DLL_PUBLIC static const char* EMULATED_VERSION;
	static const char* MANUFACTURER;
	Capabilities(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_getLanguage);
	ASFUNCTION_ATOM(_getPlayerType);
	ASFUNCTION_ATOM(_getCPUArchitecture);
	ASFUNCTION_ATOM(_getIsDebugger);
	ASFUNCTION_ATOM(_getIsEmbeddedInAcrobat);
	ASFUNCTION_ATOM(_getLocalFileReadDisable);
	ASFUNCTION_ATOM(_getManufacturer);
	ASFUNCTION_ATOM(_getOS);
	ASFUNCTION_ATOM(_getVersion);
	ASFUNCTION_ATOM(_getServerString);
	ASFUNCTION_ATOM(_getScreenResolutionX);
	ASFUNCTION_ATOM(_getScreenResolutionY);
	ASFUNCTION_ATOM(_getHasAccessibility);
	ASFUNCTION_ATOM(_getScreenDPI);

	ASFUNCTION_ATOM(_avHardwareDisable);
	ASFUNCTION_ATOM(_hasAudio);
	ASFUNCTION_ATOM(_hasAudioEncoder);
	ASFUNCTION_ATOM(_hasEmbeddedVideo);
	ASFUNCTION_ATOM(_hasIME);
	ASFUNCTION_ATOM(_hasMP3);
	ASFUNCTION_ATOM(_hasPrinting);
	ASFUNCTION_ATOM(_hasScreenBroadcast);
	ASFUNCTION_ATOM(_hasScreenPlayback);
	ASFUNCTION_ATOM(_hasStreamingAudio);
	ASFUNCTION_ATOM(_hasStreamingVideo);
	ASFUNCTION_ATOM(_hasTLS);
	ASFUNCTION_ATOM(_hasVideoEncoder);
	ASFUNCTION_ATOM(_supports32BitProcesses);
	ASFUNCTION_ATOM(_supports64BitProcesses);
	ASFUNCTION_ATOM(_touchscreenType);
	ASFUNCTION_ATOM(_pixelAspectRatio);
	ASFUNCTION_ATOM(_screenColor);
	ASFUNCTION_ATOM(_maxLevelIDC);
	ASFUNCTION_ATOM(_hasMultiChannelAudio);
};

class ApplicationDomain: public ASObject
{
private:
	std::vector<Global*> globalScopes;
	_NR<ByteArray> defaultDomainMemory;
	void cbDomainMemory(_NR<ByteArray> oldvalue);
	// list of classes where super class is not defined yet
	std::unordered_map<Class_base*,Class_base*> classesSuperNotFilled;
	// list of classes where interfaces are not yet linked
	std::vector<Class_base*> classesToLinkInterfaces;
public:
	ByteArray* currentDomainMemory;
	ApplicationDomain(ASWorker* wrk, Class_base* c, _NR<ApplicationDomain> p=NullRef);
	void finalize() override;
	void prepareShutdown() override;
	std::map<const multiname*, Class_base*> classesBeingDefined;
	std::map<QName, Class_base*> instantiatedTemplates;
	
	void SetAllClassLinks();
	void AddClassLinks(Class_base* target);
	bool newClassRecursiveLink(Class_base* target, Class_base* c);
	void addSuperClassNotFilled(Class_base* cls);
	void copyBorrowedTraitsFromSuper(Class_base* cls);

	static void sinit(Class_base* c);
	void registerGlobalScope(Global* scope);
	Global* getLastGlobalScope() const  { return globalScopes.back(); }
	ASObject* getVariableByString(const std::string& name, ASObject*& target);
	bool findTargetByMultiname(const multiname& name, ASObject*& target, ASWorker* wrk);
	GET_VARIABLE_RESULT getVariableAndTargetByMultiname(asAtom& ret, const multiname& name, ASObject*& target,ASWorker* wrk);
	void getVariableAndTargetByMultinameIncludeTemplatedClasses(asAtom& ret, const multiname& name, ASObject*& target, ASWorker* wrk);

	/*
	 * This method is an opportunistic resolution operator used by the optimizer:
	 * Only returns the value if the variable has been already defined.
	 */
	ASObject* getVariableByMultinameOpportunistic(const multiname& name, ASWorker* wrk);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getCurrentDomain);
	ASFUNCTION_ATOM(_getMinDomainMemoryLength);
	ASFUNCTION_ATOM(hasDefinition);
	ASFUNCTION_ATOM(getDefinition);
	ASPROPERTY_GETTER_SETTER(_NR<ByteArray>, domainMemory);
	ASPROPERTY_GETTER(_NR<ApplicationDomain>, parentDomain);
	static void throwRangeError();
	template<class T>
	T readFromDomainMemory(uint32_t addr)
	{
		if(currentDomainMemory->getLength() < (addr+sizeof(T)))
		{
			throwRangeError();
			return T(0);
		}
		uint8_t* buf=currentDomainMemory->getBufferNoCheck();
		return *reinterpret_cast<T*>(buf+addr);
	}
	template<class T>
	void writeToDomainMemory(uint32_t addr, T val)
	{
		if(currentDomainMemory->getLength() < (addr+sizeof(T)))
		{
			throwRangeError();
			return;
		}
		uint8_t* buf=currentDomainMemory->getBufferNoCheck();
		*reinterpret_cast<T*>(buf+addr)=val;
	}
	template<class T>
	static void loadIntN(ApplicationDomain* appDomain,call_context* th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		uint32_t addr=asAtomHandler::toUInt(*arg1);
		T ret=appDomain->readFromDomainMemory<T>(addr);
		ASATOM_DECREF_POINTER(arg1);
		RUNTIME_STACK_PUSH(th,asAtomHandler::fromInt(ret));
	}
	template<class T>
	static void storeIntN(ApplicationDomain* appDomain,call_context* th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		RUNTIME_STACK_POP_CREATE(th,arg2);
		uint32_t addr=asAtomHandler::toUInt(*arg1);
		ASATOM_DECREF_POINTER(arg1);
		int32_t val=asAtomHandler::toInt(*arg2);
		ASATOM_DECREF_POINTER(arg2);
		appDomain->writeToDomainMemory<T>(addr, val);
	}
	template<class T>
	static FORCE_INLINE void loadIntN(ApplicationDomain* appDomain,asAtom& ret, asAtom& arg1)
	{
		uint32_t addr=asAtomHandler::toUInt(arg1);
		ByteArray* dm = appDomain->currentDomainMemory;
		if(dm->getLength() < (addr+sizeof(T)))
		{
			throwRangeError();
			return;
		}
		ret = asAtomHandler::fromInt(*reinterpret_cast<T*>(dm->getBufferNoCheck()+addr));
	}
	template<class T>
	static FORCE_INLINE void storeIntN(ApplicationDomain* appDomain, asAtom& arg1, asAtom& arg2)
	{
		uint32_t addr=asAtomHandler::toUInt(arg1);
		int32_t val=asAtomHandler::toInt(arg2);
		ByteArray* dm = appDomain->currentDomainMemory;
		if(dm->getLength() < (addr+sizeof(T)))
		{
			throwRangeError();
			return;
		}
		*reinterpret_cast<T*>(dm->getBufferNoCheck()+addr)=val;
	}
	
	static FORCE_INLINE void loadFloat(ApplicationDomain* appDomain,call_context *th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		uint32_t addr=asAtomHandler::toUInt(*arg1);
		number_t ret=appDomain->readFromDomainMemory<float>(addr);
		ASATOM_DECREF_POINTER(arg1);
		RUNTIME_STACK_PUSH(th,asAtomHandler::fromNumber(appDomain->getInstanceWorker(),ret,false));
	}
	static FORCE_INLINE void loadFloat(ApplicationDomain* appDomain,asAtom& ret, asAtom& arg1)
	{
		uint32_t addr=asAtomHandler::toUInt(arg1);
		number_t res=appDomain->readFromDomainMemory<float>(addr);
		asAtom oldret = ret;
		if (asAtomHandler::replaceNumber(ret,appDomain->getInstanceWorker(),res))
			ASATOM_DECREF(oldret);
	}
	static FORCE_INLINE void loadDouble(ApplicationDomain* appDomain,call_context *th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		uint32_t addr=asAtomHandler::toUInt(*arg1);
		number_t res=appDomain->readFromDomainMemory<double>(addr);
		ASATOM_DECREF_POINTER(arg1);
		RUNTIME_STACK_PUSH(th,asAtomHandler::fromNumber(appDomain->getInstanceWorker(),res,false));
	}
	static FORCE_INLINE void loadDouble(ApplicationDomain* appDomain,asAtom& ret, asAtom& arg1)
	{
		uint32_t addr=asAtomHandler::toUInt(arg1);
		number_t res=appDomain->readFromDomainMemory<double>(addr);
		asAtom oldret = ret;
		if (asAtomHandler::replaceNumber(ret,appDomain->getInstanceWorker(),res))
			ASATOM_DECREF(oldret);
	}

	static FORCE_INLINE void storeFloat(ApplicationDomain* appDomain,call_context *th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		RUNTIME_STACK_POP_CREATE(th,arg2);
		number_t addr=asAtomHandler::toNumber(*arg1);
		ASATOM_DECREF_POINTER(arg1);
		float val=(float)asAtomHandler::toNumber(*arg2);
		ASATOM_DECREF_POINTER(arg2);
		appDomain->writeToDomainMemory<float>(addr, val);
	}
	static FORCE_INLINE void storeFloat(ApplicationDomain* appDomain, asAtom& arg1, asAtom& arg2)
	{
		number_t addr=asAtomHandler::toNumber(arg1);
		float val=(float)asAtomHandler::toNumber(arg2);
		appDomain->writeToDomainMemory<float>(addr, val);
	}

	static FORCE_INLINE void storeDouble(ApplicationDomain* appDomain,call_context *th)
	{
		RUNTIME_STACK_POP_CREATE(th,arg1);
		RUNTIME_STACK_POP_CREATE(th,arg2);
		number_t addr=asAtomHandler::toNumber(*arg1);
		ASATOM_DECREF_POINTER(arg1);
		double val=asAtomHandler::toNumber(*arg2);
		ASATOM_DECREF_POINTER(arg2);
		appDomain->writeToDomainMemory<double>(addr, val);
	}
	static FORCE_INLINE void storeDouble(ApplicationDomain* appDomain, asAtom& arg1, asAtom& arg2)
	{
		number_t addr=asAtomHandler::toNumber(arg1);
		double val=asAtomHandler::toNumber(arg2);
		appDomain->writeToDomainMemory<double>(addr, val);
	}
	void checkDomainMemory();
};

class LoaderContext: public ASObject
{
public:
	LoaderContext(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(bool, allowCodeImport);
	ASPROPERTY_GETTER_SETTER(_NR<ApplicationDomain>, applicationDomain);
	ASPROPERTY_GETTER_SETTER(bool, checkPolicyFile);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>, parameters);
	ASPROPERTY_GETTER_SETTER(_NR<SecurityDomain>, securityDomain);
	ASPROPERTY_GETTER_SETTER(tiny_string, imageDecodingPolicy);
	void finalize() override;
	bool getAllowCodeImport();
	bool getCheckPolicyFile();
};

class SecurityDomain: public ASObject
{
public:
	SecurityDomain(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getCurrentDomain);
};

class Security: public ASObject
{
public:
	Security(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_getExactSettings);
	ASFUNCTION_ATOM(_setExactSettings);
	ASFUNCTION_ATOM(_getSandboxType);
	ASFUNCTION_ATOM(allowDomain);
	ASFUNCTION_ATOM(allowInsecureDomain);
	ASFUNCTION_ATOM(loadPolicyFile);
	ASFUNCTION_ATOM(showSettings);
	ASFUNCTION_ATOM(pageDomain);
};

void fscommand(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen);

class System: public ASObject
{
public:
	System(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(totalMemory);
	ASFUNCTION_ATOM(disposeXML);
	ASFUNCTION_ATOM(pauseForGCIfCollectionImminent);
	ASFUNCTION_ATOM(gc);
	ASFUNCTION_ATOM(pause);
	ASFUNCTION_ATOM(resume);
};
class WorkerDomain;
class ParseThread;
class Prototype;
class ASWorker: public EventDispatcher, public IThreadJob
{
friend class WorkerDomain;
private:
	Mutex parsemutex;
	_NR<Loader> loader;
	_NR<ByteArray> swf;
	ParseThread* parser;
	bool giveAppPrivileges;
	bool started;
	bool inGarbageCollection;
	bool inShutdown;
	bool inFinalize;
	//Synchronization
	Mutex event_queue_mutex;
	Mutex constantrefmutex;
	Cond sem_event_cond;
	typedef std::pair<_NR<EventDispatcher>,_R<Event>> eventType;
	std::deque<eventType> events_queue;
	map<const Class_base*,_R<Prototype>> protoypeMap;
	std::unordered_set<ASObject*> garbagecollection;
	std::unordered_set<ASObject*> garbagecollectiondeleted;
	std::unordered_set<ASObject*> constantrefs;
	struct timeval last_garbagecollection;
	std::vector<ABCContext*> contexts;
public:
	asfreelist* freelist;
	asfreelist freelist_syntheticfunction;
	ASWorker(SystemState* s); // constructor for primordial worker only to be used in SystemState constructor
	ASWorker(Class_base* c);
	ASWorker(ASWorker* wrk,Class_base* c);
	void finalize() override;
	void prepareShutdown() override;
	void addABCContext(ABCContext* c) { contexts.push_back(c); }
	Prototype* getClassPrototype(const Class_base* cls);
	static void sinit(Class_base*);

	bool isExplicitlyConstructed() const { return currentCallContext != nullptr && currentCallContext->explicitConstruction; }
	//  TODO merge stacktrace handling with ABCVm
	abc_limits limits;
	std::vector<call_context*> callStack;
	call_context* currentCallContext;
	/* The current recursion level. Each call increases this by one,
	 * each return from a call decreases this. */
	uint32_t cur_recursion;
	stacktrace_entry* stacktrace;
	FORCE_INLINE call_context* incStack(asAtom o, uint32_t f)
	{
		if(USUALLY_FALSE(cur_recursion == limits.max_recursion))
		{
			throwStackOverflow();
			return currentCallContext;
		}
		stacktrace[cur_recursion].set(o,f);
		++cur_recursion; //increment current recursion depth
		return currentCallContext;
	}
	FORCE_INLINE void decStack(call_context* saved_cc)
	{
		currentCallContext = saved_cc;
		if (cur_recursion > 0)
			--cur_recursion; //decrement current recursion depth
	}
	void throwStackOverflow();
	ASFUNCTION_ATOM(_getCurrent);
	ASFUNCTION_ATOM(getSharedProperty);
	ASFUNCTION_ATOM(isSupported);
	ASPROPERTY_GETTER(bool,isPrimordial);
	ASPROPERTY_GETTER(tiny_string,state);
	ASFUNCTION_ATOM(_addEventListener);
	ASFUNCTION_ATOM(createMessageChannel);
	ASFUNCTION_ATOM(_removeEventListener);
	ASFUNCTION_ATOM(setSharedProperty);
	ASFUNCTION_ATOM(start);
	ASFUNCTION_ATOM(terminate);
	void execute() override;
	void jobFence() override;
	void threadAbort() override;
	void afterHandleEvent(Event* ev) override;
	bool addEvent(_NR<EventDispatcher> obj ,_R<Event> ev);
	_NR<RootMovieClip> rootClip;
	tiny_string getDefaultXMLNamespace() const;
	uint32_t getDefaultXMLNamespaceID() const;
	void dumpStacktrace();
	void addObjectToGarbageCollector(ASObject* o)
	{
		garbagecollection.insert(o);
	}
	void removeObjectFromGarbageCollector(ASObject* o)
	{
		garbagecollection.erase(o);
		garbagecollectiondeleted.erase(o);
	}
	void processGarbageCollection(bool force);
	FORCE_INLINE bool isInGarbageCollection() const { return inGarbageCollection; }
	FORCE_INLINE bool isDeletedInGarbageCollection(ASObject* o) const
	{
		return inGarbageCollection && garbagecollectiondeleted.find(o) != garbagecollectiondeleted.end();
	}
	void setDeletedInGarbageCollection(ASObject* o)
	{
		garbagecollectiondeleted.insert(o);
	}
	inline bool inFinalization() const { return inFinalize; }
	void registerConstantRef(ASObject* obj);
	
	// these are needed keep track of native extension calls
	std::list<asAtom> nativeExtensionAtomlist;
	std::list<uint8_t*> nativeExtensionStringlist;
	uint32_t nativeExtensionCallCount;
};
class WorkerDomain: public ASObject
{
friend class ASWorker;
friend class SystemState;
private:
	Mutex workersharedobjectmutex;
	_NR<Vector> workerlist;
	_NR<ASObject> workerSharedObject;
	std::unordered_set<MessageChannel*> messagechannellist;
public:
	WorkerDomain(ASWorker* wrk, Class_base* c);
	void finalize() override;
	void prepareShutdown() override;
	static void sinit(Class_base*);
	void addMessageChannel(MessageChannel* c);
	void removeWorker(ASWorker* w);
	void stopAllBackgroundWorkers();
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getCurrent);
	ASFUNCTION_ATOM(_isSupported);
	ASFUNCTION_ATOM(createWorker);
	ASFUNCTION_ATOM(createWorkerFromPrimordial);
	ASFUNCTION_ATOM(createWorkerFromByteArray);
	ASFUNCTION_ATOM(listWorkers);
};

class WorkerState: public ASObject
{
public:
	WorkerState(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class ImageDecodingPolicy: public ASObject
{
public:
	ImageDecodingPolicy(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class IMEConversionMode: public ASObject
{
public:
	IMEConversionMode(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};


}
#endif /* SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H */
