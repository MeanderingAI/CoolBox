#include "finance.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace finance {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kSqrtTwo = 1.41421356237309504880;
constexpr double kDefaultConfidenceZScore = 1.959963984540054;

void validate_positive(const char* name, double value) {
    if (value <= 0.0) {
        throw std::invalid_argument(std::string(name) + " must be positive");
    }
}

void validate_non_negative(const char* name, double value) {
    if (value < 0.0) {
        throw std::invalid_argument(std::string(name) + " must be non-negative");
    }
}

void validate_matching_sizes(const std::vector<double>& lhs,
                             const std::vector<double>& rhs,
                             const char* message) {
    if (lhs.size() != rhs.size()) {
        throw std::invalid_argument(message);
    }
}

void validate_confidence_z_score(double confidence_z_score) {
    validate_non_negative("confidence_z_score", confidence_z_score);
}

double total_time(const std::vector<double>& time_steps) {
    double maturity = 0.0;
    for (double time_step : time_steps) {
        validate_positive("time_steps[index]", time_step);
        maturity += time_step;
    }
    return maturity;
}

std::vector<double> scale_time_steps(const std::vector<double>& time_steps, double factor) {
    validate_positive("factor", factor);
    std::vector<double> scaled = time_steps;
    for (double& time_step : scaled) {
        time_step *= factor;
    }
    return scaled;
}

void validate_simulation_paths(const std::vector<double>& time_steps,
                               const std::vector<std::vector<double>>& normal_shock_paths) {
    if (normal_shock_paths.empty()) {
        throw std::invalid_argument("normal_shock_paths must not be empty");
    }

    for (const auto& shock_path : normal_shock_paths) {
        validate_matching_sizes(time_steps, shock_path,
                                "time_steps and each normal shock path must match in size");
    }
}

MonteCarloEstimate estimate_from_samples(const std::vector<double>& discounted_samples,
                                         double confidence_z_score) {
    validate_confidence_z_score(confidence_z_score);
    if (discounted_samples.empty()) {
        throw std::invalid_argument("discounted_samples must not be empty");
    }

    double mean = 0.0;
    for (double sample : discounted_samples) {
        mean += sample;
    }
    mean /= static_cast<double>(discounted_samples.size());

    double standard_error = 0.0;
    if (discounted_samples.size() > 1U) {
        double sum_squared = 0.0;
        for (double sample : discounted_samples) {
            const double centered = sample - mean;
            sum_squared += centered * centered;
        }
        const double sample_variance = sum_squared / static_cast<double>(discounted_samples.size() - 1U);
        standard_error = std::sqrt(sample_variance / static_cast<double>(discounted_samples.size()));
    }

    return MonteCarloEstimate{mean,
                              standard_error,
                              mean - confidence_z_score * standard_error,
                              mean + confidence_z_score * standard_error,
                              discounted_samples.size()};
}

template <typename Payoff>
std::vector<double> monte_carlo_discounted_payoffs(double spot,
                                                   double risk_free_rate,
                                                   double volatility,
                                                   const std::vector<double>& time_steps,
                                                   const std::vector<std::vector<double>>& normal_shock_paths,
                                                   Payoff payoff) {
    validate_positive("spot", spot);
    validate_non_negative("volatility", volatility);
    validate_simulation_paths(time_steps, normal_shock_paths);

    const double maturity_years = total_time(time_steps);
    const double discount = std::exp(-risk_free_rate * maturity_years);
    std::vector<double> discounted_payoffs;
    discounted_payoffs.reserve(normal_shock_paths.size());
    for (const auto& shock_path : normal_shock_paths) {
        const auto simulated_path =
            geometric_brownian_motion_path(spot, risk_free_rate, volatility, time_steps, shock_path);
        discounted_payoffs.push_back(discount * payoff(simulated_path));
    }

    return discounted_payoffs;
}

template <typename Payoff>
MonteCarloEstimate monte_carlo_estimate(double spot,
                                        double risk_free_rate,
                                        double volatility,
                                        const std::vector<double>& time_steps,
                                        const std::vector<std::vector<double>>& normal_shock_paths,
                                        double confidence_z_score,
                                        Payoff payoff) {
    return estimate_from_samples(
        monte_carlo_discounted_payoffs(spot, risk_free_rate, volatility, time_steps, normal_shock_paths, payoff),
        confidence_z_score);
}

double arithmetic_average_fixings(const std::vector<double>& path) {
    if (path.size() <= 1U) {
        throw std::invalid_argument("path must contain at least one fixing");
    }

    double sum = 0.0;
    for (std::size_t index = 1; index < path.size(); ++index) {
        sum += path[index];
    }
    return sum / static_cast<double>(path.size() - 1U);
}

double call_terminal_payoff(const std::vector<double>& path, double strike) {
    return std::max(path.back() - strike, 0.0);
}

double put_terminal_payoff(const std::vector<double>& path, double strike) {
    return std::max(strike - path.back(), 0.0);
}

double down_and_out_call_payoff(const std::vector<double>& path, double strike, double barrier) {
    for (double node : path) {
        if (node <= barrier) {
            return 0.0;
        }
    }
    return call_terminal_payoff(path, strike);
}

template <typename Payoff>
MonteCarloGreeks monte_carlo_finite_difference_greeks(double spot,
                                                      double strike,
                                                      double risk_free_rate,
                                                      double volatility,
                                                      const std::vector<double>& time_steps,
                                                      const std::vector<std::vector<double>>& normal_shock_paths,
                                                      double confidence_z_score,
                                                      double relative_bump,
                                                      double time_bump_years,
                                                      Payoff payoff) {
    validate_positive("spot", spot);
    validate_positive("strike", strike);
    validate_non_negative("volatility", volatility);
    validate_positive("relative_bump", relative_bump);
    validate_positive("time_bump_years", time_bump_years);
    validate_confidence_z_score(confidence_z_score);
    validate_simulation_paths(time_steps, normal_shock_paths);

    const double maturity_years = total_time(time_steps);
    if (time_bump_years >= maturity_years) {
        throw std::invalid_argument("time_bump_years must be smaller than total maturity");
    }

    const double spot_bump = std::max(spot * relative_bump, 1e-6);
    const double volatility_bump = std::max(volatility * relative_bump, 1e-6);
    const double rate_bump = std::max(std::abs(risk_free_rate) * relative_bump, 1e-6);
    const std::vector<double> longer_time_steps = scale_time_steps(time_steps, (maturity_years + time_bump_years) / maturity_years);
    const std::vector<double> shorter_time_steps = scale_time_steps(time_steps, (maturity_years - time_bump_years) / maturity_years);

    std::vector<double> delta_samples;
    std::vector<double> gamma_samples;
    std::vector<double> vega_samples;
    std::vector<double> theta_samples;
    std::vector<double> rho_samples;
    delta_samples.reserve(normal_shock_paths.size());
    gamma_samples.reserve(normal_shock_paths.size());
    vega_samples.reserve(normal_shock_paths.size());
    theta_samples.reserve(normal_shock_paths.size());
    rho_samples.reserve(normal_shock_paths.size());

    const double base_discount = std::exp(-risk_free_rate * maturity_years);
    const double rho_discount_up = std::exp(-(risk_free_rate + rate_bump) * maturity_years);
    const double rho_discount_down = std::exp(-(risk_free_rate - rate_bump) * maturity_years);
    const double theta_discount_long = std::exp(-risk_free_rate * (maturity_years + time_bump_years));
    const double theta_discount_short = std::exp(-risk_free_rate * (maturity_years - time_bump_years));

    for (const auto& shock_path : normal_shock_paths) {
        const auto base_path = geometric_brownian_motion_path(spot, risk_free_rate, volatility, time_steps, shock_path);
        const auto spot_up_path = geometric_brownian_motion_path(spot + spot_bump, risk_free_rate, volatility, time_steps, shock_path);
        const auto spot_down_path = geometric_brownian_motion_path(std::max(spot - spot_bump, 1e-6), risk_free_rate, volatility, time_steps, shock_path);
        const auto vol_up_path = geometric_brownian_motion_path(spot, risk_free_rate, volatility + volatility_bump, time_steps, shock_path);
        const auto vol_down_path = geometric_brownian_motion_path(spot,
                                                                  risk_free_rate,
                                                                  std::max(volatility - volatility_bump, 0.0),
                                                                  time_steps,
                                                                  shock_path);
        const auto rho_up_path = geometric_brownian_motion_path(spot, risk_free_rate + rate_bump, volatility, time_steps, shock_path);
        const auto rho_down_path = geometric_brownian_motion_path(spot, risk_free_rate - rate_bump, volatility, time_steps, shock_path);
        const auto theta_long_path = geometric_brownian_motion_path(spot, risk_free_rate, volatility, longer_time_steps, shock_path);
        const auto theta_short_path = geometric_brownian_motion_path(spot, risk_free_rate, volatility, shorter_time_steps, shock_path);

        const double base_value = base_discount * payoff(base_path);
        const double up_value = base_discount * payoff(spot_up_path);
        const double down_value = base_discount * payoff(spot_down_path);
        const double vol_up_value = base_discount * payoff(vol_up_path);
        const double vol_down_value = base_discount * payoff(vol_down_path);
        const double rho_up_value = rho_discount_up * payoff(rho_up_path);
        const double rho_down_value = rho_discount_down * payoff(rho_down_path);
        const double theta_long_value = theta_discount_long * payoff(theta_long_path);
        const double theta_short_value = theta_discount_short * payoff(theta_short_path);

        delta_samples.push_back((up_value - down_value) / (2.0 * spot_bump));
        gamma_samples.push_back((up_value - 2.0 * base_value + down_value) / (spot_bump * spot_bump));
        vega_samples.push_back((vol_up_value - vol_down_value) / (2.0 * volatility_bump));
        rho_samples.push_back((rho_up_value - rho_down_value) / (2.0 * rate_bump));
        theta_samples.push_back(-(theta_long_value - theta_short_value) / (2.0 * time_bump_years));
    }

    return MonteCarloGreeks{estimate_from_samples(delta_samples, confidence_z_score),
                            estimate_from_samples(gamma_samples, confidence_z_score),
                            estimate_from_samples(vega_samples, confidence_z_score),
                            estimate_from_samples(theta_samples, confidence_z_score),
                            estimate_from_samples(rho_samples, confidence_z_score)};
}

} // namespace

double standard_normal_pdf(double value) {
    return std::exp(-0.5 * value * value) / std::sqrt(2.0 * kPi);
}

double standard_normal_cdf(double value) {
    return 0.5 * std::erfc(-value / kSqrtTwo);
}

double black_scholes_d1(double spot,
                        double strike,
                        double risk_free_rate,
                        double volatility,
                        double maturity_years) {
    validate_positive("spot", spot);
    validate_positive("strike", strike);
    validate_positive("volatility", volatility);
    validate_positive("maturity_years", maturity_years);

    const double variance_term = volatility * std::sqrt(maturity_years);
    return (std::log(spot / strike) +
            (risk_free_rate + 0.5 * volatility * volatility) * maturity_years) /
           variance_term;
}

double black_scholes_d2(double spot,
                        double strike,
                        double risk_free_rate,
                        double volatility,
                        double maturity_years) {
    return black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years) -
           volatility * std::sqrt(maturity_years);
}

double black_scholes_call_price(double spot,
                                double strike,
                                double risk_free_rate,
                                double volatility,
                                double maturity_years) {
    const double d1 = black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years);
    const double d2 = black_scholes_d2(spot, strike, risk_free_rate, volatility, maturity_years);
    const double discount = std::exp(-risk_free_rate * maturity_years);
    return spot * standard_normal_cdf(d1) - strike * discount * standard_normal_cdf(d2);
}

double black_scholes_put_price(double spot,
                               double strike,
                               double risk_free_rate,
                               double volatility,
                               double maturity_years) {
    const double d1 = black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years);
    const double d2 = black_scholes_d2(spot, strike, risk_free_rate, volatility, maturity_years);
    const double discount = std::exp(-risk_free_rate * maturity_years);
    return strike * discount * standard_normal_cdf(-d2) - spot * standard_normal_cdf(-d1);
}

double black_scholes_call_delta(double spot,
                                double strike,
                                double risk_free_rate,
                                double volatility,
                                double maturity_years) {
    return standard_normal_cdf(
        black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years));
}

double black_scholes_put_delta(double spot,
                               double strike,
                               double risk_free_rate,
                               double volatility,
                               double maturity_years) {
    return black_scholes_call_delta(spot, strike, risk_free_rate, volatility, maturity_years) - 1.0;
}

double black_scholes_gamma(double spot,
                           double strike,
                           double risk_free_rate,
                           double volatility,
                           double maturity_years) {
    const double d1 = black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years);
    return standard_normal_pdf(d1) / (spot * volatility * std::sqrt(maturity_years));
}

double black_scholes_vega(double spot,
                          double strike,
                          double risk_free_rate,
                          double volatility,
                          double maturity_years) {
    const double d1 = black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years);
    return spot * standard_normal_pdf(d1) * std::sqrt(maturity_years);
}

double black_scholes_call_theta(double spot,
                                double strike,
                                double risk_free_rate,
                                double volatility,
                                double maturity_years) {
    const double d1 = black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years);
    const double d2 = black_scholes_d2(spot, strike, risk_free_rate, volatility, maturity_years);
    const double discount = std::exp(-risk_free_rate * maturity_years);
    const double diffusion_term =
        -(spot * standard_normal_pdf(d1) * volatility) / (2.0 * std::sqrt(maturity_years));
    const double carry_term = -risk_free_rate * strike * discount * standard_normal_cdf(d2);
    return diffusion_term + carry_term;
}

double black_scholes_put_theta(double spot,
                               double strike,
                               double risk_free_rate,
                               double volatility,
                               double maturity_years) {
    const double d1 = black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years);
    const double d2 = black_scholes_d2(spot, strike, risk_free_rate, volatility, maturity_years);
    const double discount = std::exp(-risk_free_rate * maturity_years);
    const double diffusion_term =
        -(spot * standard_normal_pdf(d1) * volatility) / (2.0 * std::sqrt(maturity_years));
    const double carry_term = risk_free_rate * strike * discount * standard_normal_cdf(-d2);
    return diffusion_term + carry_term;
}

double black_scholes_call_rho(double spot,
                              double strike,
                              double risk_free_rate,
                              double volatility,
                              double maturity_years) {
    const double d2 = black_scholes_d2(spot, strike, risk_free_rate, volatility, maturity_years);
    return strike * maturity_years * std::exp(-risk_free_rate * maturity_years) * standard_normal_cdf(d2);
}

double black_scholes_put_rho(double spot,
                             double strike,
                             double risk_free_rate,
                             double volatility,
                             double maturity_years) {
    const double d2 = black_scholes_d2(spot, strike, risk_free_rate, volatility, maturity_years);
    return -strike * maturity_years * std::exp(-risk_free_rate * maturity_years) * standard_normal_cdf(-d2);
}

double black_scholes_digital_call_price(double spot,
                                        double strike,
                                        double risk_free_rate,
                                        double volatility,
                                        double maturity_years,
                                        double cash_payoff) {
    const double d2 = black_scholes_d2(spot, strike, risk_free_rate, volatility, maturity_years);
    return cash_payoff * std::exp(-risk_free_rate * maturity_years) * standard_normal_cdf(d2);
}

double black_scholes_digital_put_price(double spot,
                                       double strike,
                                       double risk_free_rate,
                                       double volatility,
                                       double maturity_years,
                                       double cash_payoff) {
    const double d2 = black_scholes_d2(spot, strike, risk_free_rate, volatility, maturity_years);
    return cash_payoff * std::exp(-risk_free_rate * maturity_years) * standard_normal_cdf(-d2);
}

double geometric_asian_call_price(double spot,
                                  double strike,
                                  double risk_free_rate,
                                  double volatility,
                                  double maturity_years) {
    validate_positive("spot", spot);
    validate_positive("strike", strike);
    validate_non_negative("volatility", volatility);
    validate_positive("maturity_years", maturity_years);

    const double mean_log = std::log(spot) + 0.5 * (risk_free_rate - 0.5 * volatility * volatility) * maturity_years;
    const double variance_log = (volatility * volatility * maturity_years) / 3.0;
    const double stddev_log = std::sqrt(variance_log);
    const double d1 = (mean_log - std::log(strike) + variance_log) / stddev_log;
    const double d2 = d1 - stddev_log;
    const double discounted_forward = std::exp(-risk_free_rate * maturity_years + mean_log + 0.5 * variance_log);
    return discounted_forward * standard_normal_cdf(d1) -
           strike * std::exp(-risk_free_rate * maturity_years) * standard_normal_cdf(d2);
}

double geometric_asian_put_price(double spot,
                                 double strike,
                                 double risk_free_rate,
                                 double volatility,
                                 double maturity_years) {
    validate_positive("spot", spot);
    validate_positive("strike", strike);
    validate_non_negative("volatility", volatility);
    validate_positive("maturity_years", maturity_years);

    const double mean_log = std::log(spot) + 0.5 * (risk_free_rate - 0.5 * volatility * volatility) * maturity_years;
    const double variance_log = (volatility * volatility * maturity_years) / 3.0;
    const double stddev_log = std::sqrt(variance_log);
    const double d1 = (mean_log - std::log(strike) + variance_log) / stddev_log;
    const double d2 = d1 - stddev_log;
    const double discounted_forward = std::exp(-risk_free_rate * maturity_years + mean_log + 0.5 * variance_log);
    return strike * std::exp(-risk_free_rate * maturity_years) * standard_normal_cdf(-d2) -
           discounted_forward * standard_normal_cdf(-d1);
}

double black_scholes_down_and_in_call_price(double spot,
                                            double strike,
                                            double barrier,
                                            double risk_free_rate,
                                            double volatility,
                                            double maturity_years) {
    validate_positive("spot", spot);
    validate_positive("strike", strike);
    validate_positive("barrier", barrier);
    validate_positive("volatility", volatility);
    validate_positive("maturity_years", maturity_years);
    if (barrier >= spot) {
        throw std::invalid_argument("barrier must be below spot for down-and-in call pricing");
    }
    if (barrier > strike) {
        throw std::invalid_argument("barrier must be less than or equal to strike for this closed-form down-and-in call");
    }

    const double lambda = (risk_free_rate + 0.5 * volatility * volatility) / (volatility * volatility);
    const double sigma_root_t = volatility * std::sqrt(maturity_years);
    const double barrier_ratio = barrier / spot;
    const double mirrored_d1 = std::log((barrier * barrier) / (spot * strike)) / sigma_root_t + lambda * sigma_root_t;
    const double mirrored_d2 = mirrored_d1 - sigma_root_t;
    const double discount = std::exp(-risk_free_rate * maturity_years);

    return spot * std::pow(barrier_ratio, 2.0 * lambda) * standard_normal_cdf(mirrored_d1) -
           strike * discount * std::pow(barrier_ratio, 2.0 * lambda - 2.0) * standard_normal_cdf(mirrored_d2);
}

double black_scholes_down_and_out_call_price(double spot,
                                             double strike,
                                             double barrier,
                                             double risk_free_rate,
                                             double volatility,
                                             double maturity_years) {
    return black_scholes_call_price(spot, strike, risk_free_rate, volatility, maturity_years) -
           black_scholes_down_and_in_call_price(spot, strike, barrier, risk_free_rate, volatility, maturity_years);
}

std::vector<double> brownian_motion_path(const std::vector<double>& time_steps,
                                         const std::vector<double>& normal_shocks,
                                         double initial_value) {
    validate_matching_sizes(time_steps, normal_shocks, "time_steps and normal_shocks must match in size");

    std::vector<double> path;
    path.reserve(time_steps.size() + 1U);
    path.push_back(initial_value);

    for (std::size_t index = 0; index < time_steps.size(); ++index) {
        validate_positive("time_steps[index]", time_steps[index]);
        path.push_back(path.back() + std::sqrt(time_steps[index]) * normal_shocks[index]);
    }

    return path;
}

std::vector<double> geometric_brownian_motion_path(double initial_value,
                                                   double drift,
                                                   double volatility,
                                                   const std::vector<double>& time_steps,
                                                   const std::vector<double>& normal_shocks) {
    validate_positive("initial_value", initial_value);
    validate_non_negative("volatility", volatility);
    validate_matching_sizes(time_steps, normal_shocks, "time_steps and normal_shocks must match in size");

    std::vector<double> path;
    path.reserve(time_steps.size() + 1U);
    path.push_back(initial_value);

    for (std::size_t index = 0; index < time_steps.size(); ++index) {
        validate_positive("time_steps[index]", time_steps[index]);
        const double dt = time_steps[index];
        const double diffusion = volatility * std::sqrt(dt) * normal_shocks[index];
        const double drift_term = (drift - 0.5 * volatility * volatility) * dt;
        path.push_back(path.back() * std::exp(drift_term + diffusion));
    }

    return path;
}

MonteCarloEstimate monte_carlo_european_call_estimate(double spot,
                                                      double strike,
                                                      double risk_free_rate,
                                                      double volatility,
                                                      const std::vector<double>& time_steps,
                                                      const std::vector<std::vector<double>>& normal_shock_paths,
                                                      double confidence_z_score) {
    validate_positive("strike", strike);
    return monte_carlo_estimate(
        spot,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        [strike](const std::vector<double>& path) {
            return call_terminal_payoff(path, strike);
        });
}

MonteCarloEstimate monte_carlo_european_put_estimate(double spot,
                                                     double strike,
                                                     double risk_free_rate,
                                                     double volatility,
                                                     const std::vector<double>& time_steps,
                                                     const std::vector<std::vector<double>>& normal_shock_paths,
                                                     double confidence_z_score) {
    validate_positive("strike", strike);
    return monte_carlo_estimate(
        spot,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        [strike](const std::vector<double>& path) {
            return put_terminal_payoff(path, strike);
        });
}

MonteCarloEstimate monte_carlo_arithmetic_asian_call_estimate(double spot,
                                                              double strike,
                                                              double risk_free_rate,
                                                              double volatility,
                                                              const std::vector<double>& time_steps,
                                                              const std::vector<std::vector<double>>& normal_shock_paths,
                                                              double confidence_z_score) {
    validate_positive("strike", strike);
    return monte_carlo_estimate(
        spot,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        [strike](const std::vector<double>& path) {
            return std::max(arithmetic_average_fixings(path) - strike, 0.0);
        });
}

MonteCarloEstimate monte_carlo_down_and_out_call_estimate(double spot,
                                                          double strike,
                                                          double barrier,
                                                          double risk_free_rate,
                                                          double volatility,
                                                          const std::vector<double>& time_steps,
                                                          const std::vector<std::vector<double>>& normal_shock_paths,
                                                          double confidence_z_score) {
    validate_positive("strike", strike);
    validate_positive("barrier", barrier);
    return monte_carlo_estimate(
        spot,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        [strike, barrier](const std::vector<double>& path) {
            return down_and_out_call_payoff(path, strike, barrier);
        });
}

double monte_carlo_european_call_price(double spot,
                                       double strike,
                                       double risk_free_rate,
                                       double volatility,
                                       const std::vector<double>& time_steps,
                                       const std::vector<std::vector<double>>& normal_shock_paths) {
    return monte_carlo_european_call_estimate(
               spot,
               strike,
               risk_free_rate,
               volatility,
               time_steps,
               normal_shock_paths,
               kDefaultConfidenceZScore)
        .price;
}

double monte_carlo_european_put_price(double spot,
                                      double strike,
                                      double risk_free_rate,
                                      double volatility,
                                      const std::vector<double>& time_steps,
                                      const std::vector<std::vector<double>>& normal_shock_paths) {
    return monte_carlo_european_put_estimate(
               spot,
               strike,
               risk_free_rate,
               volatility,
               time_steps,
               normal_shock_paths,
               kDefaultConfidenceZScore)
        .price;
}

double monte_carlo_arithmetic_asian_call_price(double spot,
                                               double strike,
                                               double risk_free_rate,
                                               double volatility,
                                               const std::vector<double>& time_steps,
                                               const std::vector<std::vector<double>>& normal_shock_paths) {
    return monte_carlo_arithmetic_asian_call_estimate(
               spot,
               strike,
               risk_free_rate,
               volatility,
               time_steps,
               normal_shock_paths,
               kDefaultConfidenceZScore)
        .price;
}

double monte_carlo_down_and_out_call_price(double spot,
                                           double strike,
                                           double barrier,
                                           double risk_free_rate,
                                           double volatility,
                                           const std::vector<double>& time_steps,
                                           const std::vector<std::vector<double>>& normal_shock_paths) {
    return monte_carlo_down_and_out_call_estimate(
               spot,
               strike,
               barrier,
               risk_free_rate,
               volatility,
               time_steps,
               normal_shock_paths,
               kDefaultConfidenceZScore)
        .price;
}

MonteCarloGreeks monte_carlo_european_call_greeks_fd(double spot,
                                                     double strike,
                                                     double risk_free_rate,
                                                     double volatility,
                                                     const std::vector<double>& time_steps,
                                                     const std::vector<std::vector<double>>& normal_shock_paths,
                                                     double confidence_z_score,
                                                     double relative_bump,
                                                     double time_bump_years) {
    return monte_carlo_finite_difference_greeks(
        spot,
        strike,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        relative_bump,
        time_bump_years,
        [strike](const std::vector<double>& path) {
            return call_terminal_payoff(path, strike);
        });
}

MonteCarloGreeks monte_carlo_arithmetic_asian_call_greeks_fd(double spot,
                                                             double strike,
                                                             double risk_free_rate,
                                                             double volatility,
                                                             const std::vector<double>& time_steps,
                                                             const std::vector<std::vector<double>>& normal_shock_paths,
                                                             double confidence_z_score,
                                                             double relative_bump,
                                                             double time_bump_years) {
    return monte_carlo_finite_difference_greeks(
        spot,
        strike,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        relative_bump,
        time_bump_years,
        [strike](const std::vector<double>& path) {
            return std::max(arithmetic_average_fixings(path) - strike, 0.0);
        });
}

double ito_integral_left_point(const std::vector<double>& integrand_values,
                               const std::vector<double>& brownian_increments) {
    validate_matching_sizes(integrand_values, brownian_increments,
                            "integrand_values and brownian_increments must match in size");

    double total = 0.0;
    for (std::size_t index = 0; index < integrand_values.size(); ++index) {
        total += integrand_values[index] * brownian_increments[index];
    }
    return total;
}

double ito_isometry_variance(const std::vector<double>& integrand_values,
                             const std::vector<double>& time_steps) {
    validate_matching_sizes(integrand_values, time_steps,
                            "integrand_values and time_steps must match in size");

    double total = 0.0;
    for (std::size_t index = 0; index < integrand_values.size(); ++index) {
        validate_positive("time_steps[index]", time_steps[index]);
        total += integrand_values[index] * integrand_values[index] * time_steps[index];
    }
    return total;
}

double black_scholes_up_and_in_call_price(double spot,
                                          double strike,
                                          double barrier,
                                          double risk_free_rate,
                                          double volatility,
                                          double maturity_years) {
    validate_positive("spot", spot);
    validate_positive("strike", strike);
    validate_positive("barrier", barrier);
    validate_positive("volatility", volatility);
    validate_positive("maturity_years", maturity_years);
    if (barrier <= spot) {
        throw std::invalid_argument("barrier must be above spot for up-and-in call pricing");
    }
    if (barrier < strike) {
        throw std::invalid_argument("barrier must be greater than or equal to strike for this closed-form up-and-in call");
    }

    const double vanilla = black_scholes_call_price(spot, strike, risk_free_rate, volatility, maturity_years);
    const double up_and_out =
        black_scholes_up_and_out_call_price(spot, strike, barrier, risk_free_rate, volatility, maturity_years);
    return vanilla - up_and_out;
}

double black_scholes_up_and_out_call_price(double spot,
                                           double strike,
                                           double barrier,
                                           double risk_free_rate,
                                           double volatility,
                                           double maturity_years) {
    validate_positive("spot", spot);
    validate_positive("strike", strike);
    validate_positive("barrier", barrier);
    validate_positive("volatility", volatility);
    validate_positive("maturity_years", maturity_years);
    if (barrier <= spot) {
        throw std::invalid_argument("barrier must be above spot for up-and-out call pricing");
    }
    if (barrier < strike) {
        throw std::invalid_argument("barrier must be greater than or equal to strike for this closed-form up-and-out call");
    }

    const double lambda = (risk_free_rate + 0.5 * volatility * volatility) / (volatility * volatility);
    const double sigma_root_t = volatility * std::sqrt(maturity_years);
    const double d1 = black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years);
    const double d2 = d1 - sigma_root_t;
    const double barrier_ratio = barrier / spot;
    const double y = std::log((barrier * barrier) / (spot * strike)) / sigma_root_t + lambda * sigma_root_t;
    const double discount = std::exp(-risk_free_rate * maturity_years);

    const double raw_up_and_out =
        spot * (standard_normal_cdf(d1) - std::pow(barrier_ratio, 2.0 * lambda) * standard_normal_cdf(-y)) -
        strike * discount *
            (standard_normal_cdf(d2) -
             std::pow(barrier_ratio, 2.0 * lambda - 2.0) * standard_normal_cdf(-y + sigma_root_t));
    const double vanilla = black_scholes_call_price(spot, strike, risk_free_rate, volatility, maturity_years);
    return std::clamp(raw_up_and_out, 0.0, vanilla);
}

double black_scholes_down_and_in_put_price(double spot,
                                           double strike,
                                           double barrier,
                                           double risk_free_rate,
                                           double volatility,
                                           double maturity_years) {
    validate_positive("spot", spot);
    validate_positive("strike", strike);
    validate_positive("barrier", barrier);
    validate_positive("volatility", volatility);
    validate_positive("maturity_years", maturity_years);
    if (barrier >= spot) {
        throw std::invalid_argument("barrier must be below spot for down-and-in put pricing");
    }
    if (barrier > strike) {
        throw std::invalid_argument("barrier must be less than or equal to strike for this closed-form down-and-in put");
    }

    const double vanilla = black_scholes_put_price(spot, strike, risk_free_rate, volatility, maturity_years);
    const double down_and_out =
        black_scholes_down_and_out_put_price(spot, strike, barrier, risk_free_rate, volatility, maturity_years);
    return vanilla - down_and_out;
}

double black_scholes_down_and_out_put_price(double spot,
                                            double strike,
                                            double barrier,
                                            double risk_free_rate,
                                            double volatility,
                                            double maturity_years) {
    validate_positive("spot", spot);
    validate_positive("strike", strike);
    validate_positive("barrier", barrier);
    validate_positive("volatility", volatility);
    validate_positive("maturity_years", maturity_years);
    if (barrier >= spot) {
        throw std::invalid_argument("barrier must be below spot for down-and-out put pricing");
    }
    if (barrier > strike) {
        throw std::invalid_argument("barrier must be less than or equal to strike for this closed-form down-and-out put");
    }

    const double lambda = (risk_free_rate + 0.5 * volatility * volatility) / (volatility * volatility);
    const double sigma_root_t = volatility * std::sqrt(maturity_years);
    const double d1 = black_scholes_d1(spot, strike, risk_free_rate, volatility, maturity_years);
    const double d2 = d1 - sigma_root_t;
    const double barrier_ratio = barrier / spot;
    const double y = std::log((barrier * barrier) / (spot * strike)) / sigma_root_t + lambda * sigma_root_t;
    const double discount = std::exp(-risk_free_rate * maturity_years);

    const double raw_down_and_out =
        strike * discount *
            (standard_normal_cdf(-d2) -
             std::pow(barrier_ratio, 2.0 * lambda - 2.0) * standard_normal_cdf(y - sigma_root_t)) -
        spot * (standard_normal_cdf(-d1) - std::pow(barrier_ratio, 2.0 * lambda) * standard_normal_cdf(y));
    const double vanilla = black_scholes_put_price(spot, strike, risk_free_rate, volatility, maturity_years);
    return std::clamp(raw_down_and_out, 0.0, vanilla);
}

namespace {
double up_and_out_call_path_payoff(const std::vector<double>& path, double strike, double barrier) {
    for (const double price : path) {
        if (price >= barrier) {
            return 0.0; // knocked out
        }
    }
    return std::max(path.back() - strike, 0.0);
}

double down_and_out_put_payoff(const std::vector<double>& path, double strike, double barrier) {
    for (const double price : path) {
        if (price <= barrier) {
            return 0.0; // knocked out
        }
    }
    return std::max(strike - path.back(), 0.0);
}
} // anonymous namespace

MonteCarloEstimate monte_carlo_up_and_out_call_estimate(double spot,
                                                        double strike,
                                                        double barrier,
                                                        double risk_free_rate,
                                                        double volatility,
                                                        const std::vector<double>& time_steps,
                                                        const std::vector<std::vector<double>>& normal_shock_paths,
                                                        double confidence_z_score) {
    validate_positive("strike", strike);
    validate_positive("barrier", barrier);
    return monte_carlo_estimate(
        spot,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        [strike, barrier](const std::vector<double>& path) {
            return up_and_out_call_path_payoff(path, strike, barrier);
        });
}

MonteCarloEstimate monte_carlo_down_and_out_put_estimate(double spot,
                                                         double strike,
                                                         double barrier,
                                                         double risk_free_rate,
                                                         double volatility,
                                                         const std::vector<double>& time_steps,
                                                         const std::vector<std::vector<double>>& normal_shock_paths,
                                                         double confidence_z_score) {
    validate_positive("strike", strike);
    validate_positive("barrier", barrier);
    return monte_carlo_estimate(
        spot,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        [strike, barrier](const std::vector<double>& path) {
            return down_and_out_put_payoff(path, strike, barrier);
        });
}

MonteCarloEstimate monte_carlo_estimate_generic(double spot,
                                                double risk_free_rate,
                                                double volatility,
                                                const std::vector<double>& time_steps,
                                                const std::vector<std::vector<double>>& normal_shock_paths,
                                                const MonteCarloPayoff& payoff,
                                                double confidence_z_score) {
    return monte_carlo_estimate(
        spot,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        payoff);
}

MonteCarloGreeks monte_carlo_greeks_fd_generic(double spot,
                                               double strike,
                                               double risk_free_rate,
                                               double volatility,
                                               const std::vector<double>& time_steps,
                                               const std::vector<std::vector<double>>& normal_shock_paths,
                                               const MonteCarloPayoff& payoff,
                                               double confidence_z_score,
                                               double relative_bump,
                                               double time_bump_years) {
    return monte_carlo_finite_difference_greeks(
        spot,
        strike,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        relative_bump,
        time_bump_years,
        payoff);
}

double monte_carlo_up_and_out_call_price(double spot,
                                         double strike,
                                         double barrier,
                                         double risk_free_rate,
                                         double volatility,
                                         const std::vector<double>& time_steps,
                                         const std::vector<std::vector<double>>& normal_shock_paths) {
    return monte_carlo_up_and_out_call_estimate(
               spot,
               strike,
               barrier,
               risk_free_rate,
               volatility,
               time_steps,
               normal_shock_paths,
               kDefaultConfidenceZScore)
        .price;
}

double monte_carlo_down_and_out_put_price(double spot,
                                          double strike,
                                          double barrier,
                                          double risk_free_rate,
                                          double volatility,
                                          const std::vector<double>& time_steps,
                                          const std::vector<std::vector<double>>& normal_shock_paths) {
    return monte_carlo_down_and_out_put_estimate(
               spot,
               strike,
               barrier,
               risk_free_rate,
               volatility,
               time_steps,
               normal_shock_paths,
               kDefaultConfidenceZScore)
        .price;
}

MonteCarloGreeks monte_carlo_down_and_out_call_greeks_fd(double spot,
                                                         double strike,
                                                         double barrier,
                                                         double risk_free_rate,
                                                         double volatility,
                                                         const std::vector<double>& time_steps,
                                                         const std::vector<std::vector<double>>& normal_shock_paths,
                                                         double confidence_z_score,
                                                         double relative_bump,
                                                         double time_bump_years) {
    return monte_carlo_finite_difference_greeks(
        spot,
        strike,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        relative_bump,
        time_bump_years,
        [strike, barrier](const std::vector<double>& path) {
            return down_and_out_call_payoff(path, strike, barrier);
        });
}

MonteCarloGreeks monte_carlo_up_and_out_call_greeks_fd(double spot,
                                                       double strike,
                                                       double barrier,
                                                       double risk_free_rate,
                                                       double volatility,
                                                       const std::vector<double>& time_steps,
                                                       const std::vector<std::vector<double>>& normal_shock_paths,
                                                       double confidence_z_score,
                                                       double relative_bump,
                                                       double time_bump_years) {
    return monte_carlo_finite_difference_greeks(
        spot,
        strike,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        relative_bump,
        time_bump_years,
        [strike, barrier](const std::vector<double>& path) {
            return up_and_out_call_path_payoff(path, strike, barrier);
        });
}

MonteCarloGreeks monte_carlo_down_and_out_put_greeks_fd(double spot,
                                                        double strike,
                                                        double barrier,
                                                        double risk_free_rate,
                                                        double volatility,
                                                        const std::vector<double>& time_steps,
                                                        const std::vector<std::vector<double>>& normal_shock_paths,
                                                        double confidence_z_score,
                                                        double relative_bump,
                                                        double time_bump_years) {
    return monte_carlo_finite_difference_greeks(
        spot,
        strike,
        risk_free_rate,
        volatility,
        time_steps,
        normal_shock_paths,
        confidence_z_score,
        relative_bump,
        time_bump_years,
        [strike, barrier](const std::vector<double>& path) {
            return down_and_out_put_payoff(path, strike, barrier);
        });
}

} // namespace finance