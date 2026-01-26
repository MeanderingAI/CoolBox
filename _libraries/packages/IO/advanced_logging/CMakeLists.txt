
add_library(advanced_logging STATIC
    source/advanced_logging.cpp
)


target_include_directories(advanced_logging PUBLIC
    ${CMAKE_SOURCE_DIR}/_libraries/include
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
