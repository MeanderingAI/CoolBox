// Kalman Filter bindings — wraps Eigen types for JS.
#include <emscripten/bind.h>
#include "kalman_filter.h"

using namespace emscripten;

class KalmanFilterWrapper {
    std::unique_ptr<KalmanFilter> kf_;
    int state_dim_;
public:
    KalmanFilterWrapper(double dt, int state_dim, int meas_dim) : state_dim_(state_dim) {
        Eigen::MatrixXd A = Eigen::MatrixXd::Identity(state_dim, state_dim);
        Eigen::MatrixXd C = Eigen::MatrixXd::Identity(meas_dim, state_dim);
        Eigen::MatrixXd Q = Eigen::MatrixXd::Identity(state_dim, state_dim) * 0.01;
        Eigen::MatrixXd R = Eigen::MatrixXd::Identity(meas_dim, meas_dim) * 0.1;
        Eigen::MatrixXd P = Eigen::MatrixXd::Identity(state_dim, state_dim);
        kf_ = std::make_unique<KalmanFilter>(dt, A, C, Q, R, P);
    }

    void init(const std::vector<double>& x0) {
        Eigen::VectorXd v = Eigen::Map<const Eigen::VectorXd>(x0.data(), x0.size());
        kf_->init(v);
    }

    void predict() { kf_->predict(); }

    void update(const std::vector<double>& measurement) {
        Eigen::VectorXd z = Eigen::Map<const Eigen::VectorXd>(measurement.data(), measurement.size());
        kf_->update(z);
    }

    std::vector<double> state() {
        const auto& s = kf_->state();
        return std::vector<double>(s.begin(), s.end());
    }
};

EMSCRIPTEN_BINDINGS(tracker_module) {
    register_vector<double>("VectorDouble_Tracker");

    class_<KalmanFilterWrapper>("KalmanFilter")
        .constructor<double, int, int>()
        .function("init", &KalmanFilterWrapper::init)
        .function("predict", &KalmanFilterWrapper::predict)
        .function("update", &KalmanFilterWrapper::update)
        .function("state", &KalmanFilterWrapper::state)
    ;
}
