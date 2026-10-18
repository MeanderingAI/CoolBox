#include "../go_bindings/opt_math_finance_bridge.cc"
#include "../go_bindings/opt_math_finance_bridge_sources.cc"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace py = pybind11;

namespace {

constexpr std::size_t error_capacity = 1024;

enum class PyFinanceFormula {
    StandardNormalPdf = 0, StandardNormalCdf, BlackScholesD1, BlackScholesD2,
    BlackScholesCallPrice, BlackScholesPutPrice, BlackScholesCallDelta,
    BlackScholesPutDelta, BlackScholesGamma, BlackScholesVega,
    BlackScholesCallTheta, BlackScholesPutTheta, BlackScholesCallRho,
    BlackScholesPutRho, DigitalCallPrice, DigitalPutPrice,
    GeometricAsianCallPrice, GeometricAsianPutPrice, DownAndInCallPrice,
    DownAndOutCallPrice, UpAndInCallPrice, UpAndOutCallPrice,
    DownAndInPutPrice, DownAndOutPutPrice
};

enum class PyMonteCarloContract {
    EuropeanCall = 0, EuropeanPut, ArithmeticAsianCall, DownAndOutCall,
    UpAndOutCall, DownAndOutPut
};

enum class PyMonteCarloGreekContract {
    EuropeanCall = 0, ArithmeticAsianCall, DownAndOutCall, UpAndOutCall,
    DownAndOutPut
};

std::string error_message(const char* error, const char* fallback) {
    return error[0] ? std::string(error) : std::string(fallback);
}

struct PythonObjective {
    py::function function;
    std::string error;
};

double python_objective(std::uintptr_t user_data, const double* values, std::size_t count) {
    auto& context = *reinterpret_cast<PythonObjective*>(user_data);
    try {
        const std::vector<double> point(values, values + count);
        return context.function(point).cast<double>();
    } catch (const py::error_already_set& exception) {
        context.error = exception.what();
    } catch (const std::exception& exception) {
        context.error = exception.what();
    } catch (...) {
        context.error = "unknown Python objective exception";
    }
    return std::numeric_limits<double>::quiet_NaN();
}

class PyOptimizer {
public:
    PyOptimizer(int kind, const std::vector<double>& config) {
        char error[error_capacity]{};
        handle_ = coolbox_optimizer_create(kind, config.data(), config.size(),
                                           error, sizeof(error));
        if (!handle_) throw std::runtime_error(error_message(error, "failed to create optimizer"));
        kind_ = kind;
    }

    ~PyOptimizer() { coolbox_optimizer_free(handle_); }

    std::vector<double> optimize(py::function objective,
                                 const std::vector<double>& initial) {
        PythonObjective context{std::move(objective), {}};
        char error[error_capacity]{};
        if (!coolbox_optimizer_run(handle_, python_objective,
                                   reinterpret_cast<std::uintptr_t>(&context),
                                   initial.data(), initial.size(),
                                   error, sizeof(error))) {
            throw std::runtime_error(error_message(error, "optimization failed"));
        }
        if (!context.error.empty()) throw std::runtime_error(context.error);
        std::vector<double> result(coolbox_optimizer_solution_size(handle_));
        if (!coolbox_optimizer_copy_solution(handle_, result.data(), result.size(),
                                             error, sizeof(error))) {
            throw std::runtime_error(error_message(error, "failed to read optimizer solution"));
        }
        return result;
    }

    double best_score() const { return coolbox_optimizer_best_score(handle_); }

    double best_probability() const {
        if (kind_ != 3) throw std::invalid_argument("best probability is available only for quantum-control optimization");
        return coolbox_optimizer_best_probability(handle_);
    }

    std::vector<double> quantum_bounds() const {
        double result[5]{};
        char error[error_capacity]{};
        if (!coolbox_optimizer_quantum_bounds(handle_, result, 5, error, sizeof(error)))
            throw std::runtime_error(error_message(error, "failed to get quantum-control bounds"));
        return {result, result + 5};
    }

    std::vector<double> transform_frequency_point(
        const std::vector<double>& point, bool unwarp) const {
        std::vector<double> result(point.size());
        char error[error_capacity]{};
        if (!coolbox_optimizer_warp_point(handle_, point.data(), point.size(),
                                          result.data(), unwarp ? 1 : 0,
                                          error, sizeof(error))) {
            throw std::runtime_error(error_message(error, "failed to transform frequency point"));
        }
        return result;
    }

private:
    void* handle_ = nullptr;
    int kind_ = 0;
};

void bind_opt(py::module_& parent) {
    auto module = parent.def_submodule("opt", "Optimization and distributed model merging");
    py::enum_<opt::OptimizationType>(module, "OptimizerType")
        .value("GENETIC_SEARCH", opt::OptimizationType::GeneticSearch)
        .value("SIMULATED_ANNEALING", opt::OptimizationType::SimulatedAnnealing)
        .value("HILL_CLIMBING", opt::OptimizationType::HillClimbing)
        .value("QUANTUM_CONTROL_BAYESIAN", opt::OptimizationType::QuantumControlBayesianOptimization);
    py::class_<PyOptimizer>(module, "Optimizer")
        .def(py::init([](opt::OptimizationType kind, const std::vector<double>& config) {
                 return std::make_unique<PyOptimizer>(static_cast<int>(kind), config);
             }),
             py::arg("kind"), py::arg("config") = std::vector<double>{},
             "Config values, when supplied, override native defaults in declaration order.")
        .def("optimize", &PyOptimizer::optimize, py::arg("objective"), py::arg("initial_state") = std::vector<double>{})
        .def_property_readonly("best_score", &PyOptimizer::best_score)
        .def_property_readonly("best_probability", &PyOptimizer::best_probability)
        .def("quantum_bounds", &PyOptimizer::quantum_bounds)
        .def("transform_frequency_point", &PyOptimizer::transform_frequency_point,
             py::arg("point"), py::arg("unwarp") = false);

    py::class_<opt::OwaConfig>(module, "OwaConfig")
        .def(py::init<>())
        .def_readwrite("merge_iterations", &opt::OwaConfig::merge_iterations)
        .def_readwrite("learning_rate", &opt::OwaConfig::learning_rate)
        .def_readwrite("l2_regularization", &opt::OwaConfig::l2_regularization)
        .def_readwrite("project_to_simplex", &opt::OwaConfig::project_to_simplex);
    py::class_<opt::OwaResult>(module, "OwaResult")
        .def_readonly("merged_parameters", &opt::OwaResult::merged_parameters)
        .def_readonly("model_weights", &opt::OwaResult::model_weights)
        .def_readonly("validation_loss", &opt::OwaResult::validation_loss);
    py::class_<opt::OwaCrossValidationResult>(module, "OwaCrossValidationResult")
        .def_readonly("mean_loss", &opt::OwaCrossValidationResult::mean_loss)
        .def_readonly("fold_losses", &opt::OwaCrossValidationResult::fold_losses);
    py::class_<opt::NowaResult>(module, "NowaResult")
        .def_readonly("merged_layers", &opt::NowaResult::merged_layers)
        .def_readonly("layer_weights", &opt::NowaResult::layer_weights)
        .def_readonly("validation_loss", &opt::NowaResult::validation_loss);
    module.def("naive_average_parameters", &opt::naive_average_parameters,
               py::arg("local_parameters"));
    module.def("optimal_weighted_average_linear_regression",
               py::overload_cast<const std::vector<std::vector<double>>&,
                                 const std::vector<std::vector<double>>&,
                                 const std::vector<double>&,
                                 const opt::OwaConfig&>(
                   &opt::optimal_weighted_average_linear_regression),
               py::arg("local_parameters"), py::arg("validation_features"),
               py::arg("validation_targets"), py::arg("config") = opt::OwaConfig{});
    module.def("fast_owa_cross_validation_linear_regression",
               &opt::fast_owa_cross_validation_linear_regression,
               py::arg("local_parameters"), py::arg("fold_features"),
               py::arg("fold_targets"), py::arg("config") = opt::OwaConfig{});
    module.def("weighted_tensor_average", &opt::weighted_tensor_average,
               py::arg("local_tensors"), py::arg("weights"));
    module.def("nonlinear_optimal_weighted_average_layers",
               [](const std::vector<std::vector<ml::deep_learning::Tensor>>& local_models,
                  py::function validation_loss, const opt::OwaConfig& config) {
                   opt::NowaLayerLoss callback =
                       [validation_loss = std::move(validation_loss)](
                           const std::vector<ml::deep_learning::Tensor>& layers) mutable {
                           return validation_loss(layers).cast<double>();
                       };
                   return opt::nonlinear_optimal_weighted_average_layers(
                       local_models, callback, config);
               },
               py::arg("local_model_layers"), py::arg("validation_loss"),
               py::arg("config") = opt::OwaConfig{});
}

void bind_math(py::module_& parent) {
    auto module = parent.def_submodule("math", "Heat equation solvers");
    auto solve = [](const std::vector<double>& initial, std::array<std::size_t, 3> shape,
                    int dimensions, double alpha, double sigma, double dx, double dy,
                    double dz, double dt, int steps, unsigned int seed, bool stochastic) {
        if (shape[0] != 0 && shape[1] > std::numeric_limits<std::size_t>::max() / shape[0])
            throw std::invalid_argument("heat grid shape overflows native size");
        const std::size_t plane = shape[0] * shape[1];
        if (plane != 0 && shape[2] > std::numeric_limits<std::size_t>::max() / plane)
            throw std::invalid_argument("heat grid shape overflows native size");
        const std::size_t count = plane * shape[2];
        if (initial.size() != count)
            throw std::invalid_argument("heat grid shape does not match the supplied values");
        std::vector<double> result(count);
        char error[error_capacity]{};
        if (!coolbox_math_heat(dimensions, stochastic ? 1 : 0, initial.data(),
                               shape[0], shape[1], shape[2], alpha, sigma,
                               dx, dy, dz, dt, steps, seed, result.data(),
                               error, sizeof(error))) {
            throw std::runtime_error(error_message(error, "heat solver failed"));
        }
        return result;
    };
    module.def("solve_heat_equation",
               [solve](const std::vector<double>& initial,
                       std::array<std::size_t, 3> shape, int dimensions,
                       double alpha, double dx, double dt, int steps,
                       double dy, double dz) {
                   return solve(initial, shape, dimensions, alpha, 0.0, dx, dy,
                                dz, dt, steps, 0, false);
               },
               py::arg("initial"), py::arg("shape"), py::arg("dimensions"),
               py::arg("alpha"), py::arg("dx"), py::arg("dt"), py::arg("steps"),
               py::arg("dy") = 1.0, py::arg("dz") = 1.0);
    module.def("solve_stochastic_heat_equation",
               [solve](const std::vector<double>& initial, std::array<std::size_t, 3> shape,
                       int dimensions, double alpha, double sigma, double dx, double dy,
                       double dz, double dt, int steps, unsigned int seed) {
                   return solve(initial, shape, dimensions, alpha, sigma, dx, dy, dz,
                                dt, steps, seed, true);
               },
               py::arg("initial"), py::arg("shape"), py::arg("dimensions"),
               py::arg("alpha"), py::arg("sigma"), py::arg("dx"), py::arg("dy"),
               py::arg("dz"), py::arg("dt"), py::arg("steps"), py::arg("seed") = 42);
}

void bind_finance(py::module_& parent) {
    auto module = parent.def_submodule("finance", "Option pricing and stochastic-process utilities");
    py::enum_<PyFinanceFormula>(module, "Formula")
        .value("STANDARD_NORMAL_PDF", PyFinanceFormula::StandardNormalPdf)
        .value("STANDARD_NORMAL_CDF", PyFinanceFormula::StandardNormalCdf)
        .value("BLACK_SCHOLES_D1", PyFinanceFormula::BlackScholesD1)
        .value("BLACK_SCHOLES_D2", PyFinanceFormula::BlackScholesD2)
        .value("BLACK_SCHOLES_CALL_PRICE", PyFinanceFormula::BlackScholesCallPrice)
        .value("BLACK_SCHOLES_PUT_PRICE", PyFinanceFormula::BlackScholesPutPrice)
        .value("BLACK_SCHOLES_CALL_DELTA", PyFinanceFormula::BlackScholesCallDelta)
        .value("BLACK_SCHOLES_PUT_DELTA", PyFinanceFormula::BlackScholesPutDelta)
        .value("BLACK_SCHOLES_GAMMA", PyFinanceFormula::BlackScholesGamma)
        .value("BLACK_SCHOLES_VEGA", PyFinanceFormula::BlackScholesVega)
        .value("BLACK_SCHOLES_CALL_THETA", PyFinanceFormula::BlackScholesCallTheta)
        .value("BLACK_SCHOLES_PUT_THETA", PyFinanceFormula::BlackScholesPutTheta)
        .value("BLACK_SCHOLES_CALL_RHO", PyFinanceFormula::BlackScholesCallRho)
        .value("BLACK_SCHOLES_PUT_RHO", PyFinanceFormula::BlackScholesPutRho)
        .value("DIGITAL_CALL_PRICE", PyFinanceFormula::DigitalCallPrice)
        .value("DIGITAL_PUT_PRICE", PyFinanceFormula::DigitalPutPrice)
        .value("GEOMETRIC_ASIAN_CALL_PRICE", PyFinanceFormula::GeometricAsianCallPrice)
        .value("GEOMETRIC_ASIAN_PUT_PRICE", PyFinanceFormula::GeometricAsianPutPrice)
        .value("DOWN_AND_IN_CALL_PRICE", PyFinanceFormula::DownAndInCallPrice)
        .value("DOWN_AND_OUT_CALL_PRICE", PyFinanceFormula::DownAndOutCallPrice)
        .value("UP_AND_IN_CALL_PRICE", PyFinanceFormula::UpAndInCallPrice)
        .value("UP_AND_OUT_CALL_PRICE", PyFinanceFormula::UpAndOutCallPrice)
        .value("DOWN_AND_IN_PUT_PRICE", PyFinanceFormula::DownAndInPutPrice)
        .value("DOWN_AND_OUT_PUT_PRICE", PyFinanceFormula::DownAndOutPutPrice);
    py::enum_<PyMonteCarloContract>(module, "MonteCarloContract")
        .value("EUROPEAN_CALL", PyMonteCarloContract::EuropeanCall)
        .value("EUROPEAN_PUT", PyMonteCarloContract::EuropeanPut)
        .value("ARITHMETIC_ASIAN_CALL", PyMonteCarloContract::ArithmeticAsianCall)
        .value("DOWN_AND_OUT_CALL", PyMonteCarloContract::DownAndOutCall)
        .value("UP_AND_OUT_CALL", PyMonteCarloContract::UpAndOutCall)
        .value("DOWN_AND_OUT_PUT", PyMonteCarloContract::DownAndOutPut);
    py::enum_<PyMonteCarloGreekContract>(module, "MonteCarloGreekContract")
        .value("EUROPEAN_CALL", PyMonteCarloGreekContract::EuropeanCall)
        .value("ARITHMETIC_ASIAN_CALL", PyMonteCarloGreekContract::ArithmeticAsianCall)
        .value("DOWN_AND_OUT_CALL", PyMonteCarloGreekContract::DownAndOutCall)
        .value("UP_AND_OUT_CALL", PyMonteCarloGreekContract::UpAndOutCall)
        .value("DOWN_AND_OUT_PUT", PyMonteCarloGreekContract::DownAndOutPut);
    module.def("evaluate", [](PyFinanceFormula formula, const std::vector<double>& values) {
        char error[error_capacity]{};
        const double result = coolbox_finance_formula(static_cast<int>(formula), values.data(),
                                                       values.size(), error,
                                                       sizeof(error));
        if (error[0]) throw std::invalid_argument(error);
        return result;
    }, py::arg("formula"), py::arg("values"));

    py::class_<coolbox_estimate>(module, "MonteCarloEstimate")
        .def_readonly("price", &coolbox_estimate::price)
        .def_readonly("standard_error", &coolbox_estimate::standard_error)
        .def_readonly("confidence_interval_low", &coolbox_estimate::confidence_interval_low)
        .def_readonly("confidence_interval_high", &coolbox_estimate::confidence_interval_high)
        .def_readonly("sample_count", &coolbox_estimate::sample_count);
    py::class_<coolbox_greeks>(module, "MonteCarloGreeks")
        .def_readonly("delta", &coolbox_greeks::delta)
        .def_readonly("gamma", &coolbox_greeks::gamma)
        .def_readonly("vega", &coolbox_greeks::vega)
        .def_readonly("theta", &coolbox_greeks::theta)
        .def_readonly("rho", &coolbox_greeks::rho);
    module.def("brownian_motion_path", [](const std::vector<double>& times,
                                          const std::vector<double>& shocks,
                                          double initial) {
        if (times.empty() || times.size() != shocks.size())
            throw std::invalid_argument("time steps and shocks must have matching non-empty lengths");
        std::vector<double> result(times.size() + 1);
        char error[error_capacity]{};
        if (!coolbox_finance_path(0, initial, 0, 0, times.data(), times.size(),
                                  shocks.data(), result.data(), error, sizeof(error)))
            throw std::runtime_error(error_message(error, "Brownian path failed"));
        return result;
    }, py::arg("time_steps"), py::arg("normal_shocks"), py::arg("initial_value") = 0.0);
    module.def("geometric_brownian_motion_path", [](double initial, double drift,
                                                   double volatility,
                                                   const std::vector<double>& times,
                                                   const std::vector<double>& shocks) {
        if (times.empty() || times.size() != shocks.size())
            throw std::invalid_argument("time steps and shocks must have matching non-empty lengths");
        std::vector<double> result(times.size() + 1);
        char error[error_capacity]{};
        if (!coolbox_finance_path(1, initial, drift, volatility, times.data(), times.size(),
                                  shocks.data(), result.data(), error, sizeof(error)))
            throw std::runtime_error(error_message(error, "geometric Brownian path failed"));
        return result;
    }, py::arg("initial_value"), py::arg("drift"), py::arg("volatility"),
       py::arg("time_steps"), py::arg("normal_shocks"));
    module.def("ito_integral_left_point", [](const std::vector<double>& values,
                                             const std::vector<double>& increments) {
        if (values.empty() || values.size() != increments.size())
            throw std::invalid_argument("integrand and increments must have matching non-empty lengths");
        double result = 0;
        char error[error_capacity]{};
        if (!coolbox_finance_ito(0, values.data(), increments.data(), values.size(),
                                 &result, error, sizeof(error)))
            throw std::runtime_error(error_message(error, "Ito integral failed"));
        return result;
    }, py::arg("integrand_values"), py::arg("brownian_increments"));
    module.def("ito_isometry_variance", [](const std::vector<double>& values,
                                           const std::vector<double>& times) {
        if (values.empty() || values.size() != times.size())
            throw std::invalid_argument("integrand and time steps must have matching non-empty lengths");
        double result = 0;
        char error[error_capacity]{};
        if (!coolbox_finance_ito(1, values.data(), times.data(), values.size(),
                                 &result, error, sizeof(error)))
            throw std::runtime_error(error_message(error, "Ito variance failed"));
        return result;
    }, py::arg("integrand_values"), py::arg("time_steps"));

    module.def("monte_carlo_estimate", [](PyMonteCarloContract contract, double spot, double strike,
                                          double barrier, double rate, double volatility,
                                          const std::vector<double>& times,
                                          const std::vector<std::vector<double>>& paths,
                                          double confidence) {
        if (times.empty() || paths.empty())
            throw std::invalid_argument("time steps and normal shock paths must not be empty");
        std::vector<double> flat;
        if (paths.size() > std::numeric_limits<std::size_t>::max() / times.size())
            throw std::invalid_argument("normal shock dimensions overflow native size");
        flat.reserve(times.size() * paths.size());
        for (const auto& path : paths) {
            if (path.size() != times.size())
                throw std::invalid_argument("each normal shock path must match the time steps");
            flat.insert(flat.end(), path.begin(), path.end());
        }
        coolbox_estimate result{};
        char error[error_capacity]{};
        if (!coolbox_finance_monte_carlo(static_cast<int>(contract), spot, strike, barrier, rate,
                                         volatility, times.data(), times.size(),
                                         flat.data(), paths.size(), confidence, 0.01,
                                         0.01, &result, error, sizeof(error)))
            throw std::runtime_error(error_message(error, "Monte Carlo estimate failed"));
        return result;
    }, py::arg("contract"), py::arg("spot"), py::arg("strike"),
       py::arg("barrier"), py::arg("risk_free_rate"), py::arg("volatility"),
       py::arg("time_steps"), py::arg("normal_shock_paths"),
       py::arg("confidence_z_score") = 1.959963984540054);
    module.def("monte_carlo_greeks_fd", [](PyMonteCarloGreekContract contract, double spot, double strike,
                                           double barrier, double rate, double volatility,
                                           const std::vector<double>& times,
                                           const std::vector<std::vector<double>>& paths,
                                           double confidence, double relative_bump,
                                           double time_bump) {
        if (times.empty() || paths.empty())
            throw std::invalid_argument("time steps and normal shock paths must not be empty");
        std::vector<double> flat;
        if (paths.size() > std::numeric_limits<std::size_t>::max() / times.size())
            throw std::invalid_argument("normal shock dimensions overflow native size");
        flat.reserve(times.size() * paths.size());
        for (const auto& path : paths) {
            if (path.size() != times.size())
                throw std::invalid_argument("each normal shock path must match the time steps");
            flat.insert(flat.end(), path.begin(), path.end());
        }
        coolbox_greeks result{};
        char error[error_capacity]{};
        if (!coolbox_finance_monte_carlo_greeks(static_cast<int>(contract), spot, strike, barrier,
                                                rate, volatility, times.data(),
                                                times.size(), flat.data(), paths.size(),
                                                confidence, relative_bump, time_bump,
                                                &result, error, sizeof(error)))
            throw std::runtime_error(error_message(error, "Monte Carlo Greeks failed"));
        return result;
    }, py::arg("contract"), py::arg("spot"), py::arg("strike"),
       py::arg("barrier"), py::arg("risk_free_rate"), py::arg("volatility"),
       py::arg("time_steps"), py::arg("normal_shock_paths"),
       py::arg("confidence_z_score") = 1.959963984540054,
       py::arg("relative_bump") = 0.01, py::arg("time_bump_years") = 0.01);
    module.def("monte_carlo_estimate_custom_payoff",
               [](double spot, double rate, double volatility,
                  const std::vector<double>& times,
                  const std::vector<std::vector<double>>& paths,
                  py::function payoff, double confidence) {
                   if (times.empty() || paths.empty())
                       throw std::invalid_argument("time steps and normal shock paths must not be empty");
                   for (const auto& path : paths) {
                       if (path.size() != times.size())
                           throw std::invalid_argument("each normal shock path must match the time steps");
                   }
                   finance::MonteCarloPayoff callback =
                       [payoff = std::move(payoff)](const std::vector<double>& path) mutable {
                           return payoff(path).cast<double>();
                       };
                   return finance::monte_carlo_estimate_generic(
                       spot, rate, volatility, times, paths, callback, confidence);
               },
               py::arg("spot"), py::arg("risk_free_rate"), py::arg("volatility"),
               py::arg("time_steps"), py::arg("normal_shock_paths"),
               py::arg("payoff"), py::arg("confidence_z_score") = 1.959963984540054);
    module.def("monte_carlo_greeks_custom_payoff_fd",
               [](double spot, double strike, double rate, double volatility,
                  const std::vector<double>& times,
                  const std::vector<std::vector<double>>& paths,
                  py::function payoff, double confidence,
                  double relative_bump, double time_bump) {
                   if (times.empty() || paths.empty())
                       throw std::invalid_argument("time steps and normal shock paths must not be empty");
                   for (const auto& path : paths) {
                       if (path.size() != times.size())
                           throw std::invalid_argument("each normal shock path must match the time steps");
                   }
                   finance::MonteCarloPayoff callback =
                       [payoff = std::move(payoff)](const std::vector<double>& path) mutable {
                           return payoff(path).cast<double>();
                       };
                   return finance::monte_carlo_greeks_fd_generic(
                       spot, strike, rate, volatility, times, paths, callback,
                       confidence, relative_bump, time_bump);
               },
               py::arg("spot"), py::arg("strike"), py::arg("risk_free_rate"),
               py::arg("volatility"), py::arg("time_steps"),
               py::arg("normal_shock_paths"), py::arg("payoff"),
               py::arg("confidence_z_score") = 1.959963984540054,
               py::arg("relative_bump") = 0.01, py::arg("time_bump_years") = 0.01);
}

} // namespace

void bind_opt_math_finance(py::module_& module) {
    bind_opt(module);
    bind_math(module);
    bind_finance(module);
}
