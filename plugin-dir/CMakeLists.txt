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


INCLUDE(FindPkgConfig)
pkg_check_modules(MOZ_PLUGIN REQUIRED nspr mozilla-plugin)
INCLUDE_DIRECTORIES(${MOZ_PLUGIN_INCLUDE_DIRS})
INCLUDE_DIRECTORIES("include")
INCLUDE_DIRECTORIES("..")

ADD_DEFINITIONS(-DMOZ_X11 -DCOMPILE_PLUGIN)

IF(UNIX)
	ADD_DEFINITIONS(-DXP_UNIX)
ENDIF(UNIX)


SET(LIBSPARK_SOURCES ../swf.cpp ../swftypes.cpp ../tags.cpp ../geometry.cpp ../actions.cpp ../frame.cpp ../input.cpp ../streams.cpp ../tags_stub.cpp ../logger.cpp ../vm.cpp ../asobjects.cpp ../abc.cpp ../abc_codesynt.cpp ../abc_opcodes.cpp ../flashdisplay.cpp ../flashevents.cpp ../textfile.cpp ../thread_pool.cpp ../flashgeom.cpp ../flashnet.cpp ../flashsystem.cpp ../flashutils.cpp ../compat.cpp ../abc_interpreter.cpp ../flashexternal.cpp ../flashtext.cpp ../flashmedia.cpp ../flv.cpp ../netutils.cpp ../timer.cpp ../decoder.cpp ../threading.cpp ../flashxml.cpp ../graphics.cpp)
IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i[3-6]86")
	SET(LIBSPARK_SOURCES ${LIBSPARK_SOURCES} ../fastpaths_32.asm)
ELSEIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|amd64")
	SET(LIBSPARK_SOURCES ${LIBSPARK_SOURCES} ../fastpaths_64.asm)
ELSE()
	MESSAGE(SEND_ERROR "Platform ${CMAKE_SYSTEM_PROCESSOR} not supported")
ENDIF()

ADD_LIBRARY(lightsparkplugin SHARED ${LIBSPARK_SOURCES} np_entry.cpp npn_gate.cpp npp_gate.cpp plugin.cpp)

TARGET_LINK_LIBRARIES(lightsparkplugin ${lib_pthread} ${lib_glew} ${EXTRA_LIBS_LIBRARIES} ${ZLIB_LIBRARIES}
		${LLVM_LIBS_CORE} ${LLVM_LIBS_JIT} ${SDL_LIBRARY} ${CURL_LIBRARIES} ${GTK_LIBRARIES})
SET_TARGET_PROPERTIES(lightsparkplugin PROPERTIES LINK_FLAGS "-Wl,--as-needed")
#UNIX?
IF(UNIX)
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/liblightsparkplugin.so DESTINATION ${PLUGIN_DIRECTORY})
	INSTALL(TARGETS lightsparkplugin DESTINATION ${PLUGIN_DIRECTORY})
ENDIF(UNIX)

