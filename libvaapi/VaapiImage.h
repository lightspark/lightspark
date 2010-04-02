// VaapiImage.h: VA image abstraction
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

#ifndef GNASH_VAAPIIMAGE_H
#define GNASH_VAAPIIMAGE_H

#include "vaapi_common.h"
#include "VaapiImageFormat.h"
#include <boost/scoped_array.hpp>
#include <memory>

// Forward declarations
struct SwsContext;

namespace gnash {

// Forward declarations
class VaapiSurface;
class VAImageWrapper;
class SwsContextWrapper;

/// VA image abstraction
class VaapiImage {
    VaapiImageFormat    _format;
    VAImage             _image;
    boost::uint8_t *    _image_data;

    /// Create VA image
    bool create(unsigned int width, unsigned int height);

    /// Destroy VA image
    void destroy();

public:
    VaapiImage(unsigned int     width,
               unsigned int     height,
               VaapiImageFormat format = VAAPI_IMAGE_RGB32);
    ~VaapiImage();

    /// Get VA image ID
    VAImageID get() const
        { return _image.image_id; }

    /// Get image format
    VaapiImageFormat format() const
        { return _format; }

    /// Get image width
    unsigned int width() const
        { return _image.width; }

    /// Get image height
    unsigned int height() const
        { return _image.height; }

    /// Check whether the VA image is mapped
    bool isMapped() const
        { return _image_data != NULL; }

    /// Map image data
    bool map();

    /// Unmap image data
    bool unmap();

    /// Get number of planes
    unsigned int getPlaneCount() const
        { return _image.num_planes; }

    /// Get pixels for the specified plane
    boost::uint8_t *getPlane(int plane) const;

    /// Get scanline pitch for the specified plane
    unsigned int getPitch(int plane) const;
};

} // gnash namespace

#endif /* GNASH_VAAPIIMAGE_H */

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:


