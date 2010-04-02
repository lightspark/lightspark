// vaapi_utils.cpp: VA API utilities
// 
// Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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
//

#include "vaapi_utils.h"
#include <stdio.h>
#include <stdarg.h>

namespace gnash {


static bool g_vaapi_is_enabled = false;

// Enable video acceleration (with VA API)
void vaapi_set_is_enabled(bool enabled)
{
    g_vaapi_is_enabled = enabled;
}

// Check whether video acceleration is enabled
bool vaapi_is_enabled()
{
    return g_vaapi_is_enabled;
}

// Debug output
void DSOEXPORT vaapi_dprintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stdout, "[GnashVaapi] ");
    vfprintf(stdout, format, args);
    va_end(args);
}

// Check VA status for success or print out an error
bool vaapi_check_status(VAStatus status, const char *msg)
{
    if (status != VA_STATUS_SUCCESS) {
        vaapi_dprintf("%s: %s\n", msg, vaErrorStr(status));
        return false;
    }
    return true;
}

/// Return a string representation of a FOURCC
const char *string_of_FOURCC(boost::uint32_t fourcc)
{
    static int buf;
    static char str[2][5]; // XXX: 2 buffers should be enough for most purposes

    buf ^= 1;
    str[buf][0] = fourcc;
    str[buf][1] = fourcc >> 8;
    str[buf][2] = fourcc >> 16;
    str[buf][3] = fourcc >> 24;
    str[buf][4] = '\0';
    return str[buf];
}

// Return a string representation of a VAProfile
const char *string_of_VAProfile(VAProfile profile)
{
    switch (profile) {
#define PROFILE(profile) \
        case VAProfile##profile: return "VAProfile" #profile
        PROFILE(MPEG2Simple);
        PROFILE(MPEG2Main);
        PROFILE(MPEG4Simple);
        PROFILE(MPEG4AdvancedSimple);
        PROFILE(MPEG4Main);
        PROFILE(H264Baseline);
        PROFILE(H264Main);
        PROFILE(H264High);
        PROFILE(VC1Simple);
        PROFILE(VC1Main);
        PROFILE(VC1Advanced);
#undef PROFILE
    default: break;
    }
    return "<unknown>";
}

// Return a string representation of a VAEntrypoint
const char *string_of_VAEntrypoint(VAEntrypoint entrypoint)
{
    switch (entrypoint) {
#define ENTRYPOINT(entrypoint) \
        case VAEntrypoint##entrypoint: return "VAEntrypoint" #entrypoint
        ENTRYPOINT(VLD);
        ENTRYPOINT(IZZ);
        ENTRYPOINT(IDCT);
        ENTRYPOINT(MoComp);
        ENTRYPOINT(Deblocking);
#undef ENTRYPOINT
    default: break;
    }
    return "<unknown>";
}

} // gnash namespace


// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
