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
#include "scripting/flash/events/flashevents.h"
#include "backends/urlutils.h"
#include <sys/time.h>

#define MIN_DOMAIN_MEMORY_LIMIT 1024
namespace lightspark
{
class DefineScalingGridTag;
class FontTag;
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
