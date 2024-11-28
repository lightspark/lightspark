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

#ifndef UTILS_ARGS_PARSER_H
#define UTILS_ARGS_PARSER_H 1

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <lightspark/tiny_string.h>
#include <list>
#include <map>
#include <sstream>
#include <tcb/span.hpp>
#include <type_traits>
#include <vector>

#include "utils/option_parser.h"

using namespace lightspark;

// Based on SerenityOS' ArgsParser from LibCore.

class ArgsParser
{
public:
	enum class FailureMode
	{
		PrintUsageAndExit,
		PrintUsage,
		Exit,
		Ignore,
	};

	enum class ArgumentMode
	{
		None,
		Optional,
		Required,
	};

	struct Option
	{
		ArgumentMode argMode { ArgumentMode::Required };
		const char* help { nullptr };
		const char* longName { nullptr };
		const char shortName { '\0' };
		const char* valueName { nullptr };
		std::function<tiny_string()> makeValueName {};
		std::function<bool(const tiny_string&)> acceptValue {};
		tiny_string nameForDisplay(const tiny_string& delim = ", ") const
		{
			std::stringstream s;
			bool needsDelim = shortName != '\0' && longName != nullptr;
			if (shortName != '\0')
				s << '-' << shortName;
			if (needsDelim)
				s << delim;
			if (longName != nullptr)
				s << "--" << longName;
			return s.str();
		}
		static OptionParser::ArgumentRequirement toArgumentRequirement(const ArgumentMode& mode)
		{
			switch (mode)
			{
				case ArgumentMode::Required: return OptionParser::ArgumentRequirement::Required; break;
				case ArgumentMode::Optional: return OptionParser::ArgumentRequirement::Optional; break;
				default: return OptionParser::ArgumentRequirement::None; break;
			}
		}

		constexpr bool hasArg() const { return argMode != ArgumentMode::None; }
		constexpr bool hasOptionalArg() const { return argMode == ArgumentMode::Optional; }
		constexpr bool hasRequiredArg() const { return argMode == ArgumentMode::Required; }
	};

	struct Arg
	{
		const char* help { nullptr };
		const char* name { nullptr };
		int minValues { 0 };
		int maxValues { 1 };
		std::function<bool(const tiny_string&)> acceptValue;
	};
private:
	std::vector<Option> options;
	std::vector<Arg> positionalArgs;
	tiny_string appName;
	tiny_string appVersion;
	tiny_string appAdditional;

	bool showHelp;
	bool showVersion;
	const char* generalHelp;
	bool stopOnFirstNonOption;
public:
	ArgsParser() = default;
	ArgsParser(const tiny_string& _appName, const tiny_string& _appVersion, const tiny_string& _appAdditional = "");

	bool parse(const tcb::span<tiny_string>& args, FailureMode failureMode = FailureMode::PrintUsageAndExit);
	bool parse(int argc, char* argv[], FailureMode failureMode = FailureMode::PrintUsageAndExit)
	{
		tcb::span<char*> argsSpan(argv, argc);
		std::vector<tiny_string> args(argsSpan.begin(), argsSpan.end());
		return parse(tcb::make_span(args), failureMode);
	}

	void setGeneralHelp(const char* help) { generalHelp = help; }
	void setStopOnFirstNonOption(bool flag) { stopOnFirstNonOption = flag; }
	template<typename CharT>
	void printUsage(std::basic_ostream<CharT>& stream, const tiny_string& argv0);
	template<typename CharT>
	void printVersion(std::basic_ostream<CharT>& stream);

	void addOption(Option&& option);
	void addIgnored(const char* longName, char shortName);
	void addOption(bool& value, const char* help, const char* longName, char shortName = '\0');

	template<typename T, std::enable_if_t<std::is_enum<T>::value, bool> = false>
	void addOption(T& value, T newValue, const char* help, const char* longName, char shortName = '\0')
	{
		addOption
		({
			.argMode = ArgumentMode::None,
			.help = help,
			.longName = longName,
			.shortName = shortName,
			.acceptValue = [&](const tiny_string&)
			{
				value = newValue;
				return true;
			}
		});
	}

	template<typename T, std::enable_if_t<(std::is_enum<T>::value || std::is_arithmetic<T>::value), bool> = false>
	void addOption
	(
		T& value,
		const std::vector<tiny_string>& nameList,
		const char* help,
		const char* longName,
		char shortName = '\0',
		const char* valueName = nullptr,
		char listDelim = '|'
	)
	{
		addOption
		({
			.argMode = ArgumentMode::None,
			.help = help,
			.longName = longName,
			.shortName = shortName,
			.makeValueName = [nameList, listDelim]
			{
				std::stringstream s;
				for (auto it = nameList.cbegin(); it != nameList.cend(); ++it)
				{
					if (it != nameList.cbegin())
						s << listDelim;
					s << *it;
				}
				return s.str();
			},
			.acceptValue = [&value, nameList](const tiny_string& str)
			{
				auto it = std::find(nameList.cbegin(), nameList.cend(), str);
				if (it != nameList.cend())
				{
					value = T(std::distance(nameList.cbegin(), it));
					return true;
				}
				return false;
			}
		});
	}

	template<typename T, std::enable_if_t<(std::is_enum<T>::value || std::is_arithmetic<T>::value), bool> = false>
	void addOption
	(
		T& value,
		const std::map<tiny_string, T>& nameList,
		const char* help,
		const char* longName,
		char shortName = '\0',
		const char* valueName = nullptr,
		char listDelim = '|'
	)
	{
		addOption
		({
			.argMode = ArgumentMode::None,
			.help = help,
			.longName = longName,
			.shortName = shortName,
			.makeValueName = [nameList, listDelim]
			{
				std::stringstream s;
				for (auto it = nameList.cbegin(); it != nameList.cend(); ++it)
				{
					if (it != nameList.cbegin())
						s << listDelim;
					s << (*it).first;
				}
				return s.str();
			},
			.acceptValue = [&value, nameList](const tiny_string& str)
			{
				auto it = nameList.find(str);
				if (it != nameList.cend())
				{
					value = (*it).second;
					return true;
				}
				return false;
			}
		});
	}

	template<typename T, std::enable_if_t<std::is_arithmetic<std::underlying_type_t<T>>::value, bool> = false>
	void addOption(T& value, const char* help, const char* longName, char shortName, const char* valueName)
	{
		Option option
		{
			.argMode = ArgumentMode::Required,
			.help = help,
			.longName = longName,
			.shortName = shortName,
			.valueName = valueName,
			.acceptValue = [&value](const tiny_string& str)
			{
				return str.toNumber<std::underlying_type_t<T>>((std::underlying_type_t<T>&)value);
			}
		};
		addOption(std::move(option));
	}

	template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = false>
	void addOption(std::vector<T>& values, const char* help, const char* longName, char shortName, const char* valueName, char delim = ',')
	{
		Option option
		{
			.argMode = ArgumentMode::Required,
			.help = help,
			.longName = longName,
			.shortName = shortName,
			.valueName = valueName,
			.acceptValue = [&values, delim](const tiny_string& s)
			{
				bool parsedAllValues = true;
				auto strings = s.split(delim);
				for (auto str : strings)
				{
					str.tryToNumber<T>().andThen([&](const T& value)
					{
						values.push_back(value);
						return Optional<T>(value);
					}).orElse([&]
					{
						parsedAllValues = false;
						return nullOpt;
					});
				}
				return parsedAllValues;
			}
		};

		addOption(std::move(option));
	}
	void addOption(tiny_string& value, const char* help, const char* longName, char shortName, const char* valueName);

	void addOption(std::vector<tiny_string>& values, const char* help, const char* longName, char shortName, const char* valueName);

	void addPositionalArgument(Arg&& arg);
	void addPositionalArgument(tiny_string& value, const char* help, const char* name, bool required = true);
	template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = false>
	void addPositionalArgument(T& value, const char* help, const char* name, bool required = true)
	{
		Arg arg
		{
			.help = help,
			.name = name,
			.required = required,
			.minValues = 1,
			.acceptValue = [&value](const tiny_string& str)
			{
				return str.toNumber<T>(value);
			}
		};
		addPositionalArgument(std::move(arg));
	}
	void addPositionalArgument(std::vector<tiny_string>& values, const char* help, const char* name, bool required = true);

};

#endif /* UTILS_ARGS_PARSER_H */
