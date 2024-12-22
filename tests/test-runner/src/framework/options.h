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

#ifndef FRAMEWORK_OPTIONS_H
#define FRAMEWORK_OPTIONS_H 1

#include <cstdlib>
#include <regex>
#include <vector>

#include <lightspark/backends/geometry.h>
#include <lightspark/swftypes.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/timespec.h>

#include "utils/filesystem_overloads.h"

using namespace lightspark;

enum class TestFormat;

enum FlashMode
{
	Flash,
	AIR,
	AvmPlus,
};

struct Approximations
{
	std::vector<tiny_string> numberPatterns;
	Optional<double> epsilon;
	Optional<double> maxRelative;

	void compare(double actual, double expected) const;

	std::vector<std::regex> getNumberPatterns() const;
};

struct ViewportDimensions
{
	// The size/dimensions of the stage's viewport.
	Vector2 size;
	// The scaling factor of the viewport from stage pixels to device
	// pixels.
	double scale;
};

struct RenderOptions
{
	bool required { true };
	size_t sampleCount { 1 };
};

struct PlayerOptions
{
	Optional<TimeSpec> maxExecution;
	Optional<ViewportDimensions> viewportDimensions;
	Optional<RenderOptions> renderOptions;
	bool hasAudio { false };
	bool hasVideo { false };
	FlashMode flashMode { FlashMode::AvmPlus };
};

enum class FailureType
{
	Fail,
	Crash,
};

struct TestOptions
{
	tiny_string name;
	tiny_string description;
	std::vector<tiny_string> authors;
	Optional<size_t> numFrames;
	Optional<double> tickRate;
	tiny_string outputPath;
	bool ignore;
	Optional<FailureType> knownFailType;
	PlayerOptions playerOptions;
	bool logFetch;
	Optional<Approximations> approximations;
	Optional<double> screenDPI;
	bool usesAssert;

	TestOptions(const tiny_string& _name, const path_t& path, const TestFormat& testFormat);

	path_t getOutputPath(const path_t& testDir) const;
};

#endif /* FRAMEWORK_OPTIONS_H */
