#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2010  Giacomo Spigler <g.spigler@sssup.it>
#    Copyright (C) 2010  Alessandro Pignotti <a.pignotti@sssup.it>
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

pkg_check_modules(MOZ_PLUGIN REQUIRED nspr mozilla-plugin x11)
INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS} ${MOZ_PLUGIN_INCLUDE_DIRS})
ADD_DEFINITIONS(-DMOZ_X11 -DCOMPILE_PLUGIN)
IF(UNIX)
	ADD_DEFINITIONS(-DXP_UNIX)
ENDIF(UNIX)

# Firefox plugin target
ADD_LIBRARY(lightsparkplugin SHARED np_entry.cpp npn_gate.cpp npp_gate.cpp plugin.cpp)
TARGET_LINK_LIBRARIES(lightsparkplugin spark ${MOZ_PLUGIN_LIBRARIES} ${GTK_LIBRARIES})

IF(UNIX)
	INSTALL(TARGETS lightsparkplugin LIBRARY DESTINATION ${PLUGIN_DIRECTORY})
ENDIF(UNIX)

