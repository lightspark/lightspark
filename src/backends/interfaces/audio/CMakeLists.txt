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

include_directories(".")
INCLUDE_DIRECTORIES("..")

#Export symbols
IF(WIN32)
  ADD_DEFINITIONS("-DDLL_PUBLIC=__declspec(dllexport)")
ENDIF()


IF(${AUDIO_BACKEND} MATCHES "pulse")
    add_subdirectory(pulse)
ENDIF(${AUDIO_BACKEND} MATCHES "pulse")
IF(${AUDIO_BACKEND} MATCHES "openal")
    add_subdirectory(openal)
ENDIF(${AUDIO_BACKEND} MATCHES "openal")
IF(${AUDIO_BACKEND} MATCHES "sdl")
    add_subdirectory(sdl)
ENDIF(${AUDIO_BACKEND} MATCHES "sdl")
