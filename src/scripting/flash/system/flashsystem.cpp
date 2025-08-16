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

#include "version.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/system/ApplicationDomain.h"
#include "scripting/flash/system/messagechannel.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/Stage.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/avm1/scope.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "compat.h"
#include "backends/security.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/toplevel/Undefined.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/toplevel.h"
#include "platforms/engineutils.h"
#ifdef __APPLE__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include "3rdparty/pugixml/src/pugixml.hpp"

#include <istream>

using namespace lightspark;

#ifdef _WIN32
const char* Capabilities::EMULATED_VERSION = "WIN 25,0,0," SHORTVERSION;
const char* Capabilities::MANUFACTURER = "Adobe Windows";
#else
const char* Capabilities::EMULATED_VERSION = "LNX 25,0,0," SHORTVERSION;
const char* Capabilities::MANUFACTURER = "Adobe Linux";
#endif


void Capabilities::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("language","",c->getSystemState()->getBuiltinFunction(_getLanguage,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("playerType","",c->getSystemState()->getBuiltinFunction(_getPlayerType,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("version","",c->getSystemState()->getBuiltinFunction(_getVersion,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("cpuArchitecture","",c->getSystemState()->getBuiltinFunction(_getCPUArchitecture,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isDebugger","",c->getSystemState()->getBuiltinFunction(_getIsDebugger,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isEmbeddedInAcrobat","",c->getSystemState()->getBuiltinFunction(_getIsEmbeddedInAcrobat,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("localFileReadDisable","",c->getSystemState()->getBuiltinFunction(_getLocalFileReadDisable,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("manufacturer","",c->getSystemState()->getBuiltinFunction(_getManufacturer,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("os","",c->getSystemState()->getBuiltinFunction(_getOS,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("serverString","",c->getSystemState()->getBuiltinFunction(_getServerString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenResolutionX","",c->getSystemState()->getBuiltinFunction(_getScreenResolutionX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenResolutionY","",c->getSystemState()->getBuiltinFunction(_getScreenResolutionY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("hasAccessibility","",c->getSystemState()->getBuiltinFunction(_getHasAccessibility,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenDPI","",c->getSystemState()->getBuiltinFunction(_getScreenDPI,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasAudio","",c->getSystemState()->getBuiltinFunction(_hasAudio,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasAudioEncoder","",c->getSystemState()->getBuiltinFunction(_hasAudioEncoder,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasEmbeddedVideo","",c->getSystemState()->getBuiltinFunction(_hasEmbeddedVideo,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasIME","",c->getSystemState()->getBuiltinFunction(_hasIME,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasMP3","",c->getSystemState()->getBuiltinFunction(_hasMP3,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasPrinting","",c->getSystemState()->getBuiltinFunction(_hasPrinting,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasScreenBroadcast","",c->getSystemState()->getBuiltinFunction(_hasScreenBroadcast,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasScreenPlayback","",c->getSystemState()->getBuiltinFunction(_hasScreenPlayback,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasStreamingAudio","",c->getSystemState()->getBuiltinFunction(_hasStreamingAudio,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasStreamingVideo","",c->getSystemState()->getBuiltinFunction(_hasStreamingVideo,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasTLS","",c->getSystemState()->getBuiltinFunction(_hasTLS,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasVideoEncoder","",c->getSystemState()->getBuiltinFunction(_hasVideoEncoder,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("supports32BitProcesses","",c->getSystemState()->getBuiltinFunction(_supports32BitProcesses,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("supports64BitProcesses","",c->getSystemState()->getBuiltinFunction(_supports64BitProcesses,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("touchscreenType","",c->getSystemState()->getBuiltinFunction(_touchscreenType,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("avHardwareDisable","",c->getSystemState()->getBuiltinFunction(_avHardwareDisable,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("pixelAspectRatio","",c->getSystemState()->getBuiltinFunction(_pixelAspectRatio,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("screenColor","",c->getSystemState()->getBuiltinFunction(_screenColor,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasMultiChannelAudio","",c->getSystemState()->getBuiltinFunction(_hasMultiChannelAudio),NORMAL_METHOD,false);
    c->setDeclaredMethodByQName("maxLevelIDC","",c->getSystemState()->getBuiltinFunction(_maxLevelIDC,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getPlayerType)
{
	switch (wrk->getSystemState()->flashMode)
	{
		case SystemState::AIR:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"Desktop");
			break;
		default:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"PlugIn");
			break;
	}
}

ASFUNCTIONBODY_ATOM(Capabilities,_getLanguage)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),"en");
}

ASFUNCTIONBODY_ATOM(Capabilities,_getCPUArchitecture)
{
	LOG(LOG_NOT_IMPLEMENTED, "Capabilities.cpuArchitecture is not implemented");
	ret = asAtomHandler::fromString(wrk->getSystemState(),"x86");
}

ASFUNCTIONBODY_ATOM(Capabilities,_getIsDebugger)
{
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getIsEmbeddedInAcrobat)
{
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getLocalFileReadDisable)
{
	asAtomHandler::setBool(ret,true);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getManufacturer)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),MANUFACTURER);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getOS)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),wrk->getSystemState()->getEngineData()->platformOS);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getVersion)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),EMULATED_VERSION);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getServerString)
{
	LOG(LOG_NOT_IMPLEMENTED,"Capabilities: not all capabilities are reported in ServerString");
	tiny_string res = "&SA=t&SV=t";
	res +="&OS=";
	res += wrk->getSystemState()->getEngineData()->platformOS;
	res +="&PT=PlugIn&L=en&TLS=t&DD=t"; // TODO
	res +="&V=";
	res += EMULATED_VERSION;
	res +="&M=";
	res += MANUFACTURER;

	SDL_DisplayMode screen;
	if (wrk->getSystemState()->getEngineData()->getScreenData(&screen)) {
		int width = screen.w;
		int height = screen.h;
		char buf[40];
		snprintf(buf,40,"&R=%ix%i",width,height);
		res += buf;
	}

    // Add hasAudio and hasMP3 to ServerString if available
#ifdef ENABLE_LIBAVCODEC
    res += "&A=t&MP3=t";
#endif

	/*
	avHardwareDisable	AVD
	hasAccessibility	ACC
	hasAudio	A
	hasAudioEncoder	AE
	hasEmbeddedVideo	EV
	hasIME	IME
	hasMP3	MP3
	hasPrinting	PR
	hasScreenBroadcast	SB
	hasScreenPlayback	SP
	hasStreamingAudio	SA
	hasStreamingVideo	SV
	hasTLS	TLS
	hasVideoEncoder	VE
	isDebugger	DEB
	language	L
	localFileReadDisable	LFD
	manufacturer	M
	maxLevelIDC	ML
	os	OS
	pixelAspectRatio	AR
	playerType	PT
	screenColor	COL
	screenDPI	DP
	screenResolutionX	R
	screenResolutionY	R
	version	V
	supports Dolby Digital audio	DD
	supports Dolby Digital Plus audio	DDP
	supports DTS audio	DTS
	supports DTS Express audio	DTE
	supports DTS-HD High Resolution Audio	DTH
	supports DTS-HD Master Audio	DTM
	*/
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}

ASFUNCTIONBODY_ATOM(Capabilities,_avHardwareDisable)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.avHardwareDisable always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasAudio)
{
#ifdef ENABLE_LIBAVCODEC
    ret = asAtomHandler::fromBool(true);
#else
	ret = asAtomHandler::fromBool(false);
#endif
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasAudioEncoder)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasAudioEncoder always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasEmbeddedVideo)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasEmbeddedVideo always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasIME)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasIME always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasMP3)
{
#ifdef ENABLE_LIBAVCODEC
	ret = asAtomHandler::fromBool(true);
#else
	ret = asAtomHandler::fromBool(false);
#endif
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasMultiChannelAudio)
{
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasPrinting)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasPrinting always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasScreenBroadcast)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasScreenBroadcast always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasScreenPlayback)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasScreenPlayback always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasStreamingAudio)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasStreamingAudio always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasStreamingVideo)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasStreamingAudio always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasTLS)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasTLS always returns true");
    ret = asAtomHandler::fromBool(true);
}

ASFUNCTIONBODY_ATOM(Capabilities,_hasVideoEncoder)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.hasVideoEncoder always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_maxLevelIDC)
{
    ret = asAtomHandler::fromString(wrk->getSystemState(),"");
}

ASFUNCTIONBODY_ATOM(Capabilities,_pixelAspectRatio)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.pixelAspectRatio always returns 1");
    asAtomHandler::setNumber(ret,wrk,1);
}

ASFUNCTIONBODY_ATOM(Capabilities,_screenColor)
{
    ret = asAtomHandler::fromString(wrk->getSystemState(),"color");
}

ASFUNCTIONBODY_ATOM(Capabilities,_supports32BitProcesses)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.supports32BitProcesses always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_supports64BitProcesses)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.supports64BitProcesses always returns \"none\"");
    ret = asAtomHandler::fromString(wrk->getSystemState(),"none");
}

ASFUNCTIONBODY_ATOM(Capabilities,_touchscreenType)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.touchscreenType always returns empty string");
    ret = asAtomHandler::fromString(wrk->getSystemState(),"none");
}

ASFUNCTIONBODY_ATOM(Capabilities,_getScreenResolutionX)
{
	SDL_DisplayMode screen;
	if (!wrk->getSystemState()->getEngineData()->getScreenData(&screen))
		asAtomHandler::setInt(ret,wrk,0);
	else
		asAtomHandler::setInt(ret,wrk,screen.w);
}
ASFUNCTIONBODY_ATOM(Capabilities,_getScreenResolutionY)
{
	SDL_DisplayMode screen;
	if (!wrk->getSystemState()->getEngineData()->getScreenData(&screen))
		asAtomHandler::setInt(ret,wrk,0);
	else
		asAtomHandler::setInt(ret,wrk,screen.h);
}
ASFUNCTIONBODY_ATOM(Capabilities,_getHasAccessibility)
{
	LOG(LOG_NOT_IMPLEMENTED,"hasAccessibility always returns false");
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getScreenDPI)
{
	number_t dpi = wrk->getSystemState()->getEngineData()->getScreenDPI();
	asAtomHandler::setNumber(ret,wrk,dpi);
}

LoaderContext::LoaderContext(ASWorker* wrk, Class_base* c):
	ASObject(wrk,c,T_OBJECT,SUBTYPE_LOADERCONTEXT),allowCodeImport(true),checkPolicyFile(false),imageDecodingPolicy("onDemand")
{
}

void LoaderContext::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("allowLoadBytesCodeExecution","",c->getSystemState()->getBuiltinFunction(_getter_allowCodeImport),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("allowLoadBytesCodeExecution","",c->getSystemState()->getBuiltinFunction(_setter_allowCodeImport),SETTER_METHOD,false);
	REGISTER_GETTER_SETTER(c, allowCodeImport);
	REGISTER_GETTER_SETTER(c, applicationDomain);
	REGISTER_GETTER_SETTER(c, checkPolicyFile);
	REGISTER_GETTER_SETTER(c, parameters);
	REGISTER_GETTER_SETTER(c, securityDomain);
	REGISTER_GETTER_SETTER(c, imageDecodingPolicy);
}

void LoaderContext::finalize()
{
	ASObject::finalize();
	applicationDomain.reset();
	securityDomain.reset();
}

ASFUNCTIONBODY_ATOM(LoaderContext,_constructor)
{
	LoaderContext* th=asAtomHandler::as<LoaderContext>(obj);
	ARG_CHECK(ARG_UNPACK (th->checkPolicyFile, false)
		(th->applicationDomain, NullRef)
		(th->securityDomain, NullRef));
}

ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, allowCodeImport)
ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, applicationDomain)
ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, checkPolicyFile)
ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, parameters)
ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, securityDomain)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(LoaderContext, imageDecodingPolicy)

bool LoaderContext::getCheckPolicyFile()
{
	return checkPolicyFile;
}

bool LoaderContext::getAllowCodeImport()
{
	return allowCodeImport;
}

void SecurityDomain::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	//Static
	c->setDeclaredMethodByQName("currentDomain","",c->getSystemState()->getBuiltinFunction(_getCurrentDomain),GETTER_METHOD,false);
}

void SecurityDomain::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(SecurityDomain,_constructor)
{
}

ASFUNCTIONBODY_ATOM(SecurityDomain,_getCurrentDomain)
{
	SecurityDomain* res=ABCVm::getCurrentSecurityDomain(wrk->currentCallContext);
	res->incRef();
	ret = asAtomHandler::fromObject(res);
}

void Security::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	//Fully static class
	c->setDeclaredMethodByQName("exactSettings","",c->getSystemState()->getBuiltinFunction(_getExactSettings),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("exactSettings","",c->getSystemState()->getBuiltinFunction(_setExactSettings),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("sandboxType","",c->getSystemState()->getBuiltinFunction(_getSandboxType),GETTER_METHOD,false);

	// Value is undefined and not "httpResponseStatus" like stated in documentation
	c->setVariableAtomByQName("APPLICATION",nsNameAndKind(),asAtomHandler::fromObject(c->getSystemState()->getUndefinedRef()),DECLARED_TRAIT);

	c->setVariableAtomByQName("LOCAL_TRUSTED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_TRUSTED)),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOCAL_WITH_FILE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_FILE)),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOCAL_WITH_NETWORK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_NETWORK)),DECLARED_TRAIT);
	c->setVariableAtomByQName("REMOTE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::REMOTE)),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("allowDomain","",c->getSystemState()->getBuiltinFunction(allowDomain),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("allowInsecureDomain","",c->getSystemState()->getBuiltinFunction(allowInsecureDomain),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("loadPolicyFile","",c->getSystemState()->getBuiltinFunction(loadPolicyFile),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("showSettings","",c->getSystemState()->getBuiltinFunction(showSettings),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pageDomain","",c->getSystemState()->getBuiltinFunction(pageDomain),GETTER_METHOD,false);

	c->getSystemState()->securityManager->setExactSettings(true, false);
}

ASFUNCTIONBODY_ATOM(Security,_getExactSettings)
{
	asAtomHandler::setBool(ret,wrk->getSystemState()->securityManager->getExactSettings());
}

ASFUNCTIONBODY_ATOM(Security,_setExactSettings)
{
	assert(args && argslen==1);
	if(wrk->getSystemState()->securityManager->getExactSettingsLocked())
	{
		createError<SecurityError>(wrk,0,"SecurityError: Security.exactSettings already set");
		return;
	}
	wrk->getSystemState()->securityManager->setExactSettings(asAtomHandler::Boolean_concrete(args[0]));
}

ASFUNCTIONBODY_ATOM(Security,_getSandboxType)
{
	if(wrk->getSystemState()->securityManager->getSandboxType() == SecurityManager::REMOTE)
		ret = asAtomHandler::fromString(wrk->getSystemState(),wrk->getSystemState()->securityManager->getSandboxName(SecurityManager::REMOTE));
	else if(wrk->getSystemState()->securityManager->getSandboxType() == SecurityManager::LOCAL_TRUSTED)
		ret = asAtomHandler::fromString(wrk->getSystemState(),wrk->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_TRUSTED));
	else if(wrk->getSystemState()->securityManager->getSandboxType() == SecurityManager::LOCAL_WITH_FILE)
		ret = asAtomHandler::fromString(wrk->getSystemState(),wrk->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_FILE));
	else if(wrk->getSystemState()->securityManager->getSandboxType() == SecurityManager::LOCAL_WITH_NETWORK)
		ret = asAtomHandler::fromString(wrk->getSystemState(),wrk->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_NETWORK));
	else
		assert_and_throw(false);
}

ASFUNCTIONBODY_ATOM(Security, allowDomain)
{
	LOG(LOG_NOT_IMPLEMENTED, "Security::allowDomain");
}

ASFUNCTIONBODY_ATOM(Security, allowInsecureDomain)
{
	LOG(LOG_NOT_IMPLEMENTED, "Security::allowInsecureDomain");
}

ASFUNCTIONBODY_ATOM(Security, loadPolicyFile)
{
	tiny_string url = asAtomHandler::toString(args[0],wrk);
	LOG(LOG_INFO, "Loading policy file: " << wrk->getSystemState()->mainClip->getOrigin().goToURL(url));
	wrk->getSystemState()->securityManager->addPolicyFile(wrk->getSystemState()->mainClip->getOrigin().goToURL(url));
	assert_and_throw(argslen == 1);
}

ASFUNCTIONBODY_ATOM(Security, showSettings)
{
	LOG(LOG_NOT_IMPLEMENTED, "Security::showSettings");
}

ASFUNCTIONBODY_ATOM(Security, pageDomain)
{
	tiny_string s = wrk->getSystemState()->mainClip->applicationDomain->getBaseURL().getProtocol()+"://"+wrk->getSystemState()->mainClip->applicationDomain->getBaseURL().getHostname();
	ret = asAtomHandler::fromString(wrk->getSystemState(),s);
}

ASFUNCTIONBODY_ATOM(lightspark, fscommand)
{
	assert_and_throw(argslen >= 1 && argslen <= 2);
	assert_and_throw(asAtomHandler::isString(args[0]));
	tiny_string command = asAtomHandler::toString(args[0],wrk);
	// according to specs fscommand is a void method, but the abcasm tests seem to expect a result value, so we set the result to undefined
	ret = asAtomHandler::undefinedAtom;
	if(command == "quit")
	{
		if (!wrk->isPrimordial) // only allow quit from main worker
			return;
		wrk->getSystemState()->setShutdownFlag();
	}
}


void System::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("totalMemory","",c->getSystemState()->getBuiltinFunction(totalMemory),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("disposeXML","",c->getSystemState()->getBuiltinFunction(disposeXML),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pauseForGCIfCollectionImminent","",c->getSystemState()->getBuiltinFunction(pauseForGCIfCollectionImminent),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("gc","",c->getSystemState()->getBuiltinFunction(gc),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pause","",c->getSystemState()->getBuiltinFunction(pause),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("resume","",c->getSystemState()->getBuiltinFunction(resume),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(System,totalMemory)
{
#if defined (_WIN32) || defined (__APPLE__)
	LOG(LOG_NOT_IMPLEMENTED,"System.totalMemory not implemented for this platform");
	asAtomHandler::setUInt(ret,wrk,1024);
	return;
#else
	char* buf=nullptr;
	size_t size=0;
	FILE* f = open_memstream(&buf, &size);
	if (!f || malloc_info(0,f)!=0)
	{
		LOG(LOG_ERROR,"System.totalMemory failed");
		asAtomHandler::setUInt(ret,wrk,1024);
		return;
	}
	fclose(f);

	uint32_t memsize = 0;
	pugi::xml_document xmldoc;
	xmldoc.load_buffer((void*)buf,size);
	free(buf);
	pugi::xml_node m = xmldoc.root().child("malloc");
	pugi::xml_node n = m.child("heap");
	while (n.type() != pugi::node_null)
	{
		pugi::xml_node s = n.child("system");
		pugi::xml_attribute a = s.attribute("type");
		while (strcmp(a.value(),"current")!= 0)
		{
			s = s.next_sibling("system");
			if (s.type() == pugi::node_null)
				break;
			a = s.attribute("type");
		}
		memsize += s.attribute("size").as_uint(0);
		n = n.next_sibling("heap");
	}
	asAtomHandler::setUInt(ret,wrk,memsize);
#endif
}
ASFUNCTIONBODY_ATOM(System,disposeXML)
{
	_NR<XML> xmlobj;
	ARG_CHECK(ARG_UNPACK (xmlobj));
	LOG(LOG_NOT_IMPLEMENTED,"disposeXML only removes the node from its parent");
	if (!xmlobj.isNull() && xmlobj->getParentNode()->is<XML>())
	{
		XML* parent = xmlobj->getParentNode()->as<XML>();
		XMLList* l = parent->getChildrenlist();
		if (l)
			l->removeNode(xmlobj.getPtr());
		parent->decRef();
	}
}
ASFUNCTIONBODY_ATOM(System,pauseForGCIfCollectionImminent)
{
	number_t imminence;
	ARG_CHECK(ARG_UNPACK (imminence,0.75));
	LOG(LOG_NOT_IMPLEMENTED, "System.pauseForGCIfCollectionImminent not implemented");
}
ASFUNCTIONBODY_ATOM(System,gc)
{
	LOG(LOG_NOT_IMPLEMENTED, "System.gc not implemented");
	asAtomHandler::setUndefined(ret);
}
ASFUNCTIONBODY_ATOM(System,pause)
{
	LOG(LOG_NOT_IMPLEMENTED, "System.pause not implemented");
	asAtomHandler::setUndefined(ret);
}
ASFUNCTIONBODY_ATOM(System,resume)
{
	LOG(LOG_NOT_IMPLEMENTED, "System.resume not implemented");
	asAtomHandler::setUndefined(ret);
}

WorkerDomain::WorkerDomain(ASWorker* wrk, Class_base* c):
	ASObject(wrk,c)
{
	subtype = SUBTYPE_WORKERDOMAIN;
	asAtom v=asAtomHandler::invalidAtom;
	ApplicationDomain* appdomain = wrk->rootClip->applicationDomain.getPtr();
	Template<Vector>::getInstanceS(wrk,v,Class<ASWorker>::getClass(getSystemState()),appdomain);
	workerlist = _R<Vector>(asAtomHandler::as<Vector>(v));
	workerSharedObject = _MR(new_asobject(wrk));
}

void WorkerDomain::finalize()
{
	auto it = messagechannellist.begin();
	while (it != messagechannellist.end())
	{
		(*it)->decRef();
		it = messagechannellist.erase(it);
	}
	workerlist.reset();
	workerSharedObject.reset();
}

void WorkerDomain::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (workerlist)
		workerlist->prepareShutdown();
	if (workerSharedObject)
		workerSharedObject->prepareShutdown();
	for(auto it = messagechannellist.begin(); it != messagechannellist.end(); ++it)
		(*it)->prepareShutdown();
}

void WorkerDomain::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("current","",c->getSystemState()->getBuiltinFunction(_getCurrent,0,Class<WorkerDomain>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isSupported","",c->getSystemState()->getBuiltinFunction(_isSupported,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("createWorker","",c->getSystemState()->getBuiltinFunction(createWorker,1,Class<ASWorker>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("listWorkers","",c->getSystemState()->getBuiltinFunction(listWorkers),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createWorkerFromPrimordial","",c->getSystemState()->getBuiltinFunction(createWorkerFromPrimordial),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createWorkerFromByteArray","",c->getSystemState()->getBuiltinFunction(createWorkerFromByteArray),NORMAL_METHOD,true);
}

void WorkerDomain::addMessageChannel(MessageChannel* c)
{
	c->incRef();
	messagechannellist.insert(c);
}

void WorkerDomain::removeWorker(ASWorker* w)
{
	workerlist->remove(w);
	auto it = messagechannellist.begin();
	while (it != messagechannellist.end())
	{
		if ((*it)->sender == w || (*it)->receiver == w)
		{
			if ((*it)->sender == w)
			{
				(*it)->sender->removeStoredMember();
				(*it)->sender=nullptr;
			}
			if ((*it)->receiver == w)
			{
				(*it)->receiver->removeStoredMember();
				(*it)->receiver=nullptr;
			}
			(*it)->clearEventListeners();
			(*it)->decRef();
			it = messagechannellist.erase(it);
		}
		else
			it++;
	}
}

void WorkerDomain::stopAllBackgroundWorkers()
{
	for (uint32_t i= 0; i < workerlist->size(); i++)
	{
		asAtom w = workerlist->at(i);
		if (asAtomHandler::is<ASWorker>(w))
		{
			asAtomHandler::as<ASWorker>(w)->threadAbort();
		}
	}
}
ASFUNCTIONBODY_ATOM(WorkerDomain,_constructor)
{
	asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(WorkerDomain,_getCurrent)
{
	ret = asAtomHandler::fromObject(wrk->getSystemState()->workerDomain);
}
ASFUNCTIONBODY_ATOM(WorkerDomain,_isSupported)
{
	asAtomHandler::setBool(ret,true);
}
ASFUNCTIONBODY_ATOM(WorkerDomain,createWorker)
{
	ASWorker* wk = Class<ASWorker>::getInstanceS(wrk);
	ARG_CHECK(ARG_UNPACK(wk->swf)(wk->giveAppPrivileges, false));
	if (wk->giveAppPrivileges)
		LOG(LOG_NOT_IMPLEMENTED,"WorkerDomain.createWorker: giveAppPrivileges is ignored");
	ret = asAtomHandler::fromObject(wk);
}
ASFUNCTIONBODY_ATOM(WorkerDomain,createWorkerFromPrimordial)
{
	ASWorker* wk = Class<ASWorker>::getInstanceS(wrk->getSystemState()->worker);


	ByteArray* ba = Class<ByteArray>::getInstanceS(wk);
	FileStreamCache* sc = (FileStreamCache*)wrk->getSystemState()->getEngineData()->createFileStreamCache(wrk->getSystemState());
	sc->useExistingFile(wrk->getSystemState()->getDumpedSWFPath());

	ba->append(sc->createReader(),wrk->getSystemState()->swffilesize);
	wk->swf = _MR(ba);
	ret = asAtomHandler::fromObject(wk);
}
ASFUNCTIONBODY_ATOM(WorkerDomain,createWorkerFromByteArray)
{
	ASWorker* wk = Class<ASWorker>::getInstanceS(wrk);
	ARG_CHECK(ARG_UNPACK(wk->swf));
	ret = asAtomHandler::fromObject(wk);
}

ASFUNCTIONBODY_ATOM(WorkerDomain,listWorkers)
{
	WorkerDomain* th = asAtomHandler::as<WorkerDomain>(obj);

	th->workerlist->incRef();
	ret = asAtomHandler::fromObject(th->workerlist.getPtr());
}
void WorkerState::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("NEW",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"new"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RUNNING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"running"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TERMINATED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"terminated"),CONSTANT_TRAIT);
}

void ImageDecodingPolicy::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ON_DEMAND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"onDemand"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ON_LOAD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"onLoad"),CONSTANT_TRAIT);
}

void IMEConversionMode::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALPHANUMERIC_FULL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ALPHANUMERIC_FULL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALPHANUMERIC_HALF",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ALPHANUMERIC_HALF"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CHINESE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"CHINESE"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JAPANESE_HIRAGANA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"JAPANESE_HIRAGANA"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JAPANESE_KATAKANA_FULL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"JAPANESE_KATAKANA_FULL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JAPANESE_KATAKANA_HALF",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"JAPANESE_KATAKANA_HALF"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KOREAN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"KOREAN"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNKNOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"UNKNOWN"),CONSTANT_TRAIT);
}

