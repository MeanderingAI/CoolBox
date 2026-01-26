add_library(decision_tree SHARED
    source/decision_tree.cpp
    source/rule_set.cpp
    source/random_forest.cpp
    source/boost_tree.cpp
)
target_link_libraries(decision_tree PRIVATE Eigen3::Eigen)
target_include_directories(decision_tree PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
