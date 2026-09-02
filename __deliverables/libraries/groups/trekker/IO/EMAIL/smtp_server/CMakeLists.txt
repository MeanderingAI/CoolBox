cmake_minimum_required(VERSION 3.16)

add_library(email_smtp_server SHARED
    source/smtp_server.cpp
)

target_compile_features(email_smtp_server PUBLIC cxx_std_17)
target_include_directories(email_smtp_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(email_smtp_server PUBLIC email_mailbox)
