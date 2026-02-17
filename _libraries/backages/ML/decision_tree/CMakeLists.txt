add_library(decision_tree SHARED
    source/decision_tree.cpp
    source/rule_set.cpp
    source/random_forest.cpp
    source/boost_tree.cpp
)
target_link_libraries(decision_tree PRIVATE Eigen3::Eigen)
target_include_directories(decision_tree PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Tests
enable_testing()
add_executable(decision_tree_tests
    tests/test_boost_tree.cpp
    tests/test_decision_tree.cpp
    tests/test_random_forest.cpp
    tests/test_rule_set.cpp
)
target_link_libraries(decision_tree_tests PRIVATE decision_tree GTest::gtest_main)
add_test(NAME DecisionTreeTests COMMAND decision_tree_tests)
