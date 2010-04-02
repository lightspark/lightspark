// 
//   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Free Software
//   Foundation, Inc
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef GNASH_LOG_H
#define GNASH_LOG_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "dsodefs.h" // for DSOEXPORT

#include <boost/format.hpp>

// Macro to prevent repeated logging calls for the same
// event
#define LOG_ONCE(x) { \
    static bool warned = false; \
    if (!warned) { warned = true; x; } \
}

namespace gnash {

void log_debug(const char *format, ...);

class DSOEXPORT __Host_Function_Report__ {
public:
    const char *func;

    // Only print function tracing messages when multiple -v
    // options have been supplied. 
    __Host_Function_Report__(void) {
	log_debug("entering\n");
    }

    __Host_Function_Report__(char *_func) {
	func = _func;
	log_debug("%s enter\n", func);
    }

    __Host_Function_Report__(const char *_func) {
	func = _func;
	if (func) {
	    log_debug("%s enter\n", func);
	} else {
	    log_debug("No Function Name! enter\n");
	}
    }

    ~__Host_Function_Report__(void) {
	log_debug("%s returning\n", func);
    }
};

#ifndef HAVE_FUNCTION
    #ifndef HAVE_func
        #define dummystr(x) # x
        #define dummyestr(x) dummystr(x)
        #define __FUNCTION__ __FILE__":"dummyestr(__LINE__)
    #else
        #define __FUNCTION__ __func__    
    #endif
#endif

#ifndef HAVE_PRETTY_FUNCTION
    #define __PRETTY_FUNCTION__ __FUNCTION__
#endif

#if 1
#define GNASH_REPORT_FUNCTION
#define GNASH_REPORT_RETURN
#else
#if defined(__cplusplus) && defined(__GNUC__)
#define GNASH_REPORT_FUNCTION   \
    gnash::__Host_Function_Report__ __host_function_report__( __PRETTY_FUNCTION__)
#define GNASH_REPORT_RETURN
#else
#define GNASH_REPORT_FUNCTION \
    gnash::log_debug("entering\n")

#define GNASH_REPORT_RETURN \
    gnash::log_debug("returning\n")
#endif
#endif

}


#endif // GNASH_LOG_H


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
