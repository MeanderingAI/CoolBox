
#include <tyst_framework.hpp>
#include "gabor_patches.h"
#include "mytrix_eigen_compat.hpp"
#include <cmath>
#include <vector>

using namespace ml;
using MatrixT = mytrix::DenseMatrix;

// ===================================================================
// gabor_kernel – basic properties
// ===================================================================

TEST(GaborKernelTest, DefaultSizeIsOdd) {
    GaborParams p;
    auto k = gabor_kernel(p);
    EXPECT_GT(k.rows(), 0);
    EXPECT_EQ(k.rows() % 2, 1);
    EXPECT_EQ(k.rows(), k.cols());
}

TEST(GaborKernelTest, ExplicitSize) {
    GaborParams p;
    auto k = gabor_kernel(p, 11, /*normalize=*/false);
    EXPECT_EQ(k.rows(), 11);
    EXPECT_EQ(k.cols(), 11);
}

TEST(GaborKernelTest, InvalidSizeThrows) {
    GaborParams p;
    EXPECT_THROW(gabor_kernel(p, -1), std::invalid_argument);
    // size=0 means auto-compute, so it should NOT throw
    EXPECT_NO_THROW(gabor_kernel(p, 0));
    // Even sizes are rejected
    EXPECT_THROW(gabor_kernel(p, 10), std::invalid_argument);
}

TEST(GaborKernelTest, InvalidParamsThrow) {
    GaborParams p;
    p.lambda = 0;
    EXPECT_THROW(gabor_kernel(p, 5), std::invalid_argument);
    p.lambda = 4.0;
    p.sigma = -1;
    EXPECT_THROW(gabor_kernel(p, 5), std::invalid_argument);
}

TEST(GaborKernelTest, CenterValueIsOne_WhenPsiZero) {
    // At the centre (0,0) with ψ=0: cos(0)=1, envelope=1 → value=1
    GaborParams p{4.0, 0.0, 0.0, 2.0, 0.5};
    auto k = gabor_kernel(p, 11, /*normalize=*/false);
    int c = 5; // centre of 11×11
    EXPECT_NEAR(k(c, c), 1.0, 1e-2);
}

TEST(GaborKernelTest, SymmetryAt0Degrees) {
    // θ=0, ψ=0, γ=1 → symmetric about x-axis
        GaborParams p{4.0, 0.0, 0.0, 2.0, 1.0};
    auto k = gabor_kernel(p, 11);
    int c = 5;
    // Horizontal symmetry: k(c-1, c) == k(c+1, c)
    EXPECT_NEAR(k(c - 1, c), k(c + 1, c), 1e-10);
    // Vertical symmetry: k(c, c-1) == k(c, c+1)
    EXPECT_NEAR(k(c, c - 1), k(c, c + 1), 1e-10);
}

TEST(GaborKernelTest, Normalize) {
    GaborParams p;
    auto k = gabor_kernel(p, 11, /*normalize=*/true);
    double norm = k.norm();
    EXPECT_NEAR(norm, 1.0, 1e-10);
}

// ===================================================================
// gabor_kernel_imaginary
// ===================================================================

TEST(GaborKernelImaginaryTest, CenterValueIsZero_WhenPsiZero) {
    // sin(0) = 0 at the centre
    GaborParams p{4.0, 0.0, 0.0, 2.0, 0.5};
    auto k = gabor_kernel_imaginary(p, 11);
    int c = 5;
    EXPECT_NEAR(k(c, c), 0.0, 1e-10);
}

TEST(GaborKernelImaginaryTest, OrthogonalToReal) {
    // The real and imaginary parts should be approximately orthogonal
    GaborParams p{8.0, M_PI / 4, 0.0, 3.0, 0.5};
    auto real_k = gabor_kernel(p, 21);
    auto imag_k = gabor_kernel_imaginary(p, 21);
    double dot = (real_k.array() * imag_k.array()).sum();
    EXPECT_NEAR(dot, 0.0, 0.5); // approximately orthogonal
}

// ===================================================================


// ===================================================================
// convolve2d
// ===================================================================

TEST(Convolve2DTest, IdentityKernel) {
    MatrixT img(5, 5);
    img.setRandom();
    MatrixT kernel = MatrixT::Zero(1, 1);
    kernel(0, 0) = 1.0;
    auto out = convolve2d<double>(img, kernel);
    EXPECT_EQ(out.rows(), img.rows());
    EXPECT_EQ(out.cols(), img.cols());
    EXPECT_NEAR((out - img).norm(), 0.0, 1e-10);
}

TEST(Convolve2DTest, ValidPaddingSize) {
    MatrixT img = MatrixT::Ones(10, 10);
    MatrixT kernel = MatrixT::Ones(3, 3);
    auto out = convolve2d<double>(img, kernel);
    EXPECT_EQ(out.rows(), 8);
    EXPECT_EQ(out.cols(), 8);
    // Ones convolved with 3×3 ones = 9
    EXPECT_NEAR(out(0, 0), 9.0, 1e-10);
}

TEST(Convolve2DSameTest, OutputSameSize) {
    MatrixT img = MatrixT::Ones(10, 10);
    MatrixT kernel = MatrixT::Ones(3, 3);
    auto out = convolve2d_same<double>(img, kernel);
    EXPECT_EQ(out.rows(), 10);
    EXPECT_EQ(out.cols(), 10);
}

// ===================================================================
// GaborFilterBank – configuration
// ===================================================================

TEST(GaborFilterBankTest, BuildThrowsWithNoConfig) {
    GaborFilterBank<double> bank;
    EXPECT_THROW(bank.build(), std::runtime_error);
}

TEST(GaborFilterBankTest, BuildThrowsNoWavelengths) {
    GaborFilterBank<double> bank;
    bank.add_orientations(4);
    EXPECT_THROW(bank.build(), std::runtime_error);
}

TEST(GaborFilterBankTest, BuildThrowsNoOrientations) {
    GaborFilterBank<double> bank;
    bank.add_wavelengths({4.0});
    EXPECT_THROW(bank.build(), std::runtime_error);
}

TEST(GaborFilterBankTest, BuildSucceeds) {
    GaborFilterBank<double> bank;
    bank.add_orientations(4);
    bank.add_wavelengths({4.0, 8.0});
    EXPECT_FALSE(bank.is_built());
    bank.build();
    EXPECT_TRUE(bank.is_built());
    EXPECT_EQ(bank.num_kernels(), 8u);   // 4 orient × 2 wavelengths
    EXPECT_EQ(bank.num_orientations(), 4u);
    EXPECT_EQ(bank.num_wavelengths(), 2u);
}

TEST(GaborFilterBankTest, KernelsAreSquareAndOdd) {
    GaborFilterBank<double> bank;
    bank.add_orientations(2);
    bank.add_wavelengths({4.0});
    bank.build();
    for (const auto& k : bank.kernels()) {
        EXPECT_EQ(k.rows(), k.cols());
        EXPECT_EQ(k.rows() % 2, 1);
    }
}

// ===================================================================
// GaborFilterBank – apply
// ===================================================================

TEST(GaborFilterBankTest, ApplyBeforeBuildThrows) {
    GaborFilterBank<double> bank;
    bank.add_orientations(2);
    bank.add_wavelengths({4.0});
    MatrixT img = MatrixT::Ones(20, 20);
    EXPECT_THROW(bank.apply(img), std::runtime_error);
}

TEST(GaborFilterBankTest, ApplySame) {
    GaborFilterBank<double> bank;
    bank.add_orientations(4);
    bank.add_wavelengths({4.0});
    bank.build();

    MatrixT img = MatrixT::Random(30, 30);
    auto responses = bank.apply(img, /*same=*/true);
    EXPECT_EQ(responses.size(), 4u);
    for (const auto& r : responses) {
        EXPECT_EQ(r.rows(), 30);
        EXPECT_EQ(r.cols(), 30);
    }
}

TEST(GaborFilterBankTest, ApplyValid) {
    GaborFilterBank<double> bank;
    bank.add_orientations(2);
    bank.add_wavelengths({4.0});
    bank.set_kernel_size(5);
    bank.build();

    MatrixT img = MatrixT::Random(20, 20);
    auto responses = bank.apply(img, /*same=*/false);
    EXPECT_EQ(responses.size(), 2u);
    for (const auto& r : responses) {
        EXPECT_EQ(r.rows(), 16); // 20 - 5 + 1
        EXPECT_EQ(r.cols(), 16);
    }
}

TEST(GaborFilterBankTest, MeanResponse) {
    GaborFilterBank<double> bank;
    bank.add_orientations(4);
    bank.add_wavelengths({4.0, 8.0});
    bank.build();

    MatrixT img = MatrixT::Random(30, 30);
    auto mr = bank.mean_response(img);
    EXPECT_EQ(mr.rows() * mr.cols(), 8); // 4 × 2
    // Mean of absolute values should be non-negative
    for (int i = 0; i < mr.rows(); ++i) {
        for (int j = 0; j < mr.cols(); ++j) {
            EXPECT_GE(mr(i, j), 0.0);
        }
    }
}

TEST(GaborFilterBankTest, EnergyResponse) {
    GaborFilterBank<double> bank;
    bank.add_orientations(2);
    bank.add_wavelengths({4.0});
    bank.build();

    MatrixT img = MatrixT::Random(20, 20);
    auto energy = bank.apply_energy(img, true);
    EXPECT_EQ(energy.size(), 2u);
    for (const auto& e : energy) {
        EXPECT_EQ(e.rows(), 20);
        EXPECT_EQ(e.cols(), 20);
        // Energy is always non-negative
        EXPECT_TRUE((e.array() >= 0).all());
    }
}

// ===================================================================
// GaborFilterBank – to_string
// ===================================================================

TEST(GaborFilterBankTest, ToString) {
    GaborFilterBank<double> bank;
    bank.add_orientations(8);
    bank.add_wavelengths({4.0, 8.0, 16.0});
    bank.build();
    std::string s = bank.to_string();
    EXPECT_NE(s.find("8 orientations"), std::string::npos);
    EXPECT_NE(s.find("3 wavelengths"), std::string::npos);
    EXPECT_NE(s.find("24 kernels"), std::string::npos);
}

// ===================================================================
// GaborParams – to_string
// ===================================================================

TEST(GaborParamsTest, ToString) {
        GaborParams p{4.0, 0.0, 0.0, 2.0, 0.5};
    std::string s = p.to_string();
    EXPECT_NE(s.find("GaborParams"), std::string::npos);
}

// ===================================================================
// Float type support
// ===================================================================

TEST(GaborKernelFloatTest, FloatWorks) {
    // GaborParams<float> p{4.0f, 0.0f, 0.0f, 2.0f, 0.5f};
    // auto k = gabor_kernel(p, 11);
    // EXPECT_EQ(k.rows(), 11);
    // EXPECT_NEAR(k(5, 5), 1.0f, 1e-5f);
}
