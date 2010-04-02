// VaapiDisplay.cpp: VA display abstraction
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

#include "log.h"
#include "VaapiDisplay.h"
#include "VaapiException.h"
#include "vaapi_utils.h"

namespace gnash {

VaapiDisplay::VaapiDisplay(VADisplay display)
    : _display(display)
{
    GNASH_REPORT_FUNCTION;

    if (!init()) {
        throw VaapiException("Could not create VA-API display");
    }
}

VaapiDisplay::~VaapiDisplay()
{
    if (_display) {
        vaTerminate(_display);
    }
}

bool VaapiDisplay::init()
{
    GNASH_REPORT_FUNCTION;

    VAStatus status;
    int major_version, minor_version;

    if (!_display) {
        return false;
    }

    status = vaInitialize(_display, &major_version, &minor_version);

    if (!vaapi_check_status(status, "vaInitialize()")) {
        return false;
    }

    vaapi_dprintf("VA API version %d.%d\n", major_version, minor_version);

    return true;
}

} // gnash namespace


// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
