############################################################
# CMakeLists for the symbolic library.
#
# Copyright 2020. All Rights Reserved.
#
# Created: October 1, 2020
# Authors: Toki Migimatsu
############################################################

message(STATUS "Configuring ${PROJECT_NAME} examples.")

add_executable(pddl pddl.cc)

target_compile_features(pddl PUBLIC cxx_std_17)

target_link_libraries(pddl PRIVATE symbolic::symbolic)

if(SYMBOLIC_CLANG_TIDY)
    target_enable_clang_tidy(pddl)
endif()
