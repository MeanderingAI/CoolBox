cmake_minimum_required(VERSION 3.12)

find_package(OpenSSL QUIET)

add_library(cryptocurrency_utils SHARED
    source/cryptocurrency.cpp
)

target_include_directories(cryptocurrency_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(cryptocurrency_utils PUBLIC cxx_std_17)

if(OpenSSL_FOUND)
    target_link_libraries(cryptocurrency_utils PUBLIC OpenSSL::Crypto)
    target_compile_definitions(cryptocurrency_utils PRIVATE COOLBOX_CRYPTOCURRENCY_USE_OPENSSL_PROVIDER=1)
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(cryptocurrency_utils_tests
        tests/test_cryptocurrency.cpp
    )
    target_link_libraries(cryptocurrency_utils_tests PRIVATE cryptocurrency_utils tyst_framework_main)
    add_test(NAME CryptocurrencyUtilsTests COMMAND cryptocurrency_utils_tests)
endif()
