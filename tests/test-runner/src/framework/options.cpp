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

#include <cstdint>
#include <string>

#include "framework/options.h"
#include "framework/test.h"
#include "utils/filesystem_overloads.h"
#include "utils/toml++_utils.h"

template<>
struct TomlFrom<Approximations>
{
	template<typename V>
	static Approximations get(const V& view, const TestFormat& testFormat)
	{
		bool isRuffle = testFormat == TestFormat::Ruffle;
		return Approximations
		{
			// numberPatterns
			tomlValue<std::vector<tiny_string>>
			(
				view
				[
					isRuffle ?
					"number_patterns" :
					"numberPatterns"
				]
			).valueOr(std::vector<tiny_string> {}),
			// epsilon
			view["epsilon"].template value<double>(),
			// maxRelative
			view
			[
				isRuffle ?
				"max_relative" :
				"maxRelative"
			].template value<double>()
		};
	}
};

template<>
struct TomlFrom<FlashMode>
{
	template<typename V>
	static FlashMode get(const V& v)
	{
		auto str = tiny_string(*v.template value<std::string>()).lowercase();
		if (str == "air")
			return FlashMode::AIR;
		if (str == "flash")
			return FlashMode::Flash;
		if (str == "avmplus")
			return FlashMode::AvmPlus;

		throw std::bad_cast();
	}
};

template<>
struct TomlFrom<ViewportDimensions>
{
	template<typename V>
	static ViewportDimensions get(const V& view, const TestFormat& testFormat)
	{
		bool isRuffle = testFormat == TestFormat::Ruffle;
		return ViewportDimensions
		{
			// size
			Vector2
			(
				*view["width"].template value<int>(),
				*view["height"].template value<int>()
			),
			// scale
			*view
			[
				isRuffle ?
				"scale_factor" :
				"scale"
			].template value<double>()
		};
	}
};

template<>
struct TomlFrom<RenderOptions>
{
	template<typename V>
	static RenderOptions get(const V& view, const TestFormat& testFormat)
	{
		bool isRuffle = testFormat == TestFormat::Ruffle;
		bool required = view
		[
			isRuffle ?
			"optional" :
			"required"
		].value_or(false);
		return RenderOptions
		{
			// required
			isRuffle ? !required : required,
			// sampleCount
			view
			[
				isRuffle ?
				"sample_count" :
				"sampleCount"
			].value_or(size_t(1))
		};
	}
};

template<>
struct TomlFrom<PlayerOptions>
{
	template<typename V>
	static PlayerOptions get(const V& view, const TestFormat& testFormat)
	{
		bool isRuffle = testFormat == TestFormat::Ruffle;
		return PlayerOptions
		{
			// maxExecution
			tomlTryFindFlat<TimeSpec>
			(
				view,
				// Lightspark.
				"maxExecution",
				"timeout",
				// Ruffle.
				"max_execution_duration"
			),
			// viewportDimensions
			tomlTryFindFlat<ViewportDimensions>
			(
				testFormat,
				view,
				// Lightspark.
				"viewportDimensions",
				"viewport"
				// Ruffle.
				"viewport_dimensions"
			),
			// renderOptions
			tomlValue<RenderOptions>
			(
				testFormat,
				view[isRuffle ? "with_renderer" : "renderOptions"]
			),
			// hasAudio
			view[isRuffle ? "with_audio" : "hasAudio"].value_or(false),
			// hasVideo
			view[isRuffle ? "with_video" : "hasVideo"].value_or(false),
			// flashMode
			tomlValue<FlashMode>
			(
				view[isRuffle ? "runtime" : "flashMode"]
			).valueOr(isRuffle ? FlashMode::AvmPlus : FlashMode::Flash)
		};
	}
};

template<>
struct TomlFrom<FailureType>
{
	template<typename V>
	static FailureType get(const V& view, const TestFormat& testFormat)
	{
		bool isRuffle = testFormat == TestFormat::Ruffle;
		bool knownCrash = !isRuffle ? view["knownCrash"].value_or(false) : false;
		bool knownFailure = view
		[
			isRuffle ?
			"known_failure" :
			"knownFailure"
		].value_or(false);

		if (!knownFailure && !knownCrash)
			throw std::bad_cast();

		return knownCrash ? FailureType::Crash : FailureType::Fail;
	}
};

static tiny_string toOutputFileName(const TestFormat& testFormat)
{
	switch (testFormat)
	{
		// Default to our own test format, if we encounter an unknown format.
		case TestFormat::Unknown:
		case TestFormat::Lightspark: return "expected_output"; break;
		case TestFormat::Ruffle: return "output.txt"; break;
		// TODO: Implement this.
		case TestFormat::Shumway: return ""; break;
		// TODO: Implement this.
		case TestFormat::AVMPlus: return ""; break;
	}
	return "";
}

template<typename T>
bool relativeEqual
(
	const T& a,
	const T& b,
	const T& epsilon = std::numeric_limits<T>::epsilon(),
	const T& maxRelative = std::numeric_limits<T>::epsilon()
)
{
	// Handle same infs.
	if (a == b)
		return true;
	// Handle remaining infs.
	if (std::isinf(a) || std::isinf(b))
		return false;

	T absDiff = std::abs(a - b);

	// Treat the two values as equal, if they're really close together.
	if (absDiff <= epsilon)
		return true;

	// Use a relative difference comparison.
	return absDiff <= std::max(std::abs(a), std::abs(b)) * maxRelative;

}

template<typename T>
bool relativeEqual(const T& a, const T& b, const T& maxRelative)
{
	const T epsilon = std::numeric_limits<T>::epsilon();
	return relativeEqual(a, b, epsilon, maxRelative);
}


void Approximations::compare(double actual, double expected) const
{
	auto defaultEpsilon = std::numeric_limits<double>::epsilon();
	auto _epsilon = epsilon.valueOr(defaultEpsilon);
	auto _maxRelative = maxRelative.valueOr(defaultEpsilon);

	if (!relativeEqual(actual, expected, _epsilon, _maxRelative))
	{
		std::stringstream s;
		s << "Approximation failed: expected " << expected <<
		", got " << actual <<
		". Epsilon = " << _epsilon <<
		", Max Relative = " << _maxRelative;
		throw TestRunnerException(s.str());
	}
}

std::vector<std::regex> Approximations::getNumberPatterns() const
{
	std::vector<std::regex> ret;
	std::transform
	(
		numberPatterns.begin(),
		numberPatterns.end(),
		std::back_inserter(ret),
		[&](const tiny_string& pattern)
		{
			return std::regex((std::string)pattern);
		}
	);
	return ret;
}

TestOptions::TestOptions
(
	const tiny_string& _name,
	const path_t& path,
	const TestFormat& testFormat
) :
name(_name),
outputPath(toOutputFileName(testFormat)),
ignore(false),
logFetch(false),
usesAssert(false)
{
	switch (testFormat)
	{
		// Default to our own test format, if we encounter an unknown format.
		case TestFormat::Unknown:
		case TestFormat::Lightspark:
		{
			try
			{
				auto data = toml::parse_file(path.string());
				description = tomlTryFindFlat<tiny_string>
				(
					data,
					"description",
					"desc"
				).valueOr("");

				numFrames = tomlTryFind<size_t>
				(
					data,
					"numFrames",
					"frames"
				);
				tickRate = data["tickRate"].template value<double>();

				outputPath = data["outputPath"].value_or("expected_output");
				ignore = data["ignore"].value_or(false);
				knownFailType = tomlValue<FailureType>(testFormat, data);

				playerOptions = tomlValue<PlayerOptions>
				(
					testFormat,
					data
				).valueOr(PlayerOptions());
				logFetch = data["logFetch"].value_or(false);
				approximations = tomlValue<Approximations>(testFormat, data);
				screenDPI = data["screenDPI"].template value<double>();
				usesAssert = data["usesAssert"].value_or(false);
			}
			catch (const toml::parse_error& e)
			{
				auto source = e.source();
				auto _path = source.path != nullptr ? *source.path : path.string();
				std::cerr << "Error parsing file " << _path << ':' <<
				std::endl << "reason: " << e.description() <<
				std::endl << '(' << source.begin << ')' <<
				std::endl;
				std::exit(1);
			}
			break;
		}
		case TestFormat::Ruffle:
		{
			try
			{
				auto data = toml::parse_file(path.string());
				numFrames = tomlTryFindFlat<size_t>
				(
					data,
					"num_frames",
					"num_ticks"
				).orElse([&]
				{
					std::stringstream s;
					s << "Ruffle test " << name << " must have either num_frames, or num_ticks";
					throw TestRunnerException(s.str());
					return nullOpt;
				});

				tickRate = data["tick_rate"].template value<double>();

				outputPath = data["output_path"].value_or("output.txt");
				ignore = data["ignore"].value_or(false);
				knownFailType = tomlValue<FailureType>(testFormat, data);

				approximations = tomlTryFind<Approximations>
				(
					testFormat,
					data,
					"approximations"
				);

				playerOptions = tomlValue<PlayerOptions>
				(
					testFormat,
					data["player_options"]
				).valueOr(PlayerOptions());

				logFetch = data["log_fetch"].value_or(false);
			}
			catch (const toml::parse_error& e)
			{
				auto source = e.source();
				auto _path = source.path != nullptr ? *source.path : path.string();
				std::cerr << "Error parsing file " << _path << ':' <<
				std::endl << "reason: " << e.description() <<
				std::endl << '(' << source.begin << ')' <<
				std::endl;
				std::exit(1);
			}
			break;
		}
		// TODO: Implement the other formats.
		default:
			break;
	}
}

path_t TestOptions::getOutputPath(const path_t& testDir) const
{
	return testDir / outputPath;
}
