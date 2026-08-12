add_library(custom_numbers SHARED
    source/custom_numbers.cpp
)
target_include_directories(custom_numbers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(custom_numbers PUBLIC cxx_std_17)

add_executable(custom_numbers_tests
    tests/custom_numbers_test.cpp
)
target_link_libraries(custom_numbers_tests PRIVATE custom_numbers)
add_test(NAME CustomNumbersTests COMMAND custom_numbers_tests)