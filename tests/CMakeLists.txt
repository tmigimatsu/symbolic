############################################################
# CMakeLists for symbolic tests.
#
# Copyright 2018. All Rights Reserved.
#
# Created: September 11, 2018
# Authors: Toki Migimatsu
############################################################

message(STATUS "Configuring ${PROJECT_NAME} tests.")

add_executable(${PROJECT_NAME}_tests tests.cc)

ctrl_utils_add_subdirectory(Catch2)
target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    Catch2::Catch2
    symbolic::symbolic
)
add_test(${PROJECT_NAME}_tests ${PROJECT_NAME}_tests)
