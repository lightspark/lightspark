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
	static Approximations get(const V& v)
	{
		return Approximations
		{
			// numberPatterns
			tomlValue<std::vector<tiny_string>>
			(
				v["number_patterns"]
			).valueOr(std::vector<tiny_string> {}),
			// epsilon
			v["epsilon"].template value<double>(),
			// maxRelative
			v["max_relative"].template value<double>()
		};
	}
};

template<>
struct TomlFrom<FlashMode>
{
	template<typename V>
	static FlashMode get(const V& v)
	{
		auto str = v.template value<std::string>();
		return *str == "AIR" ? FlashMode::AIR : FlashMode::AvmPlus;
	}
};

template<>
struct TomlFrom<ViewportDimensions>
{
	template<typename V>
	static ViewportDimensions get(const V& view)
	{
		return ViewportDimensions
		{
			// size
			Vector2
			(
				*view["width"].template value<int>(),
				*view["height"].template value<int>()
			),
			// scale
			*view["scale_factor"].template value<double>()
		};
	}
};

template<>
struct TomlFrom<RenderOptions>
{
	template<typename V>
	static RenderOptions get(const V& view)
	{
		return RenderOptions
		{
			// required
			!view["optional"].value_or(false),
			// sampleCount
			view["sample_count"].value_or(size_t(1))
		};
	}
};

template<>
struct TomlFrom<PlayerOptions>
{
	template<typename V>
	static PlayerOptions get(const V& view)
	{
		return PlayerOptions
		{
			// maxExecution
			tomlValue<TimeSpec>(view["max_execution_duration"]),
			// viewportDimensions
			tomlValue<ViewportDimensions>(view["viewport_dimensions"]),
			// renderOptions
			tomlValue<RenderOptions>(view["with_renderer"]),
			// hasAudio
			view["with_audio"].value_or(false),
			// hasVideo
			view["with_video"].value_or(false),
			// flashMode
			tomlValue<FlashMode>(view["runtime"]).valueOr(FlashMode::AvmPlus)
		};
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
knownFailure(false),
logFetch(false)
{
	switch (testFormat)
	{
		// Default to our own test format, if we encounter an unknown format.
		case TestFormat::Unknown:
		case TestFormat::Lightspark:
		{
			// TODO: Implement this.
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
				knownFailure = data["known_failure"].value_or(false);

				approximations = tomlTryFind<Approximations>
				(
					data,
					"approximations"
				);

				playerOptions = tomlValue<PlayerOptions>
				(
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
