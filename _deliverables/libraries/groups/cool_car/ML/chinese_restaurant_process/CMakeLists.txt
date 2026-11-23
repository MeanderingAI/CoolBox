add_library(chinese_restaurant_process
    source/chinese_restaurant_process.cpp
)
target_include_directories(chinese_restaurant_process
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_compile_features(chinese_restaurant_process PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    add_executable(chinese_restaurant_process_tests
        tests/test_chinese_restaurant_process.cpp
    )
    target_link_libraries(chinese_restaurant_process_tests
        PRIVATE chinese_restaurant_process tyst_framework_main
    )
    coolbox_stage_test_runtime_dependencies(chinese_restaurant_process_tests)
    add_test(NAME ChineseRestaurantProcessTests
        COMMAND chinese_restaurant_process_tests
    )
endif()
