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
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/system/messagechannel.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/display/Stage.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "compat.h"
#include "backends/security.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/Undefined.h"
#include "parsing/streams.h"
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
		case SystemState::AVMPLUS:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"AVMPlus");
			break;
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
		gint width = screen.w;
		gint height = screen.h;
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

ApplicationDomain::ApplicationDomain(ASWorker* wrk, Class_base* c, _NR<ApplicationDomain> p):ASObject(wrk,c,T_OBJECT,SUBTYPE_APPLICATIONDOMAIN)
	,defaultDomainMemory(Class<ByteArray>::getInstanceSNoArgs(wrk)),frameRate(0),version(0),usesActionScript3(false), parentDomain(p)
{
	defaultDomainMemory->setLength(MIN_DOMAIN_MEMORY_LIMIT);
	currentDomainMemory=defaultDomainMemory.getPtr();
}

void ApplicationDomain::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	//Static
	c->setDeclaredMethodByQName("currentDomain","",c->getSystemState()->getBuiltinFunction(_getCurrentDomain,0,Class<ApplicationDomain>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("MIN_DOMAIN_MEMORY_LENGTH","",c->getSystemState()->getBuiltinFunction(_getMinDomainMemoryLength,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	//Instance
	c->setDeclaredMethodByQName("hasDefinition","",c->getSystemState()->getBuiltinFunction(hasDefinition,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDefinition","",c->getSystemState()->getBuiltinFunction(getDefinition,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,domainMemory,ByteArray);
	REGISTER_GETTER_RESULTTYPE(c,parentDomain,ApplicationDomain);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(ApplicationDomain,domainMemory,cbDomainMemory)
ASFUNCTIONBODY_GETTER(ApplicationDomain,parentDomain)

void ApplicationDomain::cbDomainMemory(_NR<ByteArray> oldvalue)
{
	checkDomainMemory();
}

void ApplicationDomain::finalize()
{
	ASObject::finalize();
	for(auto it=dictionary.begin();it!=dictionary.end();++it)
		delete it->second;
	dictionary.clear();
	aliasMap.clear();
	domainMemory.reset();
	defaultDomainMemory.reset();
	for(auto it = instantiatedTemplates.begin(); it != instantiatedTemplates.end(); ++it)
		it->second->finalize();
	//Free template instantations by decRef'ing them
	for(auto it = instantiatedTemplates.begin(); it != instantiatedTemplates.end(); ++it)
		it->second->decRef();
	globalScopes.clear();
	instantiatedTemplates.clear();
}

void ApplicationDomain::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	aliasMap.clear();
	if (domainMemory)
		domainMemory->prepareShutdown();
	if (defaultDomainMemory)
		defaultDomainMemory->prepareShutdown();
	if (parentDomain)
		parentDomain->prepareShutdown();
	for(auto it = instantiatedTemplates.begin(); it != instantiatedTemplates.end(); ++it)
		it->second->prepareShutdown();
}

void ApplicationDomain::addSuperClassNotFilled(Class_base *cls)
{
	auto i = classesBeingDefined.cbegin();
	while (i != classesBeingDefined.cend())
	{
		if(i->second == cls->super.getPtr())
		{
			classesSuperNotFilled.insert(make_pair(cls,cls->super.getPtr()));
			break;
		}
		i++;
	}
}

void ApplicationDomain::SetAllClassLinks()
{
	for (unsigned int i = 0; i < classesToLinkInterfaces.size(); i++)
	{
		Class_base* cls = classesToLinkInterfaces[i];
		if (!cls)
			continue;
		if (newClassRecursiveLink(cls, cls))
			classesToLinkInterfaces[i] = nullptr;
	}
}
void ApplicationDomain::AddClassLinks(Class_base* target)
{
	classesToLinkInterfaces.push_back(target);
}

bool ApplicationDomain::newClassRecursiveLink(Class_base* target, Class_base* c)
{
	if(c->super)
	{
		if (!newClassRecursiveLink(target, c->super.getPtr()))
			return false;
	}
	bool bAllDefined = false;
	const vector<Class_base*>& interfaces=c->getInterfaces(&bAllDefined);
	if (!bAllDefined)
	{
		return false;
	}
	for(unsigned int i=0;i<interfaces.size();i++)
	{
		LOG_CALL("Linking with interface " << interfaces[i]->class_name);
		interfaces[i]->linkInterface(target);
	}
	return true;
}

void ApplicationDomain::copyBorrowedTraitsFromSuper(Class_base *cls)
{
	if (!cls)
		return;
	bool super_initialized=true;
	bool cls_initialized=cls->super.isNull() || classesSuperNotFilled.empty();
	if (cls->super)
	{
		for (auto itsup = classesSuperNotFilled.begin();itsup != classesSuperNotFilled.end(); itsup++)
		{
			if(itsup->second == cls->super.getPtr())
			{
				super_initialized=false;
				break;
			}
		}
	}
	auto it = classesSuperNotFilled.begin();
	while (it != classesSuperNotFilled.end())
	{
		if(it->second == cls)
		{
			it->first->copyBorrowedTraits(cls);
			if (super_initialized)
			{
				it = classesSuperNotFilled.erase(it);
				cls_initialized=true;
			}
			else
			{
				// cls->super was not yet initialized, make sure all classes that have cls as super get borrowed traits from cls->super
				it->second = cls->super.getPtr();
				it++;
			}
		}
		else
			it++;
	}
	if (cls_initialized)
	{
		// borrowed traits are all available for this class, so we can link the interfaces
		if(!cls->isInterface)
		{
			//Link all the interfaces for this class and all the bases
			if (!newClassRecursiveLink(cls, cls))
			{
				// remember classes where not all interfaces are defined yet
				AddClassLinks(cls);
			}
		}
	}
}

void ApplicationDomain::addToDictionary(DictionaryTag* r)
{
	Locker l(dictSpinlock);
	dictionary[r->getId()] = r;
}

DictionaryTag* ApplicationDomain::dictionaryLookup(int id)
{
	Locker l(dictSpinlock);
	auto it = dictionary.find(id);
	if(it==dictionary.end())
	{
		LOG(LOG_ERROR,"No such Id on dictionary " << id << " for " << origin);
		//throw RunTimeException("Could not find an object on the dictionary");
		return nullptr;
	}
	return it->second;
}

DictionaryTag* ApplicationDomain::dictionaryLookupByName(uint32_t nameID)
{
	Locker l(dictSpinlock);
	auto it = dictionary.begin();
	for(;it!=dictionary.end();++it)
	{
		if(it->second->nameID==nameID)
			return it->second;
	}
	// tag not found, also check case insensitive
	if(it==dictionary.end())
	{
		
		tiny_string namelower = getSystemState()->getStringFromUniqueId(nameID).lowercase();
		it = dictionary.begin();
		for(;it!=dictionary.end();++it)
		{
			if (it->second->nameID == UINT32_MAX)
				continue;
			tiny_string dictnamelower = getSystemState()->getStringFromUniqueId(it->second->nameID);
			dictnamelower = dictnamelower.lowercase();
			if(dictnamelower==namelower)
				return it->second;
		}
	}
	LOG(LOG_ERROR,"No such name on dictionary " << getSystemState()->getStringFromUniqueId(nameID) << " for " << origin);
	return nullptr;
}

void ApplicationDomain::registerEmbeddedFont(const tiny_string fontname, FontTag* tag)
{
	if (!fontname.empty())
	{
		auto it = embeddedfonts.find(fontname);
		if (it == embeddedfonts.end())
		{
			embeddedfonts[fontname] = tag;
			// it seems that adobe allows fontnames to be lowercased and stripped of spaces and numbers
			tiny_string tmp = fontname.lowercase();
			tiny_string fontnamenormalized;
			for (auto it = tmp.begin();it != tmp.end(); it++)
			{
				if (*it == ' ' || (*it >= '0' &&  *it <= '9'))
					continue;
				fontnamenormalized += *it;
			}
			embeddedfonts[fontnamenormalized] = tag;
		}
	}
	embeddedfontsByID[tag->getId()] = tag;
}

FontTag* ApplicationDomain::getEmbeddedFont(const tiny_string fontname) const
{
	auto it = embeddedfonts.find(fontname);
	if (it != embeddedfonts.end())
		return it->second;
	it = embeddedfonts.find(fontname.lowercase());
	if (it != embeddedfonts.end())
		return it->second;
	return nullptr;
}

FontTag* ApplicationDomain::getEmbeddedFontByID(uint32_t fontID) const
{
	auto it = embeddedfontsByID.find(fontID);
	if (it != embeddedfontsByID.end())
		return it->second;
	return nullptr;
}

void ApplicationDomain::addToScalingGrids(const DefineScalingGridTag* r)
{
	Locker l(scalinggridsmutex);
	scalinggrids[r->CharacterId] = r->Splitter;
}

RECT* ApplicationDomain::ScalingGridsLookup(int id)
{
	Locker l(scalinggridsmutex);
	auto it = scalinggrids.find(id);
	if(it==scalinggrids.end())
		return nullptr;
	return &(*it).second;
}

void ApplicationDomain::addBinding(const tiny_string& name, DictionaryTag *tag)
{
	// This function will be called only be the parsing thread,
	// and will only access the last frame, so no locking needed.
	tag->bindingclassname = name;
	uint32_t pos = name.rfind(".");
	if (pos== tiny_string::npos)
		classesToBeBound[QName(getSystemState()->getUniqueStringId(name),BUILTIN_STRINGS::EMPTY)] =  tag;
	else
		classesToBeBound[QName(getSystemState()->getUniqueStringId(name.substr(pos+1,name.numChars()-(pos+1))),getSystemState()->getUniqueStringId(name.substr(0,pos)))] = tag;
}

void ApplicationDomain::bindClass(const QName& classname, Class_inherit* cls)
{
	if (cls->isBinded() || classesToBeBound.empty())
		return;
	
	auto it=classesToBeBound.find(classname);
	if(it!=classesToBeBound.end())
	{
		cls->bindToTag(it->second);
		classesToBeBound.erase(it);
	}
}
bool ApplicationDomain::AVM1registerTagClass(const tiny_string &name, _NR<IFunction> theClassConstructor)
{
	uint32_t nameID = getSystemState()->getUniqueStringId(name);
	DictionaryTag* t = dictionaryLookupByName(nameID);
	if (!t)
	{
		LOG(LOG_ERROR,"registerClass:no tag found in dictionary for "<<name);
		return false;
	}
	if (theClassConstructor.isNull())
		avm1ClassConstructors.erase(t->getId());
	else
		avm1ClassConstructors.insert(make_pair(t->getId(),theClassConstructor));
	return true;
}

void ApplicationDomain::checkBinding(DictionaryTag *tag)
{
	if (tag->bindingclassname.empty())
		return;
	multiname clsname(nullptr);
	clsname.name_type=multiname::NAME_STRING;
	clsname.isAttribute = false;
	
	uint32_t pos = tag->bindingclassname.rfind(".");
	tiny_string ns;
	tiny_string nm;
	if (pos != tiny_string::npos)
	{
		nm = tag->bindingclassname.substr(pos+1,tag->bindingclassname.numBytes());
		ns = tag->bindingclassname.substr(0,pos);
		clsname.hasEmptyNS=false;
	}
	else
	{
		nm = tag->bindingclassname;
		ns = "";
	}
	clsname.name_s_id=getSystemState()->getUniqueStringId(nm);
	clsname.ns.push_back(nsNameAndKind(getSystemState(),ns,NAMESPACE));
	
	ASObject* typeObject = nullptr;
	auto i = classesBeingDefined.cbegin();
	while (i != classesBeingDefined.cend())
	{
		if(i->first->name_s_id == clsname.name_s_id && i->first->ns[0].nsRealId == clsname.ns[0].nsRealId)
		{
			typeObject = i->second;
			break;
		}
		i++;
	}
	if (typeObject == nullptr)
	{
		ASObject* target;
		asAtom o=asAtomHandler::invalidAtom;
		getVariableAndTargetByMultiname(o,clsname,target,getInstanceWorker());
		if (asAtomHandler::isValid(o))
			typeObject=asAtomHandler::getObject(o);
	}
	if (typeObject != nullptr)
	{
		Class_inherit* cls = typeObject->as<Class_inherit>();
		if (cls)
		{
			ABCVm *vm = getVm(getSystemState());
			vm->buildClassAndBindTag(tag->bindingclassname.raw_buf(), tag,cls);
			tag->bindedTo=cls;
			tag->bindingclassname = "";
			cls->bindToTag(tag);
		}
	}
}

AVM1Function* ApplicationDomain::AVM1getClassConstructor(uint32_t spriteID)
{
	auto it = avm1ClassConstructors.find(spriteID);
	if (it == avm1ClassConstructors.end())
		return nullptr;
	return it->second->is<AVM1Function>() ? it->second->as<AVM1Function>() : nullptr;
}

void ApplicationDomain::setOrigin(const tiny_string& u, const tiny_string& filename)
{
	//We can use this origin to implement security measures.
	//Note that for plugins, this url is NOT the page url, but it is the swf file url.
	origin = URLInfo(u);
	//If this URL doesn't contain a filename, add the one passed as an argument (used in main.cpp)
	if(origin.getPathFile() == "" && filename != "")
	{
		tiny_string fileurl;
		if (g_path_is_absolute(filename.raw_buf()))
		{
			gchar* uri = g_filename_to_uri(filename.raw_buf(), nullptr,nullptr);
			fileurl = uri;
			g_free(uri);
		}
		else
			fileurl = filename;
		origin = origin.goToURL(fileurl);
	}
}

void ApplicationDomain::setFrameSize(const lightspark::RECT& f)
{
	frameSize=f;
}

RECT ApplicationDomain::getFrameSize() const
{
	return frameSize;
}

void ApplicationDomain::setFrameRate(float f)
{
	if (frameRate != f)
	{
		frameRate=f;
		if (this == getSystemState()->mainClip->applicationDomain.getPtr() )
			getSystemState()->setRenderRate(frameRate);
	}
}

float ApplicationDomain::getFrameRate() const
{
	return frameRate;
}

void ApplicationDomain::setBaseURL(const tiny_string& url)
{
	//Set the URL to be used in resolving relative paths. For the
	//plugin this is either the value of base attribute in the
	//OBJECT or EMBED tag or, if the attribute is not provided,
	//the address of the hosting HTML page.
	baseURL = URLInfo(url);
}
const URLInfo& ApplicationDomain::getBaseURL()
{
	//The plugin uses the address of the HTML page (baseURL) for
	//resolving relative paths. AIR and the standalone Lightspark
	//use the SWF location (origin).
	if(baseURL.isValid())
		return baseURL;
	else
		return getOrigin();
}
ASFUNCTIONBODY_ATOM(ApplicationDomain,_constructor)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	_NR<ApplicationDomain> parentDomain;
	ARG_CHECK(ARG_UNPACK (parentDomain, NullRef));
	if(!th->parentDomain.isNull())
		// Don't override parentDomain if it was set in the
		// C++ constructor
		return;
	else if(parentDomain.isNull())
		th->parentDomain = _MR(wrk->getSystemState()->systemDomain);
	else
		th->parentDomain = parentDomain;
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_getMinDomainMemoryLength)
{
	asAtomHandler::setUInt(ret,wrk,(uint32_t)MIN_DOMAIN_MEMORY_LIMIT);
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_getCurrentDomain)
{
	ApplicationDomain* res=ABCVm::getCurrentApplicationDomain(wrk->currentCallContext);
	res->incRef();
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,hasDefinition)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	assert(argslen==1);
	const tiny_string& tmp=asAtomHandler::toString(args[0],wrk);

	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=wrk->getSystemState()->getUniqueStringId(tmpName);
	if (nsName != "")
		name.ns.push_back(nsNameAndKind(wrk->getSystemState(),nsName,NAMESPACE));

	LOG(LOG_CALLS,"Looking for definition of " << name);
	ASObject* target;
	asAtom o=asAtomHandler::invalidAtom;
	ret = asAtomHandler::invalidAtom;
	th->getVariableAndTargetByMultinameIncludeTemplatedClasses(o,name,target,wrk);
	if(asAtomHandler::isInvalid(o))
		asAtomHandler::setBool(ret,false);
	else
	{
		LOG(LOG_CALLS,"Found definition for " << name<<"|"<<asAtomHandler::toDebugString(o)<<"|"<<asAtomHandler::toDebugString(obj));
		asAtomHandler::setBool(ret,true);
	}
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,getDefinition)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	assert(argslen==1);
	const tiny_string& tmp=asAtomHandler::toString(args[0],wrk);

	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=wrk->getSystemState()->getUniqueStringId(tmpName);
	name.hasEmptyNS=nsName == "";
	name.ns.push_back(nsNameAndKind(wrk->getSystemState(),nsName,NAMESPACE));

	LOG(LOG_CALLS,"Looking for definition of " << name);
	ret = asAtomHandler::invalidAtom;
	ASObject* target;
	th->getVariableAndTargetByMultinameIncludeTemplatedClasses(ret,name,target,wrk);
	if(asAtomHandler::isInvalid(ret))
	{
		ret = asAtomHandler::undefinedAtom;
		createError<ReferenceError>(wrk,kClassNotFoundError,name.normalizedNameUnresolved(wrk->getSystemState()));
		return;
	}

	//TODO: specs says that also namespaces and function may be returned
	//assert_and_throw(o->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,"Getting definition for " << name<<" "<<asAtomHandler::toDebugString(ret));
	ASATOM_INCREF(ret);
}

void ApplicationDomain::registerGlobalScope(Global* scope)
{
	globalScopes.push_back(scope);
}

ASObject* ApplicationDomain::getVariableByString(const std::string& str, ASObject*& target)
{
	size_t index=str.rfind('.');
	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;
	if(index==str.npos) //No dot
	{
		name.name_s_id=getSystemState()->getUniqueStringId(str);
		name.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE)); //TODO: use ns kind
	}
	else
	{
		name.name_s_id=getSystemState()->getUniqueStringId(str.substr(index+1));
		name.ns.push_back(nsNameAndKind(getSystemState(),str.substr(0,index),NAMESPACE));
		name.hasEmptyNS=false;
	}
	asAtom ret=asAtomHandler::invalidAtom;
	getVariableAndTargetByMultiname(ret,name, target,getInstanceWorker());
	return asAtomHandler::toObject(ret,getInstanceWorker());
}

bool ApplicationDomain::findTargetByMultiname(const multiname& name, ASObject*& target, ASWorker* wrk)
{
	//Check in the parent first
	if(!parentDomain.isNull())
	{
		bool ret=parentDomain->findTargetByMultiname(name, target,wrk);
		if(ret)
			return true;
	}

	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		bool ret=globalScopes[i]->hasPropertyByMultiname(name,true,true,wrk);
		if(ret)
		{
			target=globalScopes[i];
			return true;
		}
	}
	return false;
}


GET_VARIABLE_RESULT ApplicationDomain::getVariableAndTargetByMultiname(asAtom& ret, const multiname& name, ASObject*& target, ASWorker* wrk)
{
	GET_VARIABLE_RESULT res = GET_VARIABLE_RESULT::GETVAR_NORMAL;
	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		res = globalScopes[i]->getVariableByMultiname(ret,name,NO_INCREF,wrk);
		if(asAtomHandler::isValid(ret))
		{
			target=globalScopes[i];
			// No incRef, return a reference borrowed from globalScopes
			return res;
		}
	}
	auto it = classesBeingDefined.find(&name);
	if (it != classesBeingDefined.end())
	{
		target =((*it).second);
		return res;
	}
	if(!parentDomain.isNull())
	{
		res = parentDomain->getVariableAndTargetByMultiname(ret,name, target,wrk);
		if(asAtomHandler::isValid(ret))
			return res;
	}
	return res;
}
void ApplicationDomain::getVariableAndTargetByMultinameIncludeTemplatedClasses(asAtom& ret, const multiname& name, ASObject*& target, ASWorker* wrk)
{
	getVariableAndTargetByMultiname(ret,name, target,wrk);
	if (asAtomHandler::isValid(ret))
		return;
	if (name.ns.size() >= 1 && name.ns[0].nsNameId == BUILTIN_STRINGS::STRING_AS3VECTOR)
	{
		tiny_string s = getSystemState()->getStringFromUniqueId(name.name_s_id);
		if (s.startsWith("Vector.<"))
		{
			tiny_string vtype = s.substr_bytes(8,s.numBytes()-9);

			multiname tn(nullptr);
			tn.name_type=multiname::NAME_STRING;

			uint32_t lastpos = vtype.rfind("::");
			if (lastpos == tiny_string::npos)
			{
				tn.name_s_id=getSystemState()->getUniqueStringId(vtype);
				tn.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
			}
			else
			{
				tn.name_s_id=getSystemState()->getUniqueStringId(vtype.substr_bytes(lastpos+2,vtype.numBytes()-(lastpos+2)));
				tn.ns.push_back(nsNameAndKind(getSystemState(),vtype.substr_bytes(0,lastpos),NAMESPACE));
				tn.hasEmptyNS=false;
			}
			ASObject* tntarget;
			asAtom typeobj=asAtomHandler::invalidAtom;
			getVariableAndTargetByMultiname(typeobj,tn, tntarget,wrk);
			if (asAtomHandler::isValid(typeobj))
			{
				Type* t = asAtomHandler::getObject(typeobj)->as<Type>();
				ret = asAtomHandler::fromObject(Template<Vector>::getTemplateInstance(t,this).getPtr());
			}
		}
	}
}
ASObject* ApplicationDomain::getVariableByMultinameOpportunistic(const multiname& name,ASWorker* wrk)
{
	auto it = classesBeingDefined.find(&name);
	if (it != classesBeingDefined.end())
	{
		return ((*it).second);
	}
	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		asAtom o=asAtomHandler::invalidAtom;
		globalScopes[i]->getVariableByMultinameOpportunistic(o,name,wrk);
		if(asAtomHandler::isValid(o))
		{
			// No incRef, return a reference borrowed from globalScopes
			return asAtomHandler::toObject(o,wrk);
		}
	}
	if(!parentDomain.isNull())
	{
		ASObject* ret=parentDomain->getVariableByMultinameOpportunistic(name,wrk);
		if(ret)
			return ret;
	}
	return nullptr;
}

void ApplicationDomain::throwRangeError()
{
	createError<RangeError>(getWorker(),kInvalidRangeError);
}

void ApplicationDomain::checkDomainMemory()
{
	if(domainMemory.isNull())
	{
		domainMemory = defaultDomainMemory;
		domainMemory->setLength(MIN_DOMAIN_MEMORY_LIMIT);
	}
	currentDomainMemory=domainMemory.getPtr();
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

extern uint32_t asClassCount;

ASWorker::ASWorker(SystemState* s):
	EventDispatcher(this,nullptr),parser(nullptr),
	giveAppPrivileges(false),started(false),inGarbageCollection(false),inShutdown(false),inFinalize(false),
	stage(nullptr),
	freelist(new asfreelist[asClassCount]),currentCallContext(nullptr),cur_recursion(0),isPrimordial(true),state("running"),
	nativeExtensionCallCount(0)
{
	subtype = SUBTYPE_WORKER;
	setSystemState(s);
	// TODO: it seems that AIR applications have a higher default value for max_recursion
	// I haven't found any documentation about that, so we just set it to a value that seems to work...
	limits.max_recursion = s->flashMode == SystemState::AIR ? 2048 : 256;
	limits.script_timeout = 20;
	stacktrace = new stacktrace_entry[limits.max_recursion];
	last_garbagecollection = compat_msectiming();
	// start and end of gc list point to this to ensure that every added object gets a valid pointer set as its next/prev pointers
	gcNext=this;
	gcPrev=this;
}

ASWorker::ASWorker(Class_base* c):
	EventDispatcher(c->getSystemState()->worker,c),parser(nullptr),
	giveAppPrivileges(false),started(false),inGarbageCollection(false),inShutdown(false),inFinalize(false),
	stage(nullptr),
	freelist(new asfreelist[asClassCount]),currentCallContext(nullptr),cur_recursion(0),isPrimordial(false),state("new"),
	nativeExtensionCallCount(0)
{
	subtype = SUBTYPE_WORKER;
	// TODO: it seems that AIR applications have a higher default value for max_recursion
	// I haven't found any documentation about that, so we just set it to a value that seems to work...
	limits.max_recursion = c->getSystemState()->flashMode == SystemState::AIR ? 2048 : 256;
	limits.script_timeout = 20;
	stacktrace = new stacktrace_entry[limits.max_recursion];
	loader = _MR(Class<Loader>::getInstanceS(this));
	last_garbagecollection = compat_msectiming();
	// start and end of gc list point to this to ensure that every added object gets a valid pointer set as its next/prev pointers
	gcNext=this;
	gcPrev=this;
}
ASWorker::ASWorker(ASWorker* wrk, Class_base* c):
	EventDispatcher(wrk,c),parser(nullptr),
	giveAppPrivileges(false),started(false),inGarbageCollection(false),inShutdown(false),inFinalize(false),
	stage(nullptr),
	freelist(new asfreelist[asClassCount]),currentCallContext(nullptr),cur_recursion(0),isPrimordial(false),state("new"),
	nativeExtensionCallCount(0)
{
	subtype = SUBTYPE_WORKER;
	// TODO: it seems that AIR applications have a higher default value for max_recursion
	// I haven't found any documentation about that, so we just set it to a value that seems to work...
	limits.max_recursion = c->getSystemState()->flashMode == SystemState::AIR ? 2048 : 256;
	limits.script_timeout = 20;
	stacktrace = new stacktrace_entry[limits.max_recursion];
	loader = _MR(Class<Loader>::getInstanceS(this));
	last_garbagecollection = compat_msectiming();
	// start and end of gc list point to this to ensure that every added object gets a valid pointer set as its next/prev pointers
	gcNext=this;
	gcPrev=this;
}

void ASWorker::finalize()
{
	if (inFinalize)
		return;
	inFinalize=true;
	if (!this->preparedforshutdown)
		this->prepareShutdown();
	protoypeMap.clear();
	// remove all references to freelists
	for (auto it = constantrefs.begin(); it != constantrefs.end(); it++)
	{
		(*it)->prepareShutdown();
	}
	if (inShutdown)
		return;
	processGarbageCollection(true);
	if (!isPrimordial)
	{
		threadAborting = true;
		parsemutex.lock();
		if (parser)
			parser->threadAborting = true;
		parsemutex.unlock();
		threadAbort();
		sem_event_cond.signal();
		started = false;
		if (rootClip)
		{
			for(auto it = rootClip->applicationDomain->customClasses.begin(); it != rootClip->applicationDomain->customClasses.end(); ++it)
				it->second->finalize();
			for(auto it = rootClip->applicationDomain->templates.begin(); it != rootClip->applicationDomain->templates.end(); ++it)
				it->second->finalize();
			for(auto i = rootClip->applicationDomain->customClasses.begin(); i != rootClip->applicationDomain->customClasses.end(); ++i)
				i->second->decRef();
			for(auto i = rootClip->applicationDomain->templates.begin(); i != rootClip->applicationDomain->templates.end(); ++i)
				i->second->decRef();
			rootClip->applicationDomain->customClasses.clear();
			rootClip.reset();
		}
		for(size_t i=0;i<contexts.size();++i)
		{
			delete contexts[i];
		}
		if (stage)
			stage->decRef();
	}
	stage=nullptr;
	destroyContents();
	loader.reset();
	swf.reset();
	EventDispatcher::finalize();
	constantrefs.erase(this);
	ASObject* ogc = this->gcNext;
	while (ogc && ogc != this)
	{
		ogc->prepareShutdown();
		ogc=ogc->gcNext;
	}
	// remove all references to variables as they might point to other constant reffed objects
	for (auto it = constantrefs.begin(); it != constantrefs.end(); it++)
	{
		(*it)->destruct();
		(*it)->finalize();
	}
	processGarbageCollection(true);
	inShutdown=true;
	if (getWorker()==this)
		setTLSWorker(nullptr);
	// destruct all constant refs
	auto it = constantrefs.begin();
	it = constantrefs.begin();
	while (it != constantrefs.end())
	{
		ASObject* o = (*it);
		if (o->is<ASWorker>()
				|| o == getSystemState()->stage 
				|| o == getSystemState()->workerDomain
				|| o == getSystemState()->mainClip
				|| o == getSystemState()->systemDomain
				|| o == getSystemState()->getObjectClassRef()
				)
		{
			// these will be deleted later
			it++;
			continue;
		}
		it = constantrefs.erase(it);
		delete o;
	}
	constantrefs.clear();
	delete[] stacktrace;
	delete[] freelist;
	freelist=nullptr;
}

void ASWorker::prepareShutdown()
{
	if(preparedforshutdown)
		return;
	parsemutex.lock();
	EventDispatcher::prepareShutdown();
	if (rootClip)
	{
		for(auto it = rootClip->applicationDomain->customClasses.begin(); it != rootClip->applicationDomain->customClasses.end(); ++it)
			it->second->prepareShutdown();
		for(auto it = rootClip->applicationDomain->templates.begin(); it != rootClip->applicationDomain->templates.end(); ++it)
			it->second->prepareShutdown();
		rootClip->prepareShutdown();
	}
	if (loader)
		loader->prepareShutdown();
	if (swf)
		swf->prepareShutdown();
	if (stage)
		stage->prepareShutdown();
	parsemutex.unlock();
}

Prototype* ASWorker::getClassPrototype(const Class_base* cls)
{
	auto it = protoypeMap.find(cls);
	if (it == protoypeMap.end())
	{
		if (inFinalize)
			return nullptr;
		Prototype* p = cls->prototype->clonePrototype(this);
		it = protoypeMap.insert(make_pair(cls,_MR(p))).first;
	}
	return it->second.getPtr();
}

void ASWorker::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("current","",c->getSystemState()->getBuiltinFunction(_getCurrent,0,Class<ASWorker>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("getSharedProperty","",c->getSystemState()->getBuiltinFunction(getSharedProperty,1,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isSupported","",c->getSystemState()->getBuiltinFunction(isSupported,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	REGISTER_GETTER_RESULTTYPE(c, isPrimordial,Boolean);
	REGISTER_GETTER_RESULTTYPE(c, state,ASString);
	c->setDeclaredMethodByQName("addEventListener","",c->getSystemState()->getBuiltinFunction(_addEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createMessageChannel","",c->getSystemState()->getBuiltinFunction(createMessageChannel,1,Class<MessageChannel>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeEventListener","",c->getSystemState()->getBuiltinFunction(_removeEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSharedProperty","",c->getSystemState()->getBuiltinFunction(setSharedProperty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("start","",c->getSystemState()->getBuiltinFunction(start),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("terminate","",c->getSystemState()->getBuiltinFunction(terminate,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
}

void ASWorker::throwStackOverflow()
{
	createError<StackOverflowError>(this,kStackOverflowError);
}

void ASWorker::execute()
{
	setTLSWorker(this);

	streambuf *sbuf = new bytes_buf(swf->bytes,swf->getLength());
	istream s(sbuf);
	parsemutex.lock();
	parser = new ParseThread(s,_MR(Class<ApplicationDomain>::getInstanceS(this,_MR(getSystemState()->systemDomain))),getSystemState()->mainClip->securityDomain,loader.getPtr(),"");
	parsemutex.unlock();
	getSystemState()->addWorker(this);
	this->incRef();
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getInstanceWorker(),"workerState")),true);
	if (!this->threadAborting)
	{
		LOG(LOG_INFO,"start worker "<<this->toDebugString()<<" "<<this->isPrimordial<<" "<<this);
		parser->execute();
	}
	parsemutex.lock();
	delete parser;
	parser = nullptr;
	parsemutex.unlock();
	loader.reset();
	if (swf)
	{
		swf->objfreelist=nullptr;
		swf.reset();
	}
	while (true)
	{
		event_queue_mutex.lock();
		while(events_queue.empty() && !this->threadAborting)
			sem_event_cond.wait(event_queue_mutex);
		processGarbageCollection(false);
		if (this->threadAborting)
		{
			if(events_queue.empty())
			{
				event_queue_mutex.unlock();
				break;
			}
		}

		_NR<EventDispatcher> dispatcher=events_queue.front().first;
		_R<Event> e=events_queue.front().second;
		events_queue.pop_front();
	
		event_queue_mutex.unlock();
		try
		{
			if (dispatcher)
			{
				dispatcher->handleEvent(e);
				dispatcher->afterHandleEvent(e.getPtr());
			}
		}
		catch(LightsparkException& e)
		{
			LOG(LOG_ERROR,"Error in worker " << e.cause);
			getSystemState()->setError(e.cause);
			threadAborting = true;
		}
		catch(ASObject*& e)
		{
			if(e->getClass())
				LOG(LOG_ERROR,"Unhandled ActionScript exception in worker " << e->toString());
			else
				LOG(LOG_ERROR,"Unhandled ActionScript exception in worker (no type)");
			if (e->is<ASError>())
			{
				LOG(LOG_ERROR,"Unhandled ActionScript exception in worker " << e->as<ASError>()->getStackTraceString());
				if (getSystemState()->ignoreUnhandledExceptions)
					return;
				getSystemState()->setError(e->as<ASError>()->getStackTraceString());
			}
			else
				getSystemState()->setError("Unhandled ActionScript exception");
			threadAborting = true;
		}
		if (threadAborting)
		{
			while(!events_queue.empty())
			{
				_R<Event> e=events_queue.front().second;
				events_queue.pop_front();
			}
			threadAbort();
			started = false;
		}
	}
	delete sbuf;
}

void ASWorker::jobFence()
{
	state ="terminated";
	rootClip.reset();
	this->incRef();
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getInstanceWorker(),"workerState")),true);
	sem_event_cond.signal();
}

void ASWorker::threadAbort()
{
	threadAborting=true;
	sem_event_cond.signal();
	if (this->isPrimordial)
		getSystemState()->removeWorker(this);
}

void ASWorker::afterHandleEvent(Event* ev)
{
	if (ev->type=="workerState" && state=="terminated")
	{
		getSystemState()->removeWorker(this);
	}
}
bool ASWorker::addEvent(_NR<EventDispatcher> obj, _R<Event> ev)
{
	if (this->threadAborting)
	{
		if (obj)
			obj->afterHandleEvent(ev.getPtr());
		return false;
	}
	Locker l(event_queue_mutex);
	events_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	RELEASE_WRITE(ev->queued,true);
	sem_event_cond.signal();
	return true;
}

tiny_string ASWorker::getDefaultXMLNamespace() const
{
	return getSystemState()->getStringFromUniqueId(currentCallContext ? currentCallContext->defaultNamespaceUri : (uint32_t)BUILTIN_STRINGS::EMPTY);
}

uint32_t ASWorker::getDefaultXMLNamespaceID() const
{
	return currentCallContext ? currentCallContext->defaultNamespaceUri : (uint32_t)BUILTIN_STRINGS::EMPTY;
}

void ASWorker::dumpStacktrace()
{
	tiny_string strace;
	for (uint32_t i = cur_recursion; i > 0; i--)
	{
		strace += "    at ";
		strace += asAtomHandler::toObject(stacktrace[i-1].object,this)->getClassName();
		strace += "/";
		strace += this->getSystemState()->getStringFromUniqueId(stacktrace[i-1].name);
		strace += "()\n";
	}
	LOG(LOG_INFO,"current stacktrace:\n" << strace);
}

void ASWorker::addObjectToGarbageCollector(ASObject* o)
{
	if (o->gcPrev || o->gcNext || this->gcNext == o)
		return;
	assert(o!=this);
	if (this->gcNext==this)
	{
		this->gcNext=o;
		o->gcPrev=this;
		o->gcNext=this;
	}
	else
	{
		o->gcNext=this->gcNext;
		o->gcPrev=this;
		this->gcNext->gcPrev=o;
		this->gcNext=o;
	}
}

void ASWorker::removeObjectFromGarbageCollector(ASObject* o)
{
	if (!o->gcPrev || !o->gcNext || o==this)
		return;
	if (o->gcPrev !=this)
		o->gcPrev->gcNext=o->gcNext;
	else
	{
		this->gcNext=o->gcNext;
		this->gcNext->gcPrev=this;
	}
	if (o->gcNext!=this)
		o->gcNext->gcPrev=o->gcPrev;
	else
		o->gcPrev->gcNext=this;
	o->gcNext=nullptr;
	o->gcPrev=nullptr;
}
void ASWorker::processGarbageCollection(bool force)
{
	uint64_t currtime = compat_msectiming();
	int64_t diff =  currtime-last_garbagecollection;
	if (!force && diff < 10000) // ony execute garbagecollection every 10 seconds
		return;
	last_garbagecollection = currtime;
	if (this->stage)
		this->stage->cleanupDeadHiddenObjects();
	inGarbageCollection=true;
	bool hasEntries=this->gcNext && this->gcNext != this;
	// use two loops to make sure objects added during inner loop are handled _after_ the inner loop is complete
	while (hasEntries)
	{
		ASObject* ogc = this->gcNext;
		hasEntries=false;
		while (ogc && ogc != this)
		{
			ASObject* ogcnext = ogc->gcNext;
			if (!ogc->deletedingarbagecollection)
			{
				this->removeObjectFromGarbageCollector(ogc);
				ogc->markedforgarbagecollection = false;
				if (ogc->handleGarbageCollection())
				{
					this->addObjectToGarbageCollector(ogc);
					hasEntries=true;
				}
			}
			ogc = ogcnext;
		}
	}
	inGarbageCollection=false;
	// delete all objects that were destructed during gc
	ASObject* ogc = this->gcNext;
	while (ogc && ogc != this)
	{
		ASObject* ogcnext = ogc->gcNext;
		if (ogc->deletedingarbagecollection)
		{
			ogc->deletedingarbagecollection=false;
			ogc->removefromGarbageCollection();
			ogc->resetRefCount();
			ogc->setConstant(false);
			ogc->decRef();
		}
		ogc = ogcnext;
	}
	if (force && this->gcNext && this->gcNext != this)
		processGarbageCollection(true);
}

void ASWorker::registerConstantRef(ASObject* obj)
{
	if (inFinalize || obj->getConstant())
		return;
	constantrefmutex.lock();
	constantrefs.insert(obj);
	constantrefmutex.unlock();
}

ASFUNCTIONBODY_GETTER(ASWorker, state)
ASFUNCTIONBODY_GETTER(ASWorker, isPrimordial)

ASFUNCTIONBODY_ATOM(ASWorker,_getCurrent)
{
	wrk->incRef();
	ret = asAtomHandler::fromObject(wrk);
}
ASFUNCTIONBODY_ATOM(ASWorker,getSharedProperty)
{
	tiny_string key;
	ARG_CHECK(ARG_UNPACK(key));
	Locker l(wrk->getSystemState()->workerDomain->workersharedobjectmutex);
	
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId(key);
	m.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
	m.isAttribute = false;
	if (wrk->getSystemState()->workerDomain->workerSharedObject->hasPropertyByMultiname(m,true,false,wrk))
		wrk->getSystemState()->workerDomain->workerSharedObject->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,wrk);
	else
		asAtomHandler::setNull(ret);
}
ASFUNCTIONBODY_ATOM(ASWorker,isSupported)
{
	asAtomHandler::setBool(ret,true);
}
ASFUNCTIONBODY_ATOM(ASWorker,_addEventListener)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	EventDispatcher::addEventListener(ret,th,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(ASWorker,createMessageChannel)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	_NR<ASWorker> receiver;
	ARG_CHECK(ARG_UNPACK(receiver));
	if (receiver.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"receiver");
		return;
	}
	MessageChannel* channel = Class<MessageChannel>::getInstanceSNoArgs(th);
	th->incRef();
	th->addStoredMember();
	channel->sender = th;
	receiver->incRef();
	receiver->addStoredMember();
	channel->receiver = receiver.getPtr();
	wrk->getSystemState()->workerDomain->addMessageChannel(channel);
	ret = asAtomHandler::fromObjectNoPrimitive(channel);
}
ASFUNCTIONBODY_ATOM(ASWorker,_removeEventListener)
{
	EventDispatcher::removeEventListener(ret,wrk,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(ASWorker,setSharedProperty)
{
	tiny_string key;
	asAtom value=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(key)(value));
	Locker l(wrk->getSystemState()->workerDomain->workersharedobjectmutex);
	ASATOM_INCREF(value);
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=wrk->getSystemState()->getUniqueStringId(key);
	m.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
	m.isAttribute = false;
	wrk->getSystemState()->workerDomain->workerSharedObject->setVariableByMultiname(m,value,CONST_NOT_ALLOWED,nullptr,wrk);
}
ASFUNCTIONBODY_ATOM(ASWorker,start)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	if (th->started)
	{
		createError<ASError>(wrk,kWorkerAlreadyStarted);
		return;
	}
	if (!th->swf.isNull())
	{
		th->started = true;
		wrk->getSystemState()->addJob(th);
	}
	asAtomHandler::setUndefined(ret);
}
ASFUNCTIONBODY_ATOM(ASWorker,terminate)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	if (th->isPrimordial)
		asAtomHandler::setBool(ret,false);
	else
	{
		th->threadAborting = true;
		th->parsemutex.lock();
		if (th->parser)
			th->parser->threadAborting = true;
		th->parsemutex.unlock();
		th->threadAbort();
		asAtomHandler::setBool(ret,th->started);
		th->started = false;
		th->getSystemState()->removeWorker(th);
	}
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
	if(c->getSystemState()->flashMode==SystemState::AVMPLUS)
	{
		c->setDeclaredMethodByQName("createWorkerFromPrimordial","",c->getSystemState()->getBuiltinFunction(createWorkerFromPrimordial),NORMAL_METHOD,true);
		c->setDeclaredMethodByQName("createWorkerFromByteArray","",c->getSystemState()->getBuiltinFunction(createWorkerFromByteArray),NORMAL_METHOD,true);
	}

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

