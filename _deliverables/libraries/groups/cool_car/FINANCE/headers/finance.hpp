#ifndef COOLBOX__DELIVERABLES__LIBRARIES__GROUPS__COOL_CAR__FINANCE__HEADERS__FINANCE_HPP
#define COOLBOX__DELIVERABLES__LIBRARIES__GROUPS__COOL_CAR__FINANCE__HEADERS__FINANCE_HPP

#include <cstddef>
#include <functional>
#include <vector>

namespace finance {

struct MonteCarloEstimate {
    double price = 0.0;
    double standard_error = 0.0;
    double confidence_interval_low = 0.0;
    double confidence_interval_high = 0.0;
    std::size_t sample_count = 0U;
};

struct MonteCarloGreeks {
    MonteCarloEstimate delta;
    MonteCarloEstimate gamma;
    MonteCarloEstimate vega;
    MonteCarloEstimate theta;
    MonteCarloEstimate rho;
};

double standard_normal_pdf(double value);
double standard_normal_cdf(double value);

double black_scholes_d1(double spot,
                        double strike,
                        double risk_free_rate,
                        double volatility,
                        double maturity_years);
double black_scholes_d2(double spot,
                        double strike,
                        double risk_free_rate,
                        double volatility,
                        double maturity_years);
double black_scholes_call_price(double spot,
                                double strike,
                                double risk_free_rate,
                                double volatility,
                                double maturity_years);
double black_scholes_put_price(double spot,
                               double strike,
                               double risk_free_rate,
                               double volatility,
                               double maturity_years);
double black_scholes_call_delta(double spot,
                                double strike,
                                double risk_free_rate,
                                double volatility,
                                double maturity_years);
double black_scholes_put_delta(double spot,
                               double strike,
                               double risk_free_rate,
                               double volatility,
                               double maturity_years);
double black_scholes_gamma(double spot,
                           double strike,
                           double risk_free_rate,
                           double volatility,
                           double maturity_years);
double black_scholes_vega(double spot,
                          double strike,
                          double risk_free_rate,
                          double volatility,
                          double maturity_years);
double black_scholes_call_theta(double spot,
                                double strike,
                                double risk_free_rate,
                                double volatility,
                                double maturity_years);
double black_scholes_put_theta(double spot,
                               double strike,
                               double risk_free_rate,
                               double volatility,
                               double maturity_years);
double black_scholes_call_rho(double spot,
                              double strike,
                              double risk_free_rate,
                              double volatility,
                              double maturity_years);
double black_scholes_put_rho(double spot,
                             double strike,
                             double risk_free_rate,
                             double volatility,
                             double maturity_years);

double black_scholes_digital_call_price(double spot,
                                        double strike,
                                        double risk_free_rate,
                                        double volatility,
                                        double maturity_years,
                                        double cash_payoff = 1.0);
double black_scholes_digital_put_price(double spot,
                                       double strike,
                                       double risk_free_rate,
                                       double volatility,
                                       double maturity_years,
                                       double cash_payoff = 1.0);
double geometric_asian_call_price(double spot,
                                  double strike,
                                  double risk_free_rate,
                                  double volatility,
                                  double maturity_years);
double geometric_asian_put_price(double spot,
                                 double strike,
                                 double risk_free_rate,
                                 double volatility,
                                 double maturity_years);
double black_scholes_down_and_in_call_price(double spot,
                                            double strike,
                                            double barrier,
                                            double risk_free_rate,
                                            double volatility,
                                            double maturity_years);
double black_scholes_down_and_out_call_price(double spot,
                                             double strike,
                                             double barrier,
                                             double risk_free_rate,
                                             double volatility,
                                             double maturity_years);
double black_scholes_up_and_in_call_price(double spot,
                                          double strike,
                                          double barrier,
                                          double risk_free_rate,
                                          double volatility,
                                          double maturity_years);
double black_scholes_up_and_out_call_price(double spot,
                                           double strike,
                                           double barrier,
                                           double risk_free_rate,
                                           double volatility,
                                           double maturity_years);
double black_scholes_down_and_in_put_price(double spot,
                                           double strike,
                                           double barrier,
                                           double risk_free_rate,
                                           double volatility,
                                           double maturity_years);
double black_scholes_down_and_out_put_price(double spot,
                                            double strike,
                                            double barrier,
                                            double risk_free_rate,
                                            double volatility,
                                            double maturity_years);

std::vector<double> brownian_motion_path(const std::vector<double>& time_steps,
                                         const std::vector<double>& normal_shocks,
                                         double initial_value = 0.0);
std::vector<double> geometric_brownian_motion_path(double initial_value,
                                                   double drift,
                                                   double volatility,
                                                   const std::vector<double>& time_steps,
                                                   const std::vector<double>& normal_shocks);

MonteCarloEstimate monte_carlo_european_call_estimate(double spot,
                                                      double strike,
                                                      double risk_free_rate,
                                                      double volatility,
                                                      const std::vector<double>& time_steps,
                                                      const std::vector<std::vector<double>>& normal_shock_paths,
                                                      double confidence_z_score = 1.959963984540054);
MonteCarloEstimate monte_carlo_european_put_estimate(double spot,
                                                     double strike,
                                                     double risk_free_rate,
                                                     double volatility,
                                                     const std::vector<double>& time_steps,
                                                     const std::vector<std::vector<double>>& normal_shock_paths,
                                                     double confidence_z_score = 1.959963984540054);
MonteCarloEstimate monte_carlo_arithmetic_asian_call_estimate(double spot,
                                                              double strike,
                                                              double risk_free_rate,
                                                              double volatility,
                                                              const std::vector<double>& time_steps,
                                                              const std::vector<std::vector<double>>& normal_shock_paths,
                                                              double confidence_z_score = 1.959963984540054);
MonteCarloEstimate monte_carlo_down_and_out_call_estimate(double spot,
                                                          double strike,
                                                          double barrier,
                                                          double risk_free_rate,
                                                          double volatility,
                                                          const std::vector<double>& time_steps,
                                                          const std::vector<std::vector<double>>& normal_shock_paths,
                                                          double confidence_z_score = 1.959963984540054);
MonteCarloEstimate monte_carlo_up_and_out_call_estimate(double spot,
                                                        double strike,
                                                        double barrier,
                                                        double risk_free_rate,
                                                        double volatility,
                                                        const std::vector<double>& time_steps,
                                                        const std::vector<std::vector<double>>& normal_shock_paths,
                                                        double confidence_z_score = 1.959963984540054);
MonteCarloEstimate monte_carlo_down_and_out_put_estimate(double spot,
                                                         double strike,
                                                         double barrier,
                                                         double risk_free_rate,
                                                         double volatility,
                                                         const std::vector<double>& time_steps,
                                                         const std::vector<std::vector<double>>& normal_shock_paths,
                                                         double confidence_z_score = 1.959963984540054);

using MonteCarloPayoff = std::function<double(const std::vector<double>&)>;

MonteCarloEstimate monte_carlo_estimate_generic(double spot,
                                                double risk_free_rate,
                                                double volatility,
                                                const std::vector<double>& time_steps,
                                                const std::vector<std::vector<double>>& normal_shock_paths,
                                                const MonteCarloPayoff& payoff,
                                                double confidence_z_score = 1.959963984540054);
MonteCarloGreeks monte_carlo_greeks_fd_generic(double spot,
                                               double strike,
                                               double risk_free_rate,
                                               double volatility,
                                               const std::vector<double>& time_steps,
                                               const std::vector<std::vector<double>>& normal_shock_paths,
                                               const MonteCarloPayoff& payoff,
                                               double confidence_z_score = 1.959963984540054,
                                               double relative_bump = 0.01,
                                               double time_bump_years = 0.01);

double monte_carlo_european_call_price(double spot,
                                       double strike,
                                       double risk_free_rate,
                                       double volatility,
                                       const std::vector<double>& time_steps,
                                       const std::vector<std::vector<double>>& normal_shock_paths);
double monte_carlo_european_put_price(double spot,
                                      double strike,
                                      double risk_free_rate,
                                      double volatility,
                                      const std::vector<double>& time_steps,
                                      const std::vector<std::vector<double>>& normal_shock_paths);
double monte_carlo_arithmetic_asian_call_price(double spot,
                                               double strike,
                                               double risk_free_rate,
                                               double volatility,
                                               const std::vector<double>& time_steps,
                                               const std::vector<std::vector<double>>& normal_shock_paths);
double monte_carlo_down_and_out_call_price(double spot,
                                           double strike,
                                           double barrier,
                                           double risk_free_rate,
                                           double volatility,
                                           const std::vector<double>& time_steps,
                                           const std::vector<std::vector<double>>& normal_shock_paths);
double monte_carlo_up_and_out_call_price(double spot,
                                         double strike,
                                         double barrier,
                                         double risk_free_rate,
                                         double volatility,
                                         const std::vector<double>& time_steps,
                                         const std::vector<std::vector<double>>& normal_shock_paths);
double monte_carlo_down_and_out_put_price(double spot,
                                          double strike,
                                          double barrier,
                                          double risk_free_rate,
                                          double volatility,
                                          const std::vector<double>& time_steps,
                                          const std::vector<std::vector<double>>& normal_shock_paths);

MonteCarloGreeks monte_carlo_european_call_greeks_fd(double spot,
                                                     double strike,
                                                     double risk_free_rate,
                                                     double volatility,
                                                     const std::vector<double>& time_steps,
                                                     const std::vector<std::vector<double>>& normal_shock_paths,
                                                     double confidence_z_score = 1.959963984540054,
                                                     double relative_bump = 0.01,
                                                     double time_bump_years = 0.01);
MonteCarloGreeks monte_carlo_arithmetic_asian_call_greeks_fd(double spot,
                                                             double strike,
                                                             double risk_free_rate,
                                                             double volatility,
                                                             const std::vector<double>& time_steps,
                                                             const std::vector<std::vector<double>>& normal_shock_paths,
                                                             double confidence_z_score = 1.959963984540054,
                                                             double relative_bump = 0.01,
                                                             double time_bump_years = 0.01);
MonteCarloGreeks monte_carlo_down_and_out_call_greeks_fd(double spot,
                                                         double strike,
                                                         double barrier,
                                                         double risk_free_rate,
                                                         double volatility,
                                                         const std::vector<double>& time_steps,
                                                         const std::vector<std::vector<double>>& normal_shock_paths,
                                                         double confidence_z_score = 1.959963984540054,
                                                         double relative_bump = 0.01,
                                                         double time_bump_years = 0.01);
MonteCarloGreeks monte_carlo_up_and_out_call_greeks_fd(double spot,
                                                       double strike,
                                                       double barrier,
                                                       double risk_free_rate,
                                                       double volatility,
                                                       const std::vector<double>& time_steps,
                                                       const std::vector<std::vector<double>>& normal_shock_paths,
                                                       double confidence_z_score = 1.959963984540054,
                                                       double relative_bump = 0.01,
                                                       double time_bump_years = 0.01);
MonteCarloGreeks monte_carlo_down_and_out_put_greeks_fd(double spot,
                                                        double strike,
                                                        double barrier,
                                                        double risk_free_rate,
                                                        double volatility,
                                                        const std::vector<double>& time_steps,
                                                        const std::vector<std::vector<double>>& normal_shock_paths,
                                                        double confidence_z_score = 1.959963984540054,
                                                        double relative_bump = 0.01,
                                                        double time_bump_years = 0.01);

double ito_integral_left_point(const std::vector<double>& integrand_values,
                               const std::vector<double>& brownian_increments);
double ito_isometry_variance(const std::vector<double>& integrand_values,
                             const std::vector<double>& time_steps);

} // namespace finance

#endif