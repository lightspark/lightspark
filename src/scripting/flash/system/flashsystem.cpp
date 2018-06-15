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
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "compat.h"
#include "backends/security.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/toplevel/Vector.h"

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
	c->setDeclaredMethodByQName("language","",Class<IFunction>::getFunction(c->getSystemState(),_getLanguage),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("playerType","",Class<IFunction>::getFunction(c->getSystemState(),_getPlayerType),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("version","",Class<IFunction>::getFunction(c->getSystemState(),_getVersion),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("cpuArchitecture","",Class<IFunction>::getFunction(c->getSystemState(),_getCPUArchitecture),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isDebugger","",Class<IFunction>::getFunction(c->getSystemState(),_getIsDebugger),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isEmbeddedInAcrobat","",Class<IFunction>::getFunction(c->getSystemState(),_getIsEmbeddedInAcrobat),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("localFileReadDisable","",Class<IFunction>::getFunction(c->getSystemState(),_getLocalFileReadDisable),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("manufacturer","",Class<IFunction>::getFunction(c->getSystemState(),_getManufacturer),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("os","",Class<IFunction>::getFunction(c->getSystemState(),_getOS),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("serverString","",Class<IFunction>::getFunction(c->getSystemState(),_getServerString),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenResolutionX","",Class<IFunction>::getFunction(c->getSystemState(),_getScreenResolutionX),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenResolutionY","",Class<IFunction>::getFunction(c->getSystemState(),_getScreenResolutionY),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("hasAccessibility","",Class<IFunction>::getFunction(c->getSystemState(),_getHasAccessibility),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenDPI","",Class<IFunction>::getFunction(c->getSystemState(),_getScreenDPI),GETTER_METHOD,false);
	
}

ASFUNCTIONBODY_ATOM(Capabilities,_getPlayerType)
{
	switch (sys->flashMode)
	{
		case SystemState::AVMPLUS:
			ret = asAtom::fromString(sys,"AVMPlus");
			break;
		case SystemState::AIR:
			ret = asAtom::fromString(sys,"Desktop");
			break;
		default:
			ret = asAtom::fromString(sys,"PlugIn");
			break;
	}
}

ASFUNCTIONBODY_ATOM(Capabilities,_getLanguage)
{
	ret = asAtom::fromString(sys,"en");
}

ASFUNCTIONBODY_ATOM(Capabilities,_getCPUArchitecture)
{
	LOG(LOG_NOT_IMPLEMENTED, "Capabilities.cpuArchitecture is not implemented");
	ret = asAtom::fromString(sys,"x86");
}

ASFUNCTIONBODY_ATOM(Capabilities,_getIsDebugger)
{
	ret.setBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getIsEmbeddedInAcrobat)
{
	ret.setBool(false);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getLocalFileReadDisable)
{
	ret.setBool(true);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getManufacturer)
{
	ret = asAtom::fromString(sys,MANUFACTURER);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getOS)
{
#ifdef _WIN32
	ret = asAtom::fromString(sys,"Windows");
#else
	ret = asAtom::fromString(sys,"Linux");
#endif
}

ASFUNCTIONBODY_ATOM(Capabilities,_getVersion)
{
	ret = asAtom::fromString(sys,EMULATED_VERSION);
}

ASFUNCTIONBODY_ATOM(Capabilities,_getServerString)
{
	LOG(LOG_NOT_IMPLEMENTED,"Capabilities: not all capabilities are reported in ServerString");
	tiny_string res = "A=t&SA=t&SV=t&MP3=t&OS=Linux&PT=PlugIn&L=en&TLS=t&DD=t";
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
	ret = asAtom::fromString(sys,res);
}
ASFUNCTIONBODY_ATOM(Capabilities,_getScreenResolutionX)
{
	SDL_DisplayMode screen;
	if (!sys->getEngineData()->getScreenData(&screen))
		ret.setInt(0);
	else
		ret.setInt(screen.w);
}
ASFUNCTIONBODY_ATOM(Capabilities,_getScreenResolutionY)
{
	SDL_DisplayMode screen;
	if (!sys->getEngineData()->getScreenData(&screen))
		ret.setInt(0);
	else
		ret.setInt(screen.h);
}
ASFUNCTIONBODY_ATOM(Capabilities,_getHasAccessibility)
{
	LOG(LOG_NOT_IMPLEMENTED,"hasAccessibility always returns false");
	ret.setBool(false);
}
ASFUNCTIONBODY_ATOM(Capabilities,_getScreenDPI)
{
	number_t dpi = sys->getEngineData()->getScreenDPI();
	ret.setNumber(dpi);
}

ApplicationDomain::ApplicationDomain(Class_base* c, _NR<ApplicationDomain> p):ASObject(c,T_OBJECT,SUBTYPE_APPLICATIONDOMAIN),parentDomain(p)
{
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

ASFUNCTIONBODY_GETTER_SETTER(ApplicationDomain,domainMemory);
ASFUNCTIONBODY_GETTER(ApplicationDomain,parentDomain);

void ApplicationDomain::buildTraits(ASObject* o)
{
}

void ApplicationDomain::finalize()
{
	ASObject::finalize();
	domainMemory.reset();
	for(auto i = globalScopes.begin(); i != globalScopes.end(); ++i)
		(*i)->decRef();
	for(auto it = instantiatedTemplates.begin(); it != instantiatedTemplates.end(); ++it)
		it->second->finalize();
	//Free template instantations by decRef'ing them
	for(auto i = instantiatedTemplates.begin(); i != instantiatedTemplates.end(); ++i)
		i->second->decRef();
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_constructor)
{
	ApplicationDomain* th = obj.as<ApplicationDomain>();
	_NR<ApplicationDomain> parentDomain;
	ARG_UNPACK_ATOM (parentDomain, NullRef);
	if(!th->parentDomain.isNull())
		// Don't override parentDomain if it was set in the
		// C++ constructor
		return;
	else if(parentDomain.isNull())
		th->parentDomain =  sys->systemDomain;
	else
		th->parentDomain = parentDomain;
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_getMinDomainMemoryLength)
{
	ret.setUInt((uint32_t)MIN_DOMAIN_MEMORY_LIMIT);
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,_getCurrentDomain)
{
	_NR<ApplicationDomain> res=ABCVm::getCurrentApplicationDomain(getVm(sys)->currentCallContext);
	res->incRef();
	ret = asAtom::fromObject(res.getPtr());
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,hasDefinition)
{
	ApplicationDomain* th = obj.as<ApplicationDomain>();
	assert(argslen==1);
	const tiny_string& tmp=args[0].toString(sys);

	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=sys->getUniqueStringId(tmpName);
	if (nsName != "")
		name.ns.push_back(nsNameAndKind(sys,nsName,NAMESPACE));

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ASObject* target;
	asAtom o;
	th->getVariableAndTargetByMultinameIncludeTemplatedClasses(o,name,target);
	if(o.type == T_INVALID)
		ret.setBool(false);
	else
	{
		if(o.type!=T_CLASS)
			ret.setBool(false);
		else
		{
			LOG(LOG_CALLS,_("Found definition for ") << name);
			ret.setBool(true);
		}
	}
}

ASFUNCTIONBODY_ATOM(ApplicationDomain,getDefinition)
{
	ApplicationDomain* th = obj.as<ApplicationDomain>();
	assert(argslen==1);
	const tiny_string& tmp=args[0].toString(sys);

	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=sys->getUniqueStringId(tmpName);
	if (nsName != "")
		name.ns.push_back(nsNameAndKind(sys,nsName,NAMESPACE));

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ASObject* target;
	th->getVariableAndTargetByMultinameIncludeTemplatedClasses(ret,name,target);
	if(ret.type == T_INVALID)
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
	multiname name(NULL);
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
	asAtom ret;
	getVariableAndTargetByMultiname(ret,name, target);
	return ret.toObject(getSystemState());
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

void ApplicationDomain::getVariableAndTargetByMultiname(asAtom& ret, const multiname& name, ASObject*& target)
{
	//Check in the parent first
	if(!parentDomain.isNull())
	{
		parentDomain->getVariableAndTargetByMultiname(ret,name, target);
		if(ret.type != T_INVALID)
			return;
	}

	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		globalScopes[i]->getVariableByMultiname(ret,name);
		if(ret.type != T_INVALID)
		{
			target=globalScopes[i];
			// No incRef, return a reference borrowed from globalScopes
			return;
		}
	}
}
void ApplicationDomain::getVariableAndTargetByMultinameIncludeTemplatedClasses(asAtom& ret, const multiname& name, ASObject*& target)
{
	getVariableAndTargetByMultiname(ret,name, target);
	if (ret.type != T_INVALID)
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
			asAtom typeobj;
			getVariableAndTargetByMultiname(typeobj,tn, tntarget);
			if (typeobj.type != T_INVALID)
			{
				const Type* t = typeobj.getObject()->as<Type>();
				this->incRef();
				ret = asAtom::fromObject(Template<Vector>::getTemplateInstance(getSystemState(),t,_NR<ApplicationDomain>(this)).getPtr());
			}
		}
	}
}
ASObject* ApplicationDomain::getVariableByMultinameOpportunistic(const multiname& name)
{
	//Check in the parent first
	if(!parentDomain.isNull())
	{
		ASObject* ret=parentDomain->getVariableByMultinameOpportunistic(name);
		if(ret)
			return ret;
	}

	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		asAtom o;
		globalScopes[i]->getVariableByMultinameOpportunistic(o,name);
		if(o.type != T_INVALID)
		{
			// No incRef, return a reference borrowed from globalScopes
			return o.toObject(getSystemState());
		}
	}
	return NULL;
}

void ApplicationDomain::checkDomainMemory()
{
	if(domainMemory.isNull())
	{
		domainMemory = _NR<ByteArray>(Class<ByteArray>::getInstanceS(this->getSystemState()));
		domainMemory->setLength(MIN_DOMAIN_MEMORY_LIMIT);
	}
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
	LoaderContext* th=obj.as<LoaderContext>();
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
	ret = asAtom::fromObject(res.getPtr());
}

void Security::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	//Fully static class
	c->setDeclaredMethodByQName("exactSettings","",Class<IFunction>::getFunction(c->getSystemState(),_getExactSettings),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("exactSettings","",Class<IFunction>::getFunction(c->getSystemState(),_setExactSettings),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("sandboxType","",Class<IFunction>::getFunction(c->getSystemState(),_getSandboxType),GETTER_METHOD,false);
	c->setVariableAtomByQName("LOCAL_TRUSTED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_TRUSTED)),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOCAL_WITH_FILE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_FILE)),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOCAL_WITH_NETWORK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_NETWORK)),DECLARED_TRAIT);
	c->setVariableAtomByQName("REMOTE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),c->getSystemState()->securityManager->getSandboxName(SecurityManager::REMOTE)),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("allowDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowDomain),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("allowInsecureDomain","",Class<IFunction>::getFunction(c->getSystemState(),allowInsecureDomain),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("loadPolicyFile","",Class<IFunction>::getFunction(c->getSystemState(),loadPolicyFile),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("showSettings","",Class<IFunction>::getFunction(c->getSystemState(),showSettings),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("pageDomain","",Class<IFunction>::getFunction(c->getSystemState(),pageDomain),GETTER_METHOD,false);

	c->getSystemState()->securityManager->setExactSettings(true, false);
}

ASFUNCTIONBODY_ATOM(Security,_getExactSettings)
{
	ret.setBool(sys->securityManager->getExactSettings());
}

ASFUNCTIONBODY_ATOM(Security,_setExactSettings)
{
	assert(args && argslen==1);
	if(sys->securityManager->getExactSettingsLocked())
	{
		throw Class<SecurityError>::getInstanceS(sys,"SecurityError: Security.exactSettings already set");
	}
	sys->securityManager->setExactSettings(args[0].Boolean_concrete());
}

ASFUNCTIONBODY_ATOM(Security,_getSandboxType)
{
	if(sys->securityManager->getSandboxType() == SecurityManager::REMOTE)
		ret = asAtom::fromString(sys,sys->securityManager->getSandboxName(SecurityManager::REMOTE));
	else if(sys->securityManager->getSandboxType() == SecurityManager::LOCAL_TRUSTED)
		ret = asAtom::fromString(sys,sys->securityManager->getSandboxName(SecurityManager::LOCAL_TRUSTED));
	else if(sys->securityManager->getSandboxType() == SecurityManager::LOCAL_WITH_FILE)
		ret = asAtom::fromString(sys,sys->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_FILE));
	else if(sys->securityManager->getSandboxType() == SecurityManager::LOCAL_WITH_NETWORK)
		ret = asAtom::fromString(sys,sys->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_NETWORK));
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
	tiny_string url = args[0].toString(sys);
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
	ret = asAtom::fromString(sys,s);
}

ASFUNCTIONBODY_ATOM(lightspark, fscommand)
{
	assert_and_throw(argslen >= 1 && argslen <= 2);
	assert_and_throw(args[0].type == T_STRING);
	tiny_string command = args[0].toString(sys);
	if(command == "quit")
	{
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
	ret.setUInt(1024);
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
	ret.setUndefined();
}

ASWorker::ASWorker(Class_base* c):
	EventDispatcher(c)
{
	LOG(LOG_NOT_IMPLEMENTED, "Worker not implemented");
}

void ASWorker::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("current","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrent),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("getSharedProperty","",Class<IFunction>::getFunction(c->getSystemState(),getSharedProperty),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(ASWorker,_getCurrent)
{
	LOG(LOG_NOT_IMPLEMENTED, "Worker not implemented");
	ret = asAtom::fromObject(Class<ASObject>::getInstanceS(sys));
}
ASFUNCTIONBODY_ATOM(ASWorker,getSharedProperty)
{
	LOG(LOG_NOT_IMPLEMENTED, "Worker.getSharedProperty not implemented");
	ret = asAtom::fromObject(Class<ASObject>::getInstanceS(sys));
}

void ImageDecodingPolicy::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ON_DEMAND",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"onDemand"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ON_LOAD",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"onLoad"),CONSTANT_TRAIT);
}

void IMEConversionMode::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("ALPHANUMERIC_FULL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ALPHANUMERIC_FULL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ALPHANUMERIC_HALF",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ALPHANUMERIC_HALF"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CHINESE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"CHINESE"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JAPANESE_HIRAGANA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"JAPANESE_HIRAGANA"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JAPANESE_KATAKANA_FULL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"JAPANESE_KATAKANA_FULL"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("JAPANESE_KATAKANA_HALF",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"JAPANESE_KATAKANA_HALF"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("KOREAN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"KOREAN"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNKNOWN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"UNKNOWN"),CONSTANT_TRAIT);
}

