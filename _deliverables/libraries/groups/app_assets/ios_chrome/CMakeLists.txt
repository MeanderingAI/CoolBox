cmake_minimum_required(VERSION 3.12)

project(ios_chrome LANGUAGES CXX)

add_library(ios_chrome_lib
    source/ios_chrome.cpp
)

target_include_directories(ios_chrome_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(ios_chrome_lib PUBLIC full_application_window)
target_compile_features(ios_chrome_lib PUBLIC cxx_std_17)
set_target_properties(ios_chrome_lib PROPERTIES EXPORT_NAME ios_chrome)
