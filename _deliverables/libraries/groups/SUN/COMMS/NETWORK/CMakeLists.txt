add_library(sun_network
    source/sun_pooled_sql_server.cpp
)

target_include_directories(sun_network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(sun_network PUBLIC cxx_std_17)

target_link_libraries(sun_network PUBLIC
    sun_misc
    thread_pool_utils
)
