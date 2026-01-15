
# Only add executable, not a library, as there are no standalone sources for a library
add_executable(mail_server_service __src/mail_server.cpp)
target_include_directories(mail_server_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/__include)

