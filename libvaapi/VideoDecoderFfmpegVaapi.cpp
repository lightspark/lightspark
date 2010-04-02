// vaapi.cpp: VA API acceleration.
// 
//     Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA    02110-1301    USA

#include "VideoDecoderFfmpegVaapi.h"
#include "VaapiGlobalContext.h"

// TODO: use another mechanism to run-time enable VAAPI?
namespace gnash {
bool DSOEXPORT vaapi_is_enabled();
}

namespace gnash {
namespace media {
namespace ffmpeg {

/// Translates FFmpeg Codec ID to VAProfile
static VAProfile get_profile(enum CodecID codec_id)
{
    static const int mpeg2_profiles[] =
        { VAProfileMPEG2Main, VAProfileMPEG2Simple, -1 };
    static const int mpeg4_profiles[] =
        { VAProfileMPEG4Main, VAProfileMPEG4AdvancedSimple, VAProfileMPEG4Simple, -1 };
    static const int h264_profiles[] =
        { VAProfileH264High, VAProfileH264Main, VAProfileH264Baseline, -1 };
    static const int wmv3_profiles[] =
        { VAProfileVC1Main, VAProfileVC1Simple, -1 };
    static const int vc1_profiles[] =
        { VAProfileVC1Advanced, -1 };

    const int *profiles;
    switch (codec_id) {
    case CODEC_ID_MPEG2VIDEO:
        profiles = mpeg2_profiles;
        break;
    case CODEC_ID_MPEG4:
    case CODEC_ID_H263:
        profiles = mpeg4_profiles;
        break;
    case CODEC_ID_H264:
        profiles = h264_profiles;
        break;
    case CODEC_ID_WMV3:
        profiles = wmv3_profiles;
        break;
    case CODEC_ID_VC1:
        profiles = vc1_profiles;
        break;
    default:
        profiles = NULL;
        break;
    }

    if (profiles) {
        VaapiGlobalContext * const gvactx = VaapiGlobalContext::get();
        if (gvactx) {
            for (int i = 0; profiles[i] != -1; i++) {
                VAProfile profile = static_cast<VAProfile>(profiles[i]);
                if (gvactx->hasProfile(profile))
                    return profile;
            }
        }
    }
    return (VAProfile)-1;
}

/// Attach VA surface to FFmpeg picture
void vaapi_set_surface(AVFrame *pic, VaapiSurfaceFfmpeg *surface)
{
    for (int i = 0; i < 4; i++) {
        pic->data[i]     = NULL;
        pic->linesize[i] = 0;
    }

    if (surface) {
        pic->data[0] = reinterpret_cast<uint8_t *>(surface);
        pic->data[3] = reinterpret_cast<uint8_t *>((uintptr_t)surface->get()->get());
    }
}

VaapiContextFfmpeg::VaapiContextFfmpeg(enum CodecID codec_id)
    : _context(new VaapiContext(get_profile(codec_id), VAEntrypointVLD))
{
    // FFmpeg's vaapi_context must be zero-initialized
    memset(this, 0, sizeof(struct vaapi_context));
}

bool VaapiContextFfmpeg::initDecoder(unsigned int width, unsigned int height)
{
    VaapiGlobalContext * const gvactx = VaapiGlobalContext::get();
    if (!gvactx)
        return false;

    if (!_context->initDecoder(width, height))
        return false;

    this->display    = gvactx->display();
    this->context_id = _context->get();
    return true;
}

VaapiContextFfmpeg *VaapiContextFfmpeg::create(enum CodecID codec_id)
{
    if (!vaapi_is_enabled())
        return NULL;

    VaapiContextFfmpeg *vactx;
    try {
        vactx = new VaapiContextFfmpeg(codec_id);
    }
    catch (...) {
        vactx = NULL;
    }
    return vactx;
}

} // gnash.media.ffmpeg namespace 
} // gnash.media namespace 
} // gnash namespace
