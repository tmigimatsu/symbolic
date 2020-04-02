############################################################
# CMakeLists for spatial_dyn tests
#
# Copyright 2018. All Rights Reserved.
#
# Created: September 11, 2018
# Authors: Toki Migimatsu
############################################################

add_executable(${CMAKE_PROJECT_NAME}_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests.cc)

target_link_libraries(${CMAKE_PROJECT_NAME}_tests PRIVATE
    Catch2::Catch2
    symbolic::symbolic
)
add_test(${CMAKE_PROJECT_NAME}_tests ${CMAKE_PROJECT_NAME}_tests)
