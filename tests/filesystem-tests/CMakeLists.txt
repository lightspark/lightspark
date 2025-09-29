#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)
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

CMAKE_MINIMUM_REQUIRED(VERSION 2.6...3.19)

# Name & Version
PROJECT(LIGHTSPARK_FILESYSTEM_TESTS)
SET(FILESYSTEM_MAJOR_VERSION "0")
SET(FILESYSTEM_MINOR_VERSION "1")
SET(FILESYSTEM_PATCH_VERSION "0")

SET(RUNNER_DIR ${PROJECT_SOURCE_DIR}/../test-runner)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${RUNNER_DIR}/cmake)
SET(FILESYSTEM_VERSION "${FILESYSTEM_MAJOR_VERSION}.${FILESYSTEM_MINOR_VERSION}.${FILESYSTEM_PATCH_VERSION}")

ADD_SUBDIRECTORY(src)
