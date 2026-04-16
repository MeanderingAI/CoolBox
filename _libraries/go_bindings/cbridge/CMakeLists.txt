cmake_minimum_required(VERSION 3.15)
project(coolboxbridge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(COOLBOX_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

# Add a static library target that compiles bridge.cpp. The bridge.cpp
# includes repository source files directly (textual includes) so this
# target will produce a self-contained archive.
add_library(coolboxbridge STATIC bridge.cpp)

# Include dirs needed by the bridge implementation. Adjust if your
# repository layout changes.
target_include_directories(coolboxbridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..   # for bridge_forward.h
  ${COOLBOX_REPO_ROOT}/_libraries/packages/GRAPHICS/charts/headers
  ${COOLBOX_REPO_ROOT}/_libraries/packages/ML/generalized_linear_model/headers
  ${COOLBOX_REPO_ROOT}/_libraries/packages/ML/decision_tree/headers
  ${COOLBOX_REPO_ROOT}/_libraries/packages/ML/bayesian_network_ai/headers
  ${COOLBOX_REPO_ROOT}/_libraries/packages/ML/hidden_markov_model/headers
  ${COOLBOX_REPO_ROOT}/_libraries/packages/ML/dimensionality_reduction/headers
  ${COOLBOX_REPO_ROOT}/_libraries/packages/ML/support_vector_machine/headers
  ${COOLBOX_REPO_ROOT}/_libraries/packages/ML/multi_arm_bandit/headers
  ${COOLBOX_REPO_ROOT}/build/eigen-src
)

# Use optimized flags by default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Ensure position-independent code for linking into Go programs
set_target_properties(coolboxbridge PROPERTIES POSITION_INDEPENDENT_CODE ON)
