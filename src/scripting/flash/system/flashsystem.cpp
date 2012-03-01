/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "flashsystem.h"
#include "abc.h"
#include "argconv.h"
#include "compat.h"
#include "backends/security.h"

#include <istream>

using namespace lightspark;

SET_NAMESPACE("flash.system");

REGISTER_CLASS_NAME(ApplicationDomain);
REGISTER_CLASS_NAME(SecurityDomain);
REGISTER_CLASS_NAME(Capabilities);
REGISTER_CLASS_NAME(Security);

#ifdef _WIN32
const char* Capabilities::EMULATED_VERSION = "WIN 11,1,0,"SHORTVERSION;
#else
const char* Capabilities::EMULATED_VERSION = "LNX 11,1,0,"SHORTVERSION;
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
	c->setDeclaredMethodByQName("os","",Class<IFunction>::getFunction(_getOS),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("serverString","",Class<IFunction>::getFunction(_getServerString),GETTER_METHOD,false);
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

void ApplicationDomain::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	//Static
	c->setDeclaredMethodByQName("currentDomain","",Class<IFunction>::getFunction(_getCurrentDomain),GETTER_METHOD,false);
	//Instance
	c->setDeclaredMethodByQName("hasDefinition","",Class<IFunction>::getFunction(hasDefinition),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDefinition","",Class<IFunction>::getFunction(getDefinition),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c,domainMemory);
}

ASFUNCTIONBODY_GETTER_SETTER(ApplicationDomain,domainMemory);

void ApplicationDomain::buildTraits(ASObject* o)
{
}

void ApplicationDomain::finalize()
{
	ASObject::finalize();
	domainMemory.reset();
}

ASFUNCTIONBODY(ApplicationDomain,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(ApplicationDomain,_getCurrentDomain)
{
	ApplicationDomain* ret=getSys()->mainApplicationDomain.getPtr();
	ret->incRef();
	return ret;
}

ASFUNCTIONBODY(ApplicationDomain,hasDefinition)
{
	assert(args && argslen==1);
	const tiny_string& tmp=args[0]->toString();

	multiname name;
	name.name_type=multiname::NAME_STRING;
	name.ns.push_back(nsNameAndKind("",NAMESPACE)); //TODO: set type

	stringToQName(tmp,name.name_s,name.ns[0].name);

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ASObject* target;
	ASObject* o=getGlobal()->getVariableAndTargetByMultiname(name,target);
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
	assert(args && argslen==1);
	const tiny_string& tmp=args[0]->toString();

	multiname name;
	name.name_type=multiname::NAME_STRING;
	name.ns.push_back(nsNameAndKind("",NAMESPACE)); //TODO: set type

	stringToQName(tmp,name.name_s,name.ns[0].name);

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ASObject* target;
	ASObject* o=getGlobal()->getVariableAndTargetByMultiname(name,target);
	assert_and_throw(o);

	//TODO: specs says that also namespaces and function may be returned
	assert_and_throw(o->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,_("Getting definition for ") << name);
	o->incRef();
	return o;
}

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
	return Class<SecurityDomain>::getInstanceS();
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
	LOG(LOG_INFO, "Loading policy file: " << getSys()->getOrigin().goToURL(args[0]->toString()));
	getSys()->securityManager->addPolicyFile(getSys()->getOrigin().goToURL(args[0]->toString()));
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
