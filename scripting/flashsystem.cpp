/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "flashsystem.h"
#include "abc.h"
#include "class.h"
#include "compat.h"

using namespace lightspark;

SET_NAMESPACE("flash.system");

REGISTER_CLASS_NAME(ApplicationDomain);
REGISTER_CLASS_NAME(Capabilities);
REGISTER_CLASS_NAME(Security);

void Capabilities::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setGetterByQName("language","",Class<IFunction>::getFunction(_getLanguage),true);
	c->setVariableByQName("version","",Class<ASString>::getInstanceS("UNIX 10,0,0,0"));
}

ASFUNCTIONBODY(Capabilities,_constructor)
{
	obj->setVariableByQName("playerType","",Class<ASString>::getInstanceS("AVMPlus"));
	return NULL;
}

ASFUNCTIONBODY(Capabilities,_getLanguage)
{
	return Class<ASString>::getInstanceS("en");
}

void ApplicationDomain::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	//Static
	c->setGetterByQName("currentDomain","",Class<IFunction>::getFunction(_getCurrentDomain),false);
	//Instance
	c->setMethodByQName("hasDefinition","",Class<IFunction>::getFunction(hasDefinition),true);
	c->setMethodByQName("getDefinition","",Class<IFunction>::getFunction(getDefinition),true);
}

void ApplicationDomain::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(ApplicationDomain,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(ApplicationDomain,_getCurrentDomain)
{
	return Class<ApplicationDomain>::getInstanceS();
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
		//Check if the object has to be defined
		if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(LOG_CALLS,_("We got an object not yet valid"));
			Definable* d=static_cast<Definable*>(o);
			d->define(target);
			o=target->getVariableByMultiname(name);
		}

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

	//Check if the object has to be defined
	if(o->getObjectType()==T_DEFINABLE)
	{
		throw UnsupportedException("Defininable in ApplicationDomain::getDefinition");
/*		LOG(LOG_CALLS,_("We got an object not yet valid"));
		Definable* d=static_cast<Definable*>(o.obj);
		d->define(sys->currentVm->last_context->Global);
		o=sys->currentVm->last_context->Global->getVariableByQName(name,ns,owner);*/
	}

	assert(o->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,_("Getting definition for ") << name);
	return o;
}

void Security::sinit(Class_base* c)
{
	//Fully static class
	c->setConstructor(NULL);
	c->setGetterByQName("exactSettings","",Class<IFunction>::getFunction(_getExactSettings),false);
	c->setSetterByQName("exactSettings","",Class<IFunction>::getFunction(_setExactSettings),false);
	c->setGetterByQName("sandboxType","",Class<IFunction>::getFunction(_getSandboxType),false);
	c->setVariableByQName("LOCAL_TRUSTED","",Class<ASString>::getInstanceS("localTrusted"));
	c->setVariableByQName("LOCAL_WITH_FILE","",Class<ASString>::getInstanceS("localWithFile"));
	c->setVariableByQName("LOCAL_WITH_NETWORK","",Class<ASString>::getInstanceS("localWithNetwork"));
	c->setVariableByQName("REMOTE","",Class<ASString>::getInstanceS("remote"));
	c->setMethodByQName("allowDomain","",Class<IFunction>::getFunction(allowDomain),false);
	c->setMethodByQName("allowInsecureDomain","",Class<IFunction>::getFunction(allowInsecureDomain),false);
	c->setMethodByQName("loadPolicyFile","",Class<IFunction>::getFunction(loadPolicyFile),false);
	c->setMethodByQName("showSettings","",Class<IFunction>::getFunction(showSettings),false);

	sys->staticSecurityExactSettings = true;
	sys->staticSecurityExactSettingsLocked = false;
}

ASFUNCTIONBODY(Security,_getExactSettings)
{
	return abstract_b(sys->staticSecurityExactSettings);
}

ASFUNCTIONBODY(Security,_setExactSettings)
{
	assert(args && argslen==1);
	if(sys->staticSecurityExactSettingsLocked)
	{
		throw UnsupportedException("SecurityError");
	}
	if(args[0]->getObjectType() != T_BOOLEAN)
	{
		throw UnsupportedException("ArgumentError");
	}
	//Boolean* i = static_cast<Boolean*>(args[0]);
	sys->staticSecurityExactSettings = Boolean_concrete(args[0]);
	return NULL;
}

ASFUNCTIONBODY(Security,_getSandboxType)
{
	if(sys->sandboxType == REMOTE)
		return Class<ASString>::getInstanceS("remote");
	else if(sys->sandboxType == LOCAL_TRUSTED)
		return Class<ASString>::getInstanceS("localTrusted");
	else if(sys->sandboxType == LOCAL_WITH_FILE)
		return Class<ASString>::getInstanceS("localWithFile");
	else if(sys->sandboxType == LOCAL_WITH_NETWORK)
		return Class<ASString>::getInstanceS("localWithNetwork");
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
	LOG(LOG_NOT_IMPLEMENTED, _("Security::loadPolicyFile"));
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
		sys->setShutdownFlag();
	}
	return NULL;
}
