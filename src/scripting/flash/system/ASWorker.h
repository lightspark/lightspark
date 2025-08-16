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

#ifndef SCRIPTING_FLASH_SYSTEM_ASWORKER_H
#define SCRIPTING_FLASH_SYSTEM_ASWORKER_H 1

#include "interfaces/threading.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/abcutils.h"
#include "threading.h"

namespace lightspark
{
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
	std::set<ASObject*> constantrefs;
	uint64_t last_garbagecollection;
	std::vector<ABCContext*> contexts;
	unordered_map<uint32_t,IFunction*> avm1ClassConstructorsCaseSensitive;
	unordered_map<uint32_t,IFunction*> avm1ClassConstructorsCaseInsensitive;
public:
	Stage* stage; // every worker has its own stage. In case of the primordial worker this points to the stage of the SystemState.
	asfreelist* freelist;
	asfreelist freelist_syntheticfunction;
	asfreelist freelist_activationobject;
	asfreelist freelist_catchscopeobject;
	asfreelist freelist_asobject;

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
	uint32_t AVM1_cur_recursion_function; // recursion count for normal avm1 function calls
	uint32_t AVM1_cur_recursion_internal; // recursion count for internal avm1 function calls (getters,setters...)
	stacktrace_entry* stacktrace;
	void fillStackTrace(StackTraceList& strace);
	static tiny_string getStackTraceString(SystemState* sys, const StackTraceList& strace, ASObject* error);
	FORCE_INLINE call_context* incStack(asAtom o, SyntheticFunction* f)
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
	// map AVM1 class constructors to named tags
	void AVM1registerTagClass(uint32_t nameID, IFunction* theClassConstructor);
	AVM1Function* AVM1getClassConstructor(DisplayObject* d);
	std::vector<AVM1context*> AVM1callStack;
	AVM1Function* AVM1getCallee() const
	{
		return AVM1callStack.empty() ? nullptr : AVM1callStack.back()->callee;
	}
	bool AVM1isCaseSensitive() const
	{
		return
		(
			AVM1callStack.empty() ||
			AVM1callStack.back()->isCaseSensitive()
		);
	}
	uint8_t AVM1getSwfVersion() const
	{
		return AVM1callStack.empty() ? UINT8_MAX : AVM1callStack.back()->swfversion;
	}
	bool needsActionScript3() const
	{
		return AVM1getSwfVersion()==UINT8_MAX;
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
	Mutex gcmutex;
	void addObjectToGarbageCollector(ASObject* o);
	void removeObjectFromGarbageCollector(ASObject* o);
	void processGarbageCollection(bool force);
	FORCE_INLINE bool isInGarbageCollection() const { return inGarbageCollection; }
	inline bool inFinalization() const { return inFinalize; }
	void registerConstantRef(ASObject* obj);
	asAtom getCurrentGlobalAtom(const asAtom& defaultObj);
	// these are needed keep track of native extension calls
	std::list<asAtom> nativeExtensionAtomlist;
	std::list<uint8_t*> nativeExtensionStringlist;
	uint32_t nativeExtensionCallCount;
	void handleInternalEvent(Event* e);
};

}
#endif /* SCRIPTING_FLASH_SYSTEM_ASWORKER_H */
