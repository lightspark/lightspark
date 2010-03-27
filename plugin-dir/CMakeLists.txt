#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2010  Giacomo Spigler <g.spigler@sssup.it>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#**************************************************************************


IF(COMPILE_PLUGIN)

	INCLUDE(FindPkgConfig)
	pkg_check_modules(MOZ_PLUGIN REQUIRED nspr mozilla-plugin)
	INCLUDE_DIRECTORIES(${MOZ_PLUGIN_INCLUDE_DIRS})
	INCLUDE_DIRECTORIES("include")
	INCLUDE_DIRECTORIES("..")

	SET(CMAKE_CXX_FLAGS "-Wall -O3 -fPIC")

	ADD_DEFINITIONS(-DMOZ_X11)

	IF(UNIX)
		ADD_DEFINITIONS(-DXP_UNIX)
	ENDIF(UNIX)


	ADD_LIBRARY(lightsparkplugin SHARED ../fastpaths.asm ../swf.cpp ../swftypes.cpp ../tags.cpp ../geometry.cpp ../actions.cpp ../frame.cpp ../input.cpp ../streams.cpp ../tags_stub.cpp ../logger.cpp ../vm.cpp ../asobjects.cpp ../abc.cpp ../abc_codesynt.cpp ../abc_opcodes.cpp ../flashdisplay.cpp ../flashevents.cpp ../textfile.cpp ../thread_pool.cpp ../flashgeom.cpp ../flashnet.cpp ../flashsystem.cpp ../flashutils.cpp ../compat.cpp ../abc_interpreter.cpp ../flashexternal.cpp ../flashtext.cpp ../flashmedia.cpp ../flv.cpp ../netutils.cpp ../timer.cpp np_entry.cpp npn_gate.cpp npp_gate.cpp plugin.cpp)

#UNIX?
	IF(UNIX)
		INSTALL(FILES liblightsparkplugin.so DESTINATION ~/.mozilla/plugins)
	ENDIF(UNIX)

ENDIF(COMPILE_PLUGIN)
