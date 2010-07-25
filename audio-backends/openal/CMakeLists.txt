#**************************************************************************
#    Lightaudiobackend, a free flash player implementation
#
#    Copyright (C) 2010  Giacomo Spigler <g.spigler@sssup.it>
#    Copyright (C) 2010  Alessandro Pignotti <a.pignotti@sssup.it>
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

INCLUDE_DIRECTORIES(.)
SET(LIBPULSE_SOURCES ${LIBPULSE_SOURCES} openalstream.cpp)


## liblightaudiobackend-audiopulse.so target
#ADD_LIBRARY(audiobackend SHARED ${LIBPULSE_SOURCES})
#SET_TARGET_PROPERTIES(audiobackend PROPERTIES OUTPUT_NAME lightspark-pulsebackend)
#TARGET_LINK_LIBRARIES(audiobackend ${SOUND_LIBS_LIBRARIES} ${EXTRA_LIBS_LIBRARIES} ${LIBAVCODEC_LIBRARIES} ${ZLIB_LIBRARIES} ${LLVM_LIBS_CORE} ${LLVM_LIBS_JIT} ${SDL_LIBRARY} ${CURL_LIBRARIES} ${GTK_LIBRARIES} ${lib_glew})
#SET_TARGET_PROPERTIES(audiobackend PROPERTIES LINK_FLAGS "${LLVM_LDFLAGS} -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/../../lightspark-pulsebackend.expmap")
#SET_TARGET_PROPERTIES(audiobackend PROPERTIES LINK_INTERFACE_LIBRARIES "")
#
#SET_TARGET_PROPERTIES(audiobackend PROPERTIES VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")
#SET_TARGET_PROPERTIES(audiobackend PROPERTIES SOVERSION "${MAJOR_VERSION}.${MINOR_VERSION}")
