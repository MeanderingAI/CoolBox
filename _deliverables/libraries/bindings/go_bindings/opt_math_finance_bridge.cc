#include "opt_math_finance_bridge.h"

#include "optimization_factory.h"
#include "distributed_owa.h"
#include "finance.hpp"
#include "pde_solver.hpp"
#include "spde.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using OptimizerPtr = std::unique_ptr<opt::OptimizationAlgorithm>;

struct OptimizerHandle {
    explicit OptimizerHandle(OptimizerPtr optimizer) : value(std::move(optimizer)) {}
    OptimizerPtr value;
    std::vector<double> solution;
};

void set_error(char* output, size_t capacity, const char* message) {
    if (!output || capacity == 0) return;
    const size_t size = std::min(capacity - 1, std::strlen(message));
    std::memcpy(output, message, size);
    output[size] = '\0';
}

int config_int(const double* values, size_t count, size_t index, int fallback) {
    if (index >= count) return fallback;
    const double value = values[index];
    if (!std::isfinite(value) || std::trunc(value) != value ||
        value < std::numeric_limits<int>::min() ||
        value > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("optimizer integer config values must be finite native integers");
    }
    return static_cast<int>(value);
}

unsigned int config_uint(const double* values, size_t count, size_t index,
                         unsigned int fallback) {
    if (index >= count) return fallback;
    const double value = values[index];
    if (!std::isfinite(value) || std::trunc(value) != value || value < 0 ||
        value > std::numeric_limits<unsigned int>::max()) {
        throw std::invalid_argument("optimizer seed must be a finite non-negative integer");
    }
    return static_cast<unsigned int>(value);
}

template <typename Function>
int invoke(char* error, size_t capacity, Function&& function) {
    try {
        function();
        return 1;
    } catch (const std::exception& exception) {
        set_error(error, capacity, exception.what());
    } catch (...) {
        set_error(error, capacity, "unknown native exception");
    }
    return 0;
}

template <typename Function>
double invoke_double(char* error, size_t capacity, Function&& function) {
    try {
        return function();
    } catch (const std::exception& exception) {
        set_error(error, capacity, exception.what());
    } catch (...) {
        set_error(error, capacity, "unknown native exception");
    }
    return std::numeric_limits<double>::quiet_NaN();
}

OptimizerHandle& optimizer(void* handle) {
    if (!handle) throw std::invalid_argument("optimizer is null");
    return *static_cast<OptimizerHandle*>(handle);
}

mytrix::DenseMatrix matrix_from_flat(const double* input, size_t rows, size_t columns) {
    if (!input || rows == 0 || columns == 0 ||
        rows > static_cast<size_t>(std::numeric_limits<int>::max()) ||
        columns > static_cast<size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("matrix dimensions and input data must be non-empty");
    }
    return mytrix::DenseMatrix(std::vector<double>(input, input + rows * columns),
                               static_cast<int>(rows), static_cast<int>(columns));
}

void copy_matrix(const mytrix::DenseMatrix& matrix, double* output) {
    if (!output) throw std::invalid_argument("matrix output is null");
    for (int row = 0; row < matrix.rows(); ++row)
        for (int col = 0; col < matrix.cols(); ++col)
            output[static_cast<size_t>(row) * matrix.cols() + col] = matrix(row, col);
}

std::vector<std::vector<double>> shock_matrix(const double* flat,
                                               size_t paths, size_t steps) {
    if (!flat || paths == 0 || steps == 0 ||
        paths > std::numeric_limits<size_t>::max() / steps)
        throw std::invalid_argument("normal shock paths must be non-empty");
    std::vector<std::vector<double>> result(paths, std::vector<double>(steps));
    for (size_t path = 0; path < paths; ++path)
        std::copy_n(flat + path * steps, steps, result[path].begin());
    return result;
}

void write_estimate(const finance::MonteCarloEstimate& source,
                    coolbox_estimate* output) {
    if (!output) throw std::invalid_argument("Monte Carlo estimate output is null");
    output->price = source.price;
    output->standard_error = source.standard_error;
    output->confidence_interval_low = source.confidence_interval_low;
    output->confidence_interval_high = source.confidence_interval_high;
    output->sample_count = source.sample_count;
}

} // namespace

extern "C" {

void* coolbox_optimizer_create(int kind, const double* config, size_t count,
                               char* error, size_t capacity) {
    try {
        if (count && !config) throw std::invalid_argument("optimizer config is null");
        if (config && !std::all_of(config, config + count,
                                   [](double value) { return std::isfinite(value); })) {
            throw std::invalid_argument("optimizer config values must be finite");
        }
        OptimizerPtr instance;
        switch (kind) {
        case 0: {
            opt::GeneticSearch::Config c;
            c.dimensions = config_int(config, count, 0, c.dimensions);
            c.population_size = config_int(config, count, 1, c.population_size);
            c.generations = config_int(config, count, 2, c.generations);
            c.tournament_size = config_int(config, count, 3, c.tournament_size);
            if (count > 4) c.crossover_rate = config[4];
            if (count > 5) c.mutation_rate = config[5];
            if (count > 6) c.mutation_sigma = config[6];
            if (count > 7) c.lower_bound = config[7];
            if (count > 8) c.upper_bound = config[8];
            c.seed = config_uint(config, count, 9, c.seed);
            instance = opt::create_optimizer(c);
            break;
        }
        case 1: {
            opt::SimulatedAnnealing::Config c;
            c.dimensions = config_int(config, count, 0, c.dimensions);
            c.iterations_per_temperature = config_int(config, count, 1, c.iterations_per_temperature);
            if (count > 2) c.initial_temperature = config[2];
            if (count > 3) c.cooling_rate = config[3];
            if (count > 4) c.minimum_temperature = config[4];
            if (count > 5) c.step_sigma = config[5];
            if (count > 6) c.lower_bound = config[6];
            if (count > 7) c.upper_bound = config[7];
            c.seed = config_uint(config, count, 8, c.seed);
            instance = opt::create_optimizer(c);
            break;
        }
        case 2: {
            opt::HillClimbing::Config c;
            c.dimensions = config_int(config, count, 0, c.dimensions);
            c.max_iterations = config_int(config, count, 1, c.max_iterations);
            c.neighbours_per_iteration = config_int(config, count, 2, c.neighbours_per_iteration);
            c.max_restarts = config_int(config, count, 3, c.max_restarts);
            if (count > 4) c.step_sigma = config[4];
            if (count > 5) c.lower_bound = config[5];
            if (count > 6) c.upper_bound = config[6];
            c.seed = config_uint(config, count, 7, c.seed);
            instance = opt::create_optimizer(c);
            break;
        }
        case 3: {
            opt::QuantumControlBayesianOptimizer::Config c;
            if (count > 0) {
                const int policy = config_int(config, count, 0, static_cast<int>(c.policy));
                if (policy < 0 || policy > 3) throw std::invalid_argument("unknown quantum search policy");
                c.policy = static_cast<opt::QuantumControlSearchPolicy>(policy);
            }
            if (count > 1) c.bounds.frequency_center = config[1];
            if (count > 2) c.bounds.initial_frequency_ratio = config[2];
            if (count > 3) c.bounds.current_frequency_ratio = config[3];
            if (count > 4) c.bounds.amplitude_min = config[4];
            if (count > 5) c.bounds.amplitude_max = config[5];
            c.initial_samples = config_int(config, count, 6, c.initial_samples);
            c.iterations = config_int(config, count, 7, c.iterations);
            c.acquisition_candidates = config_int(config, count, 8, c.acquisition_candidates);
            if (count > 9) c.exploration_beta = config[9];
            if (count > 10) c.length_scale_frequency = config[10];
            if (count > 11) c.length_scale_amplitude = config[11];
            if (count > 12) c.observation_noise = config[12];
            if (count > 13) c.crop_min_ratio = config[13];
            if (count > 14) c.expansion_rate = config[14];
            c.seed = config_uint(config, count, 15, c.seed);
            instance = opt::create_optimizer(c);
            break;
        }
        default:
            throw std::invalid_argument("unknown optimizer type");
        }
        return new OptimizerHandle(std::move(instance));
    } catch (const std::exception& exception) {
        set_error(error, capacity, exception.what());
    } catch (...) {
        set_error(error, capacity, "unknown native exception");
    }
    return nullptr;
}

void coolbox_optimizer_free(void* handle) {
    delete static_cast<OptimizerHandle*>(handle);
}

int coolbox_optimizer_run(void* handle, coolbox_objective_fn objective,
                          uintptr_t user_data, const double* initial, size_t count,
                          char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!objective) throw std::invalid_argument("objective callback is null");
        if (count && !initial) throw std::invalid_argument("initial solution is null");
        auto& value = optimizer(handle);
        const std::vector<double> initial_state = count
            ? std::vector<double>(initial, initial + count) : std::vector<double>{};
        const auto callback = [objective, user_data](const std::vector<double>& point) {
            return objective(user_data, point.data(), point.size());
        };
        value.solution = value.value->optimize(callback, initial_state);
    });
}

size_t coolbox_optimizer_solution_size(void* handle) {
    return handle ? optimizer(handle).solution.size() : 0;
}

int coolbox_optimizer_copy_solution(void* handle, double* output, size_t count,
                                    char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        auto& value = optimizer(handle);
        if (!output || count < value.solution.size())
            throw std::invalid_argument("optimizer solution output buffer is too small");
        std::copy(value.solution.begin(), value.solution.end(), output);
    });
}

double coolbox_optimizer_best_score(void* handle) {
    return handle ? optimizer(handle).value->best_score()
                  : std::numeric_limits<double>::quiet_NaN();
}

double coolbox_optimizer_best_probability(void* handle) {
    if (!handle) return std::numeric_limits<double>::quiet_NaN();
    auto* quantum = dynamic_cast<opt::QuantumControlBayesianOptimizer*>(
        optimizer(handle).value.get());
    return quantum ? quantum->best_probability()
                   : std::numeric_limits<double>::quiet_NaN();
}

int coolbox_optimizer_quantum_bounds(void* handle, double* output, size_t count,
                                     char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        auto* quantum = dynamic_cast<opt::QuantumControlBayesianOptimizer*>(
            optimizer(handle).value.get());
        if (!quantum) throw std::invalid_argument("optimizer is not quantum-control Bayesian optimization");
        if (!output || count < 5) throw std::invalid_argument("quantum bounds output needs five values");
        const auto bounds = quantum->current_bounds();
        output[0] = bounds.frequency_center;
        output[1] = bounds.initial_frequency_ratio;
        output[2] = bounds.current_frequency_ratio;
        output[3] = bounds.amplitude_min;
        output[4] = bounds.amplitude_max;
    });
}

int coolbox_optimizer_warp_point(void* handle, const double* input, size_t count,
                                 double* output, int unwarp, char* error,
                                 size_t capacity) {
    return invoke(error, capacity, [&] {
        auto* quantum = dynamic_cast<opt::QuantumControlBayesianOptimizer*>(
            optimizer(handle).value.get());
        if (!quantum) throw std::invalid_argument("optimizer is not quantum-control Bayesian optimization");
        if (!input || !output || count == 0) throw std::invalid_argument("point input/output must not be empty");
        const std::vector<double> point(input, input + count);
        const auto transformed = unwarp ? quantum->unwarp_frequency_point(point)
                                        : quantum->warp_frequency_point(point);
        if (transformed.size() != count) throw std::runtime_error("transformed point has unexpected dimensions");
        std::copy(transformed.begin(), transformed.end(), output);
    });
}

int coolbox_math_heat(int dimensions, int stochastic, const double* input,
                      size_t nx, size_t ny, size_t nz, double alpha, double sigma,
                      double dx, double dy, double dz, double dt, int steps,
                      unsigned int seed, double* output, char* error,
                      size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!input || !output || steps < 0 ||
            !(dx > 0) || !(dy > 0) || !(dz > 0) || !(dt > 0))
            throw std::invalid_argument("heat solver requires non-empty 3+ point dimensions, positive spacings and dt, and non-negative steps");
        if (nx > static_cast<size_t>(std::numeric_limits<int>::max()) ||
            ny > static_cast<size_t>(std::numeric_limits<int>::max()) ||
            nz > static_cast<size_t>(std::numeric_limits<int>::max()))
            throw std::invalid_argument("heat grid dimensions exceed native limits");
        if (dimensions < 1 || dimensions > 3)
            throw std::invalid_argument("heat solver supports dimensions 1 through 3");
        if (nx < 3 || (dimensions >= 2 && ny < 3) || (dimensions == 3 && nz < 3))
            throw std::invalid_argument("each active heat grid dimension must have at least three points");
        if (dimensions < 3 && nz != 1) throw std::invalid_argument("nz must be one for 1D/2D heat solvers");
        if (dimensions < 2 && ny != 1) throw std::invalid_argument("ny must be one for 1D heat solvers");
        if (nx > std::numeric_limits<size_t>::max() / ny ||
            nx * ny > std::numeric_limits<size_t>::max() / nz) {
            throw std::invalid_argument("heat grid dimensions overflow native size");
        }
        const size_t count = nx * ny * nz;
        std::vector<double> flat(input, input + count);
        auto u = matrix_from_flat(flat.data(), nx, ny * nz);
        mytrix::DenseMatrix result;
        if (dimensions == 1) {
            if (stochastic) {
                std::mt19937 rng(seed);
                result = cool_car::MATH::SPDESolver::solve_stochastic_heat_eq(
                    u, alpha, sigma, dx, dt, steps, rng);
            } else {
                result = cool_car::MATH::PDESolver::solve_heat_eq(u, alpha, dx, dt, steps);
            }
        } else if (dimensions == 2) {
            if (stochastic) {
                std::mt19937 rng(seed);
                result = cool_car::MATH::SPDESolver::solve_stochastic_heat_eq_2d(
                    u, alpha, sigma, dx, dy, dt, steps, rng);
            } else {
                result = cool_car::MATH::PDESolver::solve_heat_eq_2d(
                    u, alpha, dx, dy, dt, steps);
            }
        } else {
            std::vector<mytrix::DenseMatrix> layers;
            layers.reserve(nx);
            for (size_t i = 0; i < nx; ++i) {
                std::vector<double> layer(ny * nz);
                std::copy_n(input + i * ny * nz, ny * nz, layer.begin());
                layers.emplace_back(layer, static_cast<int>(ny), static_cast<int>(nz));
            }
            std::vector<mytrix::DenseMatrix> solved;
            if (stochastic) {
                std::mt19937 rng(seed);
                solved = cool_car::MATH::SPDESolver::solve_stochastic_heat_eq_3d(
                    layers, alpha, sigma, dx, dy, dz, dt, steps, rng);
            } else {
                solved = cool_car::MATH::PDESolver::solve_heat_eq_3d(
                    layers, alpha, dx, dy, dz, dt, steps);
            }
            for (size_t i = 0; i < nx; ++i)
                copy_matrix(solved[i], output + i * ny * nz);
            return;
        }
        copy_matrix(result, output);
    });
}

double coolbox_finance_formula(int operation, const double* p, size_t count,
                               char* error, size_t capacity) {
    return invoke_double(error, capacity, [&] {
        if (count && !p) throw std::invalid_argument("finance input is null");
        const auto at = [p, count](size_t index) {
            if (index >= count) throw std::invalid_argument("not enough finance arguments");
            return p[index];
        };
        switch (operation) {
        case 0: return finance::standard_normal_pdf(at(0));
        case 1: return finance::standard_normal_cdf(at(0));
        case 2: return finance::black_scholes_d1(at(0), at(1), at(2), at(3), at(4));
        case 3: return finance::black_scholes_d2(at(0), at(1), at(2), at(3), at(4));
        case 4: return finance::black_scholes_call_price(at(0), at(1), at(2), at(3), at(4));
        case 5: return finance::black_scholes_put_price(at(0), at(1), at(2), at(3), at(4));
        case 6: return finance::black_scholes_call_delta(at(0), at(1), at(2), at(3), at(4));
        case 7: return finance::black_scholes_put_delta(at(0), at(1), at(2), at(3), at(4));
        case 8: return finance::black_scholes_gamma(at(0), at(1), at(2), at(3), at(4));
        case 9: return finance::black_scholes_vega(at(0), at(1), at(2), at(3), at(4));
        case 10: return finance::black_scholes_call_theta(at(0), at(1), at(2), at(3), at(4));
        case 11: return finance::black_scholes_put_theta(at(0), at(1), at(2), at(3), at(4));
        case 12: return finance::black_scholes_call_rho(at(0), at(1), at(2), at(3), at(4));
        case 13: return finance::black_scholes_put_rho(at(0), at(1), at(2), at(3), at(4));
        case 14: return finance::black_scholes_digital_call_price(at(0), at(1), at(2), at(3), at(4), count > 5 ? at(5) : 1.0);
        case 15: return finance::black_scholes_digital_put_price(at(0), at(1), at(2), at(3), at(4), count > 5 ? at(5) : 1.0);
        case 16: return finance::geometric_asian_call_price(at(0), at(1), at(2), at(3), at(4));
        case 17: return finance::geometric_asian_put_price(at(0), at(1), at(2), at(3), at(4));
        case 18: return finance::black_scholes_down_and_in_call_price(at(0), at(1), at(2), at(3), at(4), at(5));
        case 19: return finance::black_scholes_down_and_out_call_price(at(0), at(1), at(2), at(3), at(4), at(5));
        case 20: return finance::black_scholes_up_and_in_call_price(at(0), at(1), at(2), at(3), at(4), at(5));
        case 21: return finance::black_scholes_up_and_out_call_price(at(0), at(1), at(2), at(3), at(4), at(5));
        case 22: return finance::black_scholes_down_and_in_put_price(at(0), at(1), at(2), at(3), at(4), at(5));
        case 23: return finance::black_scholes_down_and_out_put_price(at(0), at(1), at(2), at(3), at(4), at(5));
        default: throw std::invalid_argument("unknown finance formula");
        }
    });
}

int coolbox_finance_path(int geometric, double initial_value, double drift,
                         double volatility, const double* times, size_t count,
                         const double* shocks, double* output, char* error,
                         size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!times || !shocks || !output || count == 0)
            throw std::invalid_argument("time steps, shocks, and output must be non-empty");
        const std::vector<double> steps(times, times + count);
        const std::vector<double> normals(shocks, shocks + count);
        const auto result = geometric
            ? finance::geometric_brownian_motion_path(initial_value, drift, volatility, steps, normals)
            : finance::brownian_motion_path(steps, normals, initial_value);
        std::copy(result.begin(), result.end(), output);
    });
}

int coolbox_finance_ito(int variance, const double* values, const double* increments,
                        size_t count, double* output, char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!values || !increments || !output || count == 0)
            throw std::invalid_argument("integrand, increments, and output must be non-empty");
        const std::vector<double> integrand(values, values + count);
        const std::vector<double> steps(increments, increments + count);
        *output = variance ? finance::ito_isometry_variance(integrand, steps)
                           : finance::ito_integral_left_point(integrand, steps);
    });
}

int coolbox_finance_monte_carlo(int operation, double spot, double strike,
                                double barrier, double rate, double volatility,
                                const double* time_steps, size_t step_count,
                                const double* shocks, size_t path_count,
                                double confidence, double relative_bump,
                                double time_bump, coolbox_estimate* output,
                                char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!time_steps || !shocks || !output || step_count == 0 || path_count == 0)
            throw std::invalid_argument("Monte Carlo steps, shocks, and output must be non-empty");
        std::vector<double> times(time_steps, time_steps + step_count);
        auto paths = shock_matrix(shocks, path_count, step_count);
        finance::MonteCarloEstimate estimate;
        switch (operation) {
        case 0: estimate = finance::monte_carlo_european_call_estimate(spot, strike, rate, volatility, times, paths, confidence); break;
        case 1: estimate = finance::monte_carlo_european_put_estimate(spot, strike, rate, volatility, times, paths, confidence); break;
        case 2: estimate = finance::monte_carlo_arithmetic_asian_call_estimate(spot, strike, rate, volatility, times, paths, confidence); break;
        case 3: estimate = finance::monte_carlo_down_and_out_call_estimate(spot, strike, barrier, rate, volatility, times, paths, confidence); break;
        case 4: estimate = finance::monte_carlo_up_and_out_call_estimate(spot, strike, barrier, rate, volatility, times, paths, confidence); break;
        case 5: estimate = finance::monte_carlo_down_and_out_put_estimate(spot, strike, barrier, rate, volatility, times, paths, confidence); break;
        default: throw std::invalid_argument("unknown Monte Carlo contract");
        }
        if (operation <= 5) {
            write_estimate(estimate, output);
            return;
        }
    });
}

int coolbox_finance_monte_carlo_greeks(int operation, double spot, double strike,
                                       double barrier, double rate, double volatility,
                                       const double* time_steps, size_t step_count,
                                       const double* shocks, size_t path_count,
                                       double confidence, double relative_bump,
                                       double time_bump, coolbox_greeks* output,
                                       char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!time_steps || !shocks || !output || step_count == 0 || path_count == 0)
            throw std::invalid_argument("Monte Carlo steps, shocks, and Greek output must be non-empty");
        std::vector<double> times(time_steps, time_steps + step_count);
        auto paths = shock_matrix(shocks, path_count, step_count);
        finance::MonteCarloGreeks result;
        switch (operation) {
        case 0: result = finance::monte_carlo_european_call_greeks_fd(spot, strike, rate, volatility, times, paths, confidence, relative_bump, time_bump); break;
        case 1: result = finance::monte_carlo_arithmetic_asian_call_greeks_fd(spot, strike, rate, volatility, times, paths, confidence, relative_bump, time_bump); break;
        case 2: result = finance::monte_carlo_down_and_out_call_greeks_fd(spot, strike, barrier, rate, volatility, times, paths, confidence, relative_bump, time_bump); break;
        case 3: result = finance::monte_carlo_up_and_out_call_greeks_fd(spot, strike, barrier, rate, volatility, times, paths, confidence, relative_bump, time_bump); break;
        case 4: result = finance::monte_carlo_down_and_out_put_greeks_fd(spot, strike, barrier, rate, volatility, times, paths, confidence, relative_bump, time_bump); break;
        default: throw std::invalid_argument("unknown Monte Carlo Greek contract");
        }
        write_estimate(result.delta, &output->delta);
        write_estimate(result.gamma, &output->gamma);
        write_estimate(result.vega, &output->vega);
        write_estimate(result.theta, &output->theta);
        write_estimate(result.rho, &output->rho);
    });
}

int coolbox_finance_monte_carlo_generic(
    double spot, double rate, double volatility, const double* time_steps,
    size_t step_count, const double* shocks, size_t path_count,
    double confidence, coolbox_payoff_fn payoff, uintptr_t user_data,
    coolbox_estimate* output, char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!time_steps || !shocks || !payoff || !output ||
            step_count == 0 || path_count == 0) {
            throw std::invalid_argument("Monte Carlo steps, shocks, callback, and output must be non-empty");
        }
        std::vector<double> times(time_steps, time_steps + step_count);
        auto paths = shock_matrix(shocks, path_count, step_count);
        const auto native_payoff = [payoff, user_data](const std::vector<double>& path) {
            double value = 0.0;
            if (!payoff(user_data, path.data(), path.size(), &value)) {
                throw std::runtime_error("custom Monte Carlo payoff callback failed");
            }
            if (!std::isfinite(value)) {
                throw std::runtime_error("custom Monte Carlo payoff must return a finite value");
            }
            return value;
        };
        write_estimate(finance::monte_carlo_estimate_generic(
                           spot, rate, volatility, times, paths, native_payoff,
                           confidence),
                       output);
    });
}

int coolbox_finance_monte_carlo_greeks_generic(
    double spot, double strike, double rate, double volatility,
    const double* time_steps, size_t step_count, const double* shocks,
    size_t path_count, double confidence, double relative_bump,
    double time_bump, coolbox_payoff_fn payoff, uintptr_t user_data,
    coolbox_greeks* output, char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!time_steps || !shocks || !payoff || !output ||
            step_count == 0 || path_count == 0) {
            throw std::invalid_argument("Monte Carlo steps, shocks, callback, and Greek output must be non-empty");
        }
        std::vector<double> times(time_steps, time_steps + step_count);
        auto paths = shock_matrix(shocks, path_count, step_count);
        const auto native_payoff = [payoff, user_data](const std::vector<double>& path) {
            double value = 0.0;
            if (!payoff(user_data, path.data(), path.size(), &value)) {
                throw std::runtime_error("custom Monte Carlo payoff callback failed");
            }
            if (!std::isfinite(value)) {
                throw std::runtime_error("custom Monte Carlo payoff must return a finite value");
            }
            return value;
        };
        const auto result = finance::monte_carlo_greeks_fd_generic(
            spot, strike, rate, volatility, times, paths, native_payoff,
            confidence, relative_bump, time_bump);
        write_estimate(result.delta, &output->delta);
        write_estimate(result.gamma, &output->gamma);
        write_estimate(result.vega, &output->vega);
        write_estimate(result.theta, &output->theta);
        write_estimate(result.rho, &output->rho);
    });
}

int coolbox_owa_naive_average(const double* parameters, size_t models,
                              size_t count, double* output, char* error,
                              size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!parameters || !output || models == 0 || count == 0)
            throw std::invalid_argument("parameter models and output must be non-empty");
        std::vector<std::vector<double>> local(models, std::vector<double>(count));
        for (size_t i = 0; i < models; ++i)
            std::copy_n(parameters + i * count, count, local[i].begin());
        const auto result = opt::naive_average_parameters(local);
        std::copy(result.begin(), result.end(), output);
    });
}

int coolbox_owa_linear_regression(const double* parameters, size_t models,
                                  size_t parameter_count, const double* features,
                                  size_t rows, size_t columns, const double* targets,
                                  size_t target_count, int iterations, double rate,
                                  double regularization, int project,
                                  double* merged, double* weights, double* loss,
                                  char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!parameters || !features || !targets || !merged || !weights || !loss ||
            models == 0 || parameter_count == 0 || rows == 0 || columns == 0 ||
            target_count != rows || parameter_count != columns)
            throw std::invalid_argument("OWA requires non-empty compatible model parameters and validation data");
        if (iterations < 0 || !std::isfinite(rate) || !std::isfinite(regularization))
            throw std::invalid_argument("OWA configuration must use non-negative iterations and finite rates");
        if (models > std::numeric_limits<size_t>::max() / parameter_count ||
            rows > std::numeric_limits<size_t>::max() / columns)
            throw std::invalid_argument("OWA input dimensions overflow native size");
        opt::OwaConfig config;
        config.merge_iterations = iterations;
        config.learning_rate = rate;
        config.l2_regularization = regularization;
        config.project_to_simplex = project != 0;
        std::vector<std::vector<double>> local(models, std::vector<double>(parameter_count));
        for (size_t i = 0; i < models; ++i)
            std::copy_n(parameters + i * parameter_count, parameter_count, local[i].begin());
        std::vector<std::vector<double>> x(rows, std::vector<double>(columns));
        for (size_t i = 0; i < rows; ++i)
            std::copy_n(features + i * columns, columns, x[i].begin());
        const std::vector<double> y(targets, targets + target_count);
        const auto result = opt::optimal_weighted_average_linear_regression(local, x, y, config);
        std::copy(result.merged_parameters.begin(), result.merged_parameters.end(), merged);
        std::copy(result.model_weights.begin(), result.model_weights.end(), weights);
        *loss = result.validation_loss;
    });
}

int coolbox_owa_cross_validation_linear_regression(
    const double* parameters, size_t models, size_t parameter_count,
    const double* features, const size_t* row_offsets, const double* targets,
    const size_t* target_offsets, size_t folds, size_t feature_count,
    int iterations, double rate, double regularization, int project,
    double* fold_losses, double* mean_loss, char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!parameters || !features || !row_offsets || !targets ||
            !target_offsets || !fold_losses || !mean_loss ||
            models < 2 || parameter_count == 0 || folds != models ||
            feature_count == 0 || parameter_count != feature_count ||
            row_offsets[0] != 0 || target_offsets[0] != 0) {
            throw std::invalid_argument("OWA cross-validation inputs must be non-empty and have one fold per model");
        }
        if (iterations < 0 || !std::isfinite(rate) ||
            !std::isfinite(regularization)) {
            throw std::invalid_argument("OWA configuration must use non-negative iterations and finite rates");
        }
        if (models > std::numeric_limits<size_t>::max() / parameter_count) {
            throw std::invalid_argument("OWA model dimensions overflow native size");
        }
        std::vector<std::vector<double>> local_parameters(
            models, std::vector<double>(parameter_count));
        for (size_t model = 0; model < models; ++model) {
            std::copy_n(parameters + model * parameter_count, parameter_count,
                        local_parameters[model].begin());
        }
        std::vector<std::vector<std::vector<double>>> fold_features(folds);
        std::vector<std::vector<double>> fold_targets(folds);
        for (size_t fold = 0; fold < folds; ++fold) {
            if (row_offsets[fold + 1] < row_offsets[fold] ||
                target_offsets[fold + 1] < target_offsets[fold]) {
                throw std::invalid_argument("OWA fold offsets must be non-decreasing");
            }
            const size_t rows = row_offsets[fold + 1] - row_offsets[fold];
            const size_t target_count = target_offsets[fold + 1] - target_offsets[fold];
            if (rows == 0 || rows != target_count ||
                row_offsets[fold + 1] >
                    std::numeric_limits<size_t>::max() / feature_count ||
                rows > std::numeric_limits<size_t>::max() / feature_count) {
                throw std::invalid_argument("each OWA fold needs aligned non-empty features and targets");
            }
            fold_features[fold].resize(rows, std::vector<double>(feature_count));
            for (size_t row = 0; row < rows; ++row) {
                const size_t begin = (row_offsets[fold] + row) * feature_count;
                std::copy_n(features + begin, feature_count,
                            fold_features[fold][row].begin());
            }
            fold_targets[fold].assign(targets + target_offsets[fold],
                                      targets + target_offsets[fold + 1]);
        }
        opt::OwaConfig config;
        config.merge_iterations = iterations;
        config.learning_rate = rate;
        config.l2_regularization = regularization;
        config.project_to_simplex = project != 0;
        const auto result = opt::fast_owa_cross_validation_linear_regression(
            local_parameters, fold_features, fold_targets, config);
        if (result.fold_losses.size() != folds) {
            throw std::runtime_error("OWA cross-validation returned an unexpected fold count");
        }
        std::copy(result.fold_losses.begin(), result.fold_losses.end(), fold_losses);
        *mean_loss = result.mean_loss;
    });
}

int coolbox_owa_nonlinear_layers(
    const double* local_layers, size_t models, size_t layers,
    const size_t* shapes, const size_t* shape_offsets,
    const size_t* layer_sizes, const size_t* value_offsets,
    const size_t* layer_dimensions, int iterations, double rate,
    double regularization, int project, coolbox_nowa_loss_fn loss_fn,
    uintptr_t user_data, double* merged_layers, double* layer_weights,
    double* validation_loss, char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!local_layers || models == 0 || layers == 0 || !shapes ||
            !shape_offsets || !layer_sizes || !value_offsets ||
            !layer_dimensions || !loss_fn || !merged_layers ||
            !layer_weights || !validation_loss ||
            shape_offsets[0] != 0 || value_offsets[0] != 0) {
            throw std::invalid_argument("NOWA requires non-empty model layers, shapes, and callbacks");
        }
        if (iterations < 0 || !std::isfinite(rate) ||
            !std::isfinite(regularization)) {
            throw std::invalid_argument("OWA configuration must use non-negative iterations and finite rates");
        }
        std::vector<std::vector<ml::deep_learning::Tensor>> local_model_layers(
            models);
        size_t values_per_model = 0;
        size_t shape_dimension_count = 0;
        for (size_t layer = 0; layer < layers; ++layer) {
            if (shape_offsets[layer + 1] < shape_offsets[layer] ||
                value_offsets[layer + 1] < value_offsets[layer] ||
                shape_offsets[layer] != shape_dimension_count ||
                value_offsets[layer] != values_per_model ||
                layer_dimensions[layer] == 0 || layer_sizes[layer] == 0 ||
                shape_offsets[layer + 1] - shape_offsets[layer] !=
                    layer_dimensions[layer] ||
                layer_sizes[layer] != value_offsets[layer + 1] - value_offsets[layer]) {
                throw std::invalid_argument("NOWA layer shape metadata is invalid");
            }
            size_t shape_size = 1;
            std::vector<size_t> shape;
            shape.reserve(layer_dimensions[layer]);
            for (size_t dimension = shape_offsets[layer];
                 dimension < shape_offsets[layer + 1]; ++dimension) {
                const size_t extent = shapes[dimension];
                if (extent == 0 || shape_size > std::numeric_limits<size_t>::max() / extent) {
                    throw std::invalid_argument("NOWA tensor shape is empty or overflows");
                }
                shape_size *= extent;
                shape.push_back(extent);
            }
            if (shape.size() != layer_dimensions[layer] ||
                shape_size != layer_sizes[layer] ||
                values_per_model > std::numeric_limits<size_t>::max() - layer_sizes[layer]) {
                throw std::invalid_argument("NOWA tensor shape metadata does not match its data");
            }
            values_per_model += layer_sizes[layer];
            if (shape_dimension_count >
                std::numeric_limits<size_t>::max() - layer_dimensions[layer]) {
                throw std::invalid_argument("NOWA shape dimensions overflow native size");
            }
            shape_dimension_count += layer_dimensions[layer];
        }
        if (models > std::numeric_limits<size_t>::max() / values_per_model) {
            throw std::invalid_argument("NOWA input dimensions overflow native size");
        }
        for (size_t model = 0; model < models; ++model) {
            local_model_layers[model].reserve(layers);
            for (size_t layer = 0; layer < layers; ++layer) {
                std::vector<size_t> shape(
                    shapes + shape_offsets[layer], shapes + shape_offsets[layer + 1]);
                const size_t offset = model * values_per_model + value_offsets[layer];
                std::vector<double> values(
                    local_layers + offset, local_layers + offset + layer_sizes[layer]);
                local_model_layers[model].emplace_back(shape, values);
            }
        }
        const auto native_loss = [loss_fn, user_data, layers, layer_sizes,
                                  value_offsets](const std::vector<ml::deep_learning::Tensor>& tensors) {
            if (tensors.size() != layers) {
                throw std::runtime_error("NOWA validation callback received an unexpected layer count");
            }
            std::vector<double> flat(value_offsets[layers]);
            for (size_t layer = 0; layer < layers; ++layer) {
                if (tensors[layer].size() != layer_sizes[layer]) {
                    throw std::runtime_error("NOWA validation callback received an unexpected tensor shape");
                }
                std::copy(tensors[layer].data().begin(), tensors[layer].data().end(),
                          flat.begin() + value_offsets[layer]);
            }
            double result = 0.0;
            if (!loss_fn(user_data, flat.data(), layer_sizes, layers, &result)) {
                throw std::runtime_error("NOWA validation-loss callback failed");
            }
            if (!std::isfinite(result)) {
                throw std::runtime_error("NOWA validation loss must be finite");
            }
            return result;
        };
        opt::OwaConfig config;
        config.merge_iterations = iterations;
        config.learning_rate = rate;
        config.l2_regularization = regularization;
        config.project_to_simplex = project != 0;
        const auto result = opt::nonlinear_optimal_weighted_average_layers(
            local_model_layers, native_loss, config);
        size_t merged_offset = 0;
        for (const auto& tensor : result.merged_layers) {
            std::copy(tensor.data().begin(), tensor.data().end(),
                      merged_layers + merged_offset);
            merged_offset += tensor.size();
        }
        for (size_t layer = 0; layer < layers; ++layer) {
            if (result.layer_weights[layer].size() != models) {
                throw std::runtime_error("NOWA returned an unexpected model weight count");
            }
            std::copy(result.layer_weights[layer].begin(),
                      result.layer_weights[layer].end(),
                      layer_weights + layer * models);
        }
        *validation_loss = result.validation_loss;
    });
}

int coolbox_owa_weighted_tensor_average(
    const double* local_tensors, size_t models, const size_t* shape,
    size_t shape_dimensions, const double* weights, size_t tensor_size,
    double* merged_tensor, char* error, size_t capacity) {
    return invoke(error, capacity, [&] {
        if (!local_tensors || models == 0 || !shape || shape_dimensions == 0 ||
            !weights || tensor_size == 0 || !merged_tensor) {
            throw std::invalid_argument("weighted tensor averaging requires non-empty tensors, weights, and shape");
        }
        if (models > std::numeric_limits<size_t>::max() / tensor_size) {
            throw std::invalid_argument("weighted tensor dimensions overflow native size");
        }
        size_t expected_size = 1;
        std::vector<size_t> tensor_shape(shape, shape + shape_dimensions);
        for (const size_t extent : tensor_shape) {
            if (extent == 0 || expected_size > std::numeric_limits<size_t>::max() / extent) {
                throw std::invalid_argument("weighted tensor shape is empty or overflows");
            }
            expected_size *= extent;
        }
        if (expected_size != tensor_size) {
            throw std::invalid_argument("weighted tensor data does not match its shape");
        }
        std::vector<ml::deep_learning::Tensor> tensors;
        tensors.reserve(models);
        for (size_t model = 0; model < models; ++model) {
            const size_t offset = model * tensor_size;
            std::vector<double> values(
                local_tensors + offset, local_tensors + offset + tensor_size);
            tensors.emplace_back(tensor_shape, values);
        }
        const std::vector<double> model_weights(weights, weights + models);
        const auto result = opt::weighted_tensor_average(tensors, model_weights);
        std::copy(result.data().begin(), result.data().end(), merged_tensor);
    });
}

} // extern "C"
