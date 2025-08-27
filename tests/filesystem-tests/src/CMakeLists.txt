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

find_package(PkgConfig)

SET(FILESYSTEM_SOURCES
	main.cpp
	tests.cpp
	filesystem_tests.cpp
	path_tests.cpp
)

if (WIN32)
	SET(FILESYSTEM_SOURCES
		${FILESYSTEM_SOURCES}
		win_tests.cpp
	)
endif (WIN32)

# Libraries used by cpptrace.
PKG_CHECK_MODULES(zstd libzstd)
if (${zstd_FOUND})
	MESSAGE(STATUS "Found external zstd")
	SET(CPPTRACE_USE_EXTERNAL_ZSTD ON)
endif()

PKG_CHECK_MODULES(libdwarf libdwarf)
if (${libdwarf_FOUND})
	MESSAGE(STATUS "Found external libdwarf")
	SET(CPPTRACE_USE_EXTERNAL_LIBDWARF ON)
endif()

FIND_PACKAGE(LibUnwind)
if (${LibUnwind_FOUND})
	MESSAGE(STATUS "Found libunwind")
	SET(CPPTRACE_UNWIND_WITH_LIBUNWIND ON)
	SET(SIGNAL_BACKTRACE ON)
endif()

SET(CMAKE_CXX_FLAGS "-Wall -pipe -std=c++17")

ADD_EXECUTABLE(filesystem-tests ${FILESYSTEM_SOURCES})

TARGET_INCLUDE_DIRECTORIES(
	filesystem-tests
	PUBLIC ${RUNNER_DIR}/3rdparty/include
	PUBLIC ${RUNNER_DIR}/3rdparty/include/lightspark
	PUBLIC ${RUNNER_DIR}/3rdparty/include/lightspark/scripting
	PRIVATE ${RUNNER_DIR}/src
	PRIVATE ${PROJECT_SOURCE_DIR}/src
	PRIVATE ${PROJECT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(filesystem-tests test++ spark cpptrace::cpptrace)

CONFIGURE_FILE("config.h.in" "${PROJECT_BINARY_DIR}/config.h" @ONLY)

if (WIN32)
	TARGET_LINK_OPTIONS(
		filesystem-tests
		PUBLIC "-Wl,--allow-multiple-definition"
	)
	add_custom_command(
		TARGET filesystem-tests POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:cpptrace::cpptrace>
		$<TARGET_FILE_DIR:filesystem-tests>
	)
else()
	TARGET_LINK_LIBRARIES(filesystem-tests pthread)
endif()

INSTALL(TARGETS filesystem-tests RUNTIME DESTINATION ${BINDIR})
