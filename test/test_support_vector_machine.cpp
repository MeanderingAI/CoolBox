#include <gtest/gtest.h>
#include <Eigen/Dense>
#include "ml/support_vector_machine/linear_kernel.h"
#include "ml/support_vector_machine/polynomial_kernel.h"
#include "ml/support_vector_machine/rbf_kernel.h"
#include "ml/support_vector_machine/sigmoid_kernel.h"

// Test fixture to set up common test data
class KernelTest : public ::testing::Test {
protected:
    void SetUp() override {
        v1.resize(2);
        v1 << 1.0, 2.0;
        
        v2.resize(2);
        v2 << 3.0, 4.0;
    }

    Eigen::VectorXd v1;
    Eigen::VectorXd v2;
};

// Test for the Linear Kernel
TEST_F(KernelTest, LinearKernelCalculatesCorrectly) {
    LinearKernel kernel;
    double expected_value = (1.0 * 3.0) + (2.0 * 4.0);
    ASSERT_DOUBLE_EQ(kernel.calculate(v1, v2), expected_value);
}

// Test for the Polynomial Kernel
TEST_F(KernelTest, PolynomialKernelCalculatesCorrectly) {
    double gamma = 0.5;
    double c = 1.0;
    int degree = 2;
    PolynomialKernel kernel(gamma, c, degree);
    double dot_product = (1.0 * 3.0) + (2.0 * 4.0);
    double expected_value = std::pow(gamma * dot_product + c, degree);
    ASSERT_DOUBLE_EQ(kernel.calculate(v1, v2), expected_value);
}

// Test for the RBF Kernel
TEST_F(KernelTest, RBFKernelCalculatesCorrectly) {
    double gamma = 0.1;
    RBFKernel kernel(gamma);
    double squared_distance = std::pow(1.0 - 3.0, 2) + std::pow(2.0 - 4.0, 2);
    double expected_value = std::exp(-gamma * squared_distance);
    ASSERT_DOUBLE_EQ(kernel.calculate(v1, v2), expected_value);
}

// Test for the Sigmoid Kernel
TEST_F(KernelTest, SigmoidKernelCalculatesCorrectly) {
    double gamma = 0.5;
    double c = 0.1;
    SigmoidKernel kernel(gamma, c);
    double dot_product = (1.0 * 3.0) + (2.0 * 4.0);
    double expected_value = std::tanh(gamma * dot_product + c);
    ASSERT_DOUBLE_EQ(kernel.calculate(v1, v2), expected_value);
}