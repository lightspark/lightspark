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

#ifndef LIBTESTPP_TEST_RUNNER_H
#define LIBTESTPP_TEST_RUNNER_H 1

#include <cstdint>
#include <functional>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>
#include <vector>

#include "config.h"

using namespace lightspark;

// Ported from the rust crate `libtest-mimic`.

// TODO: Make this more portable, and move it into it's own repo.

namespace libtestpp {

struct Arguments;
struct Outcome;
struct Measurement;

struct TestInfo {
	tiny_string name;
	tiny_string kind;
	bool is_ignored;
	bool is_bench;

	tiny_string test_name_with_kind() const;
};

// A single test, or benchmark.
struct Trial {
	using Runner = std::function<Outcome(bool)>;
	using test_runner_t = std::function<Optional<struct Outcome>()>;
	using bench_runner_t = std::function<Optional<Measurement>(bool)>;

	Runner runner;
	TestInfo info;

	// Creates a (non-benchmark) test with the given name, and runner.
	//
	// The runner throwing an exception is considered a failure.
	static Trial test(const tiny_string &name, const test_runner_t &runner);

	// Creates a benchmark with the given name, and runner.
	//
	// If the runner's parameter `test_mode` is `true`, the runner function
	// should run all code just once, without measuring, just to make sure it
	// doesn't throw. If the parameter is `false`, it should perform the
	// actual benchmark. If `test_mode` is `true`, you may return `nullopt`,
	// but if it's `false`, you have to return a `Measurement`, or else the
	// benchmark is considered a failure.
	//
	// `test_mode` is `true` if neither `--bench`, nor `--test` are set, and
	// `false` when `--bench` is set, if `--test` is set, benchmarks aren't
	// ran at all, and both flags can't be set at the same time.
	static Trial bench(const tiny_string &name, const bench_runner_t &runner);

	// Sets the "kind" of this test/benchmark. If this string isn't
	// empty, it's printed in square brackets before the test name
	// (e.g. `test [my-kind] test_name`). (Default: *empty*)
	Trial &with_kind(const tiny_string &kind) {
		info.kind = kind;
		return *this;
	}

	// Sets whether, or not this test is considered "ignored". (Default: `false`)
	//
	// If the `--ignored` flag is set, ignored tests are executed.
	Trial &with_ignored_flag(bool is_ignored) {
		info.is_ignored = is_ignored;
		return *this;
	}

	// Returns the name of this trial.
	constexpr const tiny_string &name() const { return info.name; }

	// Returns the kind of this trial. If you haven't set a kind, this will be
	// an empty string.
	constexpr const tiny_string &kind() const { return info.kind; }

	// Returns whether this trial has been marked as *ignored*.
	constexpr bool has_ignored_flag() const { return info.is_ignored; }

	// Returns `true` if this trial is a test (as opposed to a benchmark).
	constexpr bool is_test() const { return !info.is_bench; }

	// Returns `true` if this trial is a benchmark (as opposed to a test).
	constexpr bool is_bench() const { return info.is_bench; }
};

// Output of a benchmark.
struct Measurement {
	// Average time in ns.
	uint64_t avg;

	// Variance in ns.
	uint64_t variance;
};

// Indicates that a test/benchmark has failed. Optionally carries a message.
struct Failed {
	Optional<tiny_string> msg;
};

// The outcome of performing a test/benchmark.
struct Outcome {
	enum class Type {
		// The test passed.
		Passed,

		// The test, or benchmark failed.
		Failed,

		// The test, or benchmark was ignored.
		Ignored,

		// The benchmark was successfully ran.
		Measured,
	};

	Type type;
	union {
		Failed failed;
		Measurement measured;
	};

	Outcome() {}
	Outcome(const Outcome &other) : type(other.type) {
		switch (type) {
			case Type::Failed: new(&failed) Failed(other.failed); break;
			case Type::Measured: new(&measured) Measurement(other.measured); break;
			default: break;
		}
	}
	Outcome(const Type &_type);
	Outcome(const Failed &_failed) : type(Type::Failed), failed(_failed) {}
	constexpr Outcome(const Measurement &_measured) : type(Type::Measured), measured(_measured) {}
	~Outcome() {
		if (type == Type::Failed) {
			failed.~Failed();
		}
	}

	Outcome &operator=(const Outcome &other) {
		type = other.type;
		switch (type) {
			case Type::Failed: new(&failed) Failed(other.failed); break;
			case Type::Measured: new(&measured) Measurement(other.measured); break;
			default: break;
		}
		return *this;
	}
};

// Contains information about the entire test run. Returned by `run()`.
struct Conclusion {
	// Number of tests, and benchmarks that were filtered out (either by the
	// filter-in pattern, or by `--skip` arguments).
	uint64_t num_filtered_out { 0 };

	// Number of passed tests.
	uint64_t num_passed { 0 };

	// Number of failed tests, and benchmarks.
	uint64_t num_failed { 0 };

	// Number of ignored tests, and benchmarks.
	uint64_t num_ignored { 0 };

	// Number of benchmarks that successfully ran.
	uint64_t num_measured { 0 };

	// Returns an exit code that can be returned from `main` to signal
	// success/failure to the calling process.
	constexpr int exit_code() const { return (has_failed()) ? 101 : 0; }

	// Returns whether there have been any failures.
	constexpr bool has_failed() const { return num_failed > 0; }

	// Exits the application with an appropriate error code (0 if all tests
	// have passed, 101 if there have been failures). This uses
	// `std::exit()`, meaning that local destructors aren't called. Consider
	// using `exit_code()` instead for a proper program cleanup.
	LIBTESTPP_NORETURN void exit() const;

	// Exits the application with error code 101 if there were any failures.
	// Otherwise, returns normally. This uses `std::exit()`, meaning that local
	// destructors aren't called. Consider using `exit_code()` instead for
	// a proper program cleanup.
	void exit_if_failed() const;
};


// Runs all given trials (tests & benchmarks).
//
// This is the central function of this library. It provides the framework for
// the testing harness. It does all the printing, and house keeping.
//
// The returned value contains useful information. See
// `Conclusion` for more information. If `--list` was specified, a list is
// printed, and a dummy `Conclusion` is returned.
//
// This function is marked `nodiscard`.
LIBTESTPP_NODISCARD Conclusion run(const Arguments &args, std::vector<Trial> &tests);

}

#endif /* LIBTESTPP_TEST_RUNNER_H */
