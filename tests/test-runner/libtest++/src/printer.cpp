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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <termcolor/termcolor.hpp>

#include "args.h"
#include "color.h"
#include "printer.h"
#include "test_runner.h"

using namespace libtestpp;

tiny_string fmt_with_thousand_sep(uint64_t value);
Color color_of_outcome(const Outcome &outcome);
tiny_string json_escape(const tiny_string &str);

Printer::Printer(const Arguments &args, const std::vector<Trial> &tests) {
	auto color_arg = args.color.valueOr(ColorSetting::Auto);

	// Determine the output target.
	out = args.log_file.transformOr(&std::cout, [&](const tiny_string &log_file) {
		return new std::ofstream(log_file);
	});

	// Determine the color output.
	switch (color_arg) {
		case ColorSetting::Always: *out << termcolor::colorize; break;
		case ColorSetting::Never: *out << termcolor::nocolorize; break;
		default: break;
	}

	// Determine the correct format.
	format = (args.quiet) ? FormatSetting::Terse : args.format.valueOr(FormatSetting::Pretty);

	// Determine the max test name length, to do nice formating later.
	//
	// Unicode is hard, and there's no way to properly align/pad the test
	// names, and outcomes. Counting the number of code points is a cheap
	// way that works in most cases. Usually, these names are ASCII.
	auto it = std::max_element(
		tests.cbegin(),
		tests.cend(),
		[](const Trial& test_a, const Trial& test_b) {
			return test_a.info.name.numChars() < test_b.info.name.numChars();
		}
	);
	name_width = (it != tests.cend()) ? it->info.name.numChars() : 0;

	it = std::max_element(
		tests.cbegin(),
		tests.cend(),
		[](const Trial& test_a, const Trial& test_b) {
			auto a = (!test_a.info.kind.empty()) ? test_a.info.kind.numChars() + 3 : 0;
			auto b = (!test_b.info.kind.empty()) ? test_b.info.kind.numChars() + 3 : 0;
			return a < b;
		}
	);
	// Add 3 to account for the two square brackets [], and space.
	kind_width = (it != tests.cend()) ? it->info.kind.numChars() + 3 : 0;
}

Printer::~Printer() {
	if (out != &std::cout) {
		delete out;
	}
}

void Printer::print_title(size_t num_tests) {
	switch (format) {
		case FormatSetting::Pretty:
		case FormatSetting::Terse: {
			tiny_string plural_s = (num_tests == 1) ? "" : "s";
			*out << std::endl << "running " << num_tests << " test" << plural_s << std::endl;
			break;
		}
		case FormatSetting::Json:
			*out << R"({ "type": "suite", "event": "started", "test_count": )" << num_tests << " }" << std::endl;
			break;
	}
}

void Printer::print_test(const TestInfo &info) {
	switch (format) {
		case FormatSetting::Pretty: {
			tiny_string kind = (info.kind.empty()) ? "" : tiny_string::fromChar('[') + info.kind + "] ";

			*out << "test " << std::left << std::setw(2) << kind << std::left << std::setw(3) << info.name << " ... ";
			out->flush();
			break;
		}
		case FormatSetting::Terse:
			// In terse mode, nothing is printed before the job. Only
			// `print_single_outcome` prints one character.
			break;
		case FormatSetting::Json:
			*out << R"({ "type": "test", "event": "started", "name": ")" << json_escape(info.name) << R"(" })" << std::endl;
			break;
	}
}

void Printer::print_single_outcome(const TestInfo &info, const struct Outcome &outcome) {
	using OutcomeType = Outcome::Type;
	switch (format) {
		case FormatSetting::Pretty:
			print_outcome_pretty(outcome);
			*out << std::endl;
			break;
		case FormatSetting::Terse: {
			if (outcome.type == OutcomeType::Measured) {
				// Benchmarks are never printed in terse mode... for
				// some reason.
				print_outcome_pretty(outcome);
				*out << std::endl;
				break;
			}
			char c = 'b';
			switch (outcome.type) {
				case OutcomeType::Passed: c = '.'; break;
				case OutcomeType::Failed: c = 'F'; break;
				case OutcomeType::Ignored: c = 'i'; break;
				default: break;
			}
			*out << color_of_outcome(outcome) << c << Color::Reset;
			break;
		}
		case FormatSetting::Json: {
			bool is_bench = outcome.type == OutcomeType::Measured;
			*out << R"({ "type": ")" << (is_bench ? "bench" : "test") << R"(", "name": ")" << json_escape(info.name) << R"(", )";
			if (outcome.type == OutcomeType::Measured) {
				auto name = info.name;
				auto avg = outcome.measured.avg;
				auto variance = outcome.measured.variance;
				*out << R"("median": )" << avg <<  R"(, "deviation": )" << variance << " }" << std::endl;
				break;
			} else {
				tiny_string outcome_str;
				switch (outcome.type) {
					case OutcomeType::Passed: outcome_str = "ok"; break;
					case OutcomeType::Failed: outcome_str = "failed"; break;
					case OutcomeType::Ignored: outcome_str = "ignored"; break;
					default: break;
				}
				*out << R"("event": ")" << outcome_str;
				if (outcome.type == OutcomeType::Failed) {
					auto msg = outcome.failed.msg.valueOr("");
					*out <<  R"(, "stdout": "Error: \" )" << json_escape(msg) << R"(\"\n")";
				}
				*out << " }" << std::endl;
			}
			break;
		}
	}
}

void Printer::print_summary(const Conclusion &conclusion, double execution_time) {
	using OutcomeType = Outcome::Type;
	switch (format) {
		case FormatSetting::Pretty:
		case FormatSetting::Terse:
			*out << std::endl << "test result: ";
			print_outcome_pretty((conclusion.has_failed()) ? Outcome(Failed {}) : Outcome(OutcomeType::Passed));
			*out << ". " <<
			conclusion.num_passed << " passed; " <<
			conclusion.num_failed << " failed; " <<
			conclusion.num_ignored << " ignored; " <<
			conclusion.num_measured << " measured; " <<
			conclusion.num_filtered_out << " filtered out; " <<
			"finished in " << std::fixed << std::setprecision(2) << execution_time << 's' << std::endl << std::endl;
			break;
		case FormatSetting::Json: {
			auto event_str = (conclusion.num_failed > 0) ? "failed" : "ok";
			*out << R"({ "type": "suite", "event": ")" << event_str << '\"' <<
			R"(, "passed": )" << conclusion.num_passed <<
			R"(, "failed": )" << conclusion.num_failed <<
			R"(, "ignored": )" << conclusion.num_ignored <<
			R"(, "measured": )" << conclusion.num_measured <<
			R"(, "filtered_out": )" << conclusion.num_filtered_out <<
			R"(, "exec_time": )" << execution_time << " }" << std::endl;
			break;
		}
	}
}

void Printer::print_list(const std::vector<Trial> &tests, bool ignored) {
	for (auto test : tests) {
		auto info = test.info;
		// libtest, and libtest-mimic print out:
		// * all tests without `--ignored`
		// * just the ignored tests with `--ignored`
		if (ignored && !info.is_ignored) {
			continue;
		}

		tiny_string kind;
		if (!info.kind.empty()) {
			std::stringstream s;
			s << '[' << info.kind << "] ";
			kind = s.str();
		}

		*out << kind << info.name << ": " << ((info.is_bench) ? "bench" : "test") << std::endl;
	}
}

void Printer::print_failures(const std::vector<std::pair<TestInfo, Optional<tiny_string>>> &fails) {
	if (format == FormatSetting::Json) {
		return;
	}

	*out << std::endl << "failures:" << std::endl << std::endl;

	// Print messages of all tests.
	for (auto it : fails) {
		auto test_info = it.first;
		auto msg = it.second;
		*out << "---- " << test_info.name << " ----" << std::endl;
		(void)msg.andThen([&](const tiny_string &msg) -> Optional<tiny_string> {
			*out << msg << std::endl;
			return {};
		});
		*out << std::endl;
	}

	// Print summary list of failed tests.
	*out << std::endl << "failures:" << std::endl << std::endl;
	for (auto it : fails) {
		auto test_info = it.first;
		*out << "    " << test_info.name << std::endl;
	}
}

void Printer::print_outcome_pretty(const Outcome &outcome) {
	using OutcomeType = Outcome::Type;

	tiny_string outcome_str;
	switch (outcome.type) {
		case OutcomeType::Passed: outcome_str = "ok"; break;
		case OutcomeType::Failed: outcome_str = "FAILED"; break;
		case OutcomeType::Ignored: outcome_str = "ignored"; break;
		case OutcomeType::Measured: outcome_str = "bench"; break;
	}

	*out << color_of_outcome(outcome) << outcome_str << Color::Reset;

	if (outcome.type == OutcomeType::Measured) {
		auto avg = outcome.measured.avg;
		auto variance = outcome.measured.variance;
		*out << ": " <<
		std::left << std::setw(11) << fmt_with_thousand_sep(avg) <<
		" ns/iter (+/- " << fmt_with_thousand_sep(variance) << ')';
	}
}

// Formats the given integer with `,` as the thousands separater.
tiny_string fmt_with_thousand_sep(uint64_t value) {
	std::stringstream out;
	for (; value >= 1000; value /= 1000) {
		out << ',' << std::setfill('0') << std::setw(3) << value % 1000;
	}
	out << value;
	return out.str();
}

// Returns the `Color` associated with the given outcome.
Color color_of_outcome(const Outcome &outcome) {
	using OutcomeType = Outcome::Type;
	switch (outcome.type) {
		case OutcomeType::Passed: return Color::Green; break;
		case OutcomeType::Failed: return Color::Red; break;
		case OutcomeType::Ignored: return Color::Yellow; break;
		case OutcomeType::Measured: return Color::Cyan; break;
	}
	return Color::Reset;
}

bool is_safe_json_char(uint32_t c) {
	// Non-escaped JSON characters must be in one of the following ranges:
	// 0x20-0x21, 0x23-0x5B, 0x5D-0x10FFFF
	return (c != '\"' && c != '\\' && c <= 0x10FFFF);
}

void force_json_escape(uint32_t c, tiny_string &out) {
	switch (c) {
		case '\b':
			out += "\\b";
			break;
		case '\f':
			out += "\\f";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		case '\"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		default:
			// JSON natively supports unicode codepoints, so there's no
			// need to convert most of them into a `\uXXXX` sequence,
			// with the exception of the ASCII control characters, which
			// never require more than a single `\uXXXX` sequence.
			if (c < 0x20) {
				std::stringstream s;
				s << "\\u" << std::hex << std::setfill('0') << std::setw(4) << c;
				out += s.str();
			}
			break;
	}
}

tiny_string json_escape(const tiny_string &str) {
	tiny_string ret;

	for (auto c : str) {
		if (is_safe_json_char(c))
			ret += c;
		else
			force_json_escape(c, ret);
	}

	return ret;
}
