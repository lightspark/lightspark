#**************************************************************************
#    Lightspark, a free flash player implementation
#
#    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)
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
PROJECT(LIGHTSPARK_TEST_RUNNER)
SET(RUNNER_MAJOR_VERSION "0")
SET(RUNNER_MINOR_VERSION "1")
SET(RUNNER_PATCH_VERSION "0")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

SET(RUNNER_VERSION "${RUNNER_MAJOR_VERSION}.${RUNNER_MINOR_VERSION}.${RUNNER_PATCH_VERSION}")
SET(TESTS_DIR "${PROJECT_SOURCE_DIR}")

ADD_SUBDIRECTORY(libtest++)
ADD_SUBDIRECTORY(src)
