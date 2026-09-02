add_library(fuzzer SHARED
    source/fuzzer.cpp
)
target_include_directories(fuzzer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
