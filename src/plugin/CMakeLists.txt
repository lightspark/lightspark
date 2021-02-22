#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2010  Giacomo Spigler <g.spigler@sssup.it>
#    Copyright (C) 2010-2013  Alessandro Pignotti <a.pignotti@sssup.it>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Lesser General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#**************************************************************************

# Dependencies
INCLUDE_DIRECTORIES("include")
INCLUDE_DIRECTORIES("..")
INCLUDE_DIRECTORIES("include/npapi")

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
IF(WIN32)
	ADD_DEFINITIONS(-DXP_WIN)
ELSE()
	pkg_check_modules(X11 REQUIRED x11)
	ADD_DEFINITIONS(-DMOZ_X11)
	ADD_DEFINITIONS(-DXP_UNIX)
ENDIF()

# Firefox plugin target
SET(PLUGIN_SOURCES ${PLUGIN_SOURCES}
	np_entry.cpp
	npn_gate.cpp
	npp_gate.cpp
	plugin.cpp
	npscriptobject.cpp)

IF(WIN32)
	SET(PLUGIN_SOURCES ${PLUGIN_SOURCES} plugin.rc plugin.def)
ENDIF(WIN32)

ADD_LIBRARY(lightsparkplugin MODULE ${PLUGIN_SOURCES})

#With STATICDEPS, all deps are compiled into spark
IF(NOT STATICDEPS)
TARGET_LINK_LIBRARIES(lightsparkplugin spark)
ENDIF()

IF(WIN32)
  #For win32 firefox, plugins have to start with 'np'
  SET_TARGET_PROPERTIES(lightsparkplugin PROPERTIES OUTPUT_NAME nplightsparkplugin PREFIX "")
  SET_TARGET_PROPERTIES(lightsparkplugin PROPERTIES LINK_FLAGS "-Wl,--enable-stdcall-fixup")
  TARGET_LINK_LIBRARIES(lightsparkplugin -lopengl32 spark)
ELSE()
  TARGET_INCLUDE_DIRECTORIES(lightsparkplugin PRIVATE ${X11_INCLUDE_DIRS})
  TARGET_LINK_LIBRARIES(lightsparkplugin ${X11_LIBRARIES})
ENDIF()

PACK_LIBRARY(lightsparkplugin $<TARGET_FILE:lightsparkplugin>)
INSTALL(TARGETS lightsparkplugin DESTINATION ${PLUGIN_DIRECTORY})

