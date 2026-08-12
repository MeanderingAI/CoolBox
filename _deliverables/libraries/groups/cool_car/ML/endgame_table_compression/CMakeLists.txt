add_library(endgame_table_compression SHARED
    source/endgame_table_compression.cpp
)
target_include_directories(endgame_table_compression PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(endgame_table_compression PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(endgame_table_compression_tests
        tests/test_endgame_table_compression.cpp
    )
    target_link_libraries(endgame_table_compression_tests PRIVATE endgame_table_compression tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(endgame_table_compression_tests)
    add_test(NAME EndgameTableCompressionTests COMMAND endgame_table_compression_tests)
endif()