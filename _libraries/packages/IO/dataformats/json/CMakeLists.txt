add_library(json SHARED
    source/json.cpp
)
target_include_directories(json PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
