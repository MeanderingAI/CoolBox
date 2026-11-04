cmake_minimum_required(VERSION 3.16)

add_library(dl_rag STATIC src/rag.cpp)
target_include_directories(dl_rag PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(dl_rag PUBLIC cxx_std_17)

if(BUILD_TESTING)
    if(TARGET tyst_framework_main)
        add_executable(dl_rag_tests tests/test_rag.cpp)
        target_link_libraries(dl_rag_tests PRIVATE dl_rag tyst_framework_main)
    else()
        add_executable(dl_rag_tests
            tests/test_rag.cpp
            ${CMAKE_SOURCE_DIR}/_tyst_main/tyst_framework_main.cpp
        )
        target_include_directories(dl_rag_tests PRIVATE
            ${CMAKE_SOURCE_DIR}/_deliverables/libraries/groups/Generics/tyst_framework/headers
        )
        target_link_libraries(dl_rag_tests PRIVATE dl_rag)
    endif()
    add_test(NAME DeepLearningRagTests COMMAND dl_rag_tests)
endif()
