/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include <cstdlib>
#include <cassert>

#include "backends/event_loop.h"
#include "interfaces/backends/event_loop.h"
#include "platforms/engineutils.h"
#include "compat.h"
#include "swf.h"
#include "timer.h"

using namespace lightspark;
using namespace std;

// TODO: Implement
LSEvent SDLEvent::toLSEvent() const
{
	return LSEvent{};
}

// TODO: Implement
IEvent& SDLEvent::fromLSEvent(const LSEvent& event)
{
	return *this;
}

bool SDLEventLoop::waitEvent(IEvent& event, SystemState* sys)
{
	SDLEvent& ev = static_cast<SDLEvent&>(event);
	Locker l(listMutex);
	for (;;)
	{
		int64_t delay = -1;
		if (!timers.empty())
		{
			auto now = TimeSpec::fromNs(time->getCurrentTime_ns());
			auto deadline = timers.front().deadline();
			delay = now < deadline ? (deadline - now).toMsRound() : 0;
		}

		l.release();
		int gotEvent = SDL_WaitEventTimeout(&ev.event, delay);
		l.acquire();
		if (gotEvent && ev.event.type != LS_USEREVENT_NEW_TIMER)
			return true;

		if (sys != nullptr && sys->isShuttingDown())
			return false;
		if (timers.empty())
			continue;

		TimeEvent timer = timers.front();
		auto deadline = timer.deadline();
		auto now = TimeSpec::fromNs(time->getCurrentTime_ns());

		if (now >= deadline)
		{
			timers.pop_front();
			if (timer.job->stopMe)
			{
				timer.job->tickFence();
				continue;
			}
			if (timer.isTick)
			{
				timer.startTime = now;
				insertEventNoLock(timer);
			}

			l.release();
			timer.job->tick();
			l.acquire();

			if (!timer.isTick)
				timer.job->tickFence();
		}
	}
}

void SDLEventLoop::insertEvent(const SDLEventLoop::TimeEvent& e)
{
	Locker l(listMutex);
	insertEventNoLock(e);
}

void SDLEventLoop::insertEventNoLock(const SDLEventLoop::TimeEvent& e)
{
	if (timers.empty() || timers.front().deadline() > e.deadline())
	{
		timers.push_front(e);
		SDL_Event event;
		SDL_zero(event);
		event.type = LS_USEREVENT_NEW_TIMER;
		SDL_PushEvent(&event);
		return;
	}

	auto it = std::find_if(std::next(timers.begin()), timers.end(), [&](const TimeEvent& it)
	{
		return it.deadline() > e.deadline();
	});
	timers.insert(it, e);
}
void SDLEventLoop::addJob(uint32_t ms, bool isTick, ITickJob* job)
{
	insertEvent(TimeEvent { isTick, TimeSpec::fromNs(time->getCurrentTime_ns()), TimeSpec::fromMs(ms), job });
}

void SDLEventLoop::addTick(uint32_t tickTime, ITickJob* job)
{
	addJob(tickTime, true, job);
}

void SDLEventLoop::addWait(uint32_t waitTime, ITickJob* job)
{
	addJob(waitTime, false, job);
}

void SDLEventLoop::removeJobNoLock(ITickJob* job)
{
	auto it = std::remove_if(timers.begin(), timers.end(), [&](const TimeEvent& it)
	{
		return it.job == job;
	});
	if (it == timers.end())
		return;
	bool first = it == timers.begin();
	timers.erase(it);
	if (first)
	{
		SDL_Event event;
		SDL_zero(event);
		event.type = LS_USEREVENT_NEW_TIMER;
		SDL_PushEvent(&event);
	}
}

void SDLEventLoop::removeJob(ITickJob* job)
{
	Locker l(listMutex);
	removeJobNoLock(job);
}
