#ifndef COOLBOX_OPT_MATH_FINANCE_BRIDGE_H
#define COOLBOX_OPT_MATH_FINANCE_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*coolbox_objective_fn)(uintptr_t user_data, const double* values, size_t count);
typedef int (*coolbox_payoff_fn)(uintptr_t user_data, const double* path, size_t count,
                                 double* payoff);
typedef int (*coolbox_nowa_loss_fn)(uintptr_t user_data, const double* values,
                                    const size_t* layer_sizes, size_t layer_count,
                                    double* loss);
double coolbox_go_objective_trampoline(uintptr_t user_data, const double* values,
                                       size_t count);
coolbox_objective_fn coolbox_go_objective_callback(void);
int coolbox_go_payoff_trampoline(uintptr_t user_data, const double* path,
                                 size_t count, double* payoff);
coolbox_payoff_fn coolbox_go_payoff_callback(void);
int coolbox_go_nowa_loss_trampoline(uintptr_t user_data, const double* values,
                                    const size_t* layer_sizes, size_t layer_count,
                                    double* loss);
coolbox_nowa_loss_fn coolbox_go_nowa_loss_callback(void);

typedef struct coolbox_estimate {
    double price;
    double standard_error;
    double confidence_interval_low;
    double confidence_interval_high;
    size_t sample_count;
} coolbox_estimate;

typedef struct coolbox_greeks {
    coolbox_estimate delta;
    coolbox_estimate gamma;
    coolbox_estimate vega;
    coolbox_estimate theta;
    coolbox_estimate rho;
} coolbox_greeks;

void* coolbox_optimizer_create(int kind, const double* config, size_t config_count,
                               char* error, size_t error_capacity);
void coolbox_optimizer_free(void* optimizer);
int coolbox_optimizer_run(void* optimizer, coolbox_objective_fn objective,
                          uintptr_t user_data, const double* initial, size_t count,
                          char* error, size_t error_capacity);
size_t coolbox_optimizer_solution_size(void* optimizer);
int coolbox_optimizer_copy_solution(void* optimizer, double* output, size_t count,
                                    char* error, size_t error_capacity);
double coolbox_optimizer_best_score(void* optimizer);
double coolbox_optimizer_best_probability(void* optimizer);
int coolbox_optimizer_quantum_bounds(void* optimizer, double* output, size_t count,
                                     char* error, size_t error_capacity);
int coolbox_optimizer_warp_point(void* optimizer, const double* input, size_t count,
                                 double* output, int unwarp,
                                 char* error, size_t error_capacity);

int coolbox_math_heat(int dimensions, int stochastic, const double* input,
                      size_t nx, size_t ny, size_t nz, double alpha, double sigma,
                      double dx, double dy, double dz, double dt, int steps,
                      unsigned int seed, double* output,
                      char* error, size_t error_capacity);

double coolbox_finance_formula(int operation, const double* values, size_t count,
                               char* error, size_t error_capacity);
int coolbox_finance_path(int geometric, double initial_value, double drift,
                         double volatility, const double* time_steps,
                         size_t step_count, const double* normal_shocks,
                         double* output, char* error, size_t error_capacity);
int coolbox_finance_ito(int variance, const double* values, const double* increments,
                        size_t count, double* output, char* error,
                        size_t error_capacity);
int coolbox_finance_monte_carlo(int operation, double spot, double strike,
                                double barrier, double risk_free_rate,
                                double volatility, const double* time_steps,
                                size_t step_count, const double* shock_paths,
                                size_t path_count, double confidence_z_score,
                                double relative_bump, double time_bump_years,
                                coolbox_estimate* estimate,
                                char* error, size_t error_capacity);
int coolbox_finance_monte_carlo_greeks(int operation, double spot, double strike,
                                       double barrier, double risk_free_rate,
                                       double volatility, const double* time_steps,
                                       size_t step_count, const double* shock_paths,
                                       size_t path_count, double confidence_z_score,
                                       double relative_bump, double time_bump_years,
                                       coolbox_greeks* greeks, char* error,
                                       size_t error_capacity);
int coolbox_finance_monte_carlo_generic(double spot, double risk_free_rate,
                                        double volatility, const double* time_steps,
                                        size_t step_count, const double* shock_paths,
                                        size_t path_count, double confidence_z_score,
                                        coolbox_payoff_fn payoff, uintptr_t user_data,
                                        coolbox_estimate* estimate, char* error,
                                        size_t error_capacity);
int coolbox_finance_monte_carlo_greeks_generic(
    double spot, double strike, double risk_free_rate, double volatility,
    const double* time_steps, size_t step_count, const double* shock_paths,
    size_t path_count, double confidence_z_score, double relative_bump,
    double time_bump_years, coolbox_payoff_fn payoff, uintptr_t user_data,
    coolbox_greeks* greeks, char* error, size_t error_capacity);

int coolbox_owa_naive_average(const double* local_parameters, size_t model_count,
                              size_t parameter_count, double* output,
                              char* error, size_t error_capacity);
int coolbox_owa_linear_regression(const double* local_parameters,
                                  size_t model_count, size_t parameter_count,
                                  const double* features, size_t row_count,
                                  size_t feature_count, const double* targets,
                                  size_t target_count, int iterations,
                                  double learning_rate, double regularization,
                                  int project_to_simplex, double* merged,
                                  double* weights, double* validation_loss,
                                  char* error, size_t error_capacity);
int coolbox_owa_cross_validation_linear_regression(
    const double* local_parameters, size_t model_count, size_t parameter_count,
    const double* fold_features, const size_t* fold_row_offsets,
    const double* fold_targets, const size_t* fold_target_offsets,
    size_t fold_count, size_t feature_count, int iterations, double learning_rate,
    double regularization, int project_to_simplex, double* fold_losses,
    double* mean_loss, char* error, size_t error_capacity);
int coolbox_owa_nonlinear_layers(
    const double* local_layers, size_t model_count, size_t layer_count,
    const size_t* layer_shapes, const size_t* layer_shape_offsets,
    const size_t* layer_sizes, const size_t* layer_value_offsets,
    const size_t* layer_dimensions, int iterations, double learning_rate,
    double regularization, int project_to_simplex, coolbox_nowa_loss_fn loss_fn,
    uintptr_t user_data, double* merged_layers, double* layer_weights,
    double* validation_loss, char* error, size_t error_capacity);
int coolbox_owa_weighted_tensor_average(
    const double* local_tensors, size_t model_count, const size_t* shape,
    size_t shape_dimensions, const double* weights, size_t tensor_size,
    double* merged_tensor, char* error, size_t error_capacity);

#ifdef __cplusplus
}
#endif

#endif
