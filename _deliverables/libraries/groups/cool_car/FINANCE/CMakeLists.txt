cmake_minimum_required(VERSION 3.12)

add_library(FINANCE SHARED
    source/finance.cpp
)

target_include_directories(FINANCE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(FINANCE PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(finance_tests
        tests/test_finance.cpp
    )
    target_link_libraries(finance_tests PRIVATE FINANCE tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(finance_tests)
    add_test(NAME FinanceTests COMMAND finance_tests)
endif()