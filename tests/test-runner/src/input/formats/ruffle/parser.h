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

#ifndef INPUT_FORMATS_RUFFLE_PARSER_H
#define INPUT_FORMATS_RUFFLE_PARSER_H 1

#include <cereal/archives/json.hpp>

#include "input/events.h"
#include "input/parser.h"

class RuffleInputParser : public InputParser
{
private:
	cereal::JSONInputArchive archive;
public:
	RuffleInputParser(const std::istream& input) : InputParser(input), archive(input) {}
	const std::vector<InputEvent> parse() const override;
};

#endif /* INPUT_FORMATS_RUFFLE_PARSER_H */
