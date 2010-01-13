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

#include "abc.h"
#include "flashutils.h"
#include "asobjects.h"
#include "class.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

REGISTER_CLASS_NAME(ByteArray);
REGISTER_CLASS_NAME(Timer);

ByteArray::ByteArray():bytes(NULL),len(0)
{
}

void ByteArray::sinit(Class_base* c)
{
}

void Timer::execute()
{
	while(running)
	{
		compat_msleep(delay);
		if(running)
			sys->currentVm->addEvent(this,Class<TimerEvent>::getInstanceS(false,"timer"));
	}
}

void Timer::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

ASFUNCTIONBODY(Timer,_constructor)
{
	EventDispatcher::_constructor(obj,NULL);
	Timer* th=static_cast<Timer*>(obj->implementation);
	obj->setVariableByQName("start","",new Function(start));
	obj->setVariableByQName("reset","",new Function(reset));

	assert(args->size()==1);

	//Set only the delay for now
	th->delay=args->at(0)->toInt();
}

ASFUNCTIONBODY(Timer,start)
{
	Timer* th=static_cast<Timer*>(obj->implementation);
	th->running=true;
	sys->cur_thread_pool->addJob(th);
}

ASFUNCTIONBODY(Timer,reset)
{
	Timer* th=static_cast<Timer*>(obj->implementation);
	th->running=false;
}

ASObject* lightspark::getQualifiedClassName(ASObject* obj, arguments* args)
{
	assert(args->at(0)->prototype);
	cout << args->at(0)->prototype->class_name << endl;
	return new ASString(args->at(0)->prototype->class_name);
}

ASFUNCTIONBODY(lightspark,getDefinitionByName)
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
		LOG(LOG_CALLS,"We got an object not yet valid");
		Definable* d=static_cast<Definable*>(o.obj);
		d->define(sys->currentVm->last_context->Global);
		o=sys->currentVm->last_context->Global->getVariableByQName(name,ns,owner);
	}

	assert(o.obj->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,"Getting definition for " << ns << " :: " << name);
	return o.obj;
}

ASFUNCTIONBODY(lightspark,getTimer)
{
	uint64_t ret=compat_msectiming() - sys->startTime;
	return abstract_i(ret);
}
