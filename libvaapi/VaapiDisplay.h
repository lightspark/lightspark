// VaapiDisplay.h: VA display abstraction
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

#ifndef GNASH_VAAPIDISPLAY_H
#define GNASH_VAAPIDISPLAY_H

#include "vaapi_common.h"

namespace gnash {

/// VA display abstraction
class VaapiDisplay {
    VADisplay _display;

    bool init();

public:
    explicit VaapiDisplay(VADisplay display);
    virtual ~VaapiDisplay();

    VADisplay get() const
        { return _display; }
};

} // gnash namespace

#endif /* GNASH_VAAPIDISPLAY_H */

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:

