#include "time_series.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

using ml::time_series::TimeSeries;

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

void* coolbox_series_new(const double* values, size_t count) {
    if (count != 0 && values == nullptr) {
        return nullptr;
    }
    try {
        std::vector<double> data;
        if (count != 0) {
            data.assign(values, values + count);
        }
        return new TimeSeries(data);
    } catch (...) {
        return nullptr;
    }
}

void coolbox_series_free(void* handle) {
    delete static_cast<TimeSeries*>(handle);
}

size_t coolbox_series_size(const void* handle) {
    return handle == nullptr ? 0 : static_cast<const TimeSeries*>(handle)->size();
}

int coolbox_series_copy_values(const void* handle, double* destination, size_t capacity) {
    if (handle == nullptr) {
        return 0;
    }
    const auto& values = static_cast<const TimeSeries*>(handle)->values();
    if (capacity < values.size() || (values.size() != 0 && destination == nullptr)) {
        return 0;
    }
    std::copy(values.begin(), values.end(), destination);
    return 1;
}

int coolbox_series_statistic(const void* handle, int statistic, double* result) {
    if (handle == nullptr || result == nullptr || statistic < 0 || statistic > 4) {
        return 0;
    }
    const auto& series = *static_cast<const TimeSeries*>(handle);
    switch (statistic) {
    case 0: *result = series.mean(); break;
    case 1: *result = series.std(); break;
    case 2: *result = series.min(); break;
    case 3: *result = series.max(); break;
    case 4: *result = series.median(); break;
    }
    return 1;
}

size_t coolbox_series_autocorrelation_size(const void* handle, size_t max_lag) {
    if (handle == nullptr) {
        return 0;
    }
    const size_t size = static_cast<const TimeSeries*>(handle)->size();
    return size == 0 ? 0 : std::min(size, max_lag + 1);
}

void* coolbox_series_transform(void* handle,
                               int operation,
                               size_t integer_argument,
                               double first_argument,
                               double second_argument,
                               char* error,
                               size_t error_capacity) {
    if (handle == nullptr) {
        set_error(error, error_capacity, "time series is closed");
        return nullptr;
    }
    try {
        const auto& series = *static_cast<const TimeSeries*>(handle);
        TimeSeries transformed;
        switch (operation) {
        case 0: transformed = series.normalize(); break;
        case 1: transformed = series.min_max_scale(first_argument, second_argument); break;
        case 2: transformed = series.diff(integer_argument); break;
        case 3: transformed = series.log_transform(); break;
        case 4: transformed = series.moving_average(integer_argument); break;
        case 5: transformed = series.exponential_smoothing(first_argument); break;
        case 6:
            if (integer_argument == 1) {
                transformed = series.size() == 0
                    ? TimeSeries()
                    : TimeSeries({series.at(0)});
            } else {
                transformed = series.resample(integer_argument);
            }
            break;
        default:
            throw std::invalid_argument("unknown time-series transformation");
        }
        return new TimeSeries(std::move(transformed));
    } catch (const std::exception& exception) {
        set_error(error, error_capacity, exception.what());
    } catch (...) {
        set_error(error, error_capacity, "unknown native exception");
    }
    return nullptr;
}

int coolbox_series_copy_autocorrelation(const void* handle,
                                        size_t max_lag,
                                        double* destination,
                                        size_t capacity,
                                        char* error,
                                        size_t error_capacity) {
    return invoke(error, error_capacity, [&] {
        if (handle == nullptr) {
            throw std::invalid_argument("time series is closed");
        }
        const auto values = static_cast<const TimeSeries*>(handle)->autocorrelation(max_lag);
        if (capacity < values.size() || (!values.empty() && destination == nullptr)) {
            throw std::invalid_argument("autocorrelation output buffer is too small");
        }
        std::copy(values.begin(), values.end(), destination);
    });
}

} // extern "C"
