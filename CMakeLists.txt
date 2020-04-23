############################################################
# CMakeLists for the SpatialDyn library
#
# Copyright 2020. All Rights Reserved.
#
# Created: May 7, 2020
# Authors: Toki Migimatsu
############################################################

# Require 3.6 to support pkg_check_modules IMPORTED_TARGET
cmake_minimum_required(VERSION 3.6)

# Define project
project(symbolic VERSION 1.0.0 LANGUAGES CXX)

# Set CMake flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# Define directories
set(LIB_NAME symbolic)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(LIB_CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake)
set(LIB_EXT_DIR ${PROJECT_SOURCE_DIR}/external)
set(LIB_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(LIB_SRC_DIR ${PROJECT_SOURCE_DIR}/src)

# Find dependencies
list(INSERT CMAKE_MODULE_PATH 0 ${LIB_CMAKE_DIR})

function(download_package PACKAGE_NAME)
    set(EXT_PACKAGE_VERSION ${ARGV1})
    if(EXISTS ${LIB_CMAKE_DIR}/Find${PACKAGE_NAME}.cmake.in)
        configure_file(${LIB_CMAKE_DIR}/Find${PACKAGE_NAME}.cmake.in
            ${CMAKE_BINARY_DIR}/Find${PACKAGE_NAME}.cmake @ONLY)
    endif()
    configure_file(${LIB_CMAKE_DIR}/${PACKAGE_NAME}-CMakeLists.txt.in
        ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}/CMakeLists.txt @ONLY)
    execute_process(COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR} .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${PACKAGE_NAME})
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${PACKAGE_NAME})
endfunction()

find_package(VAL 1.0.0 QUIET)
if(NOT VAL_FOUND)
    download_package(VAL 1.0.0)
    find_package(VAL 1.0.0 REQUIRED)
endif()

# set(VAL_LIB VAL)
# set(VAL_LIB_DIR ${CMAKE_BINARY_DIR}/VAL/build)
# set(VAL_SRC_DIR ${VAL_LIB_DIR}/src)
# set(VAL_INCLUDE_DIR ${VAL_LIB_DIR}/include)
# add_library(${VAL_LIB} STATIC)

# # Set include directories
# target_include_directories(${VAL_LIB}
#     PUBLIC
#         $<INSTALL_INTERFACE:include>
#         $<BUILD_INTERFACE:${VAL_INCLUDE_DIR}>
# )

# # Attach sources
# target_sources(${VAL_LIB} PRIVATE
#     ${VAL_SRC_DIR}/DebugWriteController.cpp
#     ${VAL_SRC_DIR}/pddl+.cpp
#     ${VAL_SRC_DIR}/ptree.cpp
#     ${VAL_SRC_DIR}/typecheck.cpp
# )

set(LIB_SRC
    ${LIB_SRC_DIR}/action.cc
    ${LIB_SRC_DIR}/formula.cc
    ${LIB_SRC_DIR}/normal_form.cc
    ${LIB_SRC_DIR}/object.cc
    ${LIB_SRC_DIR}/pddl.cc
    ${LIB_SRC_DIR}/proposition.cc
    ${LIB_SRC_DIR}/state.cc
    ${LIB_SRC_DIR}/planning/planner.cc
    ${LIB_SRC_DIR}/utils/parameter_generator.cc
)

# target_include_directories(${SYMBOLIC_BIN}
#     PUBLIC
#         $<INSTALL_INTERFACE:include>
#         $<BUILD_INTERFACE:${LIB_INCLUDE_DIR}>
# )

find_package(ctrl_utils 1.1.0 REQUIRED)
if(NOT ctrl_utils_FOUND)
    download_package(ctrl_utils 1.1.0)
    find_package(ctrl_utils 1.1.0 REQUIRED)
endif()

set(SYMBOLIC_LIB lib${LIB_NAME})
add_library(${SYMBOLIC_LIB} SHARED)
set_target_properties(${SYMBOLIC_LIB} PROPERTIES OUTPUT_NAME ${LIB_NAME})
add_library(${LIB_NAME}::${LIB_NAME} ALIAS ${SYMBOLIC_LIB})
target_include_directories(${SYMBOLIC_LIB}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${LIB_INCLUDE_DIR}>
)
target_sources(${SYMBOLIC_LIB} PRIVATE ${LIB_SRC})
target_link_libraries(${SYMBOLIC_LIB} PUBLIC
    VAL::VAL
)
# configure_file(${CMAKE_BINARY_DIR}/VAL/build/parser ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COPYONLY)
# configure_file(${CMAKE_BINARY_DIR}/VAL/build/validate ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COPYONLY)

# Use GNUInstalDirs to install ibraries into correct locations on all platforms
include(GNUInstallDirs)

set(LIB_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${LIB_NAME})

# Install the library
install(TARGETS ${SYMBOLIC_LIB} ${VAL_LIB}
    EXPORT ${LIB_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install the headers
install(DIRECTORY ${LIB_INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export targets to Targets.cmake
install(EXPORT ${LIB_NAME}Targets
    DESTINATION ${LIB_CONFIG_INSTALL_DIR}
    NAMESPACE ${LIB_NAME}::
    FILE ${LIB_NAME}Targets.cmake
)

# Create ConfigVersion.cmake
include(CMakePackageConfigHelpers)
write_basic_package_version_file(${CMAKE_BINARY_DIR}/${LIB_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Create Config.cmake
configure_package_config_file(${LIB_CMAKE_DIR}/${LIB_NAME}Config.cmake.in
    ${CMAKE_BINARY_DIR}/${LIB_NAME}Config.cmake
    INSTALL_DESTINATION ${LIB_CONFIG_INSTALL_DIR})

# Install config script files
install(FILES
    ${CMAKE_BINARY_DIR}/${LIB_NAME}Config.cmake
    ${CMAKE_BINARY_DIR}/${LIB_NAME}ConfigVersion.cmake
    DESTINATION ${LIB_CONFIG_INSTALL_DIR}
)

# Export for build tree
export(TARGETS ${SYMBOLIC_LIB} ${VAL_LIB}
    NAMESPACE ${LIB_NAME}::
    FILE ${CMAKE_BINARY_DIR}/${LIB_NAME}Targets.cmake)

# Register package in user registry
export(PACKAGE ${SYMBOLIC_LIB})

# Build tests
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

add_executable(pddl ${LIB_SRC_DIR}/main.cc)
target_link_libraries(pddl PRIVATE symbolic::symbolic)

# Build python wrapper
add_subdirectory(${LIB_SRC_DIR}/python)
