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
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Number.h"

using namespace std;
using namespace lightspark;

void Timer::tick()
{
	//This will be executed once if repeatCount was originally 1
	//Otherwise it's executed until stopMe is set to true
	this->incRef();
	getVm(getSystemState())->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS(getInstanceWorker(),"timer")));

	currentCount++;
	if(repeatCount!=0)
	{
		if(currentCount>=repeatCount)
		{
			this->incRef();
			getVm(getSystemState())->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS(getInstanceWorker(),"timerComplete")));
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
	c->setDeclaredMethodByQName("currentCount","",Class<IFunction>::getFunction(c->getSystemState(),_getCurrentCount,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",Class<IFunction>::getFunction(c->getSystemState(),_getRepeatCount,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",Class<IFunction>::getFunction(c->getSystemState(),_setRepeatCount),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("running","",Class<IFunction>::getFunction(c->getSystemState(),_getRunning,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",Class<IFunction>::getFunction(c->getSystemState(),_getDelay,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",Class<IFunction>::getFunction(c->getSystemState(),_setDelay),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("start","",Class<IFunction>::getFunction(c->getSystemState(),start),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reset","",Class<IFunction>::getFunction(c->getSystemState(),reset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(c->getSystemState(),stop),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(Timer,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj,nullptr,0);
	Timer* th=asAtomHandler::as<Timer>(obj);

	th->delay=asAtomHandler::toInt(args[0]);
	if(argslen>=2)
		th->repeatCount=asAtomHandler::toInt(args[1]);
}

ASFUNCTIONBODY_ATOM(Timer,_getCurrentCount)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	asAtomHandler::setInt(ret,wrk,th->currentCount);
}

ASFUNCTIONBODY_ATOM(Timer,_getRepeatCount)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	asAtomHandler::setInt(ret,wrk,th->repeatCount);
}

ASFUNCTIONBODY_ATOM(Timer,_setRepeatCount)
{
	assert_and_throw(argslen==1);
	int32_t count=asAtomHandler::toInt(args[0]);
	Timer* th=asAtomHandler::as<Timer>(obj);
	th->repeatCount=count;
	if(th->repeatCount>0 && th->repeatCount<=th->currentCount)
	{
		getSys()->removeJob(th);
		th->running=false;
		th->tickJobInstance = NullRef;
	}
}

ASFUNCTIONBODY_ATOM(Timer,_getRunning)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	asAtomHandler::setBool(ret,th->running);
}

ASFUNCTIONBODY_ATOM(Timer,_getDelay)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	asAtomHandler::setNumber(ret,wrk,th->delay);
}

ASFUNCTIONBODY_ATOM(Timer,_setDelay)
{
	assert_and_throw(argslen==1);
	int32_t newdelay = asAtomHandler::toInt(args[0]);
	if (newdelay<0)
	{
		createError<RangeError>(wrk,2066,"delay must be positive");
		return;
	}

	Timer* th=asAtomHandler::as<Timer>(obj);
	th->delay=newdelay;
}

ASFUNCTIONBODY_ATOM(Timer,start)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	if(th->running)
		return;
	th->running=true;
	th->stopMe=false;
	th->incRef();
	th->tickJobInstance = _MNR(th);
	// according to spec Adobe handles timers 60 times per second, so minimum delay is 17 ms
	if(th->repeatCount==1)
		wrk->getSystemState()->addWait(th->delay < 17 ? 17 : th->delay,th);
	else
		wrk->getSystemState()->addTick(th->delay < 17 ? 17 : th->delay,th);
}

ASFUNCTIONBODY_ATOM(Timer,reset)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	if(th->running)
	{
		//This spin waits if the timer is running right now
		wrk->getSystemState()->removeJob(th);
		//NOTE: although no new events will be sent now there might be old events in the queue.
		//Is this behaviour right?
		//This is not anymore used by the timer, so it can die
		th->tickJobInstance = NullRef;
		th->running=false;
	}
	th->currentCount=0;
}

ASFUNCTIONBODY_ATOM(Timer,stop)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	if(th->running)
	{
		//This spin waits if the timer is running right now
		wrk->getSystemState()->removeJob(th);
		//NOTE: although no new events will be sent now there might be old events in the queue.
		//Is this behaviour right?

		//This is not anymore used by the timer, so it can die
		th->tickJobInstance = NullRef;
		th->running=false;
	}
}

