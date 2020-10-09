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
add_test(NAME ${PROJECT_NAME}_tests
    COMMAND ./${PROJECT_NAME}_tests
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

add_test(NAME py${PROJECT_NAME}_tests
    COMMAND pipenv run make -C docs doctest
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
