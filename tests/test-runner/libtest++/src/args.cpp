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

#include "args.h"
#include "test_runner.h"
#include "utils/args_parser.h"
#include "version.h"

using namespace libtestpp;

Arguments Arguments::from_args(ArgsParser &args_parser, int argc, char **argv) {
	Arguments args(args_parser);
	args.parse(args_parser, argc, argv);
	return args;
}

Arguments Arguments::from_args(int argc, char **argv) {
	Arguments args;
	args.parse(argc, argv);
	return args;
}

void Arguments::parse(ArgsParser &args_parser, int argc, char **argv)
{
	create_options(args_parser);
	args_parser.parse(argc, argv);
}

void Arguments::parse(int argc, char **argv)
{
	ArgsParser args_parser("libtest++ test runner", LIBTESTPP_VERSION);
	parse(args_parser, argc, argv);
}

bool Arguments::is_ignored(const Trial &test) const {
	return (test.info.is_ignored && !ignored && !include_ignored)
	|| (test.is_bench() && this->test)
	|| (test.is_test() && bench);
}

bool Arguments::is_filtered_out(const Trial &test) const {
	tiny_string test_name = test.name();
	// Match against the full test name, including the kind. This upholds the invariant that if
	// --list prints out:
	//
	// <some string>: test
	//
	// then "--exact <some string>" runs exactly that test.
	auto test_name_with_kind = test.info.test_name_with_kind();

	// If a filter was specified, apply this.
	if (filter.hasValue()) {
		// For exact matches, we want to match against either the test
		// name, or the test kind.
		if (exact && test_name != *filter && test_name_with_kind != *filter) {
			return true;
		} else if (!exact && test_name_with_kind.find(*filter) == tiny_string::npos) {
			return true;
		}
	}

	// If any skip patterns were specified, test for all patterns.
	for (auto skip_filter : skip) {
		// For exact matches, we want to match against either the test
		// name, or the test kind.
		if (exact && (test_name == skip_filter || test_name_with_kind == skip_filter)) {
			return true;
		} else if (!exact && test_name_with_kind.find(skip_filter) != tiny_string::npos) {
			return true;
		}
	}

	return ignored && !test.info.is_ignored;
}

void Arguments::create_options(ArgsParser &args_parser) {
	// Flags.
	args_parser.addOption(include_ignored, "Run ignored tests", "include-ignored");
	args_parser.addOption(ignored, "Run ignored tests", "ignored");
	args_parser.addOption(test, "Only run tests", "test");
	args_parser.addOption(bench, "Only run benchmarks", "bench");
	args_parser.addOption(list, "List all tests, and benchmarks", "list");
	args_parser.addOption(no_capture, "No-op (libtest++ always runs in no-capture mode)", "nocapture");
	args_parser.addOption(show_output, "No-op, ignored. libtest++ doesn't currently capture stdout", "show-output");
	args_parser.addOption(
		unstable_flags,
		{ "unstable-options" },
		"No-op, ignored. Flag only exists for CLI compatibility with libtest, and libtest-mimic",
		nullptr,
		"Z",
		"UNSTABLE_FLAGS"
	);
	args_parser.addOption(exact, "Exactly match filters, rather than by substring", "exact");
	args_parser.addOption(quiet, "Display one character per test, instead of one line. Alias of --format=terse", "quiet", "q");
	// Options.
	args_parser.addOption(
		test_threads,
		"Number of threads used for running tests in parallel. If set to 1,\n"
		"then all tests run in the main thread",
		"test-threads",
		nullptr,
		//'\0',
		"TEST_THREADS"
	);
	args_parser.addOption(log_file, "Write logs to the specified file, instead of stdout", "logfile", nullptr, "PATH");
	args_parser.addOption(skip, "Skip tests whose names contain FILTER (this flag can be used multiple times)", "skip", nullptr, "FILTER");
	args_parser.addOption(
		color,
		{ "auto", "always", "never" },
		"Configure output coloring:\n"
		"- auto = Colorize if stdout is a tty, and tests are run serially (default)\n"
		"- always = Always colorize output\n"
		"- never = Never colorize output\n",
		"color"
	);
	args_parser.addOption(
		format,
		{ "pretty", "terse", "json" },
		"Configure output format:\n"
		"- pretty = Print verbose output (default)\n"
		"- terse = Display one character per test\n"
		"- json = Print JSON events\n",
		"format"
	);

	// Positional Arguments.
	args_parser.addPositionalArgument(
		filter,
		"The FILTER string is tested against the names of all tests, and only tests "
		"whose names contain the filter are ran.",
		"FILTER",
		false
	);
}
