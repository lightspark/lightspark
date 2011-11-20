#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2010-2011  Alessandro Pignotti <a.pignotti@sssup.it>
#    Copyright (C) 2010  Giacomo Spigler <g.spigler@sssup.it>
#    Copyright (C) 2010  Alexandre Demers <papouta@hotmail.com>
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

## Compiler defaults flags for different profiles
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wnon-virtual-dtor -Woverloaded-virtual -pipe -fvisibility=hidden -fvisibility-inlines-hidden -std=c++0x")
#
#SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DEXPENSIVE_DEBUG")
#SET(CMAKE_CXX_FLAGS_PROFILE "-g -pg -O2")
#SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
#SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG")
#SET(CMAKE_CXX_FLAGS_DEBIAN "-O2 -DNDEBUG")
#SET(CMAKE_CXX_FLAGS_LEANDEBUG "-g -O2")

INCLUDE_DIRECTORIES(".")
INCLUDE_DIRECTORIES("..")

pkg_check_modules(OPENAL_LIBS REQUIRED openal)
INCLUDE_DIRECTORIES(${OPENAL_LIBS_INCLUDE_DIRS})
if(!Boost_FOUND)
  find_package(Boost COMPONENTS filesystem system regex)
  if(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  endif(Boost_FOUND)
endif(!Boost_FOUND)
ADD_DEFINITIONS(-DPULSE_BACKEND=TRUE)

SET(OPENALPLUGIN_SOURCES OpenALPlugin.cpp ../../IPlugin.cpp ../IAudioPlugin.cpp)

# liblightsparkpulseplugin.so target
ADD_LIBRARY(openalplugin SHARED ${OPENALPLUGIN_SOURCES})
TARGET_LINK_LIBRARIES(openalplugin spark) #Need to link some functions with the decoders
TARGET_LINK_LIBRARIES(openalplugin ${OPENAL_LIBS_LIBRARIES} ${Boost_LIBRARIES})
SET_TARGET_PROPERTIES(openalplugin PROPERTIES OUTPUT_NAME lightsparkopenalplugin)
#SET_TARGET_PROPERTIES(openalplugin PROPERTIES VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")
#SET_TARGET_PROPERTIES(openalplugin PROPERTIES SOVERSION "${MAJOR_VERSION}.${MINOR_VERSION}")

INSTALL(TARGETS openalplugin LIBRARY DESTINATION ${PLUGINSDIR})
