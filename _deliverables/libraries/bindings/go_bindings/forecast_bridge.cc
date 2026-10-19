#include "arima.h"
#include "arma.h"
#include "time_series.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace {

using ForecasterModel = std::variant<
    ml::time_series::MovingAverageForecaster,
    ml::time_series::ExponentialSmoothingForecaster,
    ml::time_series::AutoRegressiveModel,
    ml::time_series::ARMAModel,
    ml::time_series::ARIMAModel>;

struct ForecasterHandle {
    explicit ForecasterHandle(ForecasterModel model, int kind, size_t differencing_order)
        : model(std::move(model)), kind(kind), differencing_order(differencing_order) {}
    ForecasterModel model;
    int kind;
    size_t differencing_order;
    bool fitted = false;
};

void set_error(char* buffer, size_t capacity, const char* message) {
    if (buffer == nullptr || capacity == 0) {
        return;
    }
    const size_t length = std::min(capacity - 1, std::strlen(message));
    std::memcpy(buffer, message, length);
    buffer[length] = '\0';
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

} // namespace

extern "C" {

void* coolbox_forecaster_new(int kind,
                            size_t first_order,
                            size_t second_order,
                            size_t third_order,
                            double alpha,
                            double beta,
                            double gamma) {
    try {
        switch (kind) {
        case 0:
            return new ForecasterHandle(
                ml::time_series::MovingAverageForecaster(first_order), kind, 0);
        case 1:
            return new ForecasterHandle(
                ml::time_series::ExponentialSmoothingForecaster(alpha, beta, gamma), kind, 0);
        case 2:
            return new ForecasterHandle(
                ml::time_series::AutoRegressiveModel(first_order), kind, 0);
        case 3:
            return new ForecasterHandle(
                ml::time_series::ARMAModel(first_order, second_order), kind, 0);
        case 4:
            return new ForecasterHandle(
                ml::time_series::ARIMAModel(first_order, second_order, third_order), kind, second_order);
        default:
            return nullptr;
        }
    } catch (...) {
        return nullptr;
    }
}

void coolbox_forecaster_free(void* handle) {
    delete static_cast<ForecasterHandle*>(handle);
}

int coolbox_forecaster_fit(void* handle,
                           const double* values,
                           size_t count,
                           char* error,
                           size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (handle == nullptr || values == nullptr || count == 0) {
            throw std::invalid_argument("forecaster and non-empty observations are required");
        }
        auto& forecaster = *static_cast<ForecasterHandle*>(handle);
        if (forecaster.kind == 4 && forecaster.differencing_order >= count) {
            throw std::invalid_argument("ARIMA differencing order must be smaller than the observation count");
        }
        ml::time_series::TimeSeries series(std::vector<double>(values, values + count));
        std::visit([&](auto& model) { model.fit(series); }, forecaster.model);
        forecaster.fitted = true;
    });
}

int coolbox_forecaster_forecast(void* handle,
                                size_t steps,
                                double* destination,
                                size_t capacity,
                                char* error,
                                size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (handle == nullptr) {
            throw std::invalid_argument("forecaster is closed");
        }
        auto& forecaster = *static_cast<ForecasterHandle*>(handle);
        if (!forecaster.fitted) {
            throw std::logic_error("forecaster has not been fitted");
        }
        const auto predictions = std::visit(
            [&](const auto& model) { return model.forecast(steps); },
            forecaster.model);
        if (capacity < predictions.size() ||
            (!predictions.empty() && destination == nullptr)) {
            throw std::invalid_argument("forecast output buffer is too small");
        }
        std::copy(predictions.begin(), predictions.end(), destination);
    });
}

int coolbox_moving_average_forecast_one_step(void* handle,
                                             double* prediction,
                                             char* error,
                                             size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (handle == nullptr || prediction == nullptr) {
            throw std::invalid_argument("forecaster and prediction output are required");
        }
        auto& forecaster = *static_cast<ForecasterHandle*>(handle);
        if (!forecaster.fitted ||
            !std::holds_alternative<ml::time_series::MovingAverageForecaster>(forecaster.model)) {
            throw std::logic_error("fitted moving-average forecaster is required");
        }
        *prediction = std::get<ml::time_series::MovingAverageForecaster>(
            forecaster.model).forecast_one_step();
    });
}

} // extern "C"
