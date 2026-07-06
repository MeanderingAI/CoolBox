#include "generalized_linear_model.h"

void GLM::initialize_parameters(int num_features) {
    weights_.assign(num_features, 0.0);
    bias_ = 0.0;
}
