############################################################
# CMakeLists for VAL
#
# Copyright 2020. All Rights Reserved.
#
# Created: September 30, 2020
# Authors: Toki Migimatsu
############################################################

if(NOT TARGET VAL::VAL)
    init_git_submodule(VAL.git)
    add_subdirectory(VAL.git)
    set_target_properties(VAL PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    )
endif()
