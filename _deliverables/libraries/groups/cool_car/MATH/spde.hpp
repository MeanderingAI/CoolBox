#include "../../../trekker/DATASTRUCTURE/xarray/headers/xarray.h"
#include <type_traits>
// spde.hpp
// Basic SPDE solver interface for cool_car::MATH
#pragma once

#include <vector>
#include <random>
#include "../MATRIX/headers/matrix_dense.h"

namespace cool_car {
namespace MATH {

// Example: 1D stochastic heat equation solver (Euler-Maruyama)
class SPDESolver {
    // NDArray traits for backend selection
    template<typename T>
    struct is_mytrix_matrix : std::false_type {};
    template<>
    struct is_mytrix_matrix<mytrix::DenseMatrix> : std::true_type {};

    template<typename T>
    struct is_xarray : std::false_type {};
    template<typename V, size_t N>
    struct is_xarray<xarray<V, N>> : std::true_type {};

    // NDArray solver for mytrix::DenseMatrix (recursive vector-of-matrix ND)
    template<typename NDArray, typename RNG>
    static void solve_stochastic_heat_eq_nd_mytrix(
        NDArray& u, NDArray& u_new, const std::vector<double>& dx, double alpha, double sigma, double dt, int steps, RNG& rng, int dim = 0) {
        if constexpr (std::is_same_v<typename NDArray::value_type, mytrix::DenseMatrix>) {
            int Nx = u.size();
            int Ny = u[0].rows();
            int Nz = u[0].cols();
            std::normal_distribution<double> norm(0.0, 1.0);
            for (int t = 0; t < steps; ++t) {
                for (int i = 1; i < Nx - 1; ++i)
                    for (int j = 1; j < Ny - 1; ++j)
                        for (int k = 1; k < Nz - 1; ++k) {
                            double dW = norm(rng) * std::sqrt(dt);
                            u_new[i](j, k) = u[i](j, k)
                                + alpha * dt * (
                                    (u[i-1](j, k) - 2*u[i](j, k) + u[i+1](j, k)) / (dx[0]*dx[0])
                                  + (u[i](j-1, k) - 2*u[i](j, k) + u[i](j+1, k)) / (dx[1]*dx[1])
                                  + (u[i](j, k-1) - 2*u[i](j, k) + u[i](j, k+1)) / (dx[2]*dx[2])
                                ) + sigma * dW;
                        }
                // Dirichlet BCs (faces)
                for (int i = 0; i < Nx; ++i)
                    for (int j = 0; j < Ny; ++j)
                        u_new[i](j, 0) = u[i](j, 0), u_new[i](j, Nz-1) = u[i](j, Nz-1);
                for (int i = 0; i < Nx; ++i)
                    for (int k = 0; k < Nz; ++k)
                        u_new[i](0, k) = u[i](0, k), u_new[i](Ny-1, k) = u[i](Ny-1, k);
                for (int j = 0; j < Ny; ++j)
                    for (int k = 0; k < Nz; ++k)
                        u_new[0](j, k) = u[0](j, k), u_new[Nx-1](j, k) = u[Nx-1](j, k);
                u = u_new;
            }
        } else {
            // Recursively descend
            int N = u.size();
            for (int i = 0; i < N; ++i)
                solve_stochastic_heat_eq_nd_mytrix(u[i], u_new[i], dx, alpha, sigma, dt, steps, rng, dim + 1);
        }
    }

    // NDArray solver for xarray backend
    template<typename XArr, typename RNG>
    static void solve_stochastic_heat_eq_nd_xarray(
        XArr& u, XArr& u_new, const std::vector<double>& dx, double alpha, double sigma, double dt, int steps, RNG& rng) {
        constexpr size_t N = std::remove_reference_t<XArr>::shape_type::size();
        using idx_t = typename XArr::index_type;
        auto shape = u.shape();
        std::normal_distribution<double> norm(0.0, 1.0);
        for (int t = 0; t < steps; ++t) {
            idx_t idx{};
            // Loop over all interior points
            auto advance = [&]() {
                for (int d = N; d-- > 0;) {
                    if (++idx[d] < shape[d]-1) return true;
                    idx[d] = 1;
                }
                return false;
            };
            do {
                double lap = 0.0;
                for (size_t d = 0; d < N; ++d) {
                    idx[d]--;
                    double prev = u[idx];
                    idx[d] += 2;
                    double next = u[idx];
                    idx[d]--;
                    lap += (prev - 2*u[idx] + next) / (dx[d]*dx[d]);
                }
                double dW = norm(rng) * std::sqrt(dt);
                u_new[idx] = u[idx] + alpha * dt * lap + sigma * dW;
            } while (advance());
            // Dirichlet BCs not handled for ND (user must set)
            u = u_new;
        }
    }

    // Unified NDArray interface
    template<typename NDArray, typename RNG>
    static NDArray solve_stochastic_heat_eq_nd(
        const NDArray& u0,
        double alpha, double sigma, const std::vector<double>& dx, double dt, int steps, RNG& rng) {
        NDArray u = u0;
        NDArray u_new = u0;
        if constexpr (is_mytrix_matrix<typename NDArray::value_type>::value || std::is_same_v<typename NDArray::value_type, mytrix::DenseMatrix>) {
            solve_stochastic_heat_eq_nd_mytrix(u, u_new, dx, alpha, sigma, dt, steps, rng);
        } else if constexpr (is_xarray<NDArray>::value) {
            solve_stochastic_heat_eq_nd_xarray(u, u_new, dx, alpha, sigma, dt, steps, rng);
        } else {
            static_assert(sizeof(NDArray) == 0, "Unsupported NDArray type for SPDE solver");
        }
        return u;
    }
public:
    // Optional: Explicit Euler optimizer for extensibility
    struct ExplicitEulerOptimizer {
        double alpha;
        double sigma;
        std::vector<double> dx;
        double dt;
        int steps;
        ExplicitEulerOptimizer(double alpha_, double sigma_, std::vector<double> dx_, double dt_, int steps_)
            : alpha(alpha_), sigma(sigma_), dx(std::move(dx_)), dt(dt_), steps(steps_) {}
    };
    // Solve du = alpha * u_xx dt + sigma dW on [0, L] with Dirichlet BCs
    // u0: initial condition as DenseMatrix (Nx1), alpha: diffusion, sigma: noise, dx: space step, dt: time step, steps: time steps
    template<typename RNG>
    static mytrix::DenseMatrix solve_stochastic_heat_eq(
        const mytrix::DenseMatrix& u0, double alpha, double sigma, double dx, double dt, int steps, RNG& rng) {
        mytrix::DenseMatrix u = u0;
        mytrix::DenseMatrix u_new = u0;
        int N = u.rows();
        std::normal_distribution<double> norm(0.0, 1.0);
        for (int t = 0; t < steps; ++t) {
            for (int i = 1; i < N - 1; ++i) {
                double dW = norm(rng) * std::sqrt(dt);
                u_new(i, 0) = u(i, 0) + alpha * dt / (dx * dx) * (u(i-1, 0) - 2*u(i, 0) + u(i+1, 0)) + sigma * dW;
            }
            u_new(0, 0) = u(0, 0);
            u_new(N-1, 0) = u(N-1, 0);
            u = u_new;
        }
        return u;
    }

    // 2D stochastic heat equation
    template<typename RNG>
    static mytrix::DenseMatrix solve_stochastic_heat_eq_2d(
        const mytrix::DenseMatrix& u0, double alpha, double sigma, double dx, double dy, double dt, int steps, RNG& rng) {
        int Nx = u0.rows();
        int Ny = u0.cols();
        mytrix::DenseMatrix u = u0;
        mytrix::DenseMatrix u_new = u0;
        std::normal_distribution<double> norm(0.0, 1.0);
        for (int t = 0; t < steps; ++t) {
            for (int i = 1; i < Nx - 1; ++i) {
                for (int j = 1; j < Ny - 1; ++j) {
                    double dW = norm(rng) * std::sqrt(dt);
                    u_new(i, j) = u(i, j)
                        + alpha * dt * (
                            (u(i-1, j) - 2*u(i, j) + u(i+1, j)) / (dx*dx)
                          + (u(i, j-1) - 2*u(i, j) + u(i, j+1)) / (dy*dy)
                        ) + sigma * dW;
                }
            }
            for (int i = 0; i < Nx; ++i) { u_new(i, 0) = u(i, 0); u_new(i, Ny-1) = u(i, Ny-1); }
            for (int j = 0; j < Ny; ++j) { u_new(0, j) = u(0, j); u_new(Nx-1, j) = u(Nx-1, j); }
            u = u_new;
        }
        return u;
    }


    // 3D stochastic heat equation using mytrix::DenseMatrix as a 3D array (vector of matrices)
    template<typename RNG>
    static std::vector<mytrix::DenseMatrix> solve_stochastic_heat_eq_3d(
        const std::vector<mytrix::DenseMatrix>& u0, double alpha, double sigma, double dx, double dy, double dz, double dt, int steps, RNG& rng) {
        int Nx = u0.size();
        int Ny = u0[0].rows();
        int Nz = u0[0].cols();
        auto u = u0;
        auto u_new = u0;
        std::normal_distribution<double> norm(0.0, 1.0);
        for (int t = 0; t < steps; ++t) {
            for (int i = 1; i < Nx - 1; ++i) {
                for (int j = 1; j < Ny - 1; ++j) {
                    for (int k = 1; k < Nz - 1; ++k) {
                        double dW = norm(rng) * std::sqrt(dt);
                        u_new[i](j, k) = u[i](j, k)
                            + alpha * dt * (
                                (u[i-1](j, k) - 2*u[i](j, k) + u[i+1](j, k)) / (dx*dx)
                              + (u[i](j-1, k) - 2*u[i](j, k) + u[i](j+1, k)) / (dy*dy)
                              + (u[i](j, k-1) - 2*u[i](j, k) + u[i](j, k+1)) / (dz*dz)
                            ) + sigma * dW;
                    }
                }
            }
            // Dirichlet BCs (faces)
            for (int i = 0; i < Nx; ++i)
                for (int j = 0; j < Ny; ++j)
                    u_new[i](j, 0) = u[i](j, 0), u_new[i](j, Nz-1) = u[i](j, Nz-1);
            for (int i = 0; i < Nx; ++i)
                for (int k = 0; k < Nz; ++k)
                    u_new[i](0, k) = u[i](0, k), u_new[i](Ny-1, k) = u[i](Ny-1, k);
            for (int j = 0; j < Ny; ++j)
                for (int k = 0; k < Nz; ++k)
                    u_new[0](j, k) = u[0](j, k), u_new[Nx-1](j, k) = u[Nx-1](j, k);
            u = u_new;
        }
        return u;
    }

    // (Removed duplicate ND version)
};

} // namespace MATH
} // namespace cool_car
