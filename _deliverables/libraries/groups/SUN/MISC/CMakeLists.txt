add_library(sun_misc
    source/sun_text.cpp
)

target_include_directories(sun_misc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(sun_misc PUBLIC cxx_std_17)
