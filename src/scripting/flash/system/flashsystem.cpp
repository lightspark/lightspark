/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <libxml++/libxml++.h>
#include <libxml++/parsers/textreader.h>

#include "version.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "compat.h"
#include "backends/security.h"

#include <istream>
#include <gdk/gdk.h>

using namespace lightspark;

#ifdef _WIN32
const char* Capabilities::EMULATED_VERSION = "WIN 11,1,0," SHORTVERSION;
const char* Capabilities::MANUFACTURER = "Adobe Windows";
#else
const char* Capabilities::EMULATED_VERSION = "LNX 11,1,0," SHORTVERSION;
const char* Capabilities::MANUFACTURER = "Adobe Linux";
#endif


void Capabilities::sinit(Class_base* c)
{
	c->setDeclaredMethodByQName("language","",Class<IFunction>::getFunction(_getLanguage),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("playerType","",Class<IFunction>::getFunction(_getPlayerType),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("version","",Class<IFunction>::getFunction(_getVersion),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("cpuArchitecture","",Class<IFunction>::getFunction(_getCPUArchitecture),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isDebugger","",Class<IFunction>::getFunction(_getIsDebugger),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("isEmbeddedInAcrobat","",Class<IFunction>::getFunction(_getIsEmbeddedInAcrobat),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("localFileReadDisable","",Class<IFunction>::getFunction(_getLocalFileReadDisable),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("manufacturer","",Class<IFunction>::getFunction(_getManufacturer),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("os","",Class<IFunction>::getFunction(_getOS),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("serverString","",Class<IFunction>::getFunction(_getServerString),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenResolutionX","",Class<IFunction>::getFunction(_getScreenResolutionX),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("screenResolutionY","",Class<IFunction>::getFunction(_getScreenResolutionY),GETTER_METHOD,false);
}

ASFUNCTIONBODY(Capabilities,_getPlayerType)
{
	return Class<ASString>::getInstanceS("PlugIn");
}

ASFUNCTIONBODY(Capabilities,_getLanguage)
{
	return Class<ASString>::getInstanceS("en");
}

ASFUNCTIONBODY(Capabilities,_getCPUArchitecture)
{
	LOG(LOG_NOT_IMPLEMENTED, "Capabilities.cpuArchitecture is not implemented");
	return Class<ASString>::getInstanceS("x86");
}

ASFUNCTIONBODY(Capabilities,_getIsDebugger)
{
	return abstract_b(false);
}

ASFUNCTIONBODY(Capabilities,_getIsEmbeddedInAcrobat)
{
	return abstract_b(false);
}

ASFUNCTIONBODY(Capabilities,_getLocalFileReadDisable)
{
	return abstract_b(true);
}

ASFUNCTIONBODY(Capabilities,_getManufacturer)
{
	return Class<ASString>::getInstanceS(MANUFACTURER);
}

ASFUNCTIONBODY(Capabilities,_getOS)
{
	return Class<ASString>::getInstanceS("Linux");
}

ASFUNCTIONBODY(Capabilities,_getVersion)
{
	return Class<ASString>::getInstanceS(EMULATED_VERSION);
}

ASFUNCTIONBODY(Capabilities,_getServerString)
{
	LOG(LOG_NOT_IMPLEMENTED, "Capabilities.serverString is not implemented");
	return Class<ASString>::getInstanceS("");
}
ASFUNCTIONBODY(Capabilities,_getScreenResolutionX)
{
	GdkScreen*  screen = gdk_screen_get_default();
	gint width = gdk_screen_get_width (screen);
	return abstract_d(width);
}
ASFUNCTIONBODY(Capabilities,_getScreenResolutionY)
{
	GdkScreen*  screen = gdk_screen_get_default();
	gint height = gdk_screen_get_height (screen);
	return abstract_d(height);
}

ApplicationDomain::ApplicationDomain(Class_base* c, _NR<ApplicationDomain> p):ASObject(c),parentDomain(p)
{
}

void ApplicationDomain::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	//Static
	c->setDeclaredMethodByQName("currentDomain","",Class<IFunction>::getFunction(_getCurrentDomain),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("MIN_DOMAIN_MEMORY_LENGTH","",Class<IFunction>::getFunction(_getMinDomainMemoryLength),GETTER_METHOD,false);
	//Instance
	c->setDeclaredMethodByQName("hasDefinition","",Class<IFunction>::getFunction(hasDefinition),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDefinition","",Class<IFunction>::getFunction(getDefinition),NORMAL_METHOD,true);
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
}

ASFUNCTIONBODY(ApplicationDomain,_constructor)
{
	ApplicationDomain* th = Class<ApplicationDomain>::cast(obj);
	_NR<ApplicationDomain> parentDomain;
	ARG_UNPACK (parentDomain, NullRef);
	if(!th->parentDomain.isNull())
		// Don't override parentDomain if it was set in the
		// C++ constructor
		return NULL;
	else if(parentDomain.isNull())
		th->parentDomain =  getSys()->systemDomain;
	else
		th->parentDomain = parentDomain;
	return NULL;
}

ASFUNCTIONBODY(ApplicationDomain,_getMinDomainMemoryLength)
{
	return abstract_ui(1024);
}

ASFUNCTIONBODY(ApplicationDomain,_getCurrentDomain)
{
	_NR<ApplicationDomain> ret=ABCVm::getCurrentApplicationDomain(getVm()->currentCallContext);
	ret->incRef();
	return ret.getPtr();
}

ASFUNCTIONBODY(ApplicationDomain,hasDefinition)
{
	ApplicationDomain* th = obj->as<ApplicationDomain>();
	assert(argslen==1);
	const tiny_string& tmp=args[0]->toString();

	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=getSys()->getUniqueStringId(tmpName);
	name.ns.push_back(nsNameAndKind(nsName,NAMESPACE));

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ASObject* target;
	ASObject* o=th->getVariableAndTargetByMultiname(name,target);
	if(o==NULL)
		return abstract_b(false);
	else
	{
		if(o->getObjectType()!=T_CLASS)
			return abstract_b(false);

		LOG(LOG_CALLS,_("Found definition for ") << name);
		return abstract_b(true);
	}
}

ASFUNCTIONBODY(ApplicationDomain,getDefinition)
{
	ApplicationDomain* th = obj->as<ApplicationDomain>();
	assert(argslen==1);
	const tiny_string& tmp=args[0]->toString();

	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=getSys()->getUniqueStringId(tmpName);
	name.ns.push_back(nsNameAndKind(nsName,NAMESPACE));

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ASObject* target;
	ASObject* o=th->getVariableAndTargetByMultiname(name,target);
	assert_and_throw(o);

	//TODO: specs says that also namespaces and function may be returned
	assert_and_throw(o->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,_("Getting definition for ") << name);
	o->incRef();
	return o;
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
		name.name_s_id=getSys()->getUniqueStringId(str);
		name.ns.push_back(nsNameAndKind("",NAMESPACE)); //TODO: use ns kind
	}
	else
	{
		name.name_s_id=getSys()->getUniqueStringId(str.substr(index+1));
		name.ns.push_back(nsNameAndKind(str.substr(0,index),NAMESPACE));
	}
	return getVariableAndTargetByMultiname(name, target);
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

ASObject* ApplicationDomain::getVariableAndTargetByMultiname(const multiname& name, ASObject*& target)
{
	//Check in the parent first
	if(!parentDomain.isNull())
	{
		ASObject* ret=parentDomain->getVariableAndTargetByMultiname(name, target);
		if(ret)
			return ret;
	}

	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		_NR<ASObject> o=globalScopes[i]->getVariableByMultiname(name);
		if(!o.isNull())
		{
			target=globalScopes[i];
			// No incRef, return a reference borrowed from globalScopes
			return o.getPtr();
		}
	}
	return NULL;
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
		_NR<ASObject> o=globalScopes[i]->getVariableByMultinameOpportunistic(name);
		if(!o.isNull())
		{
			// No incRef, return a reference borrowed from globalScopes
			return o.getPtr();
		}
	}
	return NULL;
}

void LoaderContext::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	REGISTER_GETTER_SETTER(c, applicationDomain);
	REGISTER_GETTER_SETTER(c, securityDomain);
}

void LoaderContext::finalize()
{
	ASObject::finalize();
	applicationDomain.reset();
	securityDomain.reset();
}

ASFUNCTIONBODY(LoaderContext,_constructor)
{
	LoaderContext* th=Class<LoaderContext>::cast(obj);
	bool checkPolicy;
	_NR<ApplicationDomain> appDomain;
	_NR<SecurityDomain> secDomain;
	ARG_UNPACK (checkPolicy, false) (appDomain, NullRef) (secDomain, NullRef);
	//TODO: Support checkPolicyFile
	th->applicationDomain=appDomain;
	th->securityDomain=secDomain;
	return NULL;
}

ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, applicationDomain);
ASFUNCTIONBODY_GETTER_SETTER(LoaderContext, securityDomain);

void SecurityDomain::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	//Static
	c->setDeclaredMethodByQName("currentDomain","",Class<IFunction>::getFunction(_getCurrentDomain),GETTER_METHOD,false);
}

void SecurityDomain::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(SecurityDomain,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(SecurityDomain,_getCurrentDomain)
{
	_NR<SecurityDomain> ret=ABCVm::getCurrentSecurityDomain(getVm()->currentCallContext);
	ret->incRef();
	return ret.getPtr();
}

void Security::sinit(Class_base* c)
{
	//Fully static class
	c->setConstructor(NULL);
	c->setDeclaredMethodByQName("exactSettings","",Class<IFunction>::getFunction(_getExactSettings),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("exactSettings","",Class<IFunction>::getFunction(_setExactSettings),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("sandboxType","",Class<IFunction>::getFunction(_getSandboxType),GETTER_METHOD,false);
	c->setVariableByQName("LOCAL_TRUSTED","",
			Class<ASString>::getInstanceS(getSys()->securityManager->getSandboxName(SecurityManager::LOCAL_TRUSTED)),DECLARED_TRAIT);
	c->setVariableByQName("LOCAL_WITH_FILE","",
			Class<ASString>::getInstanceS(getSys()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_FILE)),DECLARED_TRAIT);
	c->setVariableByQName("LOCAL_WITH_NETWORK","",
			Class<ASString>::getInstanceS(getSys()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_NETWORK)),DECLARED_TRAIT);
	c->setVariableByQName("REMOTE","",
			Class<ASString>::getInstanceS(getSys()->securityManager->getSandboxName(SecurityManager::REMOTE)),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("allowDomain","",Class<IFunction>::getFunction(allowDomain),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("allowInsecureDomain","",Class<IFunction>::getFunction(allowInsecureDomain),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("loadPolicyFile","",Class<IFunction>::getFunction(loadPolicyFile),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("showSettings","",Class<IFunction>::getFunction(showSettings),NORMAL_METHOD,false);

	getSys()->securityManager->setExactSettings(true, false);
}

ASFUNCTIONBODY(Security,_getExactSettings)
{
	return abstract_b(getSys()->securityManager->getExactSettings());
}

ASFUNCTIONBODY(Security,_setExactSettings)
{
	assert(args && argslen==1);
	if(getSys()->securityManager->getExactSettingsLocked())
	{
		throw Class<SecurityError>::getInstanceS("SecurityError: Security.exactSettings already set");
	}
	getSys()->securityManager->setExactSettings(Boolean_concrete(args[0]));
	return NULL;
}

ASFUNCTIONBODY(Security,_getSandboxType)
{
	if(getSys()->securityManager->getSandboxType() == SecurityManager::REMOTE)
		return Class<ASString>::getInstanceS(getSys()->securityManager->getSandboxName(SecurityManager::REMOTE));
	else if(getSys()->securityManager->getSandboxType() == SecurityManager::LOCAL_TRUSTED)
		return Class<ASString>::getInstanceS(getSys()->securityManager->getSandboxName(SecurityManager::LOCAL_TRUSTED));
	else if(getSys()->securityManager->getSandboxType() == SecurityManager::LOCAL_WITH_FILE)
		return Class<ASString>::getInstanceS(getSys()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_FILE));
	else if(getSys()->securityManager->getSandboxType() == SecurityManager::LOCAL_WITH_NETWORK)
		return Class<ASString>::getInstanceS(getSys()->securityManager->getSandboxName(SecurityManager::LOCAL_WITH_NETWORK));
	assert(false);
	return NULL;
}

ASFUNCTIONBODY(Security, allowDomain)
{
	LOG(LOG_NOT_IMPLEMENTED, _("Security::allowDomain"));
	return NULL;
}

ASFUNCTIONBODY(Security, allowInsecureDomain)
{
	LOG(LOG_NOT_IMPLEMENTED, _("Security::allowInsecureDomain"));
	return NULL;
}

ASFUNCTIONBODY(Security, loadPolicyFile)
{
	LOG(LOG_INFO, "Loading policy file: " << getSys()->mainClip->getOrigin().goToURL(args[0]->toString()));
	getSys()->securityManager->addPolicyFile(getSys()->mainClip->getOrigin().goToURL(args[0]->toString()));
	assert_and_throw(argslen == 1);
	return NULL;
}

ASFUNCTIONBODY(Security, showSettings)
{
	LOG(LOG_NOT_IMPLEMENTED, _("Security::showSettings"));
	return NULL;
}

ASFUNCTIONBODY(lightspark, fscommand)
{
	assert_and_throw(argslen >= 1 && argslen <= 2);
	assert_and_throw(args[0]->getObjectType() == T_STRING);
	tiny_string command = Class<ASString>::cast(args[0])->toString();
	if(command == "quit")
	{
		getSys()->setShutdownFlag();
	}
	return NULL;
}


void System::sinit(Class_base* c)
{
	c->setDeclaredMethodByQName("totalMemory","",Class<IFunction>::getFunction(totalMemory),GETTER_METHOD,false);
}


ASFUNCTIONBODY(System,totalMemory)
{
	LOG(LOG_NOT_IMPLEMENTED, "System.totalMemory not implemented");
	return abstract_d(0);
}
