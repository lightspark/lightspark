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

SET(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Install prefix" FORCE)
SET(COMPILE_LIGHTSPARK TRUE CACHE BOOL "Compile Lightspark?")
SET(COMPILE_TIGHTSPARK TRUE CACHE BOOL "Compile Tightspark?")
SET(COMPILE_PLUGIN FALSE CACHE BOOL "Compile the Firefox Plugin?")

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/conf)

INCLUDE(CMakeASM-NASMCompiler)

#LIBRARIES
INCLUDE(FindLLVM)
INCLUDE(FindSDL)
INCLUDE(FindCURL)
INCLUDE(FindPkgConfig)

pkg_check_modules(EXTRA_LIBS REQUIRED zlib gl libpcre libpcrecpp libavcodec libxml-2.0)

INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${EXTRA_LIBS_INCLUDE_DIRS})

FIND_LIBRARY(lib_pthread pthread REQUIRED)
FIND_LIBRARY(lib_glew GLEW REQUIRED)

LINK_LIBRARIES(${lib_pthread} ${lib_glew} ${EXTRA_LIBS_LIBRARIES})
LINK_LIBRARIES(${LLVM_LIBS_CORE} ${LLVM_LIBS_JIT})
LINK_LIBRARIES(${SDL_LIBRARY})
LINK_LIBRARIES(${CURL_LIBRARIES})

ENABLE_LANGUAGE(ASM-NASM)

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Debug")
ENDIF()

IF(COMPILE_PLUGIN)
	SUBDIRS(plugin-dir)
ENDIF(COMPILE_PLUGIN)

IF(COMPILE_LIGHTSPARK OR COMPILE_TIGHTSPARK)
	SET(CMAKE_CXX_FLAGS "-Wall -Wnon-virtual-dtor -Woverloaded-virtual -pipe")
	SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -D_GLIBCXX_NO_DEBUG")
	SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

	ADD_LIBRARY(spark STATIC fastpaths.asm swf.cpp swftypes.cpp tags.cpp geometry.cpp actions.cpp frame.cpp input.cpp streams.cpp tags_stub.cpp logger.cpp vm.cpp asobjects.cpp abc.cpp abc_codesynt.cpp abc_opcodes.cpp flashdisplay.cpp flashevents.cpp textfile.cpp thread_pool.cpp flashgeom.cpp flashnet.cpp flashsystem.cpp flashutils.cpp compat.cpp abc_interpreter.cpp flashexternal.cpp flashtext.cpp flashmedia.cpp flv.cpp netutils.cpp timer.cpp)
ENDIF(COMPILE_LIGHTSPARK OR COMPILE_TIGHTSPARK)

IF(COMPILE_LIGHTSPARK)
	ADD_EXECUTABLE(lightspark main.cpp)
	TARGET_LINK_LIBRARIES(lightspark spark)


	SET(BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
	SET(DATADIR "${CMAKE_INSTALL_PREFIX}/share")

	IF(UNIX)
		INSTALL(FILES lightspark.frag lightspark.vert DESTINATION ${DATADIR}/lightspark)
		INSTALL(FILES lightspark DESTINATION ${BINDIR})
	ENDIF(UNIX)
ENDIF(COMPILE_LIGHTSPARK)

IF(COMPILE_TIGHTSPARK)
	ADD_EXECUTABLE(tightspark tightspark.cpp)
	TARGET_LINK_LIBRARIES(tightspark spark)
ENDIF(COMPILE_TIGHTSPARK)

