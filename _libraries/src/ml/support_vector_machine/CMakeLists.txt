add_library(support_vector_machine SHARED
    support_vector_machine.cpp
    linear_kernel.cpp
    rbf_kernel.cpp
    polynomial_kernel.cpp
    sigmoid_kernel.cpp
)
target_link_libraries(support_vector_machine PRIVATE Eigen3::Eigen)
target_include_directories(support_vector_machine PUBLIC ${CMAKE_SOURCE_DIR}/include)