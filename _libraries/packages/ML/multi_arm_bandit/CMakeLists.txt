add_library(multi_arm_bandit SHARED
    source/bandit_agent.cpp
    source/bandit_arm.cpp
    source/decaying_epsilon_agent.cpp
    source/epsilon_greedy_agent.cpp
    source/simulation_result.cpp
    source/thompson_sampling_agent.cpp
    source/ucb_agent.cpp
)
target_include_directories(multi_arm_bandit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
