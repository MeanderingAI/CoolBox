add_library(sun_data_structures
    source/sun_table.cpp
    source/sun_relational_db.cpp
)

target_include_directories(sun_data_structures PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(sun_data_structures PUBLIC cxx_std_17)
