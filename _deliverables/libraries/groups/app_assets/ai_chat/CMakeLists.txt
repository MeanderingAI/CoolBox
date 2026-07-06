cmake_minimum_required(VERSION 3.12)

project(ai_chat LANGUAGES CXX)

add_library(ai_chat_lib
    source/ai_chat.cpp
)

target_include_directories(ai_chat_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(ai_chat_lib PUBLIC cxx_std_17)
set_target_properties(ai_chat_lib PROPERTIES EXPORT_NAME ai_chat)

add_executable(ai_chat_voice_bridge
    tools/ai_chat_voice_bridge.cpp
)
target_link_libraries(ai_chat_voice_bridge PRIVATE ai_chat_lib)
target_compile_features(ai_chat_voice_bridge PRIVATE cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(ai_chat_tests
        tests/test_ai_chat.cpp
    )
    target_link_libraries(ai_chat_tests PRIVATE ai_chat_lib tyst_framework_main)
    add_test(NAME AiChatTests COMMAND ai_chat_tests)
endif()
