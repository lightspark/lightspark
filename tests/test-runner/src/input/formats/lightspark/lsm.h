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

#ifndef INPUT_FORMATS_LIGHTSPARK_LSM_H
#define INPUT_FORMATS_LIGHTSPARK_LSM_H 1

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <list>
#include <vector>

#include <lightspark/events.h>
#include <lightspark/swf.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/timespec.h>
#include <lightspark/utils/visitor.h>

using namespace lightspark;

namespace lightspark
{
	struct LSEventStorage;
};

// TODO: Move this into the main codebase, once TASing support is added.

// End of movie mode.
enum LSMovieEndMode
{
	// Stop on end-of-movie.
	Stop,
	// Keep playing on end-of-movie.
	KeepPlaying,
	// Loop back to the begining of the movie on end-of-movie.
	Loop,
	// Like `KeepPlaying`, but also repeat the event sequence on
	// end-of-movie.
	RepeatEvents,
};

struct LSMovieHeader
{
	// The player used to record this movie.
	tiny_string player;
	// The version of the player used to record this movie.
	//
	// A manually crafted file is signified by an empty version.
	tiny_string version;
	// The name of this movie.
	tiny_string name;
	// The description of this movie.
	tiny_string description;
	// The author(s) of this movie.
	std::list<tiny_string> authors;

	// The path of the SWF for this movie.
	tiny_string path;
	// The MD5 hash of the SWF file for this movie.
	tiny_string md5Hash;
	// The SHA-1 hash of the SWF file for this movie.
	tiny_string sha1Hash;

	// The frame count of this movie.
	size_t numFrames;
	// The re-record count of this movie.
	size_t numRerecords;

	// Allow the use of `noScale` for this movie.
	//
	// If empty, default to false.
	bool allowNoScale;
	// The end mode of this movie.
	// Determines what should happen at the end of the movie.
	//
	// If empty, default to stopping on end-of-movie.
	LSMovieEndMode endMode;

	// Optional starting resolution of this movie.
	//
	// If empty, default to the resolution of the SWF file.
	// NOTE: Only used when `allowNoScale` is true.
	Optional<Vector2f> startingResolution;
	// Optional initial RNG seed for this movie.
	//
	// If empty, default to a starting seed of zero.
	Optional<uint64_t> startingSeed;
	// Optional global event time-step for this movie.
	//
	// If empty, default to using the current event time-stepping mode.
	Optional<TimeSpec> eventTimeStep;
	// Optional scaling mode for this movie.
	//
	// If empty, default to either the current scaling mode, or `showAll`
	// if the current scaling mode is `noScale`, and `allowNoScale` is
	// true.
	Optional<SystemState::SCALE_MODE> scaleMode;
};

class LSMovie
{
friend class LSMovieParser;
public:
	using EventList = std::vector<LSEventStorage>;
private:
	size_t version { 0 };
	LSMovieHeader header;
	std::vector<size_t> frameEventIndices;
	EventList events;
public:
	LSMovie() = default;

	const LSMovieHeader& getHeader() const { return header; }
	const EventList& getEvents() const { return events; }
	// TODO: Write our own span implementation, and use that instead.
	Optional<EventList> getEventsForFrame(size_t frame) const
	{
		if (frame >= numEventFrames())
			return {};

		// Special case for the first frame, since the first frame's
		// events are always at the start of the event list.
		if (frame == 0)
		{
			// Special case for a movie with a single frame.
			if (frameEventIndices.empty())
				return events;
			return EventList
			(
				events.begin(),
				std::next(events.begin(), frameEventIndices[0])
			);
		}

		auto begin = std::next(events.begin(), frameEventIndices[frame - 1]);
		auto end =
		(
			frame < numEventFrames() - 1 ?
			events.begin() + frameEventIndices[frame] :
			// The last frame's events always stops at the end of the
			// event list.
			events.end()
		);
		return EventList(begin, end);
	}

	size_t numFrames() const { return header.numFrames; }
	size_t numEvents() const { return events.size(); }
	size_t numEventFrames() const { return frameEventIndices.size() + 1; } 
};

#endif /* INPUT_FORMATS_LIGHTSPARK_LSM_H */
