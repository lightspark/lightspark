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

#ifndef FRAMEWORK_BACKENDS_EVENT_LOOP_H
#define FRAMEWORK_BACKENDS_EVENT_LOOP_H 1

#include <atomic>
#include <lightspark/events.h>
#include <lightspark/interfaces/backends/event_loop.h>
#include <utility>

#include "framework/backends/timer.h"

using namespace lightspark;

namespace lightspark
{
	struct LSEventStorage;
	class SystemState;
};

struct TestRunner;

class TestRunnerEvent : public IEvent
{
friend class TestRunnerEventLoop;
private:
	LSEventStorage event;
public:
	TestRunnerEvent() {};
	TestRunnerEvent(const LSEvent& ev) : event(ev) {}

	LSEventStorage toLSEvent(SystemState* sys) const override { return event; }
	IEvent& fromLSEvent(const LSEvent& _event) override
	{
		event = _event;
		return *this;
	}
	void* getEvent() const override { return (void*)&event; }
};

class TestRunnerEventLoop : public IEventLoop
{
private:
	bool sendInputEvents;
	std::atomic_bool notified;
	TestRunner* runner;
public:
	TestRunnerEventLoop(TestRunner* _runner) :
	IEventLoop(new TestRunnerTime(_runner)),
	sendInputEvents(true),
	notified(false),
	runner(_runner) {}

	std::pair<bool, bool> waitEvent(IEvent& event, SystemState* sys) override;
	bool timersInEventLoop() const override { return true; }

	bool getSendInputEvents() const { return sendInputEvents; }
	void setSendInputEvents(bool _sendInputEvents) { sendInputEvents = _sendInputEvents; }
	void setNotified(bool _notified) { notified = _notified; }

};

#endif /* FRAMEWORK_BACKENDS_EVENT_LOOP_H */
