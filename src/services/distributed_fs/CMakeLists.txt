# Distributed File System library
add_library(distributed_fs SHARED
    distributed_fs.cpp
)

target_include_directories(distributed_fs PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(distributed_fs
    pthread
)

set_target_properties(distributed_fs PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
