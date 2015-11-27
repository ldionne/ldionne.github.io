# Copyright Louis Dionne 2015
# Distributed under the Boost Software License, Version 1.0.

add_custom_target(pack-indexing.chart_1
    COMMAND erb chart_1.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
)

add_custom_target(pack-indexing.chart_2
    COMMAND erb chart_2.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
)

add_custom_target(pack-indexing.chart_3
    COMMAND erb chart_3.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
)

add_custom_target(pack-indexing DEPENDS
    pack-indexing.chart_1
    pack-indexing.chart_2
    pack-indexing.chart_3
)
