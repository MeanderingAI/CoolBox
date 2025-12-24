add_library(multi_arm_bandit SHARED 
    epsilon_greedy_agent.cpp
    ucb_agent.cpp
    decaying_epsilon_agent.cpp
    thompson_sampling_agent.cpp
    bandit_arm.cpp
    bandit_agent.cpp
    simulation_result.cpp
)
target_link_libraries(multi_arm_bandit PRIVATE Eigen3::Eigen)
target_include_directories(multi_arm_bandit PUBLIC ${CMAKE_SOURCE_DIR}/include)