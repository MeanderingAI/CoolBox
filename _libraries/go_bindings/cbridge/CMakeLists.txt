cmake_minimum_required(VERSION 3.15)
project(coolboxbridge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add a static library target that compiles bridge.cpp. The bridge.cpp
# includes repository source files directly (textual includes) so this
# target will produce a self-contained archive.
add_library(coolboxbridge STATIC bridge.cpp)

# Include dirs needed by the bridge implementation. Adjust if your
# repository layout changes.
target_include_directories(coolboxbridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..   # for bridge_forward.h
    ${CMAKE_SOURCE_DIR}/_libraries/backages/GRAPHICS/charts/headers
    ${CMAKE_SOURCE_DIR}/_libraries/backages/ML/generalized_linear_model/headers
    ${CMAKE_SOURCE_DIR}/_libraries/backages/ML/decision_tree/headers
    ${CMAKE_SOURCE_DIR}/_libraries/backages/ML/bayesian_network_ai/headers
    ${CMAKE_SOURCE_DIR}/_libraries/backages/ML/hidden_markov_model/headers
    ${CMAKE_SOURCE_DIR}/_libraries/backages/ML/dimensionality_reduction/headers
    ${CMAKE_SOURCE_DIR}/_libraries/backages/ML/support_vector_machine/headers
    ${CMAKE_SOURCE_DIR}/_libraries/backages/ML/multi_arm_bandit/headers
    ${CMAKE_SOURCE_DIR}/build/eigen-src
)

# Use optimized flags by default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Ensure position-independent code for linking into Go programs
set_target_properties(coolboxbridge PROPERTIES POSITION_INDEPENDENT_CODE ON)
