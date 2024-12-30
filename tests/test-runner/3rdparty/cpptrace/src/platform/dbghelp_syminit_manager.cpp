#include "platform/platform.hpp"

#if IS_WINDOWS

#include "platform/dbghelp_syminit_manager.hpp"

#include "utils/error.hpp"
#include "utils/microfmt.hpp"

#include <unordered_set>

#ifndef WIN32_LEAN_AND_MEAN
 #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbghelp.h>

namespace cpptrace {
namespace detail {

    dbghelp_syminit_manager::~dbghelp_syminit_manager() {
        for(auto handle : set) {
            if(!SymCleanup(handle)) {
                ASSERT(false, microfmt::format("Cpptrace SymCleanup failed with code {}\n", GetLastError()).c_str());
            }
        }
    }

    void dbghelp_syminit_manager::init(HANDLE proc) {
        if(set.count(proc) == 0) {
            if(!SymInitialize(proc, NULL, TRUE)) {
                throw internal_error("SymInitialize failed {}", GetLastError());
            }
            set.insert(proc);
        }
    }

    // Thread-safety: Must only be called from symbols_with_dbghelp while the dbghelp_lock lock is held
    dbghelp_syminit_manager& get_syminit_manager() {
        static dbghelp_syminit_manager syminit_manager;
        return syminit_manager;
    }

}
}

#endif
