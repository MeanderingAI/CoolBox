add_library(decision_tree SHARED
    decision_tree.cpp
    rule_set.cpp
    random_forest.cpp
    boost_tree.cpp
)
target_link_libraries(decision_tree PRIVATE Eigen3::Eigen)
target_include_directories(decision_tree PUBLIC ${CMAKE_SOURCE_DIR}/include)