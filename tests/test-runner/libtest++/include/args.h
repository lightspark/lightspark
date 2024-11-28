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

#ifndef LIBTESTPP_ARGS_H
#define LIBTESTPP_ARGS_H 1

#include <cstdint>
#include <cstdlib>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>
#include <map>
#include <vector>

using namespace lightspark;

class ArgsParser;

// Ported from the rust crate `libtest-mimic`.

// TODO: Make this more portable, and move it into it's own repo.

namespace libtestpp {

struct Trial;

// Possible values for the `--color` option.
enum ColorSetting {
	// Colorize output, if `stdout` is a tty, and test are ran serially
	// (default).
	Auto,
	// Always colorize output.
	Always,
	// Never colorize output.
	Never,
};

// Possible values for the `-Z` option
enum UnstableFlags {
	UnstableOptions,
};

// Possible values for the `--format` option.
enum FormatSetting {
	// One line per test. Human readable output (default).
	Pretty,
	// One character per test. Useful for large test suites.
	Terse,
	// JSON output.
	Json,
};

struct Arguments {
	// ==== FLAGS ====
	// Run ignored, and non-ignored tests.
	bool include_ignored { false };
	// Run only ignored tests.
	bool ignored { false };
	// Run tests, but not benchmarks.
	bool test { false };
	// Run benchmarks, but not tests.
	bool bench { false };
	// Only list all tests, and benchmarks.
	bool list { false };
	// No-op, ignored (libtest++ always runs in no-capture mode).
	bool no_capture { false };
	// No-op, ignored. libtest++ doesn't currently capture `stdout`.
	bool show_output { false };
	// No-op, ignored. Flag only exists for CLI compatibility with libtest, and libtest-mimic.
	Optional<UnstableFlags> unstable_flags;
	// If set, filters are matched exactly, rather than by substring.
	bool exact { false };
	// If set, display a single character per test, instead of a line.
	// Really useful for large test suites.
	//
	// This is an alias for `--format=terse`. If this is set, then
	// `format` will be `nullopt`.
	bool quiet { false };

	// ==== OPTIONS ====
	// Number of threads for parallel testing.
	Optional<size_t> test_threads;
	// Path of the log file. If specified, everything will be written into the
	// file, instead of `stdout`.
	Optional<tiny_string> log_file;
	// A list of filters. Tests whose names match any part of these
	// filters are skipped.
	std::vector<tiny_string> skip;
	// Specifies whether to color the output, or not.
	Optional<ColorSetting> color;
	// Specifies the output format.
	Optional<FormatSetting> format;

	// ==== POSITIONAL ARGUMENTS ====
	// Filter string. Only tests that contain this string get ran.
	Optional<tiny_string> filter;

	// ==== FUNCTIONS ====
	Arguments() {}
	Arguments(ArgsParser &args_parser) { create_options(args_parser); }
	Arguments(ArgsParser &args_parser, int argc, char **argv) : Arguments(from_args(args_parser, argc, argv)) {}
	Arguments(int argc, char **argv) : Arguments(from_args(argc, argv)) {}

	// Parses the command line arguments given to the application.
	//
	// If parsing fails (due to invalid args), an error is shown, and
	// the application exits. If help is requested (`-h`, or `--help`),
	// a help message is shown, and the application exits, as well.
	static Arguments from_args(ArgsParser &args_parser, int argc, char **argv);

	static Arguments from_args(int argc, char **argv);

	void parse(ArgsParser &args_parser, int argc, char **argv);
	void parse(int argc, char **argv);

	// Returns `true` if the given test should be ignored.
	bool is_ignored(const Trial &test) const;

	bool is_filtered_out(const Trial &test) const;
private:
	void create_options(ArgsParser &args_parser);
};

}

#endif /* LIBTESTPP_ARGS_H */
