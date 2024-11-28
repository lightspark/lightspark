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

#ifndef LIBTESTPP_PRINTER_H
#define LIBTESTPP_PRINTER_H 1

#include <cstdint>
#include <cstdlib>
#include <iosfwd>
#include <utility>
#include <vector>

using namespace lightspark;

namespace lightspark {

template<typename T>
class Optional;
class tiny_string;

};

// Ported from the rust crate `libtest-mimic`.

// TODO: Make this more portable, and move it into it's own repo.

namespace libtestpp {

struct Argument;
struct Conclusion;
struct Outcome;
struct TestInfo;
struct Trial;

struct Printer {
	std::ostream *out;
	FormatSetting format;
	size_t name_width;
	size_t kind_width;

	// Creates a new printer configured by the given arguments (`format`,
	// `quiet`, `color`, and `logfile` options).
	Printer(const Arguments &args, const std::vector<Trial> &tests);
	~Printer();

	// Prints the first line "running n tests".
	void print_title(size_t num_tests);

	// Prints the text announcing the test (e.g. "test foo::bar ..."). Prints
	// nothing in terse mode.
	void print_test(const TestInfo &info);

	// Prints the outcome of a single test. `ok`, or `FAILED` in pretty mode,
	// and `.`, or `F` in terse mode.
	void print_single_outcome(const TestInfo &info, const Outcome &outcome);

	// Prints the summary line after all tests have been executed.
	void print_summary(const Conclusion &conclusion, double execution_time);

	// Prints a list of all tests. Used if `--list` is set.
	void print_list(const std::vector<Trial> &tests, bool ignored);

	// Prints a list of failed tests, with their messages. This is only called
	// if there were any failures.
	void print_failures(const std::vector<std::pair<TestInfo, Optional<tiny_string>>> &fails);

	// Prints a colored 'ok'/'FAILED'/'ignored'/'bench'.
	void print_outcome_pretty(const Outcome &outcome);
};

}

#endif /* LIBTESTPP_PRINTER_H */
