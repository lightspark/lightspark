#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2010  Giacomo Spigler <g.spigler@sssup.it>
#    Copyright (C) 2010-2011  Alessandro Pignotti <a.pignotti@sssup.it>
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

INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})
IF(UNIX)
	ADD_DEFINITIONS(-DMOZ_X11)
	ADD_DEFINITIONS(-DXP_UNIX)
ELSE()
	ADD_DEFINITIONS(-DXP_WIN)
ENDIF(UNIX)

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

ADD_LIBRARY(lightsparkplugin SHARED ${PLUGIN_SOURCES})

TARGET_LINK_LIBRARIES(lightsparkplugin spark)

#With STATICDEPS, all deps are compiled into spark
IF(NOT STATICDEPS)
  TARGET_LINK_LIBRARIES(lightsparkplugin spark ${GTK_LIBRARIES} ${X11_LIBRARIES} ${GLIBMM_LIBRARIES})
ENDIF()

INSTALL(TARGETS lightsparkplugin DESTINATION ${PLUGIN_DIRECTORY})

