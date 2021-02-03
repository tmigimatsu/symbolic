############################################################
# CMakeLists for Pybind11
#
# Copyright 2018. All Rights Reserved.
#
# Created: September 7, 2018
# Authors: Toki Migimatsu
############################################################

project(pysymbolic)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LIB_SOURCE_DIR}/symbolic")

ctrl_utils_add_subdirectory(pybind11)

pybind11_add_module(pysymbolic symbolic.cc)

target_link_libraries(pysymbolic
  PUBLIC
    pybind11::pybind11
    symbolic::symbolic
  PRIVATE
    VAL::VAL
)
