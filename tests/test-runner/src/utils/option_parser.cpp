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
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <list>
#include <tcb/span.hpp>
#include <vector>

#include "utils/option_parser.h"

// Based on SerenityOS' OptionParser from AK.

void OptionParser::resetState()
{
	argIndex = 0;
	parsedArgs = 0;
	currentMultiOptArgIndex = 0;
	stopOnFirstNonOption = false;
}

GetOptionResult OptionParser::getOption(const tcb::span<tiny_string>& args, const tiny_string& shortOptions, const tcb::span<const Option>& longOptions, int* outLongOptionIndex)
{
	this->args = args;
	this->shortOptions = shortOptions;
	this->longOptions = longOptions;
	this->outLongOptionIndex = outLongOptionIndex;

	stopOnFirstNonOption = shortOptions.startsWith("+");

	bool reorderArgv = !stopOnFirstNonOption;
	int res = -1;

	bool optionFound = findNextOption();
	auto arg = currentArg();

	// Do we have an option?
	if (!optionFound)
	{
		res = -1;
		parsedArgs = arg == "--";
	}
	else
	{
		res = arg.startsWith("--") ? handleLongOption() : handleShortOption();

		// Did we encounter an error?
		if (res == '?')
		{
			// Return immediately.
			return GetOptionResult
			{
				.result = '?',
				.optValue = optValue,
				.argValue = argValue,
				.parsedArgs = 0,
			};
		}
	}

	if (reorderArgv)
		shiftArgv();

	argIndex += parsedArgs;

	return GetOptionResult
	{
		.result = res,
		.optValue = optValue,
		.argValue = argValue,
		.parsedArgs = parsedArgs,
	};
}

// TODO: support multi-character short options.
OptionParser::ArgumentRequirement OptionParser::lookupShortOptionRequirement(char option) const
{
	auto parts = shortOptions.split(option);

	assert(parts.size() <= 2);
	if (parts.size() < 2)
	{
		// Couldn't find the option in the spec.
		return ArgumentRequirement::Invalid;
	}

	auto& part = parts[1];
	if (part[0] == ':')
	{
		// Two colons = optional argument.
		// One colon = required argument.
		return part[1] == ':' ? ArgumentRequirement::Optional : ArgumentRequirement::Required;
	}

	// This option doesn't have any arguments.
	return ArgumentRequirement::None;
}

int OptionParser::handleShortOption()
{
	tiny_string arg = currentArg();
	assert(arg.startsWith("-"));

	// Skip the "-", if we just started parsing this argument.
	currentMultiOptArgIndex += !currentMultiOptArgIndex;
	char option = arg[currentMultiOptArgIndex++];

	auto requirement = lookupShortOptionRequirement(option);
	if (requirement == ArgumentRequirement::Invalid)
	{
		optValue = option;
		std::cerr << "Unrecognized option -" << option << std::endl;
		return '?';
	}

	// Are we not at the end of this argument?
	if (currentMultiOptArgIndex < arg.numBytes())
	{
		if (requirement == ArgumentRequirement::None)
		{
			argValue = "";
			parsedArgs = 0;
		}
		else
		{
			// Treat everything after the first character as the value, i.e. "-ovalue".
			argValue = args[argIndex].substr_bytes(currentMultiOptArgIndex, UINT32_MAX);
			// Process the next argument, next time.
			currentMultiOptArgIndex = 0;
			parsedArgs = 1;
		}
	}
	else
	{
		currentMultiOptArgIndex = 0;
		if (requirement != ArgumentRequirement::Required)
		{
			argValue = "";
			parsedArgs = 1;
		}
		else if (argIndex + 1 < args.size())
		{
			// Treat the next argument as the value, i.e. "-o value".
			argValue = args[argIndex + 1];
			parsedArgs = 2;
		}
		else
		{
			std::cerr << "Missing value for option -" << option << std::endl;
			return '?';
		}
	}

	return option;
}

Option* OptionParser::lookupLongOption(const tiny_string& arg) const
{
	for (size_t i = 0; i < longOptions.size(); ++i)
	{
		auto& option = longOptions[i];
		if (!arg.startsWith(option.name))
			continue;

		if (arg.numBytes() >= option.name.numBytes())
		{
			setLongOptionIndex(i);
			argValue = arg[option.name.numBytes()] == '=' ? arg.substr_bytes(option.name.numBytes() + 1, UINT32_MAX) : "";
			return &option;
		}
	}
	return nullptr;
}

int OptionParser::handleLongOption()
{
	assert(currentArg().startsWith("--"));

	optValue = 0;

	auto option = lookupLongOption(args[argIndex].substr(2, UINT32_MAX));

	if (option == nullptr)
	{
		std::cerr << "Unrecognized option " << args[argIndex] << std::endl;
		return '?';
	}

	assert(option->requirement != ArgumentRequirement::Invalid);
	switch (option->requirement)
	{
		case ArgumentRequirement::None;
			if (!argValue.empty())
			{
				std::cerr << "Option --" << option->name << " doesn't accept an argument" << std::endl;
				return '?';
			}
			parsedArgs = 1;
			break;
		case ArgumentRequirement::Optional:
			parsedArgs = 1;
			break;
		case ArgumentRequirement::Required:
			if (!argValue.empty())
			{
				// Value was given in the form "--option=value".
				parsedArgs = 1;
			}
			else if (argIndex + 1 < args.size())
			{
				// Treat the next argument as the value, in the form "--option value".
				argValue = args[argIndex + 1];
				parsedArgs = 2;
			}
			else
			{
				std::cerr << "Missing value for option --" << option->name << " doesn't accept an argument" << std::endl;
				return '?';
			}
			break;
		default:
			break;
	}

	// Should we not report this option to our caller?
	if (option->flag != nullptr)
	{
		*option->flag = option->val;
		return 0;
	}
	return option->val;
}

void OptionParser::shiftArgv()
{
	// We just parsed an option (which might have a value).
	// So put it (and it's value, if it has one) in front of everything else.
	if (!parsedArgs && !skippedArgs)
	{
		// Nothing left to do.
		return;
	}

	// Before:
	// "arg" -o foo bar baz
	//       ------ parsed
	// After:
	// -o foo "arg" bar baz

	auto argStart = std::next(args.begin(), argIndex);
	auto argEnd = std::next(argStart, parsedArgs);
	std::rotate(args.begin(), argStart, argEnd);

	// `argIndex` takes `skippedArgs` into account (both are incremented in `findNextOption()`),
	// so point `argIndex` to the start of the skipped arguments.
	argIndex -= skippedArgs;
	skippedArgs = 0;
}

bool OptionParser::findNextOption()
{
	for (skippedArgs = 0; argIndex < args.size(); ++skippedArgs, ++argIndex)
	{
		tiny_string arg = currentArg();

		// Anything that doesn't start with "-" isn't an option.
		// A single "-" is a special case, since it's typically
		// used by programs to refer to stdin.
		if (!arg.startsWith("-") || arg == "-")
		{
			if (stopOnFirstNonOption)
				return false;
			continue;
		}

		// "--" is also a special case, since it's used to
		// explicitly mark the end of option parsing.
		if (arg == "--")
			return false;
		// Otherwise, we found an option.
		return true;
	}

	// We reached the end, but found no options.
	return false;
}
