#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2010-2011  Alessandro Pignotti <a.pignotti@sssup.it>
#    Copyright (C) 2010  Giacomo Spigler <g.spigler@sssup.it>
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
SET(MINOR_VERSION "5")
SET(PATCH_VERSION "2")
SET(SUBPATCH_VERSION "1")

IF(EXISTS "${CMAKE_SOURCE_DIR}/.git/")
	SET(GIT_VERSION "-git")
ELSE(EXISTS "${CMAKE_SOURCE_DIR}/.git/")
	SET(GIT_VERSION "")
ENDIF(EXISTS "${CMAKE_SOURCE_DIR}/.git/")

IF(SUBPATCH_VERSION EQUAL "0")
	ADD_DEFINITIONS(-DVERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}${GIT_VERSION}")
ELSE(SUBPATCH_VERSION EQUAL "0")
	ADD_DEFINITIONS(-DVERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}.${SUBPATCH_VERSION}${GIT_VERSION}")
ENDIF(SUBPATCH_VERSION EQUAL "0")

ADD_DEFINITIONS(-DSHORTVERSION="${MINOR_VERSION}${PATCH_VERSION}${SUBPATCH_VERSION}")

# GCC 4.4+ is required, GCC_VERSION macro taken from GCC manual
INCLUDE(CheckCSourceCompiles)
IF(CMAKE_COMPILER_IS_GNUCC)
   CHECK_C_SOURCE_COMPILES("
   #define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
   #if GCC_VERSION < 40400
   #error
   #endif
   int main() { return 0; }" GCC_IS_4_4)
   IF(NOT GCC_IS_4_4)
	  MESSAGE(FATAL_ERROR "GCC 4.4+ is required.")
   ENDIF(NOT GCC_IS_4_4)
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# Find put the path of the gnash executable, if available
FIND_PROGRAM(GNASH_EXE_PATH NAMES gnash)

IF(GNASH_EXE_PATH)
	MESSAGE(STATUS "Found gnash path: ${GNASH_EXE_PATH}")
ELSE(GNASH_EXE_PATH)
# Set a default path
	MESSAGE(STATUS "Gnash not found")
	SET(GNASH_EXE_PATH "/usr/bin/gnash")
ENDIF(GNASH_EXE_PATH)

# Find out CPU architecture ...
# we could also use IF(CMAKE_SIZEOF_VOID_P EQUAL 4) to determine if it's a 32 or a 64bit arch
#
# Setting the value of LIB_SUFFIX if not specified (We could build 32bit version on 64bit)
# However, some distro don't implement FHS the same way
# Suse, Redhat put 64bit libs in /lib64; Debian use /lib for 64bit, but specifies /lib32 for 32bit libs
# See FHS 2.3 for Lib directories under each architecture 

# Some directory shortcuts
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/conf)
# If we're gcc, then use nasm to get fastpath.  If MSVC, just use inline asm to get around
# CMake issues 
IF(CMAKE_COMPILER_IS_GNUCC)
   INCLUDE(CMakeASM-NASMCompiler)
ENDIF(CMAKE_COMPILER_IS_GNUCC)
IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^i[3-6]86$|^x86$")
	SET(i386 1)
	SET(LIB_SUFFIX "" CACHE STRING "Choose the suffix of the lib folder (if any) : None 32")
	# nasm for assembly optimizations
	IF(CMAKE_COMPILER_IS_GNUCC)
	   ENABLE_LANGUAGE(ASM-NASM)        
	ENDIF(CMAKE_COMPILER_IS_GNUCC)
ELSEIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "unknown" AND ${CMAKE_SYSTEM} MATCHES "GNU-0.3")
	# GNU Hurd is i386
	SET(i386 1)
	SET(LIB_SUFFIX "" CACHE STRING "Choose the suffix of the lib folder (if any) : None 32")
	# nasm for assembly optimizations
	IF(CMAKE_COMPILER_IS_GNUCC)
	   ENABLE_LANGUAGE(ASM-NASM)
	ENDIF ()
ELSEIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^x86_64$|^amd64$")
	SET(x86_64 1)
	SET(LIB_SUFFIX "" CACHE STRING "Choose the suffix of the lib folder (if any) : None 64")
	# nasm for assembly optimizations
	IF(CMAKE_COMPILER_IS_GNUCC)
	  ENABLE_LANGUAGE(ASM-NASM)
	ENDIF(CMAKE_COMPILER_IS_GNUCC)
ELSEIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc")
	SET(ppc 1)
	SET(LIB_SUFFIX "" CACHE STRING "Choose the suffix of the lib folder (if any) : None ppc")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maltivec -D__STDC_CONSTANT_MACROS -fPIC")
ELSE(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^i[3-6]86$|^x86$")
	# All non-x86 non-ppc architectures.
	SET(LIB_SUFFIX "" CACHE STRING "Choose the suffix of the lib folder (if any) : None")
ENDIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^i[3-6]86$|^x86$")

# Using relative folders, cmake happens by itself CMAKE_INSTALL_PREFIX
IF(UNIX)
	 SET(ETCDIR "/etc") #Using absolute folder
   SET(BINDIR "bin")
   SET(DATADIR "${CMAKE_INSTALL_PREFIX}/share")
   SET(LIBDIR "lib${LIB_SUFFIX}")
   SET(PRIVATELIBDIR "${CMAKE_INSTALL_PREFIX}/${LIBDIR}/lightspark")
   SET(CMAKE_INSTALL_RPATH "${PRIVATELIBDIR}")
ENDIF(UNIX)

# Setting variables
SET(AUDIO_BACKEND "pulseaudio" CACHE STRING "Which audio backends should be built?")
SET(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install prefix, default is /usr/local (UNIX) and C:\\Program Files (Windows)")
SET(COMPILE_LIGHTSPARK TRUE CACHE BOOL "Compile Lightspark?")
SET(COMPILE_TIGHTSPARK TRUE CACHE BOOL "Compile Tightspark?")
SET(COMPILE_PLUGIN TRUE CACHE BOOL "Compile the browser plugin?")
SET(ENABLE_CURL TRUE CACHE BOOL "Enable CURL? (Required for Downloader functionality)")
SET(ENABLE_GLES2 FALSE CACHE BOOL "Build with OpenGLES 2.0 support instead of OpenGL")
SET(ENABLE_LIBAVCODEC TRUE CACHE BOOL "Enable libavcodec and dependent functionality?")
SET(ENABLE_RTMP FALSE CACHE BOOL "Enable librtmp and dependent functionality?")
SET(ENABLE_PROFILING FALSE CACHE BOOL "Enable profiling support? (Causes performance issues)")
SET(PLUGIN_DIRECTORY "${LIBDIR}/mozilla/plugins" CACHE STRING "Directory to install Firefox plugin to")
IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: Debug Release RelWithDebInfo Profile" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# The library is searched in its private library directory
SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(PRIVATELIBDIR "${CMAKE_INSTALL_PREFIX}/${LIBDIR}/lightspark")
SET(CMAKE_INSTALL_RPATH "${PRIVATELIBDIR}")

# Libraries we need
INCLUDE(FindPkgConfig REQUIRED)
INCLUDE(FindGettext REQUIRED)
INCLUDE(FindLLVM REQUIRED)
IF(${LLVM_STRING_VERSION} VERSION_LESS 2.8)
	MESSAGE(FATAL_ERROR "LLVM >=2.8 is required!")
ENDIF(${LLVM_STRING_VERSION} VERSION_LESS 2.8)
IF(${LLVM_STRING_VERSION} VERSION_EQUAL 2.9)
	MESSAGE(FATAL_ERROR "LLVM !=2.9 is required!")
ENDIF(${LLVM_STRING_VERSION} VERSION_EQUAL 2.9)
IF(${LLVM_STRING_VERSION} VERSION_EQUAL 2.8)
	ADD_DEFINITIONS(-DLLVM_28)
ENDIF(${LLVM_STRING_VERSION} VERSION_EQUAL 2.8)
INCLUDE(FindZLIB REQUIRED)
INCLUDE(FindFreetype REQUIRED)
IF(NOT(ENABLE_GLES2))
INCLUDE(FindOpenGL REQUIRED)
INCLUDE(FindGLEW REQUIRED)
ENDIF(NOT(ENABLE_GLES2))
INCLUDE(FindPCRE REQUIRED)
INCLUDE(FindJPEG REQUIRED)
IF(UNIX)
	pkg_check_modules(XMLPP REQUIRED libxml++-2.6>=2.33.1)
ENDIF(UNIX)

find_package(Boost COMPONENTS filesystem system REQUIRED)
if(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
ENDIF(Boost_FOUND) 
IF(UNIX)
  pkg_check_modules(EXTRA_LIBS REQUIRED x11 cairo pangocairo)
ENDIF(UNIX)

IF(ENABLE_GLES2)
  pkg_check_modules(GLES2 REQUIRED egl glesv2)
  ADD_DEFINITIONS(-DENABLE_GLES2)
ENDIF(ENABLE_GLES2)

IF(WIN32)
  INCLUDE(FindWin32Stdint REQUIRED)
  INCLUDE_DIRECTORIES(${STDINT_INCLUDE_DIRS})
  SET(EXTRA_LIBS_LIBRARIES ${EXTRA_LIBS_LIBRARIES} WS2_32.lib)
ENDIF(WIN32)

IF(UNIX)
  IF(AUDIO_BACKEND)
    ADD_DEFINITIONS(-DAUDIO_BACKEND="${AUDIO_BACKEND}")
  ENDIF(AUDIO_BACKEND)
ENDIF(UNIX)

IF(ENABLE_LIBAVCODEC)
	pkg_check_modules(FFMPEG libavcodec libavutil libavformat)
	IF(NOT(FFMPEG_FOUND))
		INCLUDE(FindFFMpeg REQUIRED)
	ENDIF(NOT(FFMPEG_FOUND))
	# Compatibility checks for ffmpeg deprecated functions
  INCLUDE(CheckFunctionExists REQUIRED)
  SET(CMAKE_REQUIRED_FLAGS ${LIBAVCODEC_CFLAGS})
  SET(CMAKE_REQUIRED_INCLUDES ${FFMPEG_INCLUDE_DIRS})
  SET(CMAKE_REQUIRED_LIBRARIES ${FFMPEG_LIBRARIES})
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

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GTK REQUIRED gtk+-2.0)

INCLUDE_DIRECTORIES(${LLVM_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})
IF(ENABLE_GLES2)
	INCLUDE_DIRECTORIES(${GLES2_INCLUDE_DIRS})
ELSE()
	INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
	INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIR})
ENDIF(ENABLE_GLES2)
INCLUDE_DIRECTORIES(${PCRE_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${XMLPP_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${EXTRA_LIBS_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})

IF(ENABLE_LIBAVCODEC)
  INCLUDE_DIRECTORIES(${FFMPEG_INCLUDE_DIRS})
  SET(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${FFMPEG_LIBRARIES})
ENDIF(ENABLE_LIBAVCODEC)	

IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--no-undefined ${CMAKE_EXE_LINKER_FLAGS}")
  SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed,-z,noexecstack ${CMAKE_SHARED_LINKER_FLAGS}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

LINK_DIRECTORIES(${LLVM_LIB_DIR})

IF(COMPILE_PLUGIN)
  ADD_DEFINITIONS(-DMOZ_X11)
ENDIF(COMPILE_PLUGIN)

IF(ENABLE_CURL)
  INCLUDE(FindCURL REQUIRED)
  INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
  SET(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${CURL_LIBRARIES})
  ADD_DEFINITIONS(-DENABLE_CURL)
ENDIF(ENABLE_CURL)

IF(ENABLE_RTMP)
  pkg_check_modules(RTMP REQUIRED librtmp)
  SET(OPTIONAL_LIBRARIES ${OPTIONAL_LIBRARIES} ${RTMP_LIBRARIES})
  ADD_DEFINITIONS(-DENABLE_RTMP)
ENDIF(ENABLE_RTMP)

IF(ENABLE_PROFILING)
  ADD_DEFINITIONS(-DPROFILING_SUPPORT)
ENDIF(ENABLE_PROFILING)

IF(ENABLE_DEBIAN_ALTERNATIVES)
  SET(PLUGIN_DIRECTORY ${PRIVATELIBDIR})
ENDIF(ENABLE_DEBIAN_ALTERNATIVES)

# Compiler defaults flags for different profiles
IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_CXX_FLAGS
	  "${CMAKE_CXX_FLAGS} -Wall -Wnon-virtual-dtor -Woverloaded-virtual -pipe -fvisibility=hidden -fvisibility-inlines-hidden -std=c++0x -Wdisabled-optimization -Wextra -Wno-unused-parameter -Wno-invalid-offsetof")
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DEXPENSIVE_DEBUG")
  SET(CMAKE_CXX_FLAGS_PROFILE "-g -pg -O2")
  SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG")
  SET(CMAKE_CXX_FLAGS_LEANDEBUG "-g -O2")
  ADD_DEFINITIONS(-DLS_DATADIR="${DATADIR}/lightspark" -DGNASH_PATH="${GNASH_EXE_PATH}" -DPRIVATELIBDIR="${PRIVATELIBDIR}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

IF(MSVC)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DVPCOMPAT -DPTW32_STATIC_LIB -DPCRE_STATIC)
ENDIF(MSVC)

# Check if the system have atomic header ( gcc >= 4.6 )
INCLUDE (CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX (atomic HAVE_ATOMIC)
IF(HAVE_ATOMIC)
  ADD_DEFINITIONS(-DHAVE_ATOMIC)
ENDIF(HAVE_ATOMIC)

# Setting the output directories, so we can build all profiles without mixmatching
IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}/bin")
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}/lib${LIB_SUFFIX}")
  #SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/objs/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}/lib${LIB_SUFFIX}" CACHE PATH "Static libraries output directory")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

#Install icons and desktop file
if(UNIX)
  FOREACH(i 16 20 22 24 32 36 48 64 72 96 128 192 256 )
    INSTALL(FILES media/lightspark-ico-${i}x${i}.png
            DESTINATION ${DATADIR}/icons/hicolor/${i}x${i}/apps
            RENAME lightspark.png
           )
  ENDFOREACH(i)
  INSTALL(FILES media/lightspark-ico.svg
          DESTINATION ${DATADIR}/icons/hicolor/scalable/apps
          RENAME lightspark.svg
         )
  INSTALL(FILES media/lightspark.desktop DESTINATION ${DATADIR}/applications )
endif(UNIX)

# generate locales for gettext
GETTEXT_CREATE_TRANSLATIONS(i18n/lightspark.pot ALL i18n/pl.po i18n/fr.po
        i18n/zh_CN.po)

if(UNIX)
	INSTALL(FILES etc/xdg/lightspark.conf
		      DESTINATION ${ETCDIR}/xdg/)
endif(UNIX)

SUBDIRS(src)
