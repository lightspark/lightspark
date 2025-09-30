/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024-2025  mr b0nk 500 (b0nk@b0nk.xyz)
    Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

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

#ifndef FRAMEWORK_TEST_H
#define FRAMEWORK_TEST_H 1

#include <lightspark/logger.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/path.h>

#include "framework/options.h"

using namespace lightspark;

class InputInjector;
struct TestRunner;

enum class TestFormat
{
	Unknown,
	Lightspark,
	Ruffle,
	Shumway,
	AVMPlus,
};

class TestRunnerException : public std::exception
{
public:
	TestRunnerException(const tiny_string& _reason) : reason(_reason) {}

	const char* what() const noexcept override { return reason.raw_buf(); }
private:
	tiny_string reason;
};

struct Test
{
	enum Type
	{
		Trace,
		Rendering,
		Audio,
		Video,
	};

	TestOptions options;
	Path swfPath;
	Path inputPath;
	Path outputPath;
	Path rootPath;
	tiny_string swfFilename;
	tiny_string name;
	TestFormat format;
	Type type;

	Test
	(
		const TestOptions& _options,
		const Path& testDir,
		const tiny_string& swfFile,
		const tiny_string& _name,
		const TestFormat& _format
	);

	TestRunner createTestRunner(bool debug, const LOG_LEVEL& logLevel) const;
	InputInjector inputInjector() const;
	bool shouldRun() const { return !options.ignore; }

};

#endif /* FRAMEWORK_TEST_H */
