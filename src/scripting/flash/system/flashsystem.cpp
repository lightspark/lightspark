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
#include "scripting/flash/system/messagechannel.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "compat.h"
#include "backends/security.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/toplevel/Vector.h"
#include "parsing/streams.h"

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
	c->setDeclaredMethodByQName("language","",Class<IFunction>::getFunction(c->getSystemState(),_getLanguage,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("playerType","",Class<IFunction>::getFunction(c->getSystemState(),_getPlayerType,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("version","",Class<IFunction>::getFunction(c->getSystemState(),_getVersion,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("cpuArchitecture","",Class<IFunction>::getFunction(c->getSystemState(),_getCPUArchitecture,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isDebugger","",Class<IFunction>::getFunction(c->getSystemState(),_getIsDebugger,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isEmbeddedInAcrobat","",Class<IFunction>::getFunction(c->getSystemState(),_getIsEmbeddedInAcrobat,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("localFileReadDisable","",Class<IFunction>::getFunction(c->getSystemState(),_getLocalFileReadDisable,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("manufacturer","",Class<IFunction>::getFunction(c->getSystemState(),_getManufacturer,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("os","",Class<IFunction>::getFunction(c->getSystemState(),_getOS,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("serverString","",Class<IFunction>::getFunction(c->getSystemState(),_getServerString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenResolutionX","",Class<IFunction>::getFunction(c->getSystemState(),_getScreenResolutionX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenResolutionY","",Class<IFunction>::getFunction(c->getSystemState(),_getScreenResolutionY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("hasAccessibility","",Class<IFunction>::getFunction(c->getSystemState(),_getHasAccessibility,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenDPI","",Class<IFunction>::getFunction(c->getSystemState(),_getScreenDPI,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasAudio","",Class<IFunction>::getFunction(c->getSystemState(),_hasAudio,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasAudioEncoder","",Class<IFunction>::getFunction(c->getSystemState(),_hasAudioEncoder,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasEmbeddedVideo","",Class<IFunction>::getFunction(c->getSystemState(),_hasEmbeddedVideo,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasIME","",Class<IFunction>::getFunction(c->getSystemState(),_hasIME,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasMP3","",Class<IFunction>::getFunction(c->getSystemState(),_hasMP3,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasPrinting","",Class<IFunction>::getFunction(c->getSystemState(),_hasPrinting,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasScreenBroadcast","",Class<IFunction>::getFunction(c->getSystemState(),_hasScreenBroadcast,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasScreenPlayback","",Class<IFunction>::getFunction(c->getSystemState(),_hasScreenPlayback,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasStreamingAudio","",Class<IFunction>::getFunction(c->getSystemState(),_hasStreamingAudio,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasStreamingVideo","",Class<IFunction>::getFunction(c->getSystemState(),_hasStreamingVideo,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasTLS","",Class<IFunction>::getFunction(c->getSystemState(),_hasTLS,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasVideoEncoder","",Class<IFunction>::getFunction(c->getSystemState(),_hasVideoEncoder,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("supports32BitProcesses","",Class<IFunction>::getFunction(c->getSystemState(),_supports32BitProcesses,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("supports64BitProcesses","",Class<IFunction>::getFunction(c->getSystemState(),_supports64BitProcesses,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("touchscreenType","",Class<IFunction>::getFunction(c->getSystemState(),_touchscreenType,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("avHardwareDisable","",Class<IFunction>::getFunction(c->getSystemState(),_avHardwareDisable,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("pixelAspectRatio","",Class<IFunction>::getFunction(c->getSystemState(),_pixelAspectRatio,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("screenColor","",Class<IFunction>::getFunction(c->getSystemState(),_screenColor,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
    c->setDeclaredMethodByQName("hasMultiChannelAudio","",Class<IFunction>::getFunction(c->getSystemState(),_hasMultiChannelAudio),NORMAL_METHOD,false);
    c->setDeclaredMethodByQName("maxLevelIDC","",Class<IFunction>::getFunction(c->getSystemState(),_maxLevelIDC,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getPlayerType)
{
	switch (sys->flashMode)
	{
		case SystemState::AVMPLUS:
			ret = asAtomHandler::fromString(sys,"AVMPlus");
			break;
		case SystemState::AIR:
			ret = asAtomHandler::fromString(sys,"Desktop");
			break;
		default:
			ret = asAtomHandler::fromString(sys,"PlugIn");
			break;
	}
}

ASFUNCTIONBODY_ATOM(Capabilities,_getLanguage)
{
	ret = asAtomHandler::fromString(sys,"en");
}

ASFUNCTIONBODY_ATOM(Capabilities,_getCPUArchitecture)
{
	LOG(LOG_NOT_IMPLEMENTED, "Capabilities.cpuArchitecture is not implemented");
	ret = asAtomHandler::fromString(sys,"x86");
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
	ret = asAtomHandler::fromString(sys,MANUFACTURER);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getOS)
{
#ifdef _WIN32
	ret = asAtomHandler::fromString(sys,"Windows");
#else
	ret = asAtomHandler::fromString(sys,"Linux");
#endif
}

ASFUNCTIONBODY_ATOM(Capabilities,_getVersion)
{
	ret = asAtomHandler::fromString(sys,EMULATED_VERSION);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getServerString)
{
	LOG(LOG_NOT_IMPLEMENTED,"Capabilities: not all capabilities are reported in ServerString");
	tiny_string res = "&SA=t&SV=t&OS=Linux&PT=PlugIn&L=en&TLS=t&DD=t";
	res +="&V=";
	res += EMULATED_VERSION;
	res +="&M=";
	res += MANUFACTURER;

	SDL_DisplayMode screen;
	if (sys->getEngineData()->getScreenData(&screen)) {
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
	ret = asAtomHandler::fromString(sys,res);
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
    ret = asAtomHandler::fromString(sys,"");
}

ASFUNCTIONBODY_ATOM(Capabilities,_pixelAspectRatio)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.pixelAspectRatio always returns 1");
    asAtomHandler::setNumber(ret,sys,1);
}

ASFUNCTIONBODY_ATOM(Capabilities,_screenColor)
{
    ret = asAtomHandler::fromString(sys,"color");
}

ASFUNCTIONBODY_ATOM(Capabilities,_supports32BitProcesses)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.supports32BitProcesses always returns false");
    ret = asAtomHandler::fromBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_supports64BitProcesses)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.supports64BitProcesses always returns \"none\"");
    ret = asAtomHandler::fromString(sys,"none");
}

ASFUNCTIONBODY_ATOM(Capabilities,_touchscreenType)
{
    LOG(LOG_NOT_IMPLEMENTED,"Capabilities.touchscreenType always returns empty string");
    ret = asAtomHandler::fromString(sys,"none");
}

ASFUNCTIONBODY_ATOM(Capabilities,_getScreenResolutionX)
{
	SDL_DisplayMode screen;
	if (!sys->getEngineData()->getScreenData(&screen))
		asAtomHandler::setInt(ret,sys,0);
	else
		asAtomHandler::setInt(ret,sys,screen.w);
}
ASFUNCTIONBODY_ATOM(Capabilities,_getScreenResolutionY)
{
	SDL_DisplayMode screen;
	if (!sys->getEngineData()->getScreenData(&screen))
		asAtomHandler::setInt(ret,sys,0);
	else
		asAtomHandler::setInt(ret,sys,screen.h);
}
ASFUNCTIONBODY_ATOM(Capabilities,_getHasAccessibility)
{
	LOG(LOG_NOT_IMPLEMENTED,"hasAccessibility always returns false");
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getScreenDPI)
{
	number_t dpi = sys->getEngineData()->getScreenDPI();
	asAtomHandler::setNumber(ret,sys,dpi);
}

ApplicationDomain::ApplicationDomain(Class_base* c, _NR<ApplicationDomain> p):ASObject(c,T_OBJECT,SUBTYPE_APPLICATIONDOMAIN),defaultDomainMemory(Class<ByteArray>::getInstanceSNoArgs(c->getSystemState())), parentDomain(p)
{
	defaultDomainMemory->setLength(MIN_DOMAIN_MEMORY_LIMIT);
	currentDomainMemory=defaultDomainMemory.getPtr();
}

void ApplicationDomain::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	//Static
	c->setDeclaredMethodByQName("currentDomain","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentDomain),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("MIN_DOMAIN_MEMORY_LENGTH","",Class<IFunction>::getFunction(c->getSystemState(),_getMinDomainMemoryLength),GETTER_METHOD,false);
	//Instance
	c->setDeclaredMethodByQName("hasDefinition","",Class<IFunction>::getFunction(c->getSystemState(),hasDefinition),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDefinition","",Class<IFunction>::getFunction(c->getSystemState(),getDefinition),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,domainMemory);
	REGISTER_GETTER(c,parentDomain);
}

ASFUNCTIONBODY_GETTER_SETTER_CB(ApplicationDomain,domainMemory,cbDomainMemory);
ASFUNCTIONBODY_GETTER(ApplicationDomain,parentDomain);

void ApplicationDomain::cbDomainMemory(_NR<ByteArray> oldvalue)
{
	checkDomainMemory();
}

void ApplicationDomain::finalize()
{
	ASObject::finalize();
	domainMemory.reset();
	for(auto it = instantiatedTemplates.begin(); it != instantiatedTemplates.end(); ++it)
		it->second->finalize();
	//Free template instantations by decRef'ing them
	for(auto i = instantiatedTemplates.begin(); i != instantiatedTemplates.end(); ++i)
		i->second->decRef();
	globalScopes.clear();
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_constructor)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	_NR<ApplicationDomain> parentDomain;
	ARG_UNPACK_ATOM (parentDomain, NullRef);
	if(!th->parentDomain.isNull())
		// Don't override parentDomain if it was set in the
		// C++ constructor
		return;
	else if(parentDomain.isNull())
		th->parentDomain = _MR(sys->systemDomain);
	else
		th->parentDomain = parentDomain;
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_getMinDomainMemoryLength)
{
	asAtomHandler::setUInt(ret,sys,(uint32_t)MIN_DOMAIN_MEMORY_LIMIT);
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_getCurrentDomain)
{
	_NR<ApplicationDomain> res=ABCVm::getCurrentApplicationDomain(getVm(sys)->currentCallContext);
	res->incRef();
	ret = asAtomHandler::fromObject(res.getPtr());
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,hasDefinition)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	assert(argslen==1);
	const tiny_string& tmp=asAtomHandler::toString(args[0],sys);

	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=sys->getUniqueStringId(tmpName);
	if (nsName != "")
		name.ns.push_back(nsNameAndKind(sys,nsName,NAMESPACE));

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ASObject* target;
	asAtom o=asAtomHandler::invalidAtom;
	ret = asAtomHandler::invalidAtom;
	th->getVariableAndTargetByMultinameIncludeTemplatedClasses(o,name,target);
	if(asAtomHandler::isInvalid(o))
		asAtomHandler::setBool(ret,false);
	else
	{
		if(!asAtomHandler::isClass(o))
			asAtomHandler::setBool(ret,false);
		else
		{
			LOG(LOG_CALLS,_("Found definition for ") << name<<"|"<<asAtomHandler::toDebugString(o)<<"|"<<asAtomHandler::toDebugString(obj));
			asAtomHandler::setBool(ret,true);
		}
	}
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,getDefinition)
{
	ApplicationDomain* th = asAtomHandler::as<ApplicationDomain>(obj);
	assert(argslen==1);
	const tiny_string& tmp=asAtomHandler::toString(args[0],sys);

	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=sys->getUniqueStringId(tmpName);
	if (nsName != "")
		name.ns.push_back(nsNameAndKind(sys,nsName,NAMESPACE));

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ret = asAtomHandler::invalidAtom;
	ASObject* target;
	th->getVariableAndTargetByMultinameIncludeTemplatedClasses(ret,name,target);
	if(asAtomHandler::isInvalid(ret))
		throwError<ReferenceError>(kClassNotFoundError,name.normalizedNameUnresolved(sys));

	//TODO: specs says that also namespaces and function may be returned
	//assert_and_throw(o->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,_("Getting definition for ") << name);
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
	}
	asAtom ret=asAtomHandler::invalidAtom;
	getVariableAndTargetByMultiname(ret,name, target);
	return asAtomHandler::toObject(ret,getSystemState());
}

bool ApplicationDomain::findTargetByMultiname(const multiname& name, ASObject*& target)
{
	//Check in the parent first
	if(!parentDomain.isNull())
	{
		bool ret=parentDomain->findTargetByMultiname(name, target);
		if(ret)
			return true;
	}

	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		bool ret=globalScopes[i]->hasPropertyByMultiname(name,true,true);
		if(ret)
		{
			target=globalScopes[i];
			return true;
		}
	}
	return false;
}


GET_VARIABLE_RESULT ApplicationDomain::getVariableAndTargetByMultiname(asAtom& ret, const multiname& name, ASObject*& target)
{
	GET_VARIABLE_RESULT res = GET_VARIABLE_RESULT::GETVAR_NORMAL;
	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		res = globalScopes[i]->getVariableByMultiname(ret,name,NO_INCREF);
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
		res = parentDomain->getVariableAndTargetByMultiname(ret,name, target);
		if(asAtomHandler::isValid(ret))
			return res;
	}
	return res;
}
void ApplicationDomain::getVariableAndTargetByMultinameIncludeTemplatedClasses(asAtom& ret, const multiname& name, ASObject*& target)
{
	getVariableAndTargetByMultiname(ret,name, target);
	if (asAtomHandler::isValid(ret))
		return;
	if (name.ns.size() >= 1 && name.ns[0].nsNameId == BUILTIN_STRINGS::STRING_AS3VECTOR)
	{
		tiny_string s = getSystemState()->getStringFromUniqueId(name.name_s_id);
		if (s.startsWith("Vector.<"))
		{
			tiny_string vtype = s.substr_bytes(8,s.numBytes()-9);

			multiname tn(NULL);
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
			}
			ASObject* tntarget;
			asAtom typeobj=asAtomHandler::invalidAtom;
			getVariableAndTargetByMultiname(typeobj,tn, tntarget);
			if (asAtomHandler::isValid(typeobj))
			{
				const Type* t = asAtomHandler::getObject(typeobj)->as<Type>();
				this->incRef();
				ret = asAtomHandler::fromObject(Template<Vector>::getTemplateInstance(getSystemState(),t,_NR<ApplicationDomain>(this)).getPtr());
			}
		}
	}
}
ASObject* ApplicationDomain::getVariableByMultinameOpportunistic(const multiname& name)
{
	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		asAtom o=asAtomHandler::invalidAtom;
		globalScopes[i]->getVariableByMultinameOpportunistic(o,name);
		if(asAtomHandler::isValid(o))
		{
			// No incRef, return a reference borrowed from globalScopes
			return asAtomHandler::toObject(o,getSystemState());
		}
	}
	auto it = classesBeingDefined.find(&name);
	if (it != classesBeingDefined.end())
	{
		return ((*it).second);
	}
	if(!parentDomain.isNull())
	{
		ASObject* ret=parentDomain->getVariableByMultinameOpportunistic(name);
		if(ret)
			return ret;
	}
	return nullptr;
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

LoaderContext::LoaderContext(Class_base* c):
	ASObject(c,T_OBJECT,SUBTYPE_LOADERCONTEXT),allowCodeImport(true),checkPolicyFile(false),imageDecodingPolicy("onDemand")
{
}

void LoaderContext::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("allowLoadBytesCodeExecution","",Class<IFunction>::getFunction(c->getSystemState(),_getter_allowCodeImport),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("allowLoadBytesCodeExecution","",Class<IFunction>::getFunction(c->getSystemState(),_setter_allowCodeImport),SETTER_METHOD,false);
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
	ARG_UNPACK_ATOM (th->checkPolicyFile, false)
		(th->applicationDomain, NullRef)
		(th->securityDomain, NullRef);
}

ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, allowCodeImport);
ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, applicationDomain);
ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, checkPolicyFile);
ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, parameters);
ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, securityDomain);
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(LoaderContext, imageDecodingPolicy);

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
	c->setDeclaredMethodByQName("currentDomain","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentDomain),GETTER_METHOD,false);
}

void SecurityDomain::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(SecurityDomain,_constructor)
{
}

ASFUNCTIONBODY_ATOM(SecurityDomain,_getCurrentDomain)
{
	_NR<SecurityDomain> res=ABCVm::getCurrentSecurityDomain(getVm(sys)->currentCallContext);
	res->incRef();
	ret = asAtomHandler::fromObject(res.getPtr());
}

void Security::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	//Fully static class
	c->setDeclaredMethodByQName("exactSettings","",Class<IFunction>::getFunction(c->getSystemState(),_getExactSettings),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("exactSettings","",Class<IFunction>::getFunction(c->getSystemState(),_setExactSettings),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("sandboxType","",Class<IFunction>::getFunction(c->getSystemState(),_getSandboxType),GETTER_METHOD,false);

	// Value is undefined and not "httpResponseStatus" like stated in documentation
	c->setVariableAtomByQName("APPLICATION",nsNameAndKind(),asAtomHandler::fromObject(c->getSystemState()->getUndefinedRef()),DECLARED_TRAIT);

	c->setVariableAtomByQName("LOCAL_TRUSTED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_TRUSTED)),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOCAL_WITH_FILE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_FILE)),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOCAL_WITH_NETWORK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_NETWORK)),DECLARED_TRAIT);
	c->setVariableAtomByQName("REMOTE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::REMOTE)),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("allowDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowDomain),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("allowInsecureDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowInsecureDomain),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("loadPolicyFile","",Class<IFunction>::getFunction(c->getSystemState(),loadPolicyFile),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("showSettings","",Class<IFunction>::getFunction(c->getSystemState(),showSettings),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pageDomain","",Class<IFunction>::getFunction(c->getSystemState(),pageDomain),GETTER_METHOD,false);

	c->getSystemState()->securityManager->setExactSettings(true, false);
}

ASFUNCTIONBODY_ATOM(Security,_getExactSettings)
{
	asAtomHandler::setBool(ret,sys->securityManager->getExactSettings());
}

ASFUNCTIONBODY_ATOM(Security,_setExactSettings)
{
	assert(args && argslen==1);
	if(sys->securityManager->getExactSettingsLocked())
	{
		throw Class<SecurityError>::getInstanceS(sys,"SecurityError: Security.exactSettings already set");
	}
	sys->securityManager->setExactSettings(asAtomHandler::Boolean_concrete(args[0]));
}

ASFUNCTIONBODY_ATOM(Security,_getSandboxType)
{
	if(sys->securityManager->getSandboxType() == SecurityManager::REMOTE)
		ret = asAtomHandler::fromString(sys,sys->securityManager->getSandboxName(SecurityManager::REMOTE));
	else if(sys->securityManager->getSandboxType() == SecurityManager::LOCAL_TRUSTED)
		ret = asAtomHandler::fromString(sys,sys->securityManager->getSandboxName(SecurityManager::LOCAL_TRUSTED));
	else if(sys->securityManager->getSandboxType() == SecurityManager::LOCAL_WITH_FILE)
		ret = asAtomHandler::fromString(sys,sys->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_FILE));
	else if(sys->securityManager->getSandboxType() == SecurityManager::LOCAL_WITH_NETWORK)
		ret = asAtomHandler::fromString(sys,sys->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_NETWORK));
	else
		assert_and_throw(false);
}

ASFUNCTIONBODY_ATOM(Security, allowDomain)
{
	LOG(LOG_NOT_IMPLEMENTED, _("Security::allowDomain"));
}

ASFUNCTIONBODY_ATOM(Security, allowInsecureDomain)
{
	LOG(LOG_NOT_IMPLEMENTED, _("Security::allowInsecureDomain"));
}

ASFUNCTIONBODY_ATOM(Security, loadPolicyFile)
{
	tiny_string url = asAtomHandler::toString(args[0],sys);
	LOG(LOG_INFO, "Loading policy file: " << sys->mainClip->getOrigin().goToURL(url));
	sys->securityManager->addPolicyFile(sys->mainClip->getOrigin().goToURL(url));
	assert_and_throw(argslen == 1);
}

ASFUNCTIONBODY_ATOM(Security, showSettings)
{
	LOG(LOG_NOT_IMPLEMENTED, _("Security::showSettings"));
}

ASFUNCTIONBODY_ATOM(Security, pageDomain)
{
	tiny_string s = sys->mainClip->getBaseURL().getProtocol()+"://"+sys->mainClip->getBaseURL().getHostname();
	ret = asAtomHandler::fromString(sys,s);
}

ASFUNCTIONBODY_ATOM(lightspark, fscommand)
{
	assert_and_throw(argslen >= 1 && argslen <= 2);
	assert_and_throw(asAtomHandler::isString(args[0]));
	tiny_string command = asAtomHandler::toString(args[0],sys);
	// according to specs fscommand is a void method, but the abcasm tests seem to expect a result value, so we set the result to undefined
	ret = asAtomHandler::undefinedAtom;
	if(command == "quit")
	{
		if (getWorker() && !getWorker()->isPrimordial) // only allow quit from main worker
			return;
		sys->setShutdownFlag();
	}
}


void System::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("totalMemory","",Class<IFunction>::getFunction(c->getSystemState(),totalMemory),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("disposeXML","",Class<IFunction>::getFunction(c->getSystemState(),disposeXML),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pauseForGCIfCollectionImminent","",Class<IFunction>::getFunction(c->getSystemState(),pauseForGCIfCollectionImminent),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("gc","",Class<IFunction>::getFunction(c->getSystemState(),gc),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(System,totalMemory)
{
	LOG(LOG_NOT_IMPLEMENTED, "System.totalMemory not implemented");
	asAtomHandler::setUInt(ret,sys,1024);
}
ASFUNCTIONBODY_ATOM(System,disposeXML)
{
	_NR<XML> xmlobj;
	ARG_UNPACK_ATOM (xmlobj);
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
	ARG_UNPACK_ATOM (imminence,0.75);
	LOG(LOG_NOT_IMPLEMENTED, "System.pauseForGCIfCollectionImminent not implemented");
}
ASFUNCTIONBODY_ATOM(System,gc)
{
	LOG(LOG_NOT_IMPLEMENTED, "System.gc not implemented");
	asAtomHandler::setUndefined(ret);
}

ASWorker::ASWorker(Class_base* c):
	EventDispatcher(c),loader(_MR(Class<Loader>::getInstanceS(c->getSystemState()))),parser(nullptr),
	giveAppPrivileges(false),started(false),isPrimordial(false),state("new")
{
	subtype = SUBTYPE_WORKER;
}

void ASWorker::finalize()
{
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
	}
	
	loader.reset();
	swf.reset();
}

void ASWorker::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("current","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrent,0,Class<ASWorker>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("getSharedProperty","",Class<IFunction>::getFunction(c->getSystemState(),getSharedProperty,1,Class<ASObject>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isSupported","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrent,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	REGISTER_GETTER_RESULTTYPE(c, isPrimordial,Boolean);
	REGISTER_GETTER_RESULTTYPE(c, state,ASString);
	c->setDeclaredMethodByQName("addEventListener","",Class<IFunction>::getFunction(c->getSystemState(),_addEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createMessageChannel","",Class<IFunction>::getFunction(c->getSystemState(),createMessageChannel,1,Class<MessageChannel>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeEventListener","",Class<IFunction>::getFunction(c->getSystemState(),_removeEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSharedProperty","",Class<IFunction>::getFunction(c->getSystemState(),setSharedProperty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("start","",Class<IFunction>::getFunction(c->getSystemState(),start),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("terminate","",Class<IFunction>::getFunction(c->getSystemState(),terminate,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
}

void ASWorker::execute()
{
	setTLSWorker(this);

	streambuf *sbuf = new bytes_buf(swf->bytes,swf->getLength());
	istream s(sbuf);
	parsemutex.lock();
	parser = new ParseThread(s,getSystemState()->mainClip->applicationDomain,getSystemState()->mainClip->securityDomain,loader.getPtr(),"");
	parsemutex.unlock();
	getSystemState()->addWorker(this);
	this->incRef();
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"workerState")));
	if (!this->threadAborting)
	{
		LOG(LOG_INFO,"start worker"<<this->toDebugString()<<" "<<this->isPrimordial);
		parser->execute();
	}
	parsemutex.lock();
	delete parser;
	parser = nullptr;
	parsemutex.unlock();
	while (!this->threadAborting)
	{
		event_queue_mutex.lock();
		while(events_queue.empty() && !this->threadAborting)
			sem_event_cond.wait(event_queue_mutex);
		if (this->threadAborting)
			break;

		_NR<EventDispatcher> dispatcher=events_queue.front().first;
		_R<Event> e=events_queue.front().second;
		events_queue.pop_front();
	
		event_queue_mutex.unlock();
		try
		{
			if (dispatcher)
				dispatcher->handleEvent(e);
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
	this->incRef();
	getSystemState()->removeWorker(this);
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<Event>::getInstanceS(getSystemState(),"workerState")));
	sem_event_cond.signal();
}

void ASWorker::threadAbort()
{
	sem_event_cond.signal();
}
bool ASWorker::addEvent(_NR<EventDispatcher> obj, _R<Event> ev)
{
	Locker l(event_queue_mutex);
	events_queue.push_back(pair<_NR<EventDispatcher>,_R<Event>>(obj, ev));
	RELEASE_WRITE(ev->queued,true);
	sem_event_cond.signal();
	return true;
}

ASFUNCTIONBODY_GETTER(ASWorker, state);
ASFUNCTIONBODY_GETTER(ASWorker, isPrimordial);

ASFUNCTIONBODY_ATOM(ASWorker,_getCurrent)
{
	ASWorker* w = getWorker();
	if(!w)
		w=sys->worker;
	w->incRef();
	ret = asAtomHandler::fromObject(w);
}
ASFUNCTIONBODY_ATOM(ASWorker,getSharedProperty)
{
	tiny_string key;
	ARG_UNPACK_ATOM(key);
	Locker l(sys->workerDomain->workersharedobjectmutex);
	
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=sys->getUniqueStringId(key);
	m.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
	m.isAttribute = false;
	if (sys->workerDomain->workerSharedObject->hasPropertyByMultiname(m,true,false))
		sys->workerDomain->workerSharedObject->getVariableByMultiname(ret,m);
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
	th->worker = th;
	EventDispatcher::addEventListener(ret,sys,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(ASWorker,createMessageChannel)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	_NR<ASWorker> receiver;
	ARG_UNPACK_ATOM(receiver);
	if (receiver.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,"receiver");
	MessageChannel* channel = Class<MessageChannel>::getInstanceSNoArgs(sys);
	th->incRef();
	channel->sender = _MR(th);
	channel->receiver = receiver;
	ret = asAtomHandler::fromObjectNoPrimitive(channel);
}
ASFUNCTIONBODY_ATOM(ASWorker,_removeEventListener)
{
	EventDispatcher::removeEventListener(ret,sys,obj,args,argslen);
}
ASFUNCTIONBODY_ATOM(ASWorker,setSharedProperty)
{
	tiny_string key;
	asAtom value=asAtomHandler::invalidAtom;
	ARG_UNPACK_ATOM(key)(value);
	Locker l(sys->workerDomain->workersharedobjectmutex);
	ASATOM_INCREF(value);
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.name_s_id=sys->getUniqueStringId(key);
	m.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
	m.isAttribute = false;
	sys->workerDomain->workerSharedObject->setVariableByMultiname(m,value,CONST_NOT_ALLOWED);
}
ASFUNCTIONBODY_ATOM(ASWorker,start)
{
	ASWorker* th = asAtomHandler::as<ASWorker>(obj);
	if (th->started)
		throwError<ASError>(kWorkerAlreadyStarted);
	if (!th->swf.isNull())
	{
		th->started = true;
		sys->addJob(th);
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
	}
}

WorkerDomain::WorkerDomain(Class_base* c):
	ASObject(c)
{
	subtype = SUBTYPE_WORKERDOMAIN;
	asAtom v=asAtomHandler::invalidAtom;
	Template<Vector>::getInstanceS(v,getSystemState(),Class<ASWorker>::getClass(getSystemState()),NullRef);
	workerlist = _R<Vector>(asAtomHandler::as<Vector>(v));
	workerSharedObject = _MR(Class<ASObject>::getInstanceS(getSystemState()));
	workerSharedObject->setRefConstant();
}

void WorkerDomain::finalize()
{
	workerlist.reset();
	workerSharedObject.reset();
}

void WorkerDomain::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("current","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrent),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isSupported","",Class<IFunction>::getFunction(c->getSystemState(),_isSupported),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("createWorker","",Class<IFunction>::getFunction(c->getSystemState(),createWorker),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("listWorkers","",Class<IFunction>::getFunction(c->getSystemState(),listWorkers),NORMAL_METHOD,true);
	if(c->getSystemState()->flashMode==SystemState::AVMPLUS)
	{
		c->setDeclaredMethodByQName("createWorkerFromPrimordial","",Class<IFunction>::getFunction(c->getSystemState(),createWorkerFromPrimordial),NORMAL_METHOD,true);
		c->setDeclaredMethodByQName("createWorkerFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),createWorkerFromByteArray),NORMAL_METHOD,true);
	}

}
ASFUNCTIONBODY_ATOM(WorkerDomain,_constructor)
{
	asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(WorkerDomain,_getCurrent)
{
	ret = asAtomHandler::fromObject(sys->workerDomain);
}
ASFUNCTIONBODY_ATOM(WorkerDomain,_isSupported)
{
	asAtomHandler::setBool(ret,true);
}
ASFUNCTIONBODY_ATOM(WorkerDomain,createWorker)
{
	ASWorker* w = Class<ASWorker>::getInstanceS(sys);
	ARG_UNPACK_ATOM(w->swf)(w->giveAppPrivileges, false);
	if (w->giveAppPrivileges)
		LOG(LOG_NOT_IMPLEMENTED,"WorkerDomain.createWorker: giveAppPrivileges is ignored");
	ret = asAtomHandler::fromObject(w);
}
ASFUNCTIONBODY_ATOM(WorkerDomain,createWorkerFromPrimordial)
{
	ASWorker* w = Class<ASWorker>::getInstanceS(sys);
	
	
	ByteArray* ba = Class<ByteArray>::getInstanceS(sys);
	FileStreamCache* sc = (FileStreamCache*)sys->getEngineData()->createFileStreamCache(sys);
	sc->useExistingFile(sys->getDumpedSWFPath());
	
	ba->append(sc->createReader(),sys->swffilesize);
	w->swf = _MR(ba);
	ret = asAtomHandler::fromObject(w);
}
ASFUNCTIONBODY_ATOM(WorkerDomain,createWorkerFromByteArray)
{
	ASWorker* w = Class<ASWorker>::getInstanceS(sys);
	ARG_UNPACK_ATOM(w->swf);
	ret = asAtomHandler::fromObject(w);
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

