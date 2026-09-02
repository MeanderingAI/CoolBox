cmake_minimum_required(VERSION 3.16)

add_library(email_mailbox SHARED
    source/mailbox.cpp
)

target_compile_features(email_mailbox PUBLIC cxx_std_17)
target_include_directories(email_mailbox PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
