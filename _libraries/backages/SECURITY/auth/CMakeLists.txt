add_library(auth SHARED
    source/auth_system.cpp
)
target_include_directories(auth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(auth PUBLIC cxx_std_17)

find_package(OpenSSL QUIET)

target_link_libraries(auth PUBLIC password_hash_utils)

if(OpenSSL_FOUND)
    target_link_libraries(auth PUBLIC OpenSSL::Crypto)
    target_compile_definitions(auth PRIVATE COOLBOX_AUTH_USE_OPENSSL_PROVIDER)
endif()
