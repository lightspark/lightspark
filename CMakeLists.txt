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


CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(LIGHTSPARK)

SET(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Install prefix")
SET(COMPILE_LIGHTSPARK TRUE CACHE BOOL "Compile Lightspark?")
SET(COMPILE_TIGHTSPARK TRUE CACHE BOOL "Compile Tightspark?")
SET(COMPILE_PLUGIN FALSE CACHE BOOL "Compile the Firefox Plugin?")
SET(PLUGIN_DIRECTORY "/usr/lib/mozilla/plugins" CACHE STRING "Directory to install Firefox plugin to")

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/conf)

INCLUDE(CMakeASM-NASMCompiler)

#LIBRARIES
INCLUDE(FindLLVM REQUIRED)
INCLUDE(FindSDL REQUIRED)
INCLUDE(FindCURL REQUIRED)
INCLUDE(FindPkgConfig REQUIRED)
INCLUDE(FindZLIB REQUIRED)

pkg_check_modules(EXTRA_LIBS REQUIRED gl libpcrecpp libavcodec libavutil ftgl x11)

INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${EXTRA_LIBS_INCLUDE_DIRS})

FIND_LIBRARY(lib_pthread pthread REQUIRED)
FIND_LIBRARY(lib_glew GLEW REQUIRED)

ENABLE_LANGUAGE(ASM-NASM)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wnon-virtual-dtor -Woverloaded-virtual -pipe")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -D_GLIBCXX_NO_DEBUG -DEXPENSIVE_DEBUG")
SET(CMAKE_CXX_FLAGS_PROFILE "-g -pg -O2")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3")
SET(CMAKE_CXX_FLAGS_DEBIAN "-O2")

ADD_DEFINITIONS(-DDATADIR="${CMAKE_INSTALL_PREFIX}/share/lightspark")

SET(LIBSPARK_SOURCES swf.cpp swftypes.cpp tags.cpp geometry.cpp actions.cpp frame.cpp input.cpp streams.cpp tags_stub.cpp logger.cpp vm.cpp asobjects.cpp abc.cpp abc_codesynt.cpp abc_opcodes.cpp flashdisplay.cpp flashevents.cpp textfile.cpp thread_pool.cpp flashgeom.cpp flashnet.cpp flashsystem.cpp flashutils.cpp compat.cpp abc_interpreter.cpp flashexternal.cpp flashtext.cpp flashmedia.cpp flv.cpp netutils.cpp timer.cpp decoder.cpp threading.cpp flashxml.cpp graphics.cpp)
IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i[3-6]86")
	SET(LIBSPARK_SOURCES ${LIBSPARK_SOURCES} fastpaths_32.asm)
ELSEIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|amd64")
	SET(LIBSPARK_SOURCES ${LIBSPARK_SOURCES} fastpaths_64.asm)
ELSE()
	MESSAGE(SEND_ERROR "Platform ${CMAKE_SYSTEM_PROCESSOR} not supported")
ENDIF()


IF(COMPILE_PLUGIN)
	pkg_check_modules(GTK REQUIRED gtk+-2.0 gtkglext-1.0)
	INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})
	ADD_SUBDIRECTORY(plugin-dir)
ENDIF(COMPILE_PLUGIN)

IF(COMPILE_LIGHTSPARK OR COMPILE_TIGHTSPARK)
	ADD_LIBRARY(spark STATIC ${LIBSPARK_SOURCES})
	TARGET_LINK_LIBRARIES(spark ${lib_pthread} ${lib_glew} ${EXTRA_LIBS_LIBRARIES} ${ZLIB_LIBRARIES} ${LLVM_LIBS_CORE} ${LLVM_LIBS_JIT} 
			${SDL_LIBRARY} ${CURL_LIBRARIES})
ENDIF(COMPILE_LIGHTSPARK OR COMPILE_TIGHTSPARK)

IF(COMPILE_LIGHTSPARK)
	ADD_EXECUTABLE(lightspark main.cpp)
	TARGET_LINK_LIBRARIES(lightspark spark)
	SET_TARGET_PROPERTIES(lightspark PROPERTIES LINK_FLAGS "-Wl,--as-needed")

	SET(BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
	SET(DATADIR "${CMAKE_INSTALL_PREFIX}/share")

	IF(UNIX)
		INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lightspark.frag DESTINATION ${DATADIR}/lightspark)
		INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lightspark.vert DESTINATION ${DATADIR}/lightspark)
		INSTALL(TARGETS lightspark DESTINATION ${BINDIR})
	ENDIF(UNIX)
ENDIF(COMPILE_LIGHTSPARK)

IF(COMPILE_TIGHTSPARK)
	ADD_EXECUTABLE(tightspark tightspark.cpp)
	TARGET_LINK_LIBRARIES(tightspark spark)
	SET_TARGET_PROPERTIES(tightspark PROPERTIES LINK_FLAGS "-Wl,--as-needed")
ENDIF(COMPILE_TIGHTSPARK)
