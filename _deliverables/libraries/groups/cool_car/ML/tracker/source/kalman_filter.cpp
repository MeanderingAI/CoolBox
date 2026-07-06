
//#include "kalman_filter.h"
#include "kalman_filter.h"

KalmanFilter::KalmanFilter(double dt, const Matrix& A, const Matrix& C,
                           const Matrix& Q, const Matrix& R,
                           const Matrix& P)
    : dt(dt), A(A), C(C), Q(Q), R(R), P(P) {
    x = Vector::Zero(A.rows());
}

void KalmanFilter::init(const Vector& x0) {
    x = x0;
}

// Helper: multiply DenseMatrix by vector
static KalmanFilter::Vector multiplyVec(const KalmanFilter::Matrix& M, const KalmanFilter::Vector& v) {
    size_t rows = M.rows(), cols = M.cols();
    KalmanFilter::Vector result = KalmanFilter::Vector::Zero(rows);
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            result.at(i) += M.at(i, j) * v.at(j);
    return result;
}

// Helper: convert unique_ptr<MatrixBase> to DenseMatrix
static KalmanFilter::Matrix toDense(const std::unique_ptr<mytrix::MatrixBase>& ptr) {
    const mytrix::DenseMatrix* dm = dynamic_cast<const mytrix::DenseMatrix*>(ptr.get());
    if (!dm) throw std::runtime_error("Matrix conversion failed");
    return *dm;
}

// Helper: general matrix inverse via Eigen
static KalmanFilter::Matrix matInverse(const KalmanFilter::Matrix& M) {
    return KalmanFilter::Matrix(M.data.inverse());
}

void KalmanFilter::predict() {
    x = multiplyVec(A, x);
    auto At_ptr = A.transpose();
    const mytrix::DenseMatrix* At_dm = dynamic_cast<const mytrix::DenseMatrix*>(At_ptr.get());
    if (!At_dm) throw std::runtime_error("Matrix conversion failed");
    Matrix At = *At_dm;
    auto AP_ptr = A.multiply(P);
    const mytrix::DenseMatrix* AP_dm = dynamic_cast<const mytrix::DenseMatrix*>(AP_ptr.get());
    Matrix AP = *AP_dm;
    auto APAt_ptr = AP.multiply(At);
    const mytrix::DenseMatrix* APAt_dm = dynamic_cast<const mytrix::DenseMatrix*>(APAt_ptr.get());
    Matrix APAt = *APAt_dm;
    auto PQ_ptr = APAt.add(Q);
    const mytrix::DenseMatrix* PQ_dm = dynamic_cast<const mytrix::DenseMatrix*>(PQ_ptr.get());
    P = *PQ_dm;
}

void KalmanFilter::update(const Vector& y) {
    auto Ct_ptr = C.transpose();
    const mytrix::DenseMatrix* Ct_dm = dynamic_cast<const mytrix::DenseMatrix*>(Ct_ptr.get());
    if (!Ct_dm) throw std::runtime_error("Matrix conversion failed");
    Matrix Ct = *Ct_dm;
    auto CP_ptr = C.multiply(P);
    const mytrix::DenseMatrix* CP_dm = dynamic_cast<const mytrix::DenseMatrix*>(CP_ptr.get());
    Matrix CP = *CP_dm;
    auto CPCt_ptr = CP.multiply(Ct);
    const mytrix::DenseMatrix* CPCt_dm = dynamic_cast<const mytrix::DenseMatrix*>(CPCt_ptr.get());
    Matrix CPCt = *CPCt_dm;
    auto S_ptr = CPCt.add(R);
    const mytrix::DenseMatrix* S_dm = dynamic_cast<const mytrix::DenseMatrix*>(S_ptr.get());
    Matrix S = *S_dm;
    // Only 2x2 inverse for demo; replace with general inverse as needed
    Matrix Sinv = matInverse(S);
    auto KCt_ptr = P.multiply(Ct);
    const mytrix::DenseMatrix* KCt_dm = dynamic_cast<const mytrix::DenseMatrix*>(KCt_ptr.get());
    Matrix KCt = *KCt_dm;
    auto K_ptr = KCt.multiply(Sinv);
    const mytrix::DenseMatrix* K_dm = dynamic_cast<const mytrix::DenseMatrix*>(K_ptr.get());
    Matrix K = *K_dm;
    Vector y_minus_Cx = y;
    Vector Cx = multiplyVec(C, x);
    for (size_t i = 0; i < y.size(); ++i) y_minus_Cx.at(i) -= Cx.at(i);
    Vector K_y_minus_Cx = multiplyVec(K, y_minus_Cx);
    for (size_t i = 0; i < x.size(); ++i) x.at(i) += K_y_minus_Cx.at(i);
    Matrix I = Matrix::Identity(x.size());
    auto KC_ptr = K.multiply(C);
    const mytrix::DenseMatrix* KC_dm = dynamic_cast<const mytrix::DenseMatrix*>(KC_ptr.get());
    Matrix KC = *KC_dm;
    // I - KC
    Matrix I_minus_KC = I;
    for (size_t r = 0; r < I.rows(); ++r)
        for (size_t c = 0; c < I.cols(); ++c)
            I_minus_KC.at(r, c) -= KC.at(r, c);
    auto IP_ptr = I_minus_KC.multiply(P);
    const mytrix::DenseMatrix* IP_dm = dynamic_cast<const mytrix::DenseMatrix*>(IP_ptr.get());
    P = *IP_dm;
}

const KalmanFilter::Vector& KalmanFilter::state() const {
    return x;
}

const KalmanFilter::Matrix& KalmanFilter::covariance() const {
    return P;
}
