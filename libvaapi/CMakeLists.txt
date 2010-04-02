# 
# Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

IF(USE_VAAPI)
  ADD_LIBRARY(liblightspark-vaapi STATIC
    VaapiContext.cpp
    VaapiDisplay.cpp
    VaapiGlobalContext.cpp
    VaapiImage.cpp
    VaapiImageFormat.cpp
    VaapiSubpicture.cpp
    VaapiSurface.cpp
    VaapiSurfaceGLX.cpp
    VaapiSurfaceProxy.cpp
    VideoDecoderFfmpegVaapi.cpp
    vaapi_debug.cpp
    vaapi_utils.cpp
    )

  SET(CMAKE_CXX_FLAGS "-finline-functions -Wall -pipe -fPIC")
ENDIF(USE_VAAPI)
