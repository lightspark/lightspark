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
#include <lightspark/backends/event_loop.h>
#include <utility>

#include "framework/backends/timer.h"

using namespace lightspark;

namespace lightspark
{
	struct LSEventStorage;
	class SystemState;
};

struct TestRunner;

class TestRunnerEventLoop : public EventLoop
{
private:
	std::atomic_bool notified;
	TestRunner* runner;

	Optional<LSEventStorage> waitEventImpl(SystemState* sys) override;
	void notify() override { notified = true; }
public:
	TestRunnerEventLoop(TestRunner* _runner) :
	EventLoop(new TestRunnerTime(_runner)),
	notified(false),
	runner(_runner) {}

	bool timersInEventLoop() const override { return true; }
};

#endif /* FRAMEWORK_BACKENDS_EVENT_LOOP_H */
