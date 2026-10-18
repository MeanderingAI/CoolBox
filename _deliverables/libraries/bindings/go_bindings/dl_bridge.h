#ifndef COOLBOX_GO_DL_BRIDGE_H
#define COOLBOX_GO_DL_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* coolbox_dl_tensor_new(const size_t* shape, size_t dimensions,
                            const double* values, size_t value_count,
                            char* error, size_t error_capacity);
void* coolbox_dl_tensor_clone(void* tensor, char* error, size_t error_capacity);
void* coolbox_dl_tensor_reshape(void* tensor, const size_t* shape, size_t dimensions,
                                char* error, size_t error_capacity);
void* coolbox_dl_tensor_transpose(void* tensor, char* error, size_t error_capacity);
void* coolbox_dl_tensor_binary(void* lhs, void* rhs, int operation,
                               char* error, size_t error_capacity);
void* coolbox_dl_tensor_scalar(void* tensor, double scalar, int divide,
                               char* error, size_t error_capacity);
void* coolbox_dl_tensor_matmul(void* lhs, void* rhs,
                               char* error, size_t error_capacity);
void coolbox_dl_tensor_free(void* tensor);
size_t coolbox_dl_tensor_dimensions(void* tensor);
size_t coolbox_dl_tensor_size(void* tensor);
int coolbox_dl_tensor_shape(void* tensor, size_t* shape, size_t capacity,
                            char* error, size_t error_capacity);
int coolbox_dl_tensor_data(void* tensor, double* values, size_t capacity,
                           char* error, size_t error_capacity);
int coolbox_dl_tensor_fill(void* tensor, double value,
                           char* error, size_t error_capacity);
int coolbox_dl_tensor_randomize(void* tensor, double minimum, double maximum,
                                char* error, size_t error_capacity);
int coolbox_dl_tensor_at(void* tensor, size_t index, double* value,
                         char* error, size_t error_capacity);
int coolbox_dl_tensor_set(void* tensor, size_t index, double value,
                          char* error, size_t error_capacity);

void* coolbox_dl_layer_new(int kind, const size_t* parameters, size_t parameter_count,
                           double rate, int return_sequences,
                           char* error, size_t error_capacity);
void coolbox_dl_layer_free(void* layer);
void* coolbox_dl_layer_forward(void* layer, void* input,
                               char* error, size_t error_capacity);
void* coolbox_dl_layer_backward(void* layer, void* gradient,
                                char* error, size_t error_capacity);
int coolbox_dl_layer_update(void* layer, double learning_rate,
                            char* error, size_t error_capacity);
int coolbox_dl_layer_set_training(void* layer, int training,
                                  char* error, size_t error_capacity);
int coolbox_dl_layer_has_parameters(void* layer);
int coolbox_dl_layer_name(void* layer, char* output, size_t capacity,
                          char* error, size_t error_capacity);

void* coolbox_dl_loss_new(int kind, char* error, size_t error_capacity);
void coolbox_dl_loss_free(void* loss);
int coolbox_dl_loss_compute(void* loss, void* predictions, void* targets,
                            double* value, char* error, size_t error_capacity);
void* coolbox_dl_loss_gradient(void* loss, void* predictions, void* targets,
                               char* error, size_t error_capacity);

void* coolbox_dl_optimizer_new(int kind, double learning_rate, double parameter1,
                               double parameter2, char* error, size_t error_capacity);
void coolbox_dl_optimizer_free(void* optimizer);
int coolbox_dl_optimizer_step(void* optimizer, void* parameters, void* gradients,
                              char* error, size_t error_capacity);
int coolbox_dl_optimizer_reset(void* optimizer, char* error, size_t error_capacity);
int coolbox_dl_optimizer_name(void* optimizer, char* output, size_t capacity,
                              char* error, size_t error_capacity);

void* coolbox_dl_network_new(char* error, size_t error_capacity);
void coolbox_dl_network_free(void* network);
int coolbox_dl_network_add_layer(void* network, void* layer,
                                 char* error, size_t error_capacity);
int coolbox_dl_network_set_loss(void* network, void* loss,
                                char* error, size_t error_capacity);
int coolbox_dl_network_set_optimizer(void* network, void* optimizer,
                                     char* error, size_t error_capacity);
void* coolbox_dl_network_predict(void* network, void* input,
                                 char* error, size_t error_capacity);
int coolbox_dl_network_train_step(void* network, void* input, void* target,
                                  double* value, char* error, size_t error_capacity);
int coolbox_dl_network_evaluate(void* network, void** inputs, void** targets,
                                size_t count, double* value,
                                char* error, size_t error_capacity);
int coolbox_dl_network_train(void* network, void** inputs, void** targets,
                             size_t count, int epochs, int batch_size, int verbose,
                             char* error, size_t error_capacity);
int coolbox_dl_network_set_training(void* network, int training,
                                    char* error, size_t error_capacity);
size_t coolbox_dl_network_layer_count(void* network);

void* coolbox_dl_helix_new(size_t actions, size_t truth, int rich_feedback,
                           char* error, size_t error_capacity);
void coolbox_dl_helix_free(void* helix);
int coolbox_dl_helix_select_action(void* helix, size_t* action,
                                   char* error, size_t error_capacity);
int coolbox_dl_helix_observe(void* helix, size_t action, const char* feedback,
                             char* error, size_t error_capacity);
int coolbox_dl_helix_interact(void* helix, size_t* action,
                              char* error, size_t error_capacity);
size_t coolbox_dl_helix_rounds(void* helix);
size_t coolbox_dl_helix_mistakes(void* helix);
double coolbox_dl_helix_cumulative_regret(void* helix);
int coolbox_dl_helix_identified(void* helix);
size_t coolbox_dl_helix_version_space(void* helix, size_t* indices, size_t capacity);
int coolbox_dl_helix_dimension(size_t actions, size_t truth, int rich_feedback,
                               double epsilon, size_t max_actions, size_t* dimension,
                               char* error, size_t error_capacity);

#ifdef __cplusplus
}
#endif

#endif
