cmake_minimum_required(VERSION 3.12)

project(quic_transport LANGUAGES CXX)

add_library(quic_transport
    source/quic_transport.cpp
)

target_include_directories(quic_transport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(quic_transport PUBLIC cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(quic_transport_tests
        tests/test_quic_transport.cpp
    )
    target_link_libraries(quic_transport_tests PRIVATE quic_transport tyst_framework_main)
    add_test(NAME QuicTransportTests COMMAND quic_transport_tests)
endif()