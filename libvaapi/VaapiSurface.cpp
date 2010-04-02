// VaapiSurface.cpp: VA surface abstraction
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

#include <algorithm>

#include "log.h"
#include "VaapiSurface.h"
#include "VaapiGlobalContext.h"
#include "VaapiImage.h"
#include "VaapiSubpicture.h"
#include "vaapi_utils.h"

extern "C" {
#include <libswscale/swscale.h>
}

namespace gnash {

VaapiSurfaceImplBase::VaapiSurfaceImplBase(unsigned int width, unsigned int height)
    : _surface(VA_INVALID_SURFACE), _width(width), _height(height)
{
}

class VaapiSurfaceImpl: public VaapiSurfaceImplBase {
public:
    VaapiSurfaceImpl(const VaapiSurface *surface, unsigned int width, unsigned int height);
    ~VaapiSurfaceImpl();
};

VaapiSurfaceImpl::VaapiSurfaceImpl(const VaapiSurface *surface,
                                   unsigned int width, unsigned int height)
    : VaapiSurfaceImplBase(width, height)
{
    GNASH_REPORT_FUNCTION;

    if (width == 0 || height == 0) {
        return;
    }

    VaapiGlobalContext * gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return;
    }

    VAStatus status;
    VASurfaceID surface_id;
    status = vaCreateSurfaces(gvactx->display(),
                              width, height, VA_RT_FORMAT_YUV420,
                              1, &surface_id);
    if (!vaapi_check_status(status, "vaCreateSurfaces()")) {
        return;
    }

    reset(surface_id);
    //    log_debug("  -> surface 0x%08x\n", surface());
}

VaapiSurfaceImpl::~VaapiSurfaceImpl()
{
    log_debug("VaapiSurface::~VaapiSurface(): surface 0x%08x\n", surface());

    if (surface() == VA_INVALID_SURFACE) {
        return;
    }

    VaapiGlobalContext * gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return;
    }

    VAStatus status;
    VASurfaceID surface_id = surface();
    status = vaDestroySurfaces(gvactx->display(), &surface_id, 1);
    if (!vaapi_check_status(status, "vaDestroySurfaces()")) {
        return;
    }

    reset(VA_INVALID_SURFACE);
}

VaapiSurface::VaapiSurface(unsigned int width, unsigned int height)
    : _impl(new VaapiSurfaceImpl(this, width, height))
    , _context(NULL)
{
}

// Clear surface with black color
void VaapiSurface::clear()
{
    VaapiImage background(width(), height(), VAAPI_IMAGE_NV12);

    if (!background.map()) {
        return;
    }

    // 0x10 is the black level for Y
    boost::uint8_t *Y = background.getPlane(0);
    unsigned int i, stride = background.getPitch(0);
    for (i = 0; i < background.height(); i++, Y += stride) {
        memset(Y, 0x10, stride);
    }

    // 0x80 is the black level for Cb and Cr
    boost::uint8_t *UV = background.getPlane(1);
    stride = background.getPitch(1);
    for (i = 0; i < background.height()/2; i++, UV += stride) {
        memset(UV, 0x80, stride);
    }

    background.unmap();

    VaapiGlobalContext * gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return;
    }

    VAStatus status;
    status = vaPutImage(gvactx->display(), get(), background.get(),
                        0, 0, background.width(), background.height(),
                        0, 0, width(), height());
    if (!vaapi_check_status(status, "vaPutImage()")) {
        return;
    }
}

// Compare two subpictures
static inline bool operator== (boost::shared_ptr<VaapiSubpicture> const &a,
                               boost::shared_ptr<VaapiSubpicture> const &b)
{
    return a->get() == b->get();
}

// Associate subpicture to the surface
bool VaapiSurface::associateSubpicture(boost::shared_ptr<VaapiSubpicture> subpicture,
                                       VaapiRectangle const & src_rect,
                                       VaapiRectangle const & dst_rect)
{
    VaapiGlobalContext * gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return false;
    }

    deassociateSubpicture(subpicture);

    VAStatus status;
    VASurfaceID surface_id = this->get();
    status = vaAssociateSubpicture(gvactx->display(), subpicture->get(),
                                   &surface_id, 1,
                                   src_rect.x, src_rect.y,
                                   src_rect.width, src_rect.height,
                                   dst_rect.x, dst_rect.y,
                                   dst_rect.width, dst_rect.height,
                                   0);
    if (!vaapi_check_status(status, "vaAssociateSubpicture()")) {
        return false;
    }
    _subpictures.push_back(subpicture);

    return true;
}

// Deassociate subpicture from the surface
bool VaapiSurface::deassociateSubpicture(boost::shared_ptr<VaapiSubpicture> subpicture)
{
    std::vector< boost::shared_ptr<VaapiSubpicture> >::iterator it;
    it = std::find(_subpictures.begin(), _subpictures.end(), subpicture);
    if (it == _subpictures.end()) {
        return false;
    }
    _subpictures.erase(it);

    VaapiGlobalContext * gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return false;
    }

    VAStatus status;
    VASurfaceID surface_id = this->get();
    status = vaDeassociateSubpicture(gvactx->display(), subpicture->get(),
                                     &surface_id, 1);
    if (!vaapi_check_status(status, "vaDeassociateSubpicture()")) {
        return false;
    }

    return true;
}

} // gnash namespace


// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
