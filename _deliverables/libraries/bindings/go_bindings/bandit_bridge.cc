#include "bandit_arm.h"

#include <exception>

extern "C" {

void* coolbox_bandit_arm_new(double true_probability) {
    try {
        return new BanditArm(true_probability);
    } catch (...) {
        return nullptr;
    }
}

void coolbox_bandit_arm_free(void* handle) {
    delete static_cast<BanditArm*>(handle);
}

int coolbox_bandit_arm_pull(void* handle, double* reward) {
    if (handle == nullptr || reward == nullptr) {
        return 0;
    }
    try {
        *reward = static_cast<BanditArm*>(handle)->pull();
        return 1;
    } catch (...) {
        return 0;
    }
}

int coolbox_bandit_arm_update(void* handle, double reward) {
    if (handle == nullptr) {
        return 0;
    }
    try {
        static_cast<BanditArm*>(handle)->update(reward);
        return 1;
    } catch (...) {
        return 0;
    }
}

double coolbox_bandit_arm_estimated_probability(const void* handle) {
    return handle == nullptr ? 0.0 : static_cast<const BanditArm*>(handle)->get_estimated_prob();
}

int coolbox_bandit_arm_pull_count(const void* handle) {
    return handle == nullptr ? 0 : static_cast<const BanditArm*>(handle)->get_pull_count();
}

double coolbox_bandit_arm_true_probability(const void* handle) {
    return handle == nullptr ? 0.0 : static_cast<const BanditArm*>(handle)->get_true_prob();
}

} // extern "C"
