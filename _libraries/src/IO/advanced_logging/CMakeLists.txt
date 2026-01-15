add_library(advanced_logging STATIC
    advanced_logging.cpp
)

target_include_directories(advanced_logging PUBLIC
    ${CMAKE_SOURCE_DIR}/_libraries/include
)
