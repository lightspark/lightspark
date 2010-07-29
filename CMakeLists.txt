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


CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Name & Version
PROJECT(LIGHTSPARK)
SET(MAJOR_VERSION "0")
SET(MINOR_VERSION "4")
SET(PATCH_VERSION "3")

# GCC 4.4+ is required, GCC_VERSION macro taken from GCC manual
INCLUDE(CheckCSourceCompiles)
CHECK_C_SOURCE_COMPILES("
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GCC_VERSION < 40400
#error
#endif
int main() { return 0; }" GCC_IS_4_4)
IF(NOT GCC_IS_4_4)
	MESSAGE(FATAL_ERROR "GCC 4.4+ is required.")
ENDIF(NOT GCC_IS_4_4)

# Find put the path of the gnash executable, if available
FIND_PROGRAM(GNASH_EXE_PATH NAMES gnash)

IF(GNASH_EXE_PATH)
	MESSAGE(STATUS "Found gnash path: ${GNASH_EXE_PATH}")
ELSE()
# Set a default path
	MESSAGE(STATUS "Gnash not found")
	SET(GNASH_EXE_PATH "/usr/bin/gnash")
ENDIF()

# Find out CPU architecture ...
# we could also use IF(CMAKE_SIZEOF_VOID_P EQUAL 4) to determine if it's a 32 or a 64bit arch
#
# Setting the value of LIB_SUFFIX if not specified (We could build 32bit version on 64bit)
# However, some distro don't implement FHS the same way
# Suse, Redhat put 64bit libs in /lib64; Debian use /lib for 64bit, but specifies /lib32 for 32bit libs
# See FHS 2.3 for Lib directories under each architecture 

# Some directory shortcuts
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/conf)
INCLUDE(CMakeASM-NASMCompiler)

IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i[3-6]86")
	SET(i386 1)
	SET(LIB_SUFFIX "" CACHE STRING "Choose the suffix of the lib folder (if any) : None 32")
	# nasm for assembly optimizations
	ENABLE_LANGUAGE(ASM-NASM)
ELSEIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|amd64")
	SET(x86_64 1)
	SET(LIB_SUFFIX "" CACHE STRING "Choose the suffix of the lib folder (if any) : None 64")
	# nasm for assembly optimizations
	ENABLE_LANGUAGE(ASM-NASM)
ELSEIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc")
	SET(ppc 1)
	SET(LIB_SUFFIX "" CACHE STRING "Choose the suffix of the lib folder (if any) : None ppc")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maltivec")
ELSE()
	MESSAGE(SEND_ERROR "Platform ${CMAKE_SYSTEM_PROCESSOR} not supported")
ENDIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "i[3-6]86")

# Using relative folders, cmake happens by itself CMAKE_INSTALL_PREFIX
SET(BINDIR "bin")
SET(DATADIR "share")
SET(LIBDIR "lib${LIB_SUFFIX}")
SET(PRIVATELIBDIR "${LIBDIR}/lightspark")

# Setting variables
SET(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install prefix, default is /usr/local (UNIX) and C:\\Program Files (Windows)")
SET(COMPILE_LIGHTSPARK TRUE CACHE BOOL "Compile Lightspark?")
SET(COMPILE_TIGHTSPARK TRUE CACHE BOOL "Compile Tightspark?")
SET(COMPILE_PLUGIN FALSE CACHE BOOL "Compile the Firefox Plugin?")
SET(ENABLE_SOUND TRUE CACHE BOOL "Enable sound?")
SET(ENABLE_CURL TRUE CACHE BOOL "Enable CURL? (Required for Downloader functionality)")
SET(ENABLE_LIBAVCODEC TRUE CACHE BOOL "Enable libavcodec and dependent functionality?")
SET(PLUGIN_DIRECTORY "${LIBDIR}/mozilla/plugins" CACHE STRING "Directory to install Firefox plugin to")
IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: Debug Release RelWithDebInfo Profile Debian" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# The library is searched in its private library directory
SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${PRIVATELIBDIR}")

# Libraries we need
INCLUDE(FindLLVM REQUIRED)
INCLUDE(FindSDL REQUIRED)
INCLUDE(FindPkgConfig REQUIRED)
INCLUDE(FindZLIB REQUIRED)
INCLUDE(FindThreads REQUIRED)

pkg_check_modules(EXTRA_LIBS REQUIRED gl libpcrecpp libavutil ftgl x11 glu fontconfig glew)
IF(ENABLE_SOUND)
	ADD_DEFINITIONS(-DENABLE_SOUND)
	pkg_check_modules(SOUND_LIBS REQUIRED libpulse)
ENDIF(ENABLE_SOUND)

IF(ENABLE_LIBAVCODEC)
	# Compatibility checks for ffmpeg deprecated functions
	pkg_check_modules(LIBAVCODEC REQUIRED libavcodec)
	INCLUDE(CheckFunctionExists REQUIRED)
	SET(CMAKE_REQUIRED_FLAGS ${LIBAVCODEC_CFLAGS})
	SET(CMAKE_REQUIRED_INCLUDES ${LIBAVCODEC_INCLUDE_DIRS})
	SET(CMAKE_REQUIRED_LIBRARIES ${LIBAVCODEC_LIBRARIES})
	CHECK_FUNCTION_EXISTS(avcodec_decode_video2 HAVE_AVCODEC_DECODE_VIDEO2)
	CHECK_FUNCTION_EXISTS(avcodec_decode_audio3 HAVE_AVCODEC_DECODE_AUDIO3)
	SET(CMAKE_REQUIRED_FLAGS)
	SET(CMAKE_REQUIRED_INCLUDES)
	SET(CMAKE_REQUIRED_LIBRARIES)
	IF(HAVE_AVCODEC_DECODE_VIDEO2)
		ADD_DEFINITIONS(-DHAVE_AVCODEC_DECODE_VIDEO2)
	ENDIF(HAVE_AVCODEC_DECODE_VIDEO2)
	IF(HAVE_AVCODEC_DECODE_AUDIO3)
		ADD_DEFINITIONS(-DHAVE_AVCODEC_DECODE_AUDIO3)
	ENDIF(HAVE_AVCODEC_DECODE_AUDIO3)
	ADD_DEFINITIONS(-DENABLE_LIBAVCODEC)
ENDIF(ENABLE_LIBAVCODEC)

INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${Threads_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${EXTRA_LIBS_INCLUDE_DIRS})
IF(ENABLE_LIBAVCODEC)
	INCLUDE_DIRECTORIES(${LIBAVCODEC_INCLUDE_DIRS})
	SET(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${LIBAVCODEC_LIBRARIES})
ENDIF(ENABLE_LIBAVCODEC)	
INCLUDE_DIRECTORIES(${SOUND_LIBS_INCLUDE_DIRS})

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--no-undefined")
SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")

IF(COMPILE_PLUGIN)
	pkg_check_modules(GTK REQUIRED gtk+-2.0)
	INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})
	ADD_DEFINITIONS(-DMOZ_X11 -DCOMPILE_PLUGIN)
ENDIF(COMPILE_PLUGIN)

IF(ENABLE_CURL)
	INCLUDE(FindCURL REQUIRED)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
	SET(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${CURL_LIBRARIES})
	ADD_DEFINITIONS(-DENABLE_CURL)
ENDIF(ENABLE_CURL)

# Compiler defaults flags for different profiles
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wnon-virtual-dtor -Woverloaded-virtual -pipe -fvisibility=hidden -fvisibility-inlines-hidden -std=c++0x")

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DEXPENSIVE_DEBUG")
SET(CMAKE_CXX_FLAGS_PROFILE "-g -pg -O2")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_DEBIAN "-O2 -DNDEBUG")

ADD_DEFINITIONS(-DDATADIR="${CMAKE_INSTALL_PREFIX}/share/lightspark" -DGNASH_PATH="${GNASH_EXE_PATH}")

# Setting the output directories, so we can build all profiles without mixmatching
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}/bin")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}/lib${LIB_SUFFIX}")
#SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/objs/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}/lib${LIB_SUFFIX}" CACHE PATH "Static libraries output directory")

# liblightspark.so sources
SET(LIBSPARK_SOURCES swf.cpp swftypes.cpp tags.cpp geometry.cpp actions.cpp frame.cpp input.cpp streams.cpp tags_stub.cpp logger.cpp vm.cpp asobjects.cpp abc.cpp abc_codesynt.cpp abc_opcodes.cpp flashdisplay.cpp flashevents.cpp textfile.cpp thread_pool.cpp flashgeom.cpp flashnet.cpp flashsystem.cpp flashutils.cpp compat.cpp abc_interpreter.cpp flashexternal.cpp flashtext.cpp flashmedia.cpp flv.cpp netutils.cpp timer.cpp decoder.cpp threading.cpp flashxml.cpp graphics.cpp sound.cpp)
IF(${i386})
	SET(LIBSPARK_SOURCES ${LIBSPARK_SOURCES} fastpaths_32.asm)
ELSEIF(${x86_64})
	SET(LIBSPARK_SOURCES ${LIBSPARK_SOURCES} fastpaths_64.asm)
ELSEIF(${ppc})
	SET(LIBSPARK_SOURCES ${LIBSPARK_SOURCES} fastpaths_ppc.cpp)
ENDIF(${i386})

# liblightspark.so target
ADD_LIBRARY(spark SHARED ${LIBSPARK_SOURCES})
SET_TARGET_PROPERTIES(spark PROPERTIES OUTPUT_NAME lightspark)
TARGET_LINK_LIBRARIES(spark ${SOUND_LIBS_LIBRARIES} ${EXTRA_LIBS_LIBRARIES} ${ZLIB_LIBRARIES} ${LLVM_LIBS_CORE} ${LLVM_LIBS_JIT} ${SDL_LIBRARY} ${OPTIONAL_LIBRARIES} ${GTK_LIBRARIES} ${lib_glew})
SET_TARGET_PROPERTIES(spark PROPERTIES LINK_FLAGS "${LLVM_LDFLAGS} -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/lightspark.expmap")
SET_TARGET_PROPERTIES(spark PROPERTIES LINK_INTERFACE_LIBRARIES "")

SET_TARGET_PROPERTIES(spark PROPERTIES VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")
SET_TARGET_PROPERTIES(spark PROPERTIES SOVERSION "${MAJOR_VERSION}.${MINOR_VERSION}")

IF(UNIX)
	INSTALL(TARGETS spark LIBRARY DESTINATION ${PRIVATELIBDIR})
ENDIF(UNIX)

# lightspark executable target
IF(COMPILE_LIGHTSPARK)
	ADD_EXECUTABLE(lightspark main.cpp)
	TARGET_LINK_LIBRARIES(lightspark spark)
	TARGET_LINK_LIBRARIES(lightspark ${SDL_LIBRARY})

	IF(UNIX)
		INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lightspark.frag DESTINATION ${DATADIR}/lightspark)
		INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lightspark.vert DESTINATION ${DATADIR}/lightspark)
		INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/docs/man/lightspark.1 DESTINATION share/man/man1)
		INSTALL(TARGETS lightspark RUNTIME DESTINATION ${BINDIR})
	ENDIF(UNIX)
ENDIF(COMPILE_LIGHTSPARK)

# tightspark executable target
IF(COMPILE_TIGHTSPARK)
	ADD_EXECUTABLE(tightspark tightspark.cpp)
	TARGET_LINK_LIBRARIES(tightspark spark)

	IF(UNIX)
		INSTALL(TARGETS tightspark RUNTIME DESTINATION ${BINDIR})
	ENDIF(UNIX)
ENDIF(COMPILE_TIGHTSPARK)

# Browser plugin
IF(COMPILE_PLUGIN)
	ADD_SUBDIRECTORY(plugin-dir)
ENDIF(COMPILE_PLUGIN)

