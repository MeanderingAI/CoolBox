cmake_minimum_required(VERSION 3.20)
project(application_context LANGUAGES CXX)

add_library(application_context
    source/application_context.cpp
)

target_include_directories(application_context PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(application_context PUBLIC cxx_std_17)
target_link_libraries(application_context PUBLIC windows components)
