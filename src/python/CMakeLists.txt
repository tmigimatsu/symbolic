############################################################
# CMakeLists for Pybind11
#
# Copyright 2018. All Rights Reserved.
#
# Created: September 7, 2018
# Authors: Toki Migimatsu
############################################################

#############
# Pybind11 #
#############

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/symbolic)

set(PYBIND11_LIB_DIR ${ctrl_utils_EXTERNAL_DIR}/pybind11.git)
add_subdirectory(${PYBIND11_LIB_DIR} ${CMAKE_CURRENT_BINARY_DIR}/pybind11)

pybind11_add_module(pysymbolic symbolic.cc)

target_link_libraries(pysymbolic PUBLIC pybind11::pybind11 symbolic::symbolic)
