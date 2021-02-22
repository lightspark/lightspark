#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2016 Ludger Krämer <dbluelle@onlinehome.de>
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

include_directories(.)

# PPAPI plugin target
SET(PPAPI_PLUGIN_SOURCES ${PPAPI_PLUGIN_SOURCES}
	plugin.cpp
	ppextscriptobject.cpp
	)

ADD_LIBRARY(pepflashplayer MODULE ${PPAPI_PLUGIN_SOURCES})

TARGET_LINK_LIBRARIES(pepflashplayer spark)

#With STATICDEPS, all deps are compiled into spark
IF(NOT STATICDEPS)
TARGET_LINK_LIBRARIES(pepflashplayer spark)
ENDIF()

IF(WIN32)
  TARGET_LINK_LIBRARIES(pepflashplayer -lopengl32 spark)
ENDIF()

PACK_LIBRARY(pepflashplayer $<TARGET_FILE:pepflashplayer>)
INSTALL(TARGETS pepflashplayer DESTINATION ${PPAPI_PLUGIN_DIRECTORY})
INSTALL(FILES manifest.json DESTINATION ${PPAPI_PLUGIN_DIRECTORY})

