cmake_minimum_required(VERSION 3.16)

add_library(email_api_exposure SHARED
    source/api_exposure.cpp
)

target_compile_features(email_api_exposure PUBLIC cxx_std_17)
target_include_directories(email_api_exposure PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(email_api_exposure PUBLIC http_server servlets)
