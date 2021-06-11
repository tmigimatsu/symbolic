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
set_target_properties(pddl PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(pddl PRIVATE symbolic_v1::symbolic_v1)

if(SYMBOLIC_CLANG_TIDY)
    target_enable_clang_tidy(pddl)
endif()
