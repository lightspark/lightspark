// VaapiSurface.h: VA surface abstraction
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

#ifndef GNASH_VAAPISURFACE_H
#define GNASH_VAAPISURFACE_H

#include "vaapi_common.h"
#include <vector>

namespace gnash {

// Forward declarations
class VaapiContext;
class VaapiSubpicture;

/// VA rectangle abstraction
struct VaapiRectangle : public VARectangle {
    VaapiRectangle(unsigned int w = 0, unsigned int h = 0)
        { x = 0; y = 0; width = w; height = h; }

    VaapiRectangle(int x_, int y_, unsigned int w, unsigned int h)
        { x = x_; y = y_; width = w; height = h; }
};

/// VA surface base representation
class VaapiSurfaceImplBase {
    uintptr_t           _surface;
    unsigned int        _width;
    unsigned int        _height;

protected:
    void reset(uintptr_t surface)
        { _surface = surface; }

public:
    VaapiSurfaceImplBase(unsigned int width, unsigned int height);
    virtual ~VaapiSurfaceImplBase() { }

    /// Get VA surface
    uintptr_t surface() const
        { return _surface; }

    /// Get surface width
    unsigned int width() const
        { return _width; }

    /// Get surface height
    unsigned int height() const
        { return _height; }
};

/// VA surface abstraction
class VaapiSurface {
    std::auto_ptr<VaapiSurfaceImplBase> _impl;
    std::vector< boost::shared_ptr<VaapiSubpicture> > _subpictures;

    friend class VaapiContext;
    VaapiContext *_context;

    /// Set parent VA context
    void setContext(VaapiContext *context)
        { _context = context; }

public:
    VaapiSurface(unsigned int width, unsigned int height);

    /// Return parent VA context
    VaapiContext *getContext() const
        { return _context; }

    /// Return VA surface id
    VASurfaceID get() const
        { return static_cast<VASurfaceID>(_impl->surface()); }

    /// Get surface width
    unsigned int width() const
        { return _impl->width(); }

    /// Get surface height
    unsigned int height() const
        { return _impl->height(); }

    /// Clear surface with black color
    void clear();

    /// Associate subpicture to the surface
    bool associateSubpicture(boost::shared_ptr<VaapiSubpicture> subpicture,
                             VaapiRectangle const & src_rect,
                             VaapiRectangle const & dst_rect);

    /// Deassociate subpicture from the surface
    bool deassociateSubpicture(boost::shared_ptr<VaapiSubpicture> subpicture);
};

} // gnash namespace

#endif /* GNASH_VAAPISURFACE_H */

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:

