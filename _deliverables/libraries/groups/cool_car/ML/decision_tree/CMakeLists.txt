add_library(decision_tree SHARED
    source/decision_tree.cpp
    source/rule_set.cpp
    source/random_forest.cpp
    source/boost_tree.cpp
    source/chained_boosting.cpp
)
target_link_libraries(decision_tree PUBLIC mytrix metadata_management)
target_include_directories(decision_tree PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_compile_features(decision_tree PUBLIC cxx_std_17)

# Tests
if(BUILD_TESTING AND TARGET tyst_framework_main)
    enable_testing()
    add_executable(decision_tree_tests
        tests/test_boost_tree.cpp
        tests/test_chained_boosting.cpp
        tests/test_decision_tree.cpp
        tests/test_random_forest.cpp
        tests/test_rule_set.cpp
    )
    target_link_libraries(decision_tree_tests PRIVATE decision_tree tyst_framework_main)
    coolbox_stage_test_runtime_dependencies(decision_tree_tests)
    add_test(NAME DecisionTreeTests COMMAND decision_tree_tests)
endif()
