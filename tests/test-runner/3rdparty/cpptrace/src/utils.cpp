#include <cpptrace/utils.hpp>
#include <cpptrace/exceptions.hpp>

#include <iostream>

#include "demangle/demangle.hpp"
#include "snippets/snippet.hpp"
#include "utils/utils.hpp"
#include "platform/exception_type.hpp"

namespace cpptrace {
    std::string demangle(const std::string& name) {
        return detail::demangle(name);
    }

    std::string get_snippet(const std::string& path, std::size_t line, std::size_t context_size, bool color) {
        return detail::get_snippet(path, line, context_size, color);
    }

    bool isatty(int fd) {
        return detail::isatty(fd);
    }

    extern const int stdin_fileno = detail::fileno(stdin);
    extern const int stdout_fileno = detail::fileno(stdout);
    extern const int stderr_fileno = detail::fileno(stderr);

    CPPTRACE_FORCE_NO_INLINE void print_terminate_trace() {
        try { // try/catch can never be hit but it's needed to prevent TCO
            generate_trace(1).print(
                std::cerr,
                isatty(stderr_fileno),
                true,
                "Stack trace to reach terminate handler (most recent call first):"
            );
        } catch(...) {
            if(!detail::should_absorb_trace_exceptions()) {
                throw;
            }
        }
    }

    [[noreturn]] void terminate_handler() {
        // TODO: Support std::nested_exception?
        try {
            auto ptr = std::current_exception();
            if(ptr == nullptr) {
                fputs("terminate called without an active exception", stderr);
                print_terminate_trace();
            } else {
                std::rethrow_exception(ptr);
            }
        } catch(cpptrace::exception& e) {
            microfmt::print(
                stderr,
                "Terminate called after throwing an instance of {}: {}\n",
                demangle(typeid(e).name()),
                e.message()
            );
            e.trace().print(std::cerr, isatty(stderr_fileno));
        } catch(std::exception& e) {
            microfmt::print(
                stderr, "Terminate called after throwing an instance of {}: {}\n", demangle(typeid(e).name()), e.what()
            );
            print_terminate_trace();
        } catch(...) {
            microfmt::print(
                stderr, "Terminate called after throwing an instance of {}\n", detail::exception_type_name()
            );
            print_terminate_trace();
        }
        std::flush(std::cerr);
        abort();
    }

    void register_terminate_handler() {
        std::set_terminate(terminate_handler);
    }
}
