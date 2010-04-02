// VaapiContext.h: VA context abstraction
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

#ifndef GNASH_VAAPICONTEXT_H
#define GNASH_VAAPICONTEXT_H

#include "vaapi_common.h"
#include <queue>

namespace gnash {

// Forward declarations
class VaapiSurface;

/// VA codec
enum VaapiCodec {
    VAAPI_CODEC_UNKNOWN,
    VAAPI_CODEC_MPEG2,
    VAAPI_CODEC_MPEG4,
    VAAPI_CODEC_H264,
    VAAPI_CODEC_VC1
};

/// VA context user-data
class VaapiContextData {
public:
    virtual ~VaapiContextData()
        { }
};

/// VA context abstraction
class VaapiContext {
    typedef boost::shared_ptr<VaapiSurface> VaapiSurfaceSP;

    VADisplay                           _display;
    VAConfigID                          _config;
    VAContextID                         _context;
    VaapiCodec                          _codec;
    VAProfile                           _profile;
    VAEntrypoint                        _entrypoint;
    std::queue<VaapiSurfaceSP>          _surfaces;
    unsigned int                        _picture_width;
    unsigned int                        _picture_height;
    std::auto_ptr<VaapiContextData>     _user_data;

    bool construct();
    void destruct();
    bool createContext(unsigned int width, unsigned int height);
    void destroyContext();

public:
    VaapiContext(VAProfile profile, VAEntrypoint entrypoint);
    ~VaapiContext();

    /// Initialize VA decoder for the specified picture dimensions
    bool initDecoder(unsigned int width, unsigned int height);

    /// Return VA context
    VAContextID get() const
        { return _context; }

    /// Get a free surface
    boost::shared_ptr<VaapiSurface> acquireSurface();

    /// Release surface
    void releaseSurface(boost::shared_ptr<VaapiSurface> surface);

    /// Set user data
    void setData(std::auto_ptr<VaapiContextData> user_data)
        { _user_data = user_data; }

    /// Get user data
    VaapiContextData *getData() const
        { return _user_data.get(); }
};

} // gnash namespace

#endif // GNASH_VAAPICONTEXT_H

// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:


