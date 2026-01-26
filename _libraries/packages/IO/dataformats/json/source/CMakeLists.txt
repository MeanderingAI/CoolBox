add_library(json SHARED
    json.cpp
)

target_include_directories(json PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(json
)
