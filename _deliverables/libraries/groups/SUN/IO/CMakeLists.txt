add_library(sun_io
    source/sun_snapshot_codec.cpp
)

target_include_directories(sun_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(sun_io PUBLIC cxx_std_17)

target_link_libraries(sun_io PUBLIC
    sun_data_structures
    sun_misc
)
