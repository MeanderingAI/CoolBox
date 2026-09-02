cmake_minimum_required(VERSION 3.12)

include(CheckIncludeFileCXX)
include(CheckSymbolExists)

find_package(OpenSSL QUIET)
find_package(Threads REQUIRED)

check_include_file_cxx("argon2.h" COOLBOX_HASH_HAS_ARGON2_HEADER)
find_library(COOLBOX_HASH_ARGON2_LIBRARY NAMES argon2 libargon2)

check_include_file_cxx("crypt.h" COOLBOX_HASH_HAS_CRYPT_HEADER)
check_symbol_exists(crypt "unistd.h;crypt.h" COOLBOX_HASH_HAS_CRYPT_SYMBOL)
find_library(COOLBOX_HASH_CRYPT_LIBRARY NAMES crypt)

add_library(password_hash_utils SHARED
    source/password_hash.cpp
)

target_include_directories(password_hash_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(password_hash_utils PUBLIC cxx_std_17)
target_link_libraries(password_hash_utils PUBLIC Threads::Threads)

if(OpenSSL_FOUND)
    target_link_libraries(password_hash_utils PUBLIC OpenSSL::Crypto)
    target_compile_definitions(password_hash_utils PRIVATE COOLBOX_HASH_USE_OPENSSL_PROVIDER=1)
endif()

if(COOLBOX_HASH_HAS_ARGON2_HEADER AND COOLBOX_HASH_ARGON2_LIBRARY)
    target_link_libraries(password_hash_utils PUBLIC ${COOLBOX_HASH_ARGON2_LIBRARY})
    target_compile_definitions(password_hash_utils PRIVATE COOLBOX_HASH_USE_ARGON2_PROVIDER=1)
endif()

if(APPLE OR (COOLBOX_HASH_HAS_CRYPT_SYMBOL AND COOLBOX_HASH_CRYPT_LIBRARY))
    target_compile_definitions(password_hash_utils PRIVATE COOLBOX_HASH_USE_CRYPT_PROVIDER=1)
endif()

if(COOLBOX_HASH_HAS_CRYPT_SYMBOL AND COOLBOX_HASH_CRYPT_LIBRARY)
    target_link_libraries(password_hash_utils PUBLIC ${COOLBOX_HASH_CRYPT_LIBRARY})
endif()

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(password_hash_utils_tests
        tests/test_password_hash.cpp
        tests/test_murmur_hash.cpp
    )
    target_link_libraries(password_hash_utils_tests PRIVATE password_hash_utils tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(password_hash_utils_tests)
    add_test(NAME PasswordHashUtilsTests COMMAND password_hash_utils_tests)
endif()
