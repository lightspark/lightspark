// VaapiException.h: VA exception abstraction
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

#ifndef GNASH_VAAPIEXCEPTION_H
#define GNASH_VAAPIEXCEPTION_H

#include <stdexcept>
#include <string>

namespace gnash {

/// VA exception abstraction
struct VaapiException: public std::runtime_error {
    VaapiException(const std::string & str)
        : std::runtime_error(str)
        { }

    VaapiException()
        : std::runtime_error("Video Acceleration error")
        { }

    virtual ~VaapiException() throw()
        { }
};

} // gnash namespace

#endif /* GNASH_VAAPIEXCEPTION_H */

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:

