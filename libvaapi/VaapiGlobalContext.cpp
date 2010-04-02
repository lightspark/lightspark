// VaapiGlobalContext.cpp: VA API global context
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

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "log.h"
#include "VaapiGlobalContext.h"
#include "VaapiDisplayX11.h"
#ifdef USE_VAAPI_GLX
#include "VaapiDisplayGLX.h"
#endif
#include "VaapiException.h"
#include "vaapi_utils.h"

namespace gnash {

VaapiGlobalContext::VaapiGlobalContext(std::auto_ptr<VaapiDisplay> display)
    : _display(display)
{
    GNASH_REPORT_FUNCTION;

    if (!init())
        throw VaapiException("could not initialize VA-API global context");
}

VaapiGlobalContext::~VaapiGlobalContext()
{
}

bool
VaapiGlobalContext::init()
{
    GNASH_REPORT_FUNCTION;

    VADisplay dpy = display();
    VAStatus status;

    int num_profiles = 0;
    _profiles.resize(vaMaxNumProfiles(dpy));
    status = vaQueryConfigProfiles(dpy, &_profiles[0], &num_profiles);
    if (!vaapi_check_status(status, "vaQueryConfigProfiles()")) {
        return false;
    }
    _profiles.resize(num_profiles);

    int num_image_formats = 0;
    _image_formats.resize(vaMaxNumImageFormats(dpy));
    status = vaQueryImageFormats(dpy, &_image_formats[0], &num_image_formats);
    if (!vaapi_check_status(status, "vaQueryImageFormats()")) {
        return false;
    }
    _image_formats.resize(num_image_formats);

    unsigned int num_subpicture_formats = 0;
    std::vector<unsigned int> flags;
    flags.resize(vaMaxNumSubpictureFormats(dpy));
    _subpicture_formats.resize(vaMaxNumSubpictureFormats(dpy));
    status = vaQuerySubpictureFormats(dpy, &_subpicture_formats[0], &flags[0], &num_subpicture_formats);
    if (!vaapi_check_status(status, "vaQuerySubpictureFormats()")) {
        return false;
    }
    _subpicture_formats.resize(num_subpicture_formats);
    return true;
}

bool
VaapiGlobalContext::hasProfile(VAProfile profile) const
{
    for (unsigned int i = 0; i < _profiles.size(); i++) {
        if (_profiles[i] == profile) {
            return true;
        }
    }
    return false;
}

const VAImageFormat *
VaapiGlobalContext::getImageFormat(VaapiImageFormat format) const
{
    for (unsigned int i = 0; i < _image_formats.size(); i++) {
        if (vaapi_get_image_format(_image_formats[i]) == format)
            return &_image_formats[i];
    }
    return NULL;
}

static std::vector<VaapiImageFormat>
get_formats(std::vector<VAImageFormat> const &vaFormats)
{
    std::vector<VaapiImageFormat> formats;
    for (unsigned int i = 0; i < vaFormats.size(); i++) {
        VaapiImageFormat format = vaapi_get_image_format(vaFormats[i]);
        if (format != VAAPI_IMAGE_NONE)
            formats.push_back(format);
    }
    return formats;
}

std::vector<VaapiImageFormat>
VaapiGlobalContext::getImageFormats() const
{
    return get_formats(_image_formats);
}

std::vector<VaapiImageFormat>
VaapiGlobalContext::getSubpictureFormats() const
{
    return get_formats(_subpicture_formats);
}

/// A wrapper around a VaapiGlobalContext to ensure it's free'd on destruction.
VaapiGlobalContext *VaapiGlobalContext::get()
{
    LOG_ONCE(GNASH_REPORT_FUNCTION);

    static std::auto_ptr<VaapiGlobalContext> vaapi_global_context;

    if (!vaapi_global_context.get()) {
        std::auto_ptr<VaapiDisplay> dpy;
        /* XXX: this won't work with multiple renders built-in */
        try {
#ifdef USE_VAAPI_GLX
            dpy.reset(new VaapiDisplayGLX());
#else
            dpy.reset(new VaapiDisplayX11());
#endif
            if (!dpy.get()) {
                return NULL;
            }
            vaapi_global_context.reset(new VaapiGlobalContext(dpy));
        }
        catch (...) {
            vaapi_set_is_enabled(false);
            return NULL;
        }
    }
    return vaapi_global_context.get();
}

} // gnash namespace

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:

