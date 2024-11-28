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
#include <chrono>
#include <concurrentqueue/blockingconcurrentqueue.h>
#include <sstream>
#include <ThreadPool/ThreadPool.h>
#include <vector>
#include <utility>

#include "args.h"
#include "printer.h"
#include "test_runner.h"

using namespace libtestpp;
using OutcomeType = Outcome::Type;
using FailedPair = std::pair<TestInfo, Optional<tiny_string>>;

Outcome run_single(const Trial::Runner &runner, bool test_mode);

Trial Trial::test(const tiny_string &name, const test_runner_t &runner) {
	auto test_runner = [=](bool) {
		try {
			return runner().valueOr(Outcome(OutcomeType::Passed));
		} catch (std::exception &e) {
			return Outcome(Failed { tiny_string(e.what(), true) });
		}
	};

	return Trial {
		test_runner,
		TestInfo {
			// name
			name,
			// kind
			tiny_string(),
			// is_ignored
			false,
			// is_bench
			false
		}
	};
}

Trial Trial::bench(const tiny_string &name, const bench_runner_t &runner) {
	auto test_runner = [=](bool test_mode) {
		try {
			auto measurement = runner(test_mode);
			if (measurement.hasValue())
				return Outcome(*measurement);
			else if (test_mode)
				return Outcome(OutcomeType::Passed);
			else
				return Outcome(Failed { tiny_string("bench runner returned `nullopt` in bench mode", true) });
		} catch (std::exception &e) {
			return Outcome(Failed { tiny_string(e.what(), true) });
		}
	};

	return Trial {
		test_runner,
		TestInfo {
			// name
			name,
			// kind
			tiny_string(),
			// is_ignored
			false,
			// is_bench
			true
		}
	};
}

tiny_string TestInfo::test_name_with_kind() const {
	if (kind.empty()) {
		return name;
	} else {
		std::stringstream s;
		s << '[' << kind << "] " << name;
		return s.str();
	}
}

Outcome::Outcome(const Outcome::Type &_type) : type(_type) {
	assert(type != OutcomeType::Failed || type != OutcomeType::Measured);
}

LIBTESTPP_NORETURN void Conclusion::exit() const {
	exit_if_failed();
	std::exit(0);
}

void Conclusion::exit_if_failed() const {
	if (has_failed()) {
		std::exit(101);
	}
}

namespace libtestpp {
LIBTESTPP_NODISCARD Conclusion run(const Arguments &args, std::vector<Trial> &tests) {
	auto start_time = std::chrono::steady_clock::now();
	Conclusion conclusion;

	// Apply filtering.
	if (args.filter.hasValue() || !args.skip.empty() || args.ignored) {
		auto size_before = tests.size();

		tests.erase(
			std::remove_if(
				tests.begin(),
				tests.end(),
				[&](const Trial &test) {
					return args.is_filtered_out(test);
				}
			),
			tests.end()
		);

		conclusion.num_filtered_out = size_before - tests.size();
	}

	// Create the printer that'll be used for all output.
	Printer printer(args, tests);

	// If `--list` is specified, just print the list, and return.
	if (args.list) {
		printer.print_list(tests, args.ignored);
		return Conclusion {};
	}

	// Print the number of tests.
	printer.print_title(tests.size());

	std::vector<FailedPair> failed_tests;
	auto handle_outcome = [&](const Outcome &outcome, const TestInfo &test) {
		printer.print_single_outcome(test, outcome);

		// Handle outcome.
		switch (outcome.type) {
			case OutcomeType::Passed: conclusion.num_passed++; break;
			case OutcomeType::Failed:
				failed_tests.push_back(std::make_pair(test, outcome.failed.msg));
				conclusion.num_failed++;
				break;
			case OutcomeType::Ignored: conclusion.num_ignored++; break;
			case OutcomeType::Measured: conclusion.num_measured++; break;
		}
	};

	// Execute all tests.
	bool test_mode = !args.bench;
	if (args.test_threads.hasValue() && args.test_threads.getValue() == 1) {
		// Run tests on the main thread.
		for (auto test : tests) {
			// Print `test foo    ...`, run the test, and print the
			// outcome on the same line.
			printer.print_test(test.info);
			handle_outcome(
				(!args.is_ignored(test)) ? run_single(
					test.runner,
					test_mode
				) : Outcome(OutcomeType::Ignored),
				test.info
			);
		}
	} else {
		ThreadPool pool(args.test_threads.valueOr(std::thread::hardware_concurrency()));
		moodycamel::BlockingConcurrentQueue<std::pair<Outcome, TestInfo>> queue;
		size_t num_tests = tests.size();
		for (auto test : tests) {
			if (args.is_ignored(test)) {
				queue.enqueue(std::make_pair(
					Outcome(OutcomeType::Ignored),
					test.info
				));
			} else {
				pool.enqueue(NoFutureTag {}, [=, &queue](const Trial &test) {
					auto outcome = run_single(test.runner, test_mode);
					queue.enqueue(std::make_pair(outcome, test.info));
				}, test);
			}
		}

		for (size_t i = 0; i < num_tests; ++i) {
			Outcome outcome;
			TestInfo test_info;

			auto item = std::tie(outcome, test_info);
			queue.wait_dequeue(item);

			// In multi-threading mode, only print the start of the
			// line, after the test ran, otherwise it'd lead to terribly
			// interleaved output.
			printer.print_test(test_info);
			handle_outcome(outcome, test_info);
		}
	}

	// Print any failures, and the final summary.
	if (!failed_tests.empty()) {
		printer.print_failures(failed_tests);
	}

	std::chrono::duration<double> execution_time = std::chrono::steady_clock::now() - start_time;
	printer.print_summary(conclusion, execution_time.count());
	return conclusion;
}
}

// Runs the given runner, catching any unhandled exceptions, and treating
// them as failures.
Outcome run_single(const Trial::Runner &runner, bool test_mode) {
	try {
		return runner(test_mode);
	} catch (std::exception &e) {
		std::stringstream s;
		s << "test panicked";

		if (*e.what() != '\0') {
			s << ": " << e.what();
		}

		return Outcome(Failed { tiny_string(s.str()) });
	}
}
