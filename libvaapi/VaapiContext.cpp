// VaapiContext.cpp: VA context abstraction
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

#include "log.h"
#include "VaapiContext.h"
#include "VaapiGlobalContext.h"
#include "VaapiException.h"
#include "VaapiDisplay.h"
#include "VaapiSurface.h"
#include "vaapi_utils.h"

namespace gnash {

/// Translates VAProfile to VaapiCodec
static VaapiCodec get_codec(VAProfile profile)
{
    switch (profile) {
    case VAProfileMPEG2Simple:
    case VAProfileMPEG2Main:
        return VAAPI_CODEC_MPEG2;
    case VAProfileMPEG4Simple:
    case VAProfileMPEG4AdvancedSimple:
    case VAProfileMPEG4Main:
        return VAAPI_CODEC_MPEG4;
    case VAProfileH264Baseline:
    case VAProfileH264Main:
    case VAProfileH264High:
        return VAAPI_CODEC_H264;
    case VAProfileVC1Simple:
    case VAProfileVC1Main:
    case VAProfileVC1Advanced:
        return VAAPI_CODEC_VC1;
    default:
        break;
    }
    return VAAPI_CODEC_UNKNOWN;
}

/// Returns number of VA surfaces to create for a specified codec
static unsigned int get_max_surfaces(VaapiCodec codec)
{
    // Number of scratch surfaces beyond those used as reference
    const unsigned int SCRATCH_SURFACES_COUNT = 8;

    // Make sure pool of created surfaces for H.264 is under 64 MB for 1080p
    const unsigned int MAX_SURFACE_SIZE   = (1920 * 1080 * 3) / 2;
    const unsigned int MAX_VIDEO_MEM_SIZE = 64 * 1024 * 1024;
    const unsigned int MAX_SURFACES_COUNT = MAX_VIDEO_MEM_SIZE / MAX_SURFACE_SIZE;

    unsigned int max_surfaces;
    max_surfaces = (codec == VAAPI_CODEC_H264 ? 16 : 2) + SCRATCH_SURFACES_COUNT;
    if (max_surfaces > MAX_SURFACES_COUNT) {
        max_surfaces = MAX_SURFACES_COUNT;
    }

    return max_surfaces;
}

VaapiContext::VaapiContext(VAProfile profile, VAEntrypoint entrypoint)
    : _config(VA_INVALID_ID)
    , _context(VA_INVALID_ID)
    , _codec(get_codec(profile))
    , _profile(profile)
    , _entrypoint(entrypoint)
    , _picture_width(0), _picture_height(0)
{
    log_debug("VaapiContext::VaapiContext(): profile %d, entrypoint %d\n", profile, entrypoint);

    if (!construct()) {
        boost::format msg;
        msg = boost::format("Could not create VA API context for profile %s")
            % string_of_VAProfile(profile);
        throw VaapiException(msg.str());
    }
}

VaapiContext::~VaapiContext()
{
    log_debug("VaapiContext::~VaapiContext(): context 0x%08x\n", _context);

    destruct();
}

bool VaapiContext::construct()
{
    GNASH_REPORT_FUNCTION;

    VaapiGlobalContext * const gvactx = VaapiGlobalContext::get();
    if (!gvactx) {
        return false;
    }

    _display = gvactx->display();
    if (!_display) {
        return false;
    }

    if (_codec == VAAPI_CODEC_UNKNOWN)
        return false;

    VAStatus status;
    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    status = vaGetConfigAttributes(_display, _profile, _entrypoint, &attrib, 1);
    if (!vaapi_check_status(status, "vaGetConfigAttributes()"))
        return false;

    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
        return false;

    VAConfigID config;
    status = vaCreateConfig(_display, _profile, _entrypoint, &attrib, 1, &config);
    if (!vaapi_check_status(status, "vaCreateConfig()"))
        return false;

    _config = config;
    return true;
}

void VaapiContext::destruct()
{
    GNASH_REPORT_FUNCTION;

    destroyContext();

    if (_config != VA_INVALID_ID) {
        VAStatus status = vaDestroyConfig(_display, _config);
        vaapi_check_status(status, "vaDestroyConfig()");
    }
}

bool VaapiContext::createContext(unsigned int width, unsigned int height)
{
    GNASH_REPORT_FUNCTION;

    if (_config == VA_INVALID_ID) {
        return false;
    }

    const unsigned int num_surfaces = get_max_surfaces(_codec);
    std::vector<VASurfaceID> surface_ids;
    surface_ids.reserve(num_surfaces);
    for (unsigned int i = 0; i < num_surfaces; i++) {
        VaapiSurfaceSP surface(new VaapiSurface(width, height));
        surface->setContext(this);
        _surfaces.push(surface);
        surface_ids.push_back(surface->get());
    }

    VAStatus status;
    VAContextID context;
    status = vaCreateContext(_display,
                             _config,
                             width, height,
                             VA_PROGRESSIVE,
                             &surface_ids[0], surface_ids.size(),
                             &context);
    if (!vaapi_check_status(status, "vaCreateContext()"))
        return false;

    _context            = context;
    _picture_width      = width;
    _picture_height     = height;
    log_debug("  -> context 0x%08x\n", _context);

    return true;
}

void VaapiContext::destroyContext()
{
    VAStatus status;

    if (_context != VA_INVALID_ID) {
        status = vaDestroyContext(_display,_context);
        if (!vaapi_check_status(status, "vaDestroyContext()"))
            return;
        _context = VA_INVALID_ID;
    }

    for (unsigned int i = 0; i < _surfaces.size(); i++)
        _surfaces.pop();
    _picture_width  = 0;
    _picture_height = 0;
}

bool VaapiContext::initDecoder(unsigned int width, unsigned int height)
{
    GNASH_REPORT_FUNCTION;

    if (_picture_width == width && _picture_height == height) {
        return true;
    }

    destroyContext();
    return createContext(width, height);
}

/// Get a free surface
boost::shared_ptr<VaapiSurface> VaapiContext::acquireSurface()
{
    boost::shared_ptr<VaapiSurface> surface;

    if (_surfaces.empty()) {
        log_debug("VaapiContext::acquireSurface(): empty pool\n");
        return surface;
    }

    surface = _surfaces.front();
    _surfaces.pop();
    log_debug("VaapiContext::acquireSurface(): surface 0x%08x\n", surface->get());

    return surface;
}

/// Release surface
void VaapiContext::releaseSurface(boost::shared_ptr<VaapiSurface> surface)
{
    log_debug("VaapiContext::releaseSurface(): surface 0x%08x\n", surface->get());
    _surfaces.push(surface);
}

} // gnash namespace


// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
