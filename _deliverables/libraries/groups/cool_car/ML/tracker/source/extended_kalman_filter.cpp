
#include "extended_kalman_filter.h"
#include <functional>

ExtendedKalmanFilter::ExtendedKalmanFilter(
    const Vector& x0, const Matrix& P0,
    const Matrix& Q, const Matrix& R)
    : x_(x0), P_(P0), Q_(Q), R_(R) {}

void ExtendedKalmanFilter::setProcessModel(
    const std::function<Vector(const Vector&)>& f,
    const std::function<Matrix(const Vector&)>& F) {
    f_ = f;
    F_ = F;
}

void ExtendedKalmanFilter::setMeasurementModel(
    const std::function<Vector(const Vector&)>& h,
    const std::function<Matrix(const Vector&)>& H) {
    h_ = h;
    H_ = H;
}


// Helper: multiply DenseMatrix by vector
static ExtendedKalmanFilter::Vector multiplyVec(const ExtendedKalmanFilter::Matrix& M, const ExtendedKalmanFilter::Vector& v) {
    size_t rows = M.rows(), cols = M.cols();
    ExtendedKalmanFilter::Vector result = ExtendedKalmanFilter::Vector::Zero(rows);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            result.at(i) += M.at(i, j) * v.at(j);
    return result;
}

// Helper: convert unique_ptr<MatrixBase> to DenseMatrix
static ExtendedKalmanFilter::Matrix toDense(const std::unique_ptr<mytrix::MatrixBase>& ptr) {
    const mytrix::DenseMatrix* dm = dynamic_cast<const mytrix::DenseMatrix*>(ptr.get());
    if (!dm) throw std::runtime_error("Matrix conversion failed");
    return *dm;
}

void ExtendedKalmanFilter::predict() {
    Matrix F_mat = F_(x_);
    x_ = f_(x_);
    // P_ = F_mat * P_ * F_mat.transpose() + Q_;
    // F_mat * P_ * F_mat.transpose() + Q_
    std::unique_ptr<mytrix::MatrixBase> FP_ptr = F_mat.multiply(P_);
    Matrix FP = toDense(FP_ptr);
    std::unique_ptr<mytrix::MatrixBase> Ft_ptr = F_mat.transpose();
    Matrix Ft = toDense(Ft_ptr);
    std::unique_ptr<mytrix::MatrixBase> FPFt_ptr = FP.multiply(Ft);
    Matrix FPFt = toDense(FPFt_ptr);
    std::unique_ptr<mytrix::MatrixBase> PQ_ptr = FPFt.add(Q_);
    P_ = toDense(PQ_ptr);
}

void ExtendedKalmanFilter::update(const Vector& z) {
    Matrix H_mat = H_(x_);
    Vector y = z;
    const Vector hx = h_(x_);
    for (size_t i = 0; i < y.size(); ++i) y.at(i) -= hx.at(i);
    std::unique_ptr<mytrix::MatrixBase> HP_ptr = H_mat.multiply(P_);
    Matrix HP = toDense(HP_ptr);
    std::unique_ptr<mytrix::MatrixBase> Ht_ptr = H_mat.transpose();
    Matrix Ht = toDense(Ht_ptr);
    std::unique_ptr<mytrix::MatrixBase> HPHt_ptr = HP.multiply(Ht);
    Matrix HPHt = toDense(HPHt_ptr);
    std::unique_ptr<mytrix::MatrixBase> S_ptr = HPHt.add(R_);
    Matrix S = toDense(S_ptr);
    // For now, use identity as S inverse (should implement real inverse)
    Matrix S_inv = Matrix(S.data.inverse());
    std::unique_ptr<mytrix::MatrixBase> PHt_ptr = P_.multiply(Ht);
    Matrix PHt = toDense(PHt_ptr);
    std::unique_ptr<mytrix::MatrixBase> K_ptr = PHt.multiply(S_inv);
    Matrix K = toDense(K_ptr);
    // x_ = x_ + K * y;
    Vector Ky = multiplyVec(K, y);
    for (size_t i = 0; i < x_.size(); ++i) x_.at(i) += Ky.at(i);
    // I - K*H
    Matrix I = Matrix::Identity(x_.size());
    std::unique_ptr<mytrix::MatrixBase> KH_ptr = K.multiply(H_mat);
    Matrix KH = toDense(KH_ptr);
    Matrix I_minus_KH = I;
    for (size_t r = 0; r < I.rows(); ++r)
        for (size_t c = 0; c < I.cols(); ++c)
            I_minus_KH.at(r, c) -= KH.at(r, c);
    std::unique_ptr<mytrix::MatrixBase> IP_ptr = I_minus_KH.multiply(P_);
    P_ = toDense(IP_ptr);
}

const ExtendedKalmanFilter::Vector& ExtendedKalmanFilter::state() const { return x_; }
const ExtendedKalmanFilter::Matrix& ExtendedKalmanFilter::covariance() const { return P_; }
