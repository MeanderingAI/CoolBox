add_library(bbtree INTERFACE)
target_include_directories(bbtree INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/headers>)
target_compile_features(bbtree INTERFACE cxx_std_17)

if(BUILD_TESTING AND TARGET tyst_framework_main)
    add_executable(bbtree_tests tests/test_bbtree.cpp)
    target_link_libraries(bbtree_tests PRIVATE bbtree tyst_framework_main)
    add_test(NAME BBTreeMapTests COMMAND bbtree_tests)
endif()
