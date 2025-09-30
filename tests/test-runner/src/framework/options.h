/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024-2025  mr b0nk 500 (b0nk@b0nk.xyz)
    Copyright (C) 2024  Ludger Krämer <dbluelle@onlinehome.de>

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

using namespace lightspark;

namespace lightspark
{
	class Path;
};

enum class TestFormat;

enum FlashMode
{
	Flash,
	AIR,
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

struct ImageTrigger
{
	enum Type
	{
		LastFrame,
		SpecificIteration,
		FsCommand,
	};

	Type type { Type::LastFrame };
	Optional<uint32_t> iteration;

	ImageTrigger() = default;

	ImageTrigger(uint32_t _iteration) :
	type(Type::SpecificIteration),
	iteration(_iteration) {}

	ImageTrigger(const Type& _type);
};

struct ImageComparison
{
	uint8_t tolerance { 0 };
	size_t maxOutliers { 0 };
	ImageTrigger trigger;
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
	FlashMode flashMode { FlashMode::Flash };
};

enum class FailureType
{
	Fail,
	Crash,
};

struct TestOptions
{
	using ImageComparisonMap = std::unordered_map
	<
		tiny_string,
		ImageComparison
	>;
	tiny_string name;
	tiny_string description;
	std::vector<tiny_string> authors;
	Optional<size_t> numFrames;
	Optional<double> tickRate;
	tiny_string outputPath;
	ImageComparisonMap imageComparisons;
	bool ignore;
	Optional<FailureType> knownFailType;
	PlayerOptions playerOptions;
	bool logFetch;
	Optional<Approximations> approximations;
	Optional<double> screenDPI;
	bool usesAssert;

	TestOptions(const tiny_string& _name, const Path& path, const TestFormat& testFormat);

	Path getOutputPath(const Path& testDir) const;
};

#endif /* FRAMEWORK_OPTIONS_H */
