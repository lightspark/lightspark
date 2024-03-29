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

#ifndef UTILS_OPTION_PARSER_H
#define UTILS_OPTION_PARSER_H 1

#include <cstdlib>
#include <lightspark/tiny_string.h>
#include <tcb/span.hpp>
#include <type_traits>

// Based on SerenityOS' OptionParser from AK.

class OptionParser
{
public:
	enum class ArgumentRequirement
	{
		None,
		Optional,
		Required,
		Invalid = -1,
	};

	struct Option
	{
		tiny_string name;
		ArgumentRequirement requirement { ArgumentRequirement::Invalid };
		int* flag { nullptr };
		int val { 0 };
	};

	struct GetOptionResult
	{
		int result;
		int optValue;
		tiny_string argValue;
		size_t parsedArgs;
	};
private:
	ArgumentRequirement lookupShortOptionRequirement(char option) const;
	int handleShortOption();
	Option lookupLongOption(const tiny_string& arg) const;
	int handleLongOption();

	void shiftArgv();
	bool findNextOption();

	tiny_string currentArg() const
	{
		if (argIndex >= args.size())
			return "";
		return args[argIndex];
	}

	void setLongOptionIndex(int index)
	{
		if (outLongOptionIndex != nullptr)
			*outLongOptionIndex = index;
	}

	tcb::span<tiny_string> args;
	tiny_string shortOptions;
	tcb::span<const Option> longOptions;
	mutable int* outLongOptionIndex { nullptr };
	mutable int optValue;
	mutable tiny_string argValue;

	size_t argIndex { 0 };
	size_t skippedArgs { 0 };
	size_t parsedArgs { 0 };
	size_t currentMultiOptArgIndex { 0 };
	bool stopOnFirstNonOption { false };
public:
	GetOptionResult getOption(const tcb::span<tiny_string>& args, const tiny_string& shortOptions, const tcb::span<const Option>& longOptions, int* outLongOptionIndex);
	void resetState();
};

#endif /* UTILS_OPTION_PARSER_H */
