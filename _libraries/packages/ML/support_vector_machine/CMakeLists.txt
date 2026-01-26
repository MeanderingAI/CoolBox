add_library(support_vector_machine SHARED
    source/linear_kernel.cpp
    source/polynomial_kernel.cpp
    source/rbf_kernel.cpp
    source/sigmoid_kernel.cpp
    source/support_vector_machine.cpp
)
target_include_directories(support_vector_machine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
