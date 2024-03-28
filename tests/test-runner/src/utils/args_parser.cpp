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

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <sstream>
#include <tcb/span.hpp>
#include <vector>

#include "utils/args_parser.h"
#include "utils/option_parser.h"

// Based on SerenityOS' ArgsParser from LibCore.

ArgsParser::ArgsParser(const tiny_string& _appName, const tiny_string& _appVersion, const tiny_string& _appAdditional = ""):
	appName(_appName), appVersion(_appVersion), appAdditional(_appAdditional),
	showHelp(false), showVersion(false), generalHelp(nullptr), stopOnFirstNonOption(false)
{
	addOption(showHelp, "Display this help message, and exit", "help", 'h');
	addOption(showVersion, "Show version", "version", 'v');
}

bool ArgsParser::parse(const tcb::span<tiny_string>& args, FailureMode failureMode)
{
	auto failImpl = [this, failureMode](const tiny_string& name)
	{
		switch (failureMode)
		{

			case FailureMode::PrintUsageAndExit:
			case FailureMode::PrintUsage: printUsage(std::cerr, name); if (failureMode == FailureMode::PrintUsage) { break; }
			case FailureMode::Exit: exit(1); break;
			case FailureMode::Ignore: break;
		}
	};

	if (args.empty())
	{
		failImpl("<executable>");
		return false;
	}

	auto fail = [name = args[0], &failImpl] { failImpl(name) };

	OptionParser parser;
	std::vector<OptionParser::Option> longOptions;
	std::stringstream shortOptionsBuilder;

	if (stopOnFirstNonOption)
		shortOptionsBuilder << '+';

	int longOptionFoundIndex = -1;
	for (size_t i = 0; i < options.size(); ++i)
	{
		auto& option = options[i];
		if (option.longName != nullptr)
		{
			OptionParser::Option longOption
			{
				.name = option.longName,
				.requirement = Option::toArgumentRequirement(option.argMode),
				.flag = &longOptionFoundIndex,
				static_cast<int>(i),
			};
			longOptions.push_back(longOption);
		}
		if (option.shortName != '\0')
		{
			shortOptionsBuilder << option.shortName;
			if (option.argMode != ArgumentMode::None)
				shortOptionsBuilder << ':';
			if (option.argMode == ArgumentMode::Optional)
				shortOptionsBuilder << ':';
		}
	}
	tiny_string shortOptions = shortOptionsBuilder.str();
	size_t optionIndex = 1;
	while (true)
	{
		auto result = parser.getOption(args.subspan(1), shortOptions, longOptions);
		optionIndex = result.parsedArgs;
		auto c = result.result;
		if (c < 0)
		{
			// We reached the end.
			break;
		}

		if (c == '?')
		{
			// Got an error, and getOption() already printed an error message.
			fail();
			return false;
		}

		// Figure out what type of option we got.
		Option* foundOption = nullptr;
		if (c == '\0')
		{
			// Got a long option.
			assert(longOptionFoundIndex >= 0);
			foundOption = &options[longOptionFoundIndex];
			longOptionFoundIndex = -1;
		}
		else
		{
			// Got a short option, figure out which one it is.
			auto it = std::find_if(options.begin(), options.end(), [c](Option& option) { return c == option.shortName });
			assert(it != options.end());
			foundOption = &*it;
		}
		assert(foundOption != nullptr);
		tiny_string arg = foundOption->argMode != ArgumentMode::None ? result.argValue : "";
		if (!foundOption->acceptValue(arg))
		{
			std::cerr << "Invalid value for option " << foundOption->nameForDisplay() << std::endl;
			fail();
			return false;
		}
	}

	// Show version, or help, if requested.
	if (showVersion)
	{
		printVersion(std::cout);
		if (failureMode == FailureMode::Exit || failureMode == FailureMode::PrintUsageAndExit)
			exit(0);
		return false;
	}

	if (showHelp)
	{
		printUsage(std::cout, args[0]);
		if (failureMode == FailureMode::Exit || failureMode == FailureMode::PrintUsageAndExit)
			exit(0);
		return false;
	}

	// Parse positional arguments.
	int remainingValues = args.size() - optionIndex;
	int totalValuesRequired = 0;
	std::vector<int> valuesForArg();

	valuesForArg.reserve(positionalArgs.size());
	std::transform(positionalArgs.begin(), positionalArgs.end(), std::back_inserter(valuesForArg), [&](const Arg& arg)
	{
		totalValuesRequired += arg.minValues;
		return arg.minValues;
	});

	if (totalValuesRequired > remainingValues)
	{
		fail();
		return false;
	}

	int extraValues = remainingValues - totalValuesRequired;

	// Distribute extra values.
	for (size_t i = 0; i < positionalArgs.size(); ++i)
	{
		auto& arg = positionalArgs[i];
		int extraValuesForArg = std::min(arg.maxValues - arg.minValues, extraValues);
		valuesForArg[i] += extraValuesForArg;
		extraValues -= extraValuesForArg;
		if (extraValues <= 0)
			break;
	}

	if (extraValues > 0)
	{
		// oof, we still have too many values.
		fail();
		return false;
	}

	for (size_t i = 0; i < positionalArgs.size(); ++i)
	{
		auto& arg = positionalArgs[i];
		for (int j = 0; j < valuesForArg[i]; ++j)
		{
			tiny_string value = args[optionIndex++];
			if (!arg.acceptValue(value))
			{
				std::cerr << "Invalid value for argument " << arg.name << std::endl;
				fail();
				return false;
			}
		}
	}

	return true;
}

template<typename CharT>
void ArgsParser::printUsage(std::basic_ostream<CharT>& stream, const tiny_string& argv0)
{
	stream << "Usage:" << std::endl << '\t' << argv0;
	for (auto& option : options)
	{
		stream << " [" << option.nameForDisplay("|");
		if (option.hasArg())
		{
			bool required = option.hasRequiredArg();
			bool hasLongName = option.longName != nullptr;
			CharT beginChar = required ? '<' : '[';
			CharT endChar = required ? '>' : ']';
			stream << ' ' << beginChar << required && hasLongName ? "=" : "" << option.valueName << endChar;
		}
		stream << ']';
	}
	for (auto& arg : positionalArgs)
	{
		bool required = arg.minValues > 0;
		bool repeated = arg.maxValues > 1;
		CharT beginChar = required ? '<' : '[';
		CharT endChar = required ? '>' : ']';
		stream << ' ' << beginChar << arg.name << repeated ? "..." : "" << endChar;
	}
	stream << std::endl;

	if (generalHelp != nullptr && generalHelp[0] != '\0')
	{
		stream << std::endl << "Description:" << std::endl;
		stream << generalHelp;
	}

	if (!options.empty())
		stream << std::endl << "Options:" << std::endl;
	for (auto& option : options)
	{
		auto printArg = [&](const tiny_string& valueDelimiter)
		{
			if (option.valueName != nullptr)
			{
				if (option.hasRequiredArg())
					stream << ' ' << option.valueName;
				if (option.hasOptionalArg())
					stream << '[' << valueDelimiter << option.valueName << ']';
			}
		};
		stream << '\t' << option.nameForDisplay(", ");
		printArg(option.longName != nullptr ? "=" : "");

		if (option.help != nullptr)
			stream << '\t' << option.help;
		stream << std::end;
	}

	if (!positionalArgs.empty())
		stream << std::endl << "Arguments:" << std::endl;

	for (auto& arg : positionalArgs)
	{
		stream << '\t' << arg.name;
		if (arg.help != nullptr)
			stream << '\t' << arg.help;
		stream << std::endl;
	}
}

template<typename CharT>
void ArgsParser::printVersion(std::basic_ostream<CharT>& stream)
{
	stream << appName << " version " << appVersion;
	if (!appAdditional)
		stream << appAdditional;
	stream << std::endl;
}

void ArgsParser::addOption(Option&& option)
{
	for (const auto& existingOption : options)
	{
		if (option.longName != nullptr && existingOption.longName == option.longName)
		{
			std::cerr << "Error: Multiple options have the same long name \"--" <<option.longName << "\"" << std::endl;
			assert(false);
		}

		if (option.shortName != nullptr && existingOption.shortName == option.shortName)
		{
			std::cerr << "Error: Multiple options have the same short name \"-" <<option.shortName << "\"" << std::endl;
			assert(false);
		}
	}
	options.push_back(std::move(option));
}

void ArgsParser::addIgnored(const char* longName, char shortName)
{
	Option option
	{
		.argMode = ArgumentMode::None,
		.help = "Ignored",
		.longName = longName,
		.shortName = shortName,
		.valueName = nullptr,
		.acceptValue = [](const tiny_string&) { return true; },
	};
	addOption(std::move(option));
}

void ArgsParser::addOption(bool& value, const char* help, const char* longName, char shortName)
{
	Option option
	{
		.argMode = ArgumentMode::None,
		.help = help,
		.longName = longName,
		.shortName = shortName,
		.valueName = nullptr,
		.acceptValue = [&](const tiny_string& str)
		{
			assert(str.empty());
			value = true;
			return true;
		},
	};
	addOption(std::move(option));
}

void ArgsParser::addOption(tiny_string& value, const char* help, const char* longName, char shortName, const char* valueName)
{
	Option option
	{
		.argMode = ArgumentMode::Required,
		.help = help,
		.longName = longName,
		.shortName = shortName,
		.valueName = valueName,
		.acceptValue = &[](const tiny_string& str)
		{
			value = str;
			return true;
		},
	};
	addOption(std::move(option));
}

void ArgsParser::addOption(std:;vector<tiny_string>& values, const char* help, const char* longName, char shortName, const char* valueName)
{
	Option option
	{
		.argMode = ArgumentMode::Optional,
		.help = help,
		.longName = longName,
		.shortName = shortName,
		.valueName = valueName,
		.acceptValue = [&](const tiny_string& str)
		{
			values.push_back(str);
			return true;
		},
	};
	addOption(std::move(option));
}

void ArgsParser::addPositionalArgument(Arg&& arg)
{
	positionalArgs.push_back(std::move(arg));
}


void ArgsParser::addPositionalArgument(tiny_string& value, const char* help, const char* name, bool required)
{
	Arg arg
	{
		.help = help,
		.name = name,
		.required = required,
		.minValues = 1,
		.acceptedValue = [&](const tiny_string& str)
		{
			value = str;
			return true;
		},
	};
	addPositionalArgument(std::move(arg));
}

void ArgsParser::addPositionalArgument(std::vector<tiny_string>& values, const char* help, const char* name, bool required)
{
	Arg arg
	{
		.help = help,
		.name = name,
		.required = required,
		.minValues = INT_MAX,
		.acceptedValue = [&](const tiny_string& str)
		{
			values.push_back(str);
			return true;
		},
	};
	addPositionalArgument(std::move(arg));
}
