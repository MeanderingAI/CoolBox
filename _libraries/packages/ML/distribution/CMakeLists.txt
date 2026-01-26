add_library(distribution SHARED
    source/bernoulli_distribution.cpp
    source/binomial_distribution.cpp
    source/categorical_distribution.cpp
    source/discrete_distribution.cpp
    source/exponential_distribution.cpp
    source/gamma_distribution.cpp
    source/inverse_gaussian_distribution.cpp
    source/laplace_distribution.cpp
    source/multinomial_distribution.cpp
    source/normal_distribution.cpp
    source/poisson_distribution.cpp
    source/util.cpp
)
target_include_directories(distribution PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers)
