// VaapiImageFormat.cpp: VA image format abstraction
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

#include "VaapiImageFormat.h"

// Get colorspace for the specified image format
VaapiColorspace vaapi_image_format_get_colorspace(VaapiImageFormat format)
{
    switch (format) {
    case VAAPI_IMAGE_NV12:
    case VAAPI_IMAGE_YV12:
    case VAAPI_IMAGE_I420:
        return VAAPI_COLORSPACE_YUV;
    case VAAPI_IMAGE_ARGB:
    case VAAPI_IMAGE_RGBA:
    case VAAPI_IMAGE_ABGR:
    case VAAPI_IMAGE_BGRA:
    case VAAPI_IMAGE_RGB32:
    case VAAPI_IMAGE_RGB24:
        return VAAPI_COLORSPACE_RGB;
    default:;
    }
    return VAAPI_COLORSPACE_UNKNOWN;
}

// Return image format from a VA image format
VaapiImageFormat vaapi_get_image_format(VAImageFormat const &format)
{
    /* XXX: check RGB formats and endianess */
    return static_cast<VaapiImageFormat>(format.fourcc);
}

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:

