#include "opt_math_finance_bridge.h"

extern "C" double coolboxGoObjective(uintptr_t user_data, const double* values,
                                      size_t count);
extern "C" int coolboxGoPayoff(uintptr_t user_data, const double* path,
                                size_t count, double* payoff);
extern "C" int coolboxGoNowaLoss(uintptr_t user_data, const double* values,
                                  const size_t* layer_sizes, size_t layer_count,
                                  double* loss);

extern "C" double coolbox_go_objective_trampoline(uintptr_t user_data,
                                                   const double* values,
                                                   size_t count) {
    return coolboxGoObjective(user_data, values, count);
}

extern "C" coolbox_objective_fn coolbox_go_objective_callback(void) {
    return coolbox_go_objective_trampoline;
}

extern "C" int coolbox_go_payoff_trampoline(uintptr_t user_data,
                                             const double* path, size_t count,
                                             double* payoff) {
    return coolboxGoPayoff(user_data, path, count, payoff);
}

extern "C" coolbox_payoff_fn coolbox_go_payoff_callback(void) {
    return coolbox_go_payoff_trampoline;
}

extern "C" int coolbox_go_nowa_loss_trampoline(
    uintptr_t user_data, const double* values, const size_t* layer_sizes,
    size_t layer_count, double* loss) {
    return coolboxGoNowaLoss(user_data, values, layer_sizes, layer_count, loss);
}

extern "C" coolbox_nowa_loss_fn coolbox_go_nowa_loss_callback(void) {
    return coolbox_go_nowa_loss_trampoline;
}
