cmake_minimum_required(VERSION 3.20)
project(email_center_api LANGUAGES CXX)

find_package(Threads REQUIRED)

set(EMAIL_CENTER_API_INCLUDES
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/IO/http_server/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/IO/servlets/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/IO/EMAIL/smtp_server/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/IO/EMAIL/imap_server/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/IO/EMAIL/mailbox/headers
    ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/trekker/IO/EMAIL/api_exposure/headers
)

add_executable(email_center_api main.cpp)
target_compile_features(email_center_api PRIVATE cxx_std_17)
target_include_directories(email_center_api PRIVATE ${EMAIL_CENTER_API_INCLUDES})
target_link_libraries(email_center_api PRIVATE
    email_smtp_server
    email_imap_server
    email_mailbox
    email_api_exposure
    http_server
    servlets
    Threads::Threads
)
