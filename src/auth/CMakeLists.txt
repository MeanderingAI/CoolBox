# Authentication Library
add_library(auth SHARED
    auth_system.cpp
)

target_include_directories(auth PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(auth
    pthread
)

set_target_properties(auth PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
