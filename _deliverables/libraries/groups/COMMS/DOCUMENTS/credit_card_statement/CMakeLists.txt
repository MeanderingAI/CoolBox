cmake_minimum_required(VERSION 3.20)
project(credit_card_statement)

add_library(credit_card_statement
    source/credit_card_statement.cpp
)

target_include_directories(credit_card_statement PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

target_compile_features(credit_card_statement PUBLIC cxx_std_17)

option(BUILD_TESTS_CREDIT_CARD_STATEMENT "Build credit_card_statement tests" ON)
if(BUILD_TESTS_CREDIT_CARD_STATEMENT)
    enable_testing()
    add_executable(credit_card_statement_test tests/test_credit_card_statement.cpp)
    target_link_libraries(credit_card_statement_test PRIVATE credit_card_statement)
    add_test(NAME credit_card_statement_test COMMAND credit_card_statement_test)
endif()

install(TARGETS credit_card_statement)
