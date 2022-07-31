############################################################
# CMakeLists for symbolic docs.
#
# Copyright 2018. All Rights Reserved.
#
# Created: September 11, 2018
# Authors: Toki Migimatsu
############################################################

message(STATUS "Configuring ${PROJECT_NAME} documentation.")

add_custom_target(symbolic_docs ALL
    COMMAND make -C docs clean && make -C docs
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
add_dependencies(symbolic_docs pysymbolic)
