// VaapiImage.cpp: VA image abstraction
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

#include <boost/format.hpp>
#include <cstring>

#include "log.h"
#include "VaapiImage.h"
#include "VaapiSurface.h"
#include "VaapiGlobalContext.h"
#include "VaapiException.h"
#include "vaapi_utils.h"

namespace gnash {

VaapiImage::VaapiImage(unsigned int     width,
                       unsigned int     height,
                       VaapiImageFormat format)
    : _format(format)
    , _image_data(NULL)
{
    log_debug("VaapiImage::VaapiImage(): format '%s'\n", string_of_FOURCC(format));

    memset(&_image, 0, sizeof(_image));
    _image.image_id = VA_INVALID_ID;

    if (!create(width, height)) {
        boost::format msg;
        msg = boost::format("Could not create %s image")
            % string_of_FOURCC(_format);
        throw VaapiException(msg.str());
    }
}

VaapiImage::~VaapiImage()
{

    GNASH_REPORT_FUNCTION;

    destroy();
}

// Create VA image
bool VaapiImage::create(unsigned int width, unsigned int height)
{
    GNASH_REPORT_FUNCTION;

    VaapiGlobalContext * const gvactx = VaapiGlobalContext::get();
    if (!gvactx)
        return false;

    const VAImageFormat *va_format = gvactx->getImageFormat(_format);
    if (!va_format) {
        return false;
    }

    VAStatus status;
    _image.image_id = VA_INVALID_ID;
    status = vaCreateImage(gvactx->display(),
                           const_cast<VAImageFormat *>(va_format),
                           width, height,
                           &_image);
    if (!vaapi_check_status(status, "vaCreateImage()"))
        return false;

    log_debug("  image 0x%08x, format '%s'\n", get(), string_of_FOURCC(_format));

    return true;
}

// Destroy VA image
void VaapiImage::destroy()
{
    unmap();

    if (_image.image_id == VA_INVALID_ID) {
        return;
    }

    VaapiGlobalContext * const gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return;
    }

    VAStatus status;
    status = vaDestroyImage(gvactx->display(), _image.image_id);
    if (!vaapi_check_status(status, "vaDestroyImage()")) {
        return;
    }
}

// Map image data
bool VaapiImage::map()
{
    if (isMapped()) {
        return true;
    }

    if (_image.image_id == VA_INVALID_ID) {
        return false;
    }

    VaapiGlobalContext * const gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return false;
    }

    VAStatus status;
    status = vaMapBuffer(gvactx->display(), _image.buf, (void **)&_image_data);
    if (!vaapi_check_status(status, "vaMapBuffer()")) {
        return false;
    }

    return true;
}

// Unmap image data
bool VaapiImage::unmap()
{
    if (!isMapped()) {
        return true;
    }

    _image_data = NULL;

    VaapiGlobalContext * const gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return false;
    }

    VAStatus status;
    status = vaUnmapBuffer(gvactx->display(), _image.buf);
    if (!vaapi_check_status(status, "vaUnmapBuffer()")) {
        return false;
    }
    return true;
}

// Get pixels for the specified plane
boost::uint8_t *VaapiImage::getPlane(int plane) const
{
    if (!isMapped()) {
        throw VaapiException("VaapiImage::getPixels(): unmapped image");
    }

    return _image_data + _image.offsets[plane];
}

// Get scanline pitch for the specified plane
unsigned int VaapiImage::getPitch(int plane) const
{   
    if (!isMapped()) {
        throw VaapiException("VaapiImage::getPitch(): unmapped image");
    }

    return _image.pitches[plane];
}

} // gnash namespace

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
