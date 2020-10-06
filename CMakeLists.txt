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
string(TOUPPER ${PROJECT_NAME} LIB_CMAKE_NAME)

# Detect whether this project is built by itself or included as a subdirectory.
lib_dependent_option(${LIB_CMAKE_NAME}_MAIN_BUILD "Build ${PROJECT_NAME} as a main project."
    "CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME")

# Define BUILD_TESTING option.
if(${LIB_CMAKE_NAME}_MAIN_BUILD)
    include(CTest)
endif()

# Define CMake options.
lib_option(CLANG_TIDY "Perform clang-tidy checks." ON)
lib_option(BUILD_TESTING "Build tests." ON)
lib_option(BUILD_EXAMPLES "Build examples." ON)
lib_option(BUILD_DOCS "Build docs." OFF)

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
if(${LIB_CMAKE_NAME}_BUILD_EXAMPLES)
    add_subdirectory(apps)
endif()

# Build tests.
if(${LIB_CMAKE_NAME}_BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Build docs.
if(${LIB_CMAKE_NAME}_BUILD_DOCS)
    find_package(Doxygen)
    if(NOT Doxygen_FOUND)
        message(WARNING "Unable to find Doxygen for ${PROJECT_NAME} docs.")
    else()
        add_subdirectory(docs)
    endif()
endif()
