// KalmanFilter header cleaned up for DenseMatrix/my_vector only
#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include "matrix_dense.h"
#include "base_kalman_filter.h"
#include <vector>

class KalmanFilter : public BaseKalmanFilter {
public:
    using Vector = mytrix::Vector;
    using Matrix = mytrix::Matrix;

    KalmanFilter(double dt,
                 const Matrix& A,
                 const Matrix& C,
                 const Matrix& Q,
                 const Matrix& R,
                 const Matrix& P);

    void init(const Vector& x0);
    void predict() override;
    void update(const Vector& y) override;
    const Vector& state() const override;
    const Matrix& covariance() const override;

private:
    double dt;
    Vector x;
    Matrix P;
    Matrix A;
    Matrix C;
    Matrix Q;
    Matrix R;
};

#endif // KALMAN_FILTER_H