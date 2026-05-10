# CMakeLists.txt
#
#     Author: Fabian Meyer
# Created On: 26 Dec 2015
#    License: MIT

cmake_minimum_required(VERSION 3.15)

project(inifile-cpp)

include(CTest)

set(INICPP_CXX_STANDARD "11" CACHE STRING "C++ standard to use when building tests & examples.")
option(GENERATE_COVERAGE "Enable generating code coverage" OFF)
option(BUILD_TESTS "Enable building unit tests" OFF)
option(BUILD_EXAMPLES "Enable building example applications" OFF)

set(CMAKE_CXX_STANDARD ${INICPP_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wall -Wextra)
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/WX /wd4530)
endif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")


add_subdirectory(dep)

add_library(inicpp INTERFACE)
target_include_directories(inicpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
add_library(inicpp::inicpp ALIAS inicpp)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/inicpp.h TYPE INCLUDE)

if(GENERATE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Add required flags (GCC & LLVM/Clang)
    target_compile_options(inicpp INTERFACE -O0 -g --coverage)
    target_link_options(inicpp INTERFACE --coverage)
endif(GENERATE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

if(${BUILD_TESTS})
    add_subdirectory(test)
endif(${BUILD_TESTS})

if(${BUILD_EXAMPLES})
    add_subdirectory(examples)
endif(${BUILD_EXAMPLES})
