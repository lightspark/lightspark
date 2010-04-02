// VaapiGlobalContext.h: VA API global context
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

#ifndef GNASH_VAAPIGLOBALCONTEXT_H
#define GNASH_VAAPIGLOBALCONTEXT_H

#include "vaapi_common.h"
#include <vector>
#include "VaapiDisplay.h"
#include "VaapiImageFormat.h"

namespace gnash {

/// VA API global context
class DSOEXPORT VaapiGlobalContext {
    std::auto_ptr<VaapiDisplay> _display;
    std::vector<VAProfile>      _profiles;
    std::vector<VAImageFormat>  _image_formats;
    std::vector<VAImageFormat>  _subpicture_formats;

    bool init();

public:
    VaapiGlobalContext(std::auto_ptr<VaapiDisplay> display);
    ~VaapiGlobalContext();

    /// Get the unique global VA context
    //
    /// @return     The global VA context
    static VaapiGlobalContext *get();

    /// Check VA profile is supported
    bool hasProfile(VAProfile profile) const;

    /// Get the VA image format matching format
    //
    /// @return     The VA image format
    const VAImageFormat *getImageFormat(VaapiImageFormat format) const;

    /// Get the list of supported image formats
    //
    /// @return     The list of image formats
    std::vector<VaapiImageFormat> getImageFormats() const;

    /// Get the list of supported subpicture formats
    //
    /// @return     The list of subpicture formats
    std::vector<VaapiImageFormat> getSubpictureFormats() const;

    /// Get the VA display
    //
    /// @return     The VA display
    VADisplay display() const
        { return _display->get(); }
};

} // gnash namespace

#endif // GNASH_VAAPIGLOBALCONTEXT_H


// local Variables:
// mode: C++
// indent-tabs-mode: nil
// End:
