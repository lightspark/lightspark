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
#include "utils/token_parser/enum.h"
#include "utils/token_parser/member.h"
#include "utils/token_parser/token_parser.h"
#include "utils/toml++_utils.h"

static Approximations parseApproximations
(
	const LSMemberInfo& memberInfo,
	const LSToken::Expr& expr
);
static ViewportDimensions parseViewportDimensions(const LSToken::Expr& expr);
static RenderOptions parseRenderOptions(const LSToken::Expr& expr);
static PlayerOptions parsePlayerOptions(const LSToken::Expr& expr);

template<>
struct GetValue<Approximations>
{
	static Approximations getValue(const LSMemberInfo& memberInfo, const tiny_string&, const Expr& expr)
	{
		return parseApproximations(memberInfo, expr);
	}
};

template<>
struct GetValue<ViewportDimensions>
{
	static ViewportDimensions getValue(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		return parseViewportDimensions(expr);
	}
};

template<>
struct GetValue<RenderOptions>
{
	static RenderOptions getValue(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		return parseRenderOptions(expr);
	}
};

template<>
struct GetValue<PlayerOptions>
{
	static PlayerOptions getValue(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		return parsePlayerOptions(expr);
	}
};

template<>
struct GetValue<FailureType>
{
	static FailureType getValue
	(
		const LSMemberInfo& memberInfo,
		const tiny_string& name,
		const Expr& expr
	)
	{
		if (name == "knownFailure" || name == "knownCrash")
		{
			if (!expr.isBool())
			{
				std::stringstream s;
				s << "FailureType: `" << name << "` requires a bool.";
				throw TestRunnerException(s.str());
			}

			// Throw `ReturnNullOptException`, to return an empty
			// `Optional`.
			if (!expr.eval<bool>())
			{
				std::stringstream s;
				s << "FailureType: `" << name << "` is false.";
				throw ReturnNullOptException(s.str());
			}

			return
			(
				name.endsWith("Crash") ?
				FailureType::Crash :
				FailureType::Fail
			);
		}
		return getEnumValue<FailureType>(memberInfo, name, expr);
	}
};

static LSMemberInfo testOptionsInfo
{
	// memberMap.
	{
		MEMBER(TestOptions, name),
		MEMBER(TestOptions, description),
		MEMBER(TestOptions, authors),
		// Single entry version of `authors`.
		MEMBER_NAME_LONG(TestOptions, authors, author, true),
		MEMBER(TestOptions, numFrames),
		MEMBER(TestOptions, tickRate),
		MEMBER(TestOptions, outputPath),
		MEMBER(TestOptions, ignore),
		MEMBER(TestOptions, knownFailType),
		MEMBER(TestOptions, playerOptions),
		MEMBER(TestOptions, logFetch),
		MEMBER(TestOptions, screenDPI),
		MEMBER(TestOptions, usesAssert),
	},
	// memberAliases.
	{
		// `description`.
		{ "desc", "description" },
		{ "info", "description" },
		// `numFrames`.
		{ "frames", "numFrames" },
		{ "frameCount", "numFrames" },
		// `knownFailType`.
		// Same as `knownFailType = Fail;`.
		{ "knownFailure", "knownFailType" },
		// Same as `knownFailType = Crash;`.
		{ "knownCrash", "knownFailType" },
		// `screenDPI`.
		{ "dpi", "screenDPI" },
	},
	// enumMap.
	{
		{
			"knownFailType",
			{
				LSShortEnum(),
				LSEnum
				({
					{ "Fail", ssize_t(FailureType::Fail) },
					{ "Crash", ssize_t(FailureType::Crash) },
				})
			}
		},
	}
};

static LSMemberInfo playerOptionsInfo
{
	// memberMap.
	{
		MEMBER(PlayerOptions, maxExecution),
		MEMBER(PlayerOptions, viewportDimensions),
		MEMBER(PlayerOptions, renderOptions),
		MEMBER(PlayerOptions, hasAudio),
		MEMBER(PlayerOptions, hasVideo),
		MEMBER(PlayerOptions, flashMode),
	},
	// memberAliases.
	{
		// `maxExecution`.
		{ "timeout", "maxExecution" },
		// `viewportDimensions`.
		{ "viewport", "viewportDimensions" },
	},
	// enumMap.
	{
		{
			"flashMode",
			{
				LSShortEnum(),
				LSEnum
				({
					{ "AIR", FlashMode::AIR },
					{ "Flash", FlashMode::Flash },
					{ "AvmPlus", FlashMode::AvmPlus },
					// Aliases.
					{ "air", FlashMode::AIR },
					{ "flash", FlashMode::Flash },
					{ "avmplus", FlashMode::AvmPlus },
					{ "FlashPlayer", FlashMode::Flash },
					{ "fp", FlashMode::Flash },
				})
			}
		},
	}
};

static bool isTestInfoFile(const path_t& path, const LSToken& token)
{
	return
	(
		path.filename() == "test_info" &&
		token.isDirective() &&
		token.dir.name == "test"
	);
}

static Approximations parseApproximations
(
	const LSMemberInfo& memberInfo,
	const LSToken::Expr& expr
)
{
	if (!expr.isBlock())
		throw TestRunnerException("Approximations: Expression must be a block.");

	Approximations ret;
	parseAssignBlock(expr.block, [&](const tiny_string& name, const Expr& expr)
	{
		if (name == "numberPatterns")
		{
			ret.numberPatterns = GetValue<std::vector<tiny_string>>::getValue
			(
				memberInfo,
				name,
				expr
			);
		}
		else if (name == "epsilon")
			ret.epsilon = expr.eval<double>();
		else if (name == "maxRelative")
			ret.maxRelative = expr.eval<double>();
	});
	return ret;
}

static ViewportDimensions parseViewportDimensions(const LSToken::Expr& expr)
{
	if (!expr.isBlock())
		throw TestRunnerException("ViewportDimensions: Expression must be a block.");

	ViewportDimensions ret;
	parseAssignBlock(expr.block, [&](const tiny_string& name, const Expr& expr)
	{
		if (name == "size")
			ret.size = parsePoint<int>(expr);
		else if (name == "scale")
			ret.scale = expr.eval<double>();
	});
	return ret;
}

static RenderOptions parseRenderOptions(const LSToken::Expr& expr)
{
	if (!expr.isBlock())
		throw TestRunnerException("RenderOptions: Expression must be a block.");

	RenderOptions ret;
	parseAssignBlock(expr.block, [&](const tiny_string& name, const Expr& expr)
	{
		if (name == "required")
			ret.required = expr.eval<bool>();
		else if (name == "sampleCount")
			ret.sampleCount = expr.eval<size_t>();
	});
	return ret;
}

static PlayerOptions parsePlayerOptions(const LSToken::Expr& expr)
{
	if (!expr.isBlock())
		throw TestRunnerException("PlayerOptions: Expression must be a block.");

	PlayerOptions ret;
	parseAssignBlock(expr.block, [&](const tiny_string& name, const Expr& expr)
	{
		playerOptionsInfo.getMember(name).set
		(
			&ret,
			playerOptionsInfo,
			name,
			expr
		);
	});
	return ret;
}

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

template<>
struct TomlFrom<FailureType>
{
	template<typename V>
	static FailureType get(const V& view)
	{
		if (!view["known_failure"].value_or(false))
			throw std::bad_cast();
		return FailureType::Fail;
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
				auto block = LSTokenParser().parseFile(path.string());
				if (!isTestInfoFile(path, block.front()))
				{
					std::stringstream s;
					s << path << " isn't a valid `test_info` file.";
					throw TestRunnerException(s.str());
				}

				// Remove the starting `#test` directive.
				block.pop_front();

				parseAssignBlock(block, [&](const tiny_string& name, const Expr& expr)
				{
					testOptionsInfo.getMember(name).set
					(
						this,
						testOptionsInfo,
						name,
						expr
					);
				});
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error parsing file " << path <<
				". Reason: " << e.what() << std::endl;
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
				knownFailType = tomlValue<FailureType>(data);

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
