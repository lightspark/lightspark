#include <stdio.h>
#include <stdarg.h>
#include "log.h"

// XXX: replace with lightspark's logger
#ifndef USE_VAAPI_DEBUG
#define USE_VAAPI_DEBUG 0
#endif

namespace gnash {

void log_debug(const char *format, ...)
{
#if USE_VAAPI_DEBUG
    va_list args;

    va_start(args, format);
    fprintf(stdout, "[GnashVaapi] ");
    vfprintf(stdout, format, args);
    va_end(args);
#endif
}

}
