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

#include "scripting/abc.h"
#include "asobject.h"
#include "scripting/class.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/Timer.h"

using namespace std;
using namespace lightspark;

void Timer::tick()
{
	//This will be executed once if repeatCount was originally 1
	//Otherwise it's executed until stopMe is set to true
	this->incRef();
	getVm()->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS("timer")));

	currentCount++;
	if(repeatCount!=0)
	{
		if(currentCount==repeatCount)
		{
			this->incRef();
			getVm()->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS("timerComplete")));
			stopMe=true;
			running=false;
		}
	}
}

void Timer::tickFence()
{
	tickJobInstance = NullRef;
}


void Timer::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("currentCount","",Class<IFunction>::getFunction(_getCurrentCount),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",Class<IFunction>::getFunction(_getRepeatCount),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",Class<IFunction>::getFunction(_setRepeatCount),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("running","",Class<IFunction>::getFunction(_getRunning),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",Class<IFunction>::getFunction(_getDelay),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",Class<IFunction>::getFunction(_setDelay),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("start","",Class<IFunction>::getFunction(start),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reset","",Class<IFunction>::getFunction(reset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(stop),NORMAL_METHOD,true);
}

ASFUNCTIONBODY(Timer,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	Timer* th=static_cast<Timer*>(obj);

	th->delay=args[0]->toInt();
	if(argslen>=2)
		th->repeatCount=args[1]->toInt();

	return NULL;
}

ASFUNCTIONBODY(Timer,_getCurrentCount)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_i(th->currentCount);
}

ASFUNCTIONBODY(Timer,_getRepeatCount)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_i(th->repeatCount);
}

ASFUNCTIONBODY(Timer,_setRepeatCount)
{
	assert_and_throw(argslen==1);
	int32_t count=args[0]->toInt();
	Timer* th=static_cast<Timer*>(obj);
	th->repeatCount=count;
	if(th->repeatCount>0 && th->repeatCount<=th->currentCount)
	{
		getSys()->removeJob(th);
		th->running=false;
		th->tickJobInstance = NullRef;
	}
	return NULL;
}

ASFUNCTIONBODY(Timer,_getRunning)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_b(th->running);
}

ASFUNCTIONBODY(Timer,_getDelay)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_i(th->delay);
}

ASFUNCTIONBODY(Timer,_setDelay)
{
	assert_and_throw(argslen==1);
	int32_t newdelay = args[0]->toInt();
	if (newdelay<=0)
		throw Class<RangeError>::getInstanceS("delay must be positive", 2066);

	Timer* th=static_cast<Timer*>(obj);
	th->delay=newdelay;

	return NULL;
}

ASFUNCTIONBODY(Timer,start)
{
	Timer* th=static_cast<Timer*>(obj);
	if(th->running)
		return NULL;
	th->running=true;
	th->stopMe=false;
	th->incRef();
	th->tickJobInstance = _MNR(th);
	if(th->repeatCount==1)
		getSys()->addWait(th->delay,th);
	else
		getSys()->addTick(th->delay,th);
	return NULL;
}

ASFUNCTIONBODY(Timer,reset)
{
	Timer* th=static_cast<Timer*>(obj);
	if(th->running)
	{
		//This spin waits if the timer is running right now
		getSys()->removeJob(th);
		//NOTE: although no new events will be sent now there might be old events in the queue.
		//Is this behaviour right?
		//This is not anymore used by the timer, so it can die
		th->tickJobInstance = NullRef;
		th->running=false;
	}
	th->currentCount=0;
	return NULL;
}

ASFUNCTIONBODY(Timer,stop)
{
	Timer* th=static_cast<Timer*>(obj);
	if(th->running)
	{
		//This spin waits if the timer is running right now
		getSys()->removeJob(th);
		//NOTE: although no new events will be sent now there might be old events in the queue.
		//Is this behaviour right?

		//This is not anymore used by the timer, so it can die
		th->tickJobInstance = NullRef;
		th->running=false;
	}
	return NULL;
}

