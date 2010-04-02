// VaapiDisplayX11.h: VA/X11 display representation
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

#ifndef GNASH_VAAPIDISPLAYX11_H
#define GNASH_VAAPIDISPLAYX11_H

#include <va/va_x11.h>
#include "VaapiDisplay.h"

namespace gnash {

/// X11 display
class X11Display {
    Display *_x_display;

public:
    X11Display()
        { _x_display = XOpenDisplay(NULL); }

    ~X11Display()
        { if (_x_display) XCloseDisplay(_x_display); }

    Display *get() const
        { return _x_display; }
};

/// VA/X11 display representation
struct VaapiDisplayX11 : public X11Display, VaapiDisplay {
    VaapiDisplayX11()
        : VaapiDisplay(vaGetDisplay(X11Display::get()))
        { }
};

} // gnash namespace

#endif /* GNASH_VAAPIDISPLAY_H */

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:

