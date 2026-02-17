add_library(auth SHARED
    source/auth_system.cpp
)
target_include_directories(auth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
