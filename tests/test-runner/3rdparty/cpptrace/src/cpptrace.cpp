#include <cpptrace/cpptrace.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "symbols/symbols.hpp"
#include "unwind/unwind.hpp"
#include "demangle/demangle.hpp"
#include "utils/common.hpp"
#include "utils/microfmt.hpp"
#include "utils/utils.hpp"
#include "binary/object.hpp"
#include "binary/safe_dl.hpp"
#include "snippets/snippet.hpp"

namespace cpptrace {
    CPPTRACE_FORCE_NO_INLINE
    raw_trace raw_trace::current(std::size_t skip) {
        try { // try/catch can never be hit but it's needed to prevent TCO
            return generate_raw_trace(skip + 1);
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return raw_trace{};
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    raw_trace raw_trace::current(std::size_t skip, std::size_t max_depth) {
        try { // try/catch can never be hit but it's needed to prevent TCO
            return generate_raw_trace(skip + 1, max_depth);
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return raw_trace{};
        }
    }

    object_trace raw_trace::resolve_object_trace() const {
        try {
            return object_trace{detail::get_frames_object_info(frames)};
        } catch(...) { // NOSONAR
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return object_trace{};
        }
    }

    stacktrace raw_trace::resolve() const {
        try {
            std::vector<stacktrace_frame> trace = detail::resolve_frames(frames);
            for(auto& frame : trace) {
                frame.symbol = detail::demangle(frame.symbol);
            }
            return {std::move(trace)};
        } catch(...) { // NOSONAR
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return stacktrace{};
        }
    }

    void raw_trace::clear() {
        frames.clear();
    }

    bool raw_trace::empty() const noexcept {
        return frames.empty();
    }

    CPPTRACE_FORCE_NO_INLINE
    object_trace object_trace::current(std::size_t skip) {
        try { // try/catch can never be hit but it's needed to prevent TCO
            return generate_object_trace(skip + 1);
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return object_trace{};
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    object_trace object_trace::current(std::size_t skip, std::size_t max_depth) {
        try { // try/catch can never be hit but it's needed to prevent TCO
            return generate_object_trace(skip + 1, max_depth);
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return object_trace{};
        }
    }

    stacktrace object_trace::resolve() const {
        try {
            std::vector<stacktrace_frame> trace = detail::resolve_frames(frames);
            for(auto& frame : trace) {
                frame.symbol = detail::demangle(frame.symbol);
            }
            return {std::move(trace)};
        } catch(...) { // NOSONAR
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return stacktrace();
        }
    }

    void object_trace::clear() {
        frames.clear();
    }

    bool object_trace::empty() const noexcept {
        return frames.empty();
    }

    object_frame stacktrace_frame::get_object_info() const {
        return detail::get_frame_object_info(raw_address);
    }

    static std::string frame_to_string(
        bool color,
        const stacktrace_frame& frame
    ) {
        const auto reset   = color ? RESET : "";
        const auto green   = color ? GREEN : "";
        const auto yellow  = color ? YELLOW : "";
        const auto blue    = color ? BLUE : "";
        std::string str;
        if(frame.is_inline) {
            str += microfmt::format("{<{}}", 2 * sizeof(frame_ptr) + 2, "(inlined)");
        } else {
            str += microfmt::format("{}0x{>{}:0h}{}", blue, 2 * sizeof(frame_ptr), frame.raw_address, reset);
        }
        if(!frame.symbol.empty()) {
            str += microfmt::format(" in {}{}{}", yellow, frame.symbol, reset);
        }
        if(!frame.filename.empty()) {
            str += microfmt::format(" at {}{}{}", green, frame.filename, reset);
            if(frame.line.has_value()) {
                str += microfmt::format(":{}{}{}", blue, frame.line.value(), reset);
                if(frame.column.has_value()) {
                    str += microfmt::format(":{}{}{}", blue, frame.column.value(), reset);
                }
            }
        }
        return str;
    }

    std::string stacktrace_frame::to_string() const {
        return to_string(false);
    }

    std::string stacktrace_frame::to_string(bool color) const {
        return frame_to_string(color, *this);
    }

    std::ostream& operator<<(std::ostream& stream, const stacktrace_frame& frame) {
        return stream << frame.to_string();
    }

    CPPTRACE_FORCE_NO_INLINE
    stacktrace stacktrace::current(std::size_t skip) {
        try { // try/catch can never be hit but it's needed to prevent TCO
            return generate_trace(skip + 1);
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return stacktrace{};
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    stacktrace stacktrace::current(std::size_t skip, std::size_t max_depth) {
        try { // try/catch can never be hit but it's needed to prevent TCO
            return generate_trace(skip + 1, max_depth);
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return stacktrace{};
        }
    }

    void stacktrace::print() const {
        print(std::cerr, true);
    }

    void stacktrace::print(std::ostream& stream) const {
        print(stream, true);
    }

    void stacktrace::print(std::ostream& stream, bool color) const {
        print(stream, color, true, nullptr);
    }

    static void print_frame(
        std::ostream& stream,
        bool color,
        unsigned frame_number_width,
        std::size_t counter,
        const stacktrace_frame& frame
    ) {
        std::string line = microfmt::format("#{<{}} {}", frame_number_width, counter, frame.to_string(color));
        stream << line;
    }

    void stacktrace::print(std::ostream& stream, bool color, bool newline_at_end, const char* header) const {
        if(
            color && (
                (&stream == &std::cout && isatty(stdout_fileno)) || (&stream == &std::cerr && isatty(stderr_fileno))
            )
        ) {
            detail::enable_virtual_terminal_processing_if_needed();
        }
        stream << (header ? header : "Stack trace (most recent call first):") << '\n';
        std::size_t counter = 0;
        if(frames.empty()) {
            stream << "<empty trace>\n";
            return;
        }
        const auto frame_number_width = detail::n_digits(static_cast<int>(frames.size()) - 1);
        for(const auto& frame : frames) {
            print_frame(stream, color, frame_number_width, counter, frame);
            if(newline_at_end || &frame != &frames.back()) {
                stream << '\n';
            }
            counter++;
        }
    }

    void stacktrace::print_with_snippets() const {
        print_with_snippets(std::cerr, true);
    }

    void stacktrace::print_with_snippets(std::ostream& stream) const {
        print_with_snippets(stream, true);
    }

    void stacktrace::print_with_snippets(std::ostream& stream, bool color) const {
        print_with_snippets(stream, color, true, nullptr);
    }

    void stacktrace::print_with_snippets(std::ostream& stream, bool color, bool newline_at_end, const char* header) const {
        if(
            color && (
                (&stream == &std::cout && isatty(stdout_fileno)) || (&stream == &std::cerr && isatty(stderr_fileno))
            )
        ) {
            detail::enable_virtual_terminal_processing_if_needed();
        }
        stream << (header ? header : "Stack trace (most recent call first):") << '\n';
        std::size_t counter = 0;
        if(frames.empty()) {
            stream << "<empty trace>" << '\n';
            return;
        }
        const auto frame_number_width = detail::n_digits(static_cast<int>(frames.size()) - 1);
        for(const auto& frame : frames) {
            print_frame(stream, color, frame_number_width, counter, frame);
            if(newline_at_end || &frame != &frames.back()) {
                stream << '\n';
            }
            if(frame.line.has_value() && !frame.filename.empty()) {
                stream << detail::get_snippet(frame.filename, frame.line.value(), 2, color);
            }
            counter++;
        }
    }

    void stacktrace::clear() {
        frames.clear();
    }

    bool stacktrace::empty() const noexcept {
        return frames.empty();
    }

    std::string stacktrace::to_string(bool color) const {
        std::ostringstream oss;
        print(oss, color, false, nullptr);
        return std::move(oss).str();
    }

    std::ostream& operator<<(std::ostream& stream, const stacktrace& trace) {
        return stream << trace.to_string();
    }

    CPPTRACE_FORCE_NO_INLINE
    raw_trace generate_raw_trace(std::size_t skip) {
        try {
            return raw_trace{detail::capture_frames(skip + 1, SIZE_MAX)};
        } catch(...) { // NOSONAR
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return raw_trace{};
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    raw_trace generate_raw_trace(std::size_t skip, std::size_t max_depth) {
        try {
            return raw_trace{detail::capture_frames(skip + 1, max_depth)};
        } catch(...) { // NOSONAR
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return raw_trace{};
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    std::size_t safe_generate_raw_trace(frame_ptr* buffer, std::size_t size, std::size_t skip) {
        try { // try/catch can never be hit but it's needed to prevent TCO
            return detail::safe_capture_frames(buffer, size, skip + 1, SIZE_MAX);
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return 0;
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    std::size_t safe_generate_raw_trace(
         frame_ptr* buffer,
         std::size_t size,
         std::size_t skip,
         std::size_t max_depth
    ) {
        try { // try/catch can never be hit but it's needed to prevent TCO
            return detail::safe_capture_frames(buffer, size, skip + 1, max_depth);
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return 0;
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    object_trace generate_object_trace(std::size_t skip) {
        try {
            return object_trace{detail::get_frames_object_info(detail::capture_frames(skip + 1, SIZE_MAX))};
        } catch(...) { // NOSONAR
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return object_trace{};
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    object_trace generate_object_trace(std::size_t skip, std::size_t max_depth) {
        try {
            return object_trace{detail::get_frames_object_info(detail::capture_frames(skip + 1, max_depth))};
        } catch(...) { // NOSONAR
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return object_trace{};
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    stacktrace generate_trace(std::size_t skip) {
        try { // try/catch can never be hit but it's needed to prevent TCO
            return generate_trace(skip + 1, SIZE_MAX);
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return stacktrace{};
        }
    }

    CPPTRACE_FORCE_NO_INLINE
    stacktrace generate_trace(std::size_t skip, std::size_t max_depth) {
        try {
            std::vector<frame_ptr> frames = detail::capture_frames(skip + 1, max_depth);
            std::vector<stacktrace_frame> trace = detail::resolve_frames(frames);
            for(auto& frame : trace) {
                frame.symbol = detail::demangle(frame.symbol);
            }
            return {std::move(trace)};
        } catch(...) { // NOSONAR
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
            return stacktrace();
        }
    }

    object_frame safe_object_frame::resolve() const {
        return detail::resolve_safe_object_frame(*this);
    }

    void get_safe_object_frame(frame_ptr address, safe_object_frame* out) {
        detail::get_safe_object_frame(address, out);
    }

    bool can_signal_safe_unwind() {
        return detail::has_safe_unwind();
    }
}
