# Copyright Louis Dionne 2015
# Distributed under the Boost Software License, Version 1.0.

add_custom_target(tuple-quest.get_assembly
    COMMAND erb chart.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/get_assembly
    USES_TERMINAL
)

add_custom_target(tuple-quest.nrvo
    COMMAND erb table.md
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/nrvo
    USES_TERMINAL
)

add_custom_target(tuple-quest.get_compile
    COMMAND erb chart.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/get_compile
    USES_TERMINAL
)

add_custom_target(tuple-quest.make_compile
    COMMAND erb chart.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/make_compile
    USES_TERMINAL
)

add_custom_target(tuple-quest DEPENDS
    tuple-quest.get_assembly
    tuple-quest.get_compile
    tuple-quest.make_compile
    tuple-quest.nrvo
)
