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

AUTOMAKE_OPTIONS = foreign

pkglib_LTLIBRARIES = libgnashvaapi.la

libgnashvaapi_la_CPPFLAGS = \
	-I$(top_srcdir)/libbase	\
	$(BOOST_CFLAGS)	\
	$(LIBSWSCALE_CFLAGS) \
	$(LIBVA_CFLAGS)	\
	$(LIBVA_X11_CFLAGS) \
	$(NULL)

libgnashvaapi_la_LIBADD = \
	$(X11_LIBS) \
	$(BOOST_LIBS) \
	$(PTHREAD_LIBS) \
	$(LIBSWSCALE_LIBS) \
	$(LIBVA_LIBS) \
	$(LIBVA_X11_LIBS) \
	$(NULL)

libgnashvaapi_la_SOURCES = \
	vaapi_utils.cpp	\
	VaapiContext.cpp \
	VaapiDisplay.cpp \
	VaapiGlobalContext.cpp \
	VaapiImage.cpp \
	VaapiImageFormat.cpp \
	VaapiSubpicture.cpp \
	VaapiSurface.cpp \
	VaapiSurfaceProxy.cpp \
	$(NULL)

noinst_HEADERS = \
	vaapi_common.h \
	vaapi_utils.h \
	VaapiContext.h \
	VaapiDisplay.h \
	VaapiDisplayX11.h \
	VaapiDisplayGLX.h \
	VaapiException.h \
	VaapiGlobalContext.h \
	VaapiImage.h \
	VaapiImageFormat.h \
	VaapiSubpicture.h \
	VaapiSurface.h \
	VaapiSurfaceGLX.h \
	VaapiSurfaceProxy.h \
	$(NULL)

if USE_VAAPI_GLX
libgnashvaapi_la_CPPFLAGS += \
	$(LIBVA_GLX_CFLAGS) \
	$(NULL)

libgnashvaapi_la_LIBADD += \
	$(LIBVA_GLX_LIBS) \
	$(NULL)

libgnashvaapi_la_SOURCES += \
	VaapiSurfaceGLX.cpp \
	$(NULL)
endif

libgnashvaapi_la_LDFLAGS = -release $(VERSION)

# Rebuild with GCC 4.x Mudflap support
mudflap:
	@echo "Rebuilding with GCC Mudflap support"
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -fmudflap" LDFLAGS="$(LDFLAGS) -lmudflap"

clean-hook:
	-rm -f core.* *.obj
