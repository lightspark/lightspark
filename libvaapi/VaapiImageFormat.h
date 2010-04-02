// VaapiImageFormat.h: VA image format abstraction
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

#ifndef GNASH_VAAPIIMAGEFORMAT_H
#define GNASH_VAAPIIMAGEFORMAT_H

#include "vaapi_common.h"

/// Color spaces
enum VaapiColorspace {
    VAAPI_COLORSPACE_UNKNOWN,   /// Unknown colorspace
    VAAPI_COLORSPACE_YUV,       /// YUV colorspace
    VAAPI_COLORSPACE_RGB        /// RGB colorspace
};

/// Image types
enum VaapiImageFormat {
    /// Best format for the underlying hardware
    VAAPI_IMAGE_NONE  = 0,
    /// Planar YUV 4:2:0, 12-bit, 1 plane for Y and 1 plane for UV
    VAAPI_IMAGE_NV12  = VA_FOURCC('N','V','1','2'),
    /// Planar YUV 4:2:0, 12-bit, 3 planes for Y V U
    VAAPI_IMAGE_YV12  = VA_FOURCC('Y','V','1','2'),
    /// Planar YUV 4:2:0, 12-bit, 3 planes for Y U V
    VAAPI_IMAGE_I420  = VA_FOURCC('I','4','2','0'),
    /// Packed RGB 8:8:8, 32-bit, A R G B
    VAAPI_IMAGE_ARGB  = VA_FOURCC('A','R','G','B'),
    /// Packed RGB 8:8:8, 32-bit, R G B A
    VAAPI_IMAGE_RGBA  = VA_FOURCC('R','G','B','A'),
    /// Packed RGB 8:8:8, 32-bit, A R G B
    VAAPI_IMAGE_ABGR  = VA_FOURCC('A','B','G','R'),
    /// Packed RGB 8:8:8, 32-bit, R G B A
    VAAPI_IMAGE_BGRA  = VA_FOURCC('B','G','R','A'),
    /// Packed RGB 8:8:8, 32-bit, A R G B, native endian byte-order
    VAAPI_IMAGE_RGB32 = VA_FOURCC('R','G','B', 32),
    /// Packed RGB 8:8:8, 24-bit, R G B
    VAAPI_IMAGE_RGB24 = VA_FOURCC('R','G','B', 24)
};

/// Get colorspace for the specified image format
VaapiColorspace vaapi_image_format_get_colorspace(VaapiImageFormat format);

/// Check whether image format is RGB
static inline bool vaapi_image_format_is_rgb(VaapiImageFormat format)
{
    return vaapi_image_format_get_colorspace(format) == VAAPI_COLORSPACE_RGB;
}

/// Check whether image format is YUV
static inline bool vaapi_image_format_is_yuv(VaapiImageFormat format)
{
    return vaapi_image_format_get_colorspace(format) == VAAPI_COLORSPACE_YUV;
}

/// Return image format from a VA image format
VaapiImageFormat vaapi_get_image_format(VAImageFormat const &format);

#endif /* GNASH_VAAPIIMAGEFORMAT_H */

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:


