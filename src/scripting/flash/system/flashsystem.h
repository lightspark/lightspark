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

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/toplevel/Error.h"
#include "scripting/flash/events/flashevents.h"

#define MIN_DOMAIN_MEMORY_LIMIT 1024
namespace lightspark
{

class SecurityDomain;

class Capabilities: public ASObject
{
public:
	DLL_PUBLIC static const char* EMULATED_VERSION;
	static const char* MANUFACTURER;
	Capabilities(Class_base* c):ASObject(c){}
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
	_R<ByteArray> defaultDomainMemory;
	void cbDomainMemory(_NR<ByteArray> oldvalue);
public:
	ByteArray* currentDomainMemory;
	ApplicationDomain(Class_base* c, _NR<ApplicationDomain> p=NullRef);
	void finalize();
	std::map<const multiname*, Class_base*> classesBeingDefined;
	std::map<QName, Class_base*> instantiatedTemplates;

	// list of classes where super class is not defined yet 
	std::list<Class_base*> classesSuperNotFilled;

	static void sinit(Class_base* c);
	void registerGlobalScope(Global* scope);
	Global* getLastGlobalScope() const  { return globalScopes.back(); }
	ASObject* getVariableByString(const std::string& name, ASObject*& target);
	bool findTargetByMultiname(const multiname& name, ASObject*& target);
	GET_VARIABLE_RESULT getVariableAndTargetByMultiname(asAtom& ret, const multiname& name, ASObject*& target);
	void getVariableAndTargetByMultinameIncludeTemplatedClasses(asAtom& ret, const multiname& name, ASObject*& target);

	/*
	 * This method is an opportunistic resolution operator used by the optimizer:
	 * Only returns the value if the variable has been already defined.
	 */
	ASObject* getVariableByMultinameOpportunistic(const multiname& name);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getCurrentDomain);
	ASFUNCTION_ATOM(_getMinDomainMemoryLength);
	ASFUNCTION_ATOM(hasDefinition);
	ASFUNCTION_ATOM(getDefinition);
	ASPROPERTY_GETTER_SETTER(_NR<ByteArray>, domainMemory);
	ASPROPERTY_GETTER(_NR<ApplicationDomain>, parentDomain);
	template<class T>
	T readFromDomainMemory(uint32_t addr)
	{
		if(currentDomainMemory->getLength() < (addr+sizeof(T)))
			throwError<RangeError>(kInvalidRangeError);
		uint8_t* buf=currentDomainMemory->getBufferNoCheck();
		return *reinterpret_cast<T*>(buf+addr);
	}
	template<class T>
	void writeToDomainMemory(uint32_t addr, T val)
	{
		if(currentDomainMemory->getLength() < (addr+sizeof(T)))
			throwError<RangeError>(kInvalidRangeError);
		uint8_t* buf=currentDomainMemory->getBufferNoCheck();
		*reinterpret_cast<T*>(buf+addr)=val;
	}
	void checkDomainMemory();
};

class LoaderContext: public ASObject
{
public:
	LoaderContext(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(bool, allowCodeImport);
	ASPROPERTY_GETTER_SETTER(_NR<ApplicationDomain>, applicationDomain);
	ASPROPERTY_GETTER_SETTER(bool, checkPolicyFile);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>, parameters);
	ASPROPERTY_GETTER_SETTER(_NR<SecurityDomain>, securityDomain);
	ASPROPERTY_GETTER_SETTER(tiny_string, imageDecodingPolicy);
	void finalize();
	bool getAllowCodeImport();
	bool getCheckPolicyFile();
};

class SecurityDomain: public ASObject
{
public:
	SecurityDomain(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getCurrentDomain);
};

class Security: public ASObject
{
public:
	Security(Class_base* c):ASObject(c){}
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

void fscommand(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen);

class System: public ASObject
{
public:
	System(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(totalMemory);
	ASFUNCTION_ATOM(disposeXML);
	ASFUNCTION_ATOM(pauseForGCIfCollectionImminent);
	ASFUNCTION_ATOM(gc);
};
class WorkerDomain;
class ParseThread;
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
	//Synchronization
	Mutex event_queue_mutex;
	Cond sem_event_cond;
	typedef std::pair<_NR<EventDispatcher>,_R<Event>> eventType;
	std::deque<eventType> events_queue;
public:
	ASWorker(Class_base* c);
	void finalize() override;
	static void sinit(Class_base*);
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
	bool addEvent(_NR<EventDispatcher> obj ,_R<Event> ev);
};
class WorkerDomain: public ASObject
{
friend class ASWorker;
friend class SystemState;
private:
	Mutex workersharedobjectmutex;
	_NR<Vector> workerlist;
	_NR<ASObject> workerSharedObject;
public:
	WorkerDomain(Class_base* c);
	void finalize() override;
	static void sinit(Class_base*);
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
	WorkerState(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class ImageDecodingPolicy: public ASObject
{
public:
	ImageDecodingPolicy(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class IMEConversionMode: public ASObject
{
public:
	IMEConversionMode(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};


}
#endif /* SCRIPTING_FLASH_SYSTEM_FLASHSYSTEM_H */
