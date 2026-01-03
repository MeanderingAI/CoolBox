cmake_minimum_required(VERSION 3.0)
project(advanced_logging_js)

add_library(advanced_logging_js SHARED advanced_logging_bindings.cpp)

target_include_directories(advanced_logging_js PRIVATE
    ${CMAKE_SOURCE_DIR}/_libraries/include
    ${CMAKE_SOURCE_DIR}/_libraries/src/advanced_logging
)

target_link_libraries(advanced_logging_js PRIVATE advanced_logging)

# Emscripten-specific flags
if(EMSCRIPTEN)
    set_target_properties(advanced_logging_js PROPERTIES
        OUTPUT_NAME "advanced_logging"
        SUFFIX ".js"
    )
    if(EMSCRIPTEN_MODULARIZE)
        target_compile_options(advanced_logging_js PRIVATE -s MODULARIZE=1 -s EXPORT_NAME="createAdvancedLoggingModule")
    else()
        target_compile_options(advanced_logging_js PRIVATE -s MODULARIZE=0 -s EXPORT_NAME="Module")
    endif()
endif()
