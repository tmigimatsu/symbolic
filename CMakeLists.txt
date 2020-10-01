############################################################
# CMakeLists for the symbolic library.
#
# Copyright 2020. All Rights Reserved.
#
# Created: May 7, 2020
# Authors: Toki Migimatsu
############################################################

cmake_minimum_required(VERSION 3.11)

# Define project.
project(symbolic
    VERSION 1.0.0
    DESCRIPTION "Library for manipulating PDDL symbols"
    LANGUAGES CXX)

# Detect whether this project is built by itself or included as a subdirectory.
include(CMakeDependentOption)
cmake_dependent_option(SYMBOLIC_MAIN_BUILD "Build symbolic as a main project."
    ON "CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME" OFF)

# Define BUILD_TESTING option.
if(SYMBOLIC_MAIN_BUILD)
    include(CTest)
endif()

# Define CMake options.
cmake_dependent_option(SYMBOLIC_CLANG_TIDY "Perform clang-tidy checks." ON SYMBOLIC_MAIN_BUILD OFF)
cmake_dependent_option(SYMBOLIC_BUILD_EXAMPLES "Build examples." ON SYMBOLIC_MAIN_BUILD OFF)
cmake_dependent_option(SYMBOLIC_BUILD_TESTING "Build tests." ON "SYMBOLIC_MAIN_BUILD AND BUILD_TESTING" OFF)
option(SYMBOLIC_BUILD_DOCS "Generate docs." OFF)

# Set default build type to release.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set output directories.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Include custom CMake utilities.
include(cmake/utils.cmake)

# Standardize install directories.
include(GNUInstallDirs)

# Build the library.
add_subdirectory(src)

# Build examples.
if(SYMBOLIC_BUILD_EXAMPLES)
    add_subdirectory(apps)
endif()

# Build tests.
if(SYMBOLIC_BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Build docs.
if(SYMBOLIC_BUILD_DOCS)
    find_package(Doxygen)
    if(NOT Doxygen_FOUND)
        message(WARNING "Unable to find Doxygen for ${PROJECT_NAME} docs.")
    else()
        add_subdirectory(docs)
    endif()
endif()
