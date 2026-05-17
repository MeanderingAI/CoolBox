cmake_minimum_required(VERSION 3.12)

include(CheckIncludeFileCXX)

find_package(OpenSSL QUIET)

add_library(uuid_generation SHARED
    source/uuid_generation.cpp
)

target_include_directories(uuid_generation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(uuid_generation PUBLIC cxx_std_17)

if(OpenSSL_FOUND)
    target_link_libraries(uuid_generation PUBLIC OpenSSL::Crypto)
    target_compile_definitions(uuid_generation PRIVATE COOLBOX_UUID_USE_OPENSSL=1)
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(uuid_generation_tests
        tests/test_uuid_generation.cpp
    )
    target_link_libraries(uuid_generation_tests PRIVATE uuid_generation tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(uuid_generation_tests)
    add_test(NAME UuidGenerationTests COMMAND uuid_generation_tests)
endif()
