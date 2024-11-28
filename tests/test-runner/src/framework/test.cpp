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

#include <cassert>
#include <fstream>
#include <memory>
#include <lightspark/utils/impl.h>

#include "framework/test.h"
#include "framework/runner.h"

#include "input/formats/ruffle/parser.h"
#include "input/injector.h"
#include "input/parser.h"

static tiny_string toInputFileName(const TestFormat& testFormat)
{
	switch (testFormat)
	{
		// Default to our own test format, if we encounter an unknown format.
		case TestFormat::Unknown:
		case TestFormat::Lightspark: return "input.lsm"; break;
		case TestFormat::Ruffle: return "input.json"; break;
		// TODO: Implement this.
		case TestFormat::Shumway: return ""; break;
		// TODO: Implement this.
		case TestFormat::AVMPlus: return ""; break;
	}
	return "";
}

static ::Impl<InputParser> toInputParser(const TestFormat& testFormat, const path_t& inputPath)
{
	switch (testFormat)
	{
		// Default to our own test format, if we encounter an unknown format.
		case TestFormat::Unknown:
		// TODO: Implement our own test format.
		case TestFormat::Lightspark: return Impl<InputParser>(); break;
		case TestFormat::Ruffle: return RuffleInputParser(inputPath); break;
		// TODO: Implement this.
		case TestFormat::Shumway: return Impl<InputParser>(); break;
		// TODO: Implement this.
		case TestFormat::AVMPlus: return Impl<InputParser>(); break;
	}
	return Impl<InputParser>();
}

Test::Test
(
	const TestOptions& _options,
	const path_t& testDir,
	const tiny_string& swfFile,
	const tiny_string& _name,
	const TestFormat& _format
) :
options(_options),
swfPath(testDir/swfFile),
inputPath(testDir/toInputFileName(_format)),
outputPath(options.getOutputPath(testDir)),
rootPath(testDir),
name(_name),
format(_format)
{
	auto& playerOptions = options.playerOptions;
	type = playerOptions.renderOptions.transformOr
	(
		playerOptions.hasAudio ? Type::Audio : Type::Trace,
		[&](const RenderOptions& renderOptions)
		{
			if (renderOptions.required)
				return playerOptions.hasVideo ? Type::Video : Type::Rendering;
			return Type::Trace;
		}
	);
}

TestRunner Test::createTestRunner(bool debug, const LOG_LEVEL& logLevel) const
{
	return TestRunner
	(
		*this,
		inputInjector(),
		debug,
		logLevel
	);
}

InputInjector Test::inputInjector() const
{
	bool hasInputFile = fs::is_regular_file(inputPath);
	return hasInputFile ? InputInjector(toInputParser(format, inputPath)) : InputInjector();
}
