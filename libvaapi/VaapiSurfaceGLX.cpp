// VaapiSurfaceGLX.cpp: VA/GLX surface abstraction
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

#include <va/va_glx.h>
#include <va/va_x11.h>

#include "log.h"
#include "VaapiSurfaceGLX.h"
#include "VaapiGlobalContext.h"
#include "VaapiException.h"
#include "vaapi_utils.h"

namespace gnash {

// Returns a string representation of an OpenGL error
static const char *gl_get_error_string(GLenum error)
{
    static const struct {
        GLenum val;
        const char *str;
    }
    gl_errors[] = {
        { GL_NO_ERROR,          "no error" },
        { GL_INVALID_ENUM,      "invalid enumerant" },
        { GL_INVALID_VALUE,     "invalid value" },
        { GL_INVALID_OPERATION, "invalid operation" },
        { GL_STACK_OVERFLOW,    "stack overflow" },
        { GL_STACK_UNDERFLOW,   "stack underflow" },
        { GL_OUT_OF_MEMORY,     "out of memory" },
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION_EXT
        { GL_INVALID_FRAMEBUFFER_OPERATION_EXT, "invalid framebuffer operation" },
#endif
        { ~0, NULL }
    };

    int i;
    for (i = 0; gl_errors[i].str; i++) {
        if (gl_errors[i].val == error) {
            return gl_errors[i].str;
        }
    }
    return "unknown";
}

static inline bool gl_do_check_error(int report)
{
    GLenum error;
    bool is_error = false;
    while ((error = glGetError()) != GL_NO_ERROR) {
        if (report) {
            vaapi_dprintf("glError: %s caught\n", gl_get_error_string(error));
        }
        is_error = true;
    }
    return is_error;
}

static inline void gl_purge_errors(void)
{
    gl_do_check_error(0);
}

static inline bool gl_check_error(void)
{
    return gl_do_check_error(1);
}

// glGetIntegerv() wrapper
static bool gl_get_param(GLenum param, unsigned int *pval)
{
    GLint val;

    gl_purge_errors();
    glGetIntegerv(param, &val);
    if (gl_check_error()) {
        return false;
    }
    if (pval) {
        *pval = val;
    }
    return true;
}

/// OpenGL texture state
struct TextureState {
    unsigned int target;
    unsigned int old_texture;
    unsigned int was_enabled : 1;
    unsigned int was_bound   : 1;
};

/// Bind texture, preserve previous texture state
static bool bind_texture(TextureState *ts, GLenum target, GLuint texture)
{
    ts->target      = target;
    ts->old_texture = 0;
    ts->was_bound   = 0;
    ts->was_enabled = glIsEnabled(target);

    GLenum texture_binding;
    switch (target) {
    case GL_TEXTURE_1D:
        texture_binding = GL_TEXTURE_BINDING_1D;
        break;
    case GL_TEXTURE_2D:
        texture_binding = GL_TEXTURE_BINDING_2D;
        break;
    case GL_TEXTURE_3D:
        texture_binding = GL_TEXTURE_BINDING_3D;
        break;
    case GL_TEXTURE_RECTANGLE_ARB:
        texture_binding = GL_TEXTURE_BINDING_RECTANGLE_ARB;
        break;
    default:
        assert(!target);
        return false;
    }

    if (!ts->was_enabled) {
        glEnable(target);
    } else if (gl_get_param(texture_binding, &ts->old_texture)) {
        ts->was_bound = texture == ts->old_texture;
    } else {
        return false;
    }

    if (!ts->was_bound) {
        gl_purge_errors();
        glBindTexture(target, texture);
        if (gl_check_error()) {
            return false;
        }
    }
    return true;
}

/// Release texture, restore previous texture state
static void unbind_texture(TextureState *ts)
{
    if (!ts->was_bound && ts->old_texture) {
        glBindTexture(ts->target, ts->old_texture);
    }
    if (!ts->was_enabled) {
        glDisable(ts->target);
    }

    gl_check_error();
}

class VaapiSurfaceGLXImpl: public VaapiSurfaceImplBase {
    void *surface() const
        { return reinterpret_cast<void *>(VaapiSurfaceImplBase::surface()); }

public:
    VaapiSurfaceGLXImpl(GLenum target, GLuint texture);
    ~VaapiSurfaceGLXImpl();

    bool create(GLenum target, GLuint texture);
    void destroy();
    bool update(VaapiSurface *surface);
};

VaapiSurfaceGLXImpl::VaapiSurfaceGLXImpl(GLenum target, GLuint texture)
    : VaapiSurfaceImplBase(0, 0)
{
    GNASH_REPORT_FUNCTION;

    log_debug("VaapiSurfaceGLX::VaapiSurfaceGLX(): target 0x%04x, texture %d\n",
              target, texture);

    reset(0);

    if (!create(target, texture))
        throw VaapiException("Could not create VA/GLX surface");

    log_debug("  -> surface %p\n", this->surface());
}

VaapiSurfaceGLXImpl::~VaapiSurfaceGLXImpl()
{
    GNASH_REPORT_FUNCTION;

    log_debug("VaapiSurfaceGLX::~VaapiSurfaceGLX(): surface %p\n", surface());

    destroy();
}

bool
VaapiSurfaceGLXImpl::create(GLenum target, GLuint texture)
{
    if (target == 0 || texture == 0) {
        return false;
    }

    VaapiGlobalContext * gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return false;
    }

    VAStatus status;
    void *surface = NULL;
    status = vaCreateSurfaceGLX(gvactx->display(), target, texture, &surface);
    if (!vaapi_check_status(status, "vaCreateSurfaceGLX()")) {
        return false;
    }

    reset(reinterpret_cast<uintptr_t>(surface));
    return true;
}

void
VaapiSurfaceGLXImpl::destroy()
{
    if (!surface()) {
        return;
    }

    VaapiGlobalContext * gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return;
    }

    VAStatus status;
    status = vaDestroySurfaceGLX(gvactx->display(), surface());
    if (!vaapi_check_status(status, "vaDestroySurfaceGLX()")) {
        return;
    }

    reset(0);
}

bool
VaapiSurfaceGLXImpl::update(VaapiSurface *surface)
{
    GNASH_REPORT_FUNCTION;

    if (!this->surface()) {
        return false;
    }

    VaapiGlobalContext * gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return false;
    }

    VAStatus status;
    status = vaSyncSurface(gvactx->display(), surface->get());
    if (!vaapi_check_status(status, "vaSyncSurface()"))
        return false;

    status = vaCopySurfaceGLX(gvactx->display(), this->surface(),
                              surface->get(), VA_FRAME_PICTURE);
    if (!vaapi_check_status(status, "vaCopySurfaceGLX()")) {
        return false;
    }

    return true;
}

VaapiSurfaceGLX::VaapiSurfaceGLX(GLenum target, GLuint texture)
    : _impl(new VaapiSurfaceGLXImpl(target, texture))
{
}

bool
VaapiSurfaceGLX::update(boost::shared_ptr<VaapiSurface> surface)
{
    return this->update(surface.get());
}

bool
VaapiSurfaceGLX::update(VaapiSurface *surface)
{
    log_debug("VaapiSurfaceGLX::update(): from surface 0x%08x\n", surface->get());

    return dynamic_cast<VaapiSurfaceGLXImpl *>(_impl.get())->update(surface);
}

} // gnash namespace


// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
