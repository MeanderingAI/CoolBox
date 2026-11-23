add_library(indian_buffet_process
    source/indian_buffet_process.cpp
)
target_include_directories(indian_buffet_process
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_compile_features(indian_buffet_process PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    add_executable(indian_buffet_process_tests
        tests/test_indian_buffet_process.cpp
    )
    target_link_libraries(indian_buffet_process_tests
        PRIVATE indian_buffet_process tyst_framework_main
    )
    coolbox_stage_test_runtime_dependencies(indian_buffet_process_tests)
    add_test(NAME IndianBuffetProcessTests
        COMMAND indian_buffet_process_tests
    )
endif()
