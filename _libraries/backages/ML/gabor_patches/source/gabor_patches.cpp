#include "../headers/gabor_patches.h"

#include <algorithm>
#include <numeric>
#include <iostream>

namespace ml {

// ===================================================================
// GaborParams
// ===================================================================

template <typename Scalar>
int GaborParams<Scalar>::default_kernel_size() const {
    int half = static_cast<int>(std::ceil(3.0 * sigma));
    return 2 * half + 1;
}

template <typename Scalar>
std::string GaborParams<Scalar>::to_string() const {
    std::ostringstream os;
    os << std::fixed << std::setprecision(3)
       << "GaborParams(λ=" << lambda << ", θ=" << theta
       << ", ψ=" << psi << ", σ=" << sigma << ", γ=" << gamma << ")";
    return os.str();
}

// ===================================================================
// gabor_kernel (real / cosine part)
// ===================================================================

template <typename Scalar>
Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
gabor_kernel(const GaborParams<Scalar>& params, int size, bool normalize)
{
    using MatrixT = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;

    if (size == 0) size = params.default_kernel_size();
    if (size < 1)
        throw std::invalid_argument("gabor_kernel: size must be >= 1");
    if (size % 2 == 0)
        throw std::invalid_argument("gabor_kernel: size must be odd");
    if (params.lambda <= 0)
        throw std::invalid_argument("gabor_kernel: lambda must be > 0");
    if (params.sigma <= 0)
        throw std::invalid_argument("gabor_kernel: sigma must be > 0");

    const int half = size / 2;
    MatrixT kernel(size, size);

    const Scalar cos_t  = std::cos(params.theta);
    const Scalar sin_t  = std::sin(params.theta);
    const Scalar sigma2 = params.sigma * params.sigma;
    const Scalar gamma2 = params.gamma * params.gamma;
    const Scalar freq   = Scalar(2.0) * Scalar(M_PI) / params.lambda;

    for (int j = -half; j <= half; ++j) {
        for (int i = -half; i <= half; ++i) {
            Scalar x = static_cast<Scalar>(i);
            Scalar y = static_cast<Scalar>(j);

            Scalar x_prime =  x * cos_t + y * sin_t;
            Scalar y_prime = -x * sin_t + y * cos_t;

            Scalar envelope = std::exp(
                -(x_prime * x_prime + gamma2 * y_prime * y_prime) /
                (Scalar(2.0) * sigma2));

            Scalar carrier = std::cos(freq * x_prime + params.psi);

            kernel(j + half, i + half) = envelope * carrier;
        }
    }

    if (normalize) {
        Scalar norm = kernel.norm();
        if (norm > Scalar(1e-12)) kernel /= norm;
    }

    return kernel;
}

// ===================================================================
// gabor_kernel_imaginary (sine part)
// ===================================================================

template <typename Scalar>
Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
gabor_kernel_imaginary(const GaborParams<Scalar>& params, int size, bool normalize)
{
    using MatrixT = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;

    if (size == 0) size = params.default_kernel_size();
    if (size < 1 || size % 2 == 0)
        throw std::invalid_argument("gabor_kernel_imaginary: size must be odd and >= 1");

    const int half = size / 2;
    MatrixT kernel(size, size);

    const Scalar cos_t  = std::cos(params.theta);
    const Scalar sin_t  = std::sin(params.theta);
    const Scalar sigma2 = params.sigma * params.sigma;
    const Scalar gamma2 = params.gamma * params.gamma;
    const Scalar freq   = Scalar(2.0) * Scalar(M_PI) / params.lambda;

    for (int j = -half; j <= half; ++j) {
        for (int i = -half; i <= half; ++i) {
            Scalar x = static_cast<Scalar>(i);
            Scalar y = static_cast<Scalar>(j);

            Scalar x_prime =  x * cos_t + y * sin_t;
            Scalar y_prime = -x * sin_t + y * cos_t;

            Scalar envelope = std::exp(
                -(x_prime * x_prime + gamma2 * y_prime * y_prime) /
                (Scalar(2.0) * sigma2));

            Scalar carrier = std::sin(freq * x_prime + params.psi);

            kernel(j + half, i + half) = envelope * carrier;
        }
    }

    if (normalize) {
        Scalar norm = kernel.norm();
        if (norm > Scalar(1e-12)) kernel /= norm;
    }

    return kernel;
}

// ===================================================================
// gabor_energy
// ===================================================================

template <typename Scalar>
Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
gabor_energy(const GaborParams<Scalar>& params, int size)
{
    auto real_k = gabor_kernel(params, size);
    auto imag_k = gabor_kernel_imaginary(params, size);
    return (real_k.array().square() + imag_k.array().square()).sqrt().matrix();
}

// ===================================================================
// convolve2d (valid padding)
// ===================================================================

template <typename Scalar>
Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
convolve2d(const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& image,
           const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& kernel)
{
    using MatrixT = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;

    const int ir = image.rows(), ic = image.cols();
    const int kr = kernel.rows(), kc = kernel.cols();

    if (kr > ir || kc > ic)
        throw std::invalid_argument("convolve2d: kernel larger than image");

    const int or_ = ir - kr + 1;
    const int oc  = ic - kc + 1;
    MatrixT out(or_, oc);

    for (int j = 0; j < or_; ++j) {
        for (int i = 0; i < oc; ++i) {
            out(j, i) = (image.block(j, i, kr, kc).array() *
                         kernel.array()).sum();
        }
    }
    return out;
}

// ===================================================================
// convolve2d_same (zero-padding, output = input size)
// ===================================================================

template <typename Scalar>
Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>
convolve2d_same(const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& image,
                const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>& kernel)
{
    using MatrixT = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;

    const int ir = image.rows(), ic = image.cols();
    const int kr = kernel.rows(), kc = kernel.cols();
    const int pad_r = kr / 2, pad_c = kc / 2;

    MatrixT padded = MatrixT::Zero(ir + 2 * pad_r, ic + 2 * pad_c);
    padded.block(pad_r, pad_c, ir, ic) = image;

    return convolve2d(padded, kernel);
}

// ===================================================================
// GaborFilterBank – configuration
// ===================================================================

template <typename Scalar>
void GaborFilterBank<Scalar>::add_orientations(int n) {
    if (n < 1)
        throw std::invalid_argument("add_orientations: n must be >= 1");
    for (int i = 0; i < n; ++i)
        thetas_.push_back(Scalar(M_PI) * Scalar(i) / Scalar(n));
}

template <typename Scalar>
void GaborFilterBank<Scalar>::add_orientation(Scalar theta) {
    thetas_.push_back(theta);
}

template <typename Scalar>
void GaborFilterBank<Scalar>::add_wavelengths(const std::vector<Scalar>& lambdas) {
    lambdas_.insert(lambdas_.end(), lambdas.begin(), lambdas.end());
}

template <typename Scalar>
void GaborFilterBank<Scalar>::add_wavelength(Scalar lambda) {
    lambdas_.push_back(lambda);
}

template <typename Scalar>
void GaborFilterBank<Scalar>::set_sigma(Scalar sigma) { sigma_ = sigma; }

template <typename Scalar>
void GaborFilterBank<Scalar>::set_gamma(Scalar gamma) { gamma_ = gamma; }

template <typename Scalar>
void GaborFilterBank<Scalar>::set_psi(Scalar psi) { psi_ = psi; }

template <typename Scalar>
void GaborFilterBank<Scalar>::set_kernel_size(int size) { kernel_size_ = size; }

// ===================================================================
// GaborFilterBank – build
// ===================================================================

template <typename Scalar>
void GaborFilterBank<Scalar>::build() {
    if (thetas_.empty())
        throw std::runtime_error("GaborFilterBank::build: no orientations set");
    if (lambdas_.empty())
        throw std::runtime_error("GaborFilterBank::build: no wavelengths set");

    kernels_.clear();
    params_.clear();

    for (const auto& lam : lambdas_) {
        for (const auto& th : thetas_) {
            GaborParams<Scalar> p;
            p.lambda = lam;
            p.theta  = th;
            p.psi    = psi_;
            p.sigma  = (sigma_ > 0) ? sigma_ : Scalar(0.56) * lam;
            p.gamma  = gamma_;

            params_.push_back(p);
            kernels_.push_back(gabor_kernel(p, kernel_size_, /*normalize=*/true));
        }
    }

    built_ = true;
}

// ===================================================================
// GaborFilterBank – apply
// ===================================================================

template <typename Scalar>
std::vector<typename GaborFilterBank<Scalar>::MatrixT>
GaborFilterBank<Scalar>::apply(const MatrixT& image, bool same) const {
    if (!built_)
        throw std::runtime_error("GaborFilterBank::apply: call build() first");

    std::vector<MatrixT> responses;
    responses.reserve(kernels_.size());

    for (const auto& k : kernels_) {
        if (same)
            responses.push_back(convolve2d_same(image, k));
        else
            responses.push_back(convolve2d(image, k));
    }
    return responses;
}

template <typename Scalar>
Eigen::Matrix<Scalar, Eigen::Dynamic, 1>
GaborFilterBank<Scalar>::mean_response(const MatrixT& image, bool same) const {
    auto responses = apply(image, same);
    Eigen::Matrix<Scalar, Eigen::Dynamic, 1> out(responses.size());
    for (size_t i = 0; i < responses.size(); ++i) {
        out(static_cast<int>(i)) =
            responses[i].array().abs().mean();
    }
    return out;
}

template <typename Scalar>
std::vector<typename GaborFilterBank<Scalar>::MatrixT>
GaborFilterBank<Scalar>::apply_energy(const MatrixT& image, bool same) const {
    if (!built_)
        throw std::runtime_error("GaborFilterBank::apply_energy: call build() first");

    std::vector<MatrixT> responses;
    responses.reserve(params_.size());

    for (const auto& p : params_) {
        auto real_k = gabor_kernel(p, kernel_size_, true);
        auto imag_k = gabor_kernel_imaginary(p, kernel_size_, true);

        MatrixT real_resp, imag_resp;
        if (same) {
            real_resp = convolve2d_same(image, real_k);
            imag_resp = convolve2d_same(image, imag_k);
        } else {
            real_resp = convolve2d(image, real_k);
            imag_resp = convolve2d(image, imag_k);
        }

        responses.push_back(
            (real_resp.array().square() + imag_resp.array().square()).sqrt().matrix());
    }
    return responses;
}

// ===================================================================
// GaborFilterBank – to_string
// ===================================================================

template <typename Scalar>
std::string GaborFilterBank<Scalar>::to_string() const {
    std::ostringstream os;
    os << "GaborFilterBank(" << num_orientations() << " orientations × "
       << num_wavelengths() << " wavelengths = "
       << num_kernels() << " kernels)";
    return os.str();
}

// ===================================================================
// Explicit template instantiations
// ===================================================================

template struct GaborParams<float>;
template struct GaborParams<double>;

template Eigen::MatrixXf gabor_kernel<float>(const GaborParams<float>&, int, bool);
template Eigen::MatrixXd gabor_kernel<double>(const GaborParams<double>&, int, bool);

template Eigen::MatrixXf gabor_kernel_imaginary<float>(const GaborParams<float>&, int, bool);
template Eigen::MatrixXd gabor_kernel_imaginary<double>(const GaborParams<double>&, int, bool);

template Eigen::MatrixXf gabor_energy<float>(const GaborParams<float>&, int);
template Eigen::MatrixXd gabor_energy<double>(const GaborParams<double>&, int);

template Eigen::MatrixXf convolve2d<float>(const Eigen::MatrixXf&, const Eigen::MatrixXf&);
template Eigen::MatrixXd convolve2d<double>(const Eigen::MatrixXd&, const Eigen::MatrixXd&);

template Eigen::MatrixXf convolve2d_same<float>(const Eigen::MatrixXf&, const Eigen::MatrixXf&);
template Eigen::MatrixXd convolve2d_same<double>(const Eigen::MatrixXd&, const Eigen::MatrixXd&);

template class GaborFilterBank<float>;
template class GaborFilterBank<double>;

} // namespace ml
