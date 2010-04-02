// vaapi.h: VA API acceleration.
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

#ifndef GNASH_MEDIA_VIDEODECODERFFMPEGVAAPI_H
#define GNASH_MEDIA_VIDEODECODERFFMPEGVAAPI_H

#include "VaapiContext.h"
#include "VaapiSurface.h"
#include "VaapiSurfaceProxy.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/vaapi.h>
}

namespace gnash {
namespace media {
namespace ffmpeg {

/// VA surface implementation for FFmpeg
class VaapiSurfaceFfmpeg : public VaapiSurfaceProxy {
    unsigned int _pic_num;

public:
    VaapiSurfaceFfmpeg(boost::shared_ptr<VaapiSurface> surface,
                       boost::shared_ptr<VaapiContext> context)
        : VaapiSurfaceProxy(surface, context), _pic_num(0)
        { }

    unsigned int getPicNum() const
        { return _pic_num; }

    void setPicNum(unsigned int pic_num)
        { _pic_num = pic_num; }
};

void vaapi_set_surface(AVFrame *pic, VaapiSurfaceFfmpeg *surface);

static inline VaapiSurfaceFfmpeg *vaapi_get_surface(const AVFrame *pic)
{
    return reinterpret_cast<VaapiSurfaceFfmpeg *>(pic->data[0]);
}

/// VA context implementation for FFmpeg
class VaapiContextFfmpeg : public vaapi_context {
    boost::shared_ptr<VaapiContext> _context;

public:
    VaapiContextFfmpeg(enum CodecID codec_id);

    bool initDecoder(unsigned int width, unsigned int height);

    VaapiSurfaceFfmpeg *getSurface()
        { return new VaapiSurfaceFfmpeg(_context->acquireSurface(), _context); }

    static VaapiContextFfmpeg *create(enum CodecID codec_id);
};
    
} // gnash.media.ffmpeg namespace 
} // gnash.media namespace 
} // gnash namespace

#endif /* GNASH_MEDIA_VIDEODECODERFFMPEGVAAPI_H */
