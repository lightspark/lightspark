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

INCLUDE(FetchContent)

SET(RUNNER_SOURCES
	framework/backends/engineutils.cpp
	framework/backends/event_loop.cpp
	framework/backends/logger.cpp
	framework/backends/netutils.cpp
	framework/backends/timer.cpp
	framework/options.cpp
	framework/runner.cpp
	framework/test.cpp
	
	input/injector.cpp
	input/formats/ruffle/parser.cpp

	utils/args_parser.cpp
	utils/option_parser.cpp

	main.cpp
)

FetchContent_Declare(
	cpptrace
	GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
	GIT_TAG 4354eb21ea3565a73afdf32ec0ba768fd648c425
)

if (NOT WIN32)
	FIND_PACKAGE(LibUnwind)
	if (${LibUnwind_FOUND})
		MESSAGE(STATUS "Found libunwind")
		SET(CPPTRACE_UNWIND_WITH_LIBUNWIND ON)
		SET(SIGNAL_BACKTRACE ON)
	endif()
endif()

FetchContent_MakeAvailable(cpptrace)

SET(CMAKE_CXX_FLAGS "-Wall -pipe -std=c++17")

ADD_EXECUTABLE(test-runner ${RUNNER_SOURCES})

TARGET_INCLUDE_DIRECTORIES(
	test-runner
	PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/include
	PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/include/lightspark
	PUBLIC ${PROJECT_SOURCE_DIR}/3rdparty/include/lightspark/scripting
	PRIVATE ${PROJECT_SOURCE_DIR}/src
	PRIVATE ${PROJECT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(test-runner test++ spark cpptrace::cpptrace)

CONFIGURE_FILE("config.h.in" "${PROJECT_BINARY_DIR}/config.h" @ONLY)

if (WIN32)
	TARGET_LINK_OPTIONS(
		test-runner
		PUBLIC "-Wl,--allow-multiple-definition"
	)
	add_custom_command(
		TARGET test-runner POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:cpptrace::cpptrace>
		$<TARGET_FILE_DIR:test-runner>
	)
endif()

INSTALL(TARGETS test-runner RUNTIME DESTINATION ${BINDIR})
