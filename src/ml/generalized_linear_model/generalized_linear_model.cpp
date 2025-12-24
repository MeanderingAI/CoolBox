
#include "ml/generalized_linear_model/generalized_linear_model.h"

/**
 * @brief Initialize the model parameters.
 * @param num_features The number of features.
 */
void GLM::initialize_parameters(int num_features) {
    // Initialize weights vector with zeros.
    weights_.assign(num_features, 0.0);
    // Initialize bias to zero.
    bias_ = 0.0;
}