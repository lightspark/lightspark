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
#include "timer.h"
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
	currentCount++;
	//This will be executed once if repeatCount was originally 1
	//Otherwise it's executed until stopMe is set to true
	if (getSystemState()->runSingleThreaded)
		ABCVm::publicHandleEvent(this,_MR(Class<TimerEvent>::getInstanceS(getInstanceWorker(),"timer")));
	else
	{
		this->incRef();
		getVm(getSystemState())->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS(getInstanceWorker(),"timer")));
		signal.wait();
	}
	if(repeatCount!=0)
	{
		if(currentCount>=repeatCount)
		{
			running=false;
			if (getSystemState()->runSingleThreaded)
				ABCVm::publicHandleEvent(this,_MR(Class<TimerEvent>::getInstanceS(getInstanceWorker(),"timerComplete")));
			else
			{
				this->incRef();
				getVm(getSystemState())->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS(getInstanceWorker(),"timerComplete")));
				signal.wait();
			}
			stopMe=true;
		}
	}
}

void Timer::tickFence()
{
	this->removeStoredMember();
}

void Timer::stopTimer()
{
	if(running)
	{
		running=false;
		getSystemState()->removeJob(this);
	}
}
void Timer::startTimer(ASWorker* wrk)
{
	if (!running)
	{
		running=true;
		stopMe=false;
		incRef();
		addStoredMember();
		// according to spec Adobe handles timers 60 times per second, so minimum delay is 17 ms
		if(repeatCount==1)
			wrk->getSystemState()->addWait(delay < 17 ? 17 : delay,this);
		else
			wrk->getSystemState()->addTick(delay < 17 ? 17 : delay,this);
	}
}

Timer::Timer(ASWorker* wrk, Class_base* c)
	:EventDispatcher(wrk,c)
	,signal(0)
	,running(false)
	,delay(0)
	,repeatCount(0)
	,currentCount(0)
{
	subtype=SUBTYPE_TIMER;
}


void Timer::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("currentCount","",c->getSystemState()->getBuiltinFunction(_getCurrentCount,0,Class<Integer>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",c->getSystemState()->getBuiltinFunction(_getRepeatCount,0,Class<Integer>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",c->getSystemState()->getBuiltinFunction(_setRepeatCount),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("running","",c->getSystemState()->getBuiltinFunction(_getRunning,0,Class<Boolean>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",c->getSystemState()->getBuiltinFunction(_getDelay,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",c->getSystemState()->getBuiltinFunction(_setDelay),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("start","",c->getSystemState()->getBuiltinFunction(start),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reset","",c->getSystemState()->getBuiltinFunction(reset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",c->getSystemState()->getBuiltinFunction(stop),NORMAL_METHOD,true);
}

void Timer::finalize()
{
	EventDispatcher::finalize();
}
bool Timer::destruct()
{
	running=false;
	delay=0;
	repeatCount=0;
	currentCount=0;
	return EventDispatcher::destruct();
}
void Timer::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	EventDispatcher::prepareShutdown();
}
bool Timer::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	return EventDispatcher::countCylicMemberReferences(gcstate);
}

void Timer::afterHandleEvent(Event* ev)
{
	if (ev->is<TimerEvent>())
		signal.signal();
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
	asAtomHandler::setInt(ret,th->currentCount);
}

ASFUNCTIONBODY_ATOM(Timer,_getRepeatCount)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	asAtomHandler::setInt(ret,th->repeatCount);
}

ASFUNCTIONBODY_ATOM(Timer,_setRepeatCount)
{
	assert_and_throw(argslen==1);
	int32_t count=asAtomHandler::toInt(args[0]);
	Timer* th=asAtomHandler::as<Timer>(obj);
	th->repeatCount=count;
	if (th->repeatCount <= th->currentCount)
		th->stopTimer();
}

ASFUNCTIONBODY_ATOM(Timer,_getRunning)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	asAtomHandler::setBool(ret,th->running);
}

ASFUNCTIONBODY_ATOM(Timer,_getDelay)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	asAtomHandler::setNumber(ret, th->delay);
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
	if(th->running)
	{
		th->stopTimer();
		th->startTimer(wrk);
	}
}

ASFUNCTIONBODY_ATOM(Timer,start)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	if(th->running)
		return;
	th->startTimer(wrk);
}

ASFUNCTIONBODY_ATOM(Timer,reset)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	th->stopTimer();
	th->currentCount=0;
}

ASFUNCTIONBODY_ATOM(Timer,stop)
{
	Timer* th=asAtomHandler::as<Timer>(obj);
	th->stopTimer();
}

