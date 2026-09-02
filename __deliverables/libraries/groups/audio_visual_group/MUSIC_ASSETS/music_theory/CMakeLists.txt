cmake_minimum_required(VERSION 3.16)

add_library(music_theory SHARED
    source/scale.cpp
    source/chord.cpp
    source/interval.cpp
)

target_compile_features(music_theory PUBLIC cxx_std_17)
target_include_directories(music_theory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(music_theory_tests tests/test_music_theory.cpp)
    target_link_libraries(music_theory_tests PRIVATE music_theory tyst_framework_main)
    add_test(NAME MusicTheoryTests COMMAND music_theory_tests)
endif()
