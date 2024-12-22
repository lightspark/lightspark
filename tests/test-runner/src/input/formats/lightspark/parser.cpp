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

#include <sstream>

#include <lightspark/events.h>
#include <lightspark/tiny_string.h>

#include "framework/test.h"
#include "input/formats/lightspark/lsm_parser.h"
#include "input/formats/lightspark/parser.h"

std::vector<LSEventStorage> LSInputParser::parse()
{
	try
	{
		const auto movie = LSMovieParser(path.string()).getMovie();
		std::vector<LSEventStorage> ret;
		for (size_t i = 0; i < movie.numFrames(); ++i)
		{
			(void)movie.getEventsForFrame(i).andThen([&](const auto& frameEvents) -> Optional<decltype(ret)>
			{
				ret.insert
				(
					ret.end(),
					frameEvents.begin(),
					frameEvents.end()
				);
				return nullOpt;
			});
			if (i < movie.numFrames() - 1)
				ret.push_back(TestRunnerNextFrameEvent {});
		}
		return ret;
	}
	catch (const std::exception& e)
	{
		std::stringstream s;
		s << "Failed to Parse LSM file " << path << ". Reason: " << e.what();
		throw TestRunnerException(s.str());
	}
}
