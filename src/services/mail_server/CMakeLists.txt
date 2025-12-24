add_library(mail_server mail_server.cpp)
target_include_directories(mail_server PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mail_server PUBLIC pthread)
