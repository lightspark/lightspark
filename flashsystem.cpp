/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "flashsystem.h"
#include "abc.h"
#include "class.h"

using namespace lightspark;

REGISTER_CLASS_NAME(ApplicationDomain);

Capabilities::Capabilities()
{
	setVariableByQName("playerType","",new ASString("AVMPlus"));
	setVariableByQName("version","",new ASString("UNIX 10,0,0,0"));
}

void ApplicationDomain::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->setGetterByQName("currentDomain","",new Function(_getCurrentDomain));
}

ASFUNCTIONBODY(ApplicationDomain,_constructor)
{
	obj->setVariableByQName("hasDefinition","",new Function(hasDefinition));
	obj->setVariableByQName("getDefinition","",new Function(getDefinition));
}

ASFUNCTIONBODY(ApplicationDomain,_getCurrentDomain)
{
	return Class<ApplicationDomain>::getInstanceS(true)->obj;
}

void ApplicationDomain::stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns)
{
	//Ok, let's split our string into namespace and name part
	for(int i=tmp.len()-1;i>0;i--)
	{
		if(tmp[i]==':')
		{
			assert(tmp[i-1]==':');
			ns=tmp.substr(0,i-1);
			name=tmp.substr(i+1,tmp.len());
			return;
		}
	}
	//No double colons in the string
	name=tmp;
	ns="";
}

ASFUNCTIONBODY(ApplicationDomain,hasDefinition)
{
	assert(args && args->size()==1);
	const tiny_string& tmp=args->at(0)->toString();
	tiny_string name,ns;

	stringToQName(tmp,name,ns);

	ASObject* owner;
	LOG(LOG_CALLS,"Looking for definition of " << ns << " :: " << name);
	objAndLevel o=sys->currentVm->last_context->Global->getVariableByQName(name,ns,owner);
	if(owner==NULL)
		return abstract_b(false);
	else
	{
		//Check if the object has to be defined
		if(o.obj->getObjectType()==T_DEFINABLE)
		{
			LOG(LOG_CALLS,"We got an object not yet valid");
			Definable* d=static_cast<Definable*>(o.obj);
			d->define(sys->currentVm->last_context->Global);
			o=sys->currentVm->last_context->Global->getVariableByQName(name,ns,owner);
		}

		if(o.obj->getObjectType()!=T_CLASS)
			return abstract_b(false);

		LOG(LOG_CALLS,"Found definition for " << ns << " :: " << name);
		return abstract_b(true);
	}
}

ASFUNCTIONBODY(ApplicationDomain,getDefinition)
{
	assert(args && args->size()==1);
	const tiny_string& tmp=args->at(0)->toString();
	tiny_string name,ns;

	stringToQName(tmp,name,ns);

	ASObject* owner;
	LOG(LOG_CALLS,"Looking for definition of " << ns << " :: " << name);
	objAndLevel o=sys->currentVm->last_context->Global->getVariableByQName(name,ns,owner);
	assert(owner);

	//Check if the object has to be defined
	if(o.obj->getObjectType()==T_DEFINABLE)
	{
		abort();
/*		LOG(LOG_CALLS,"We got an object not yet valid");
		Definable* d=static_cast<Definable*>(o.obj);
		d->define(sys->currentVm->last_context->Global);
		o=sys->currentVm->last_context->Global->getVariableByQName(name,ns,owner);*/
	}

	assert(o.obj->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,"Getting definition for " << ns << " :: " << name);
	return o.obj;
}
