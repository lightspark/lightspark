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

#ifndef INPUT_INJECTOR_H
#define INPUT_INJECTOR_H 1

#include <cstdlib>
#include <functional>
#include <vector>

#include "input/parser.h"
#include "input/events.h"

using InjectorCallback = std::function<void(const InputEvent &)>;

class InputInjector
{
private:
	//InputParser* parser;
	std::vector<InputEvent> events;
	size_t eventIndex;
public:
	InputInjector(InputParser& parser) : events(parser.parse()), eventIndex(0) {}
	// Run all events for this frame.
	void runFrame(InjectorCallback callback);
};

#endif /* INPUT_INJECTOR_H */
