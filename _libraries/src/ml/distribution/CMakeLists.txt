add_library(probability_distribution SHARED
    util.cpp
    bernoulli_distribution.cpp
    binomial_distribution.cpp
    categorical_distribution.cpp
    discrete_distribution.cpp
    exponential_distribution.cpp
    gamma_distribution.cpp
    inverse_gaussian_distribution.cpp
    laplace_distribution.cpp
    normal_distribution.cpp
    poisson_distribution.cpp
    categorical_distribution.cpp
)
target_include_directories(probability_distribution PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link the GSL library to your distribution library.
# Link against GNU GSL (and its BLAS backend)
target_link_libraries(probability_distribution
    PRIVATE GSL::gsl GSL::gslcblas
)