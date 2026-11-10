#include "coolbox_jbridge.h"
#include "generics_timer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using generics::timer::ScopedTimer;
using generics::timer::Timer;
struct StatsHandle {
    bool concurrent;
    std::unique_ptr<generics::timer::TimingStats> plain;
    std::unique_ptr<generics::timer::ConcurrentTimingStats> safe;
    explicit StatsHandle(bool is_concurrent) : concurrent(is_concurrent) {
        if (concurrent) safe = std::make_unique<generics::timer::ConcurrentTimingStats>();
        else plain = std::make_unique<generics::timer::TimingStats>();
    }
    void record(int64_t ns) { if (concurrent) safe->record(std::chrono::nanoseconds(ns)); else plain->record(std::chrono::nanoseconds(ns)); }
    generics::timer::TimingSnapshot snapshot() const { return concurrent ? safe->snapshot() : plain->snapshot(); }
    void reset() { if (concurrent) safe->reset(); else plain->reset(); }
};
struct ScopedHandle {
    using Callback = void (*)(double, void*);
    Callback callback;
    void* user;
    std::unique_ptr<ScopedTimer> timer;
    ScopedHandle(Callback cb, void* data) : callback(cb), user(data), timer(std::make_unique<ScopedTimer>([this](std::chrono::duration<double, std::milli> elapsed) { if (callback) callback(elapsed.count(), user); })) {}
};
void require_count(size_t value, const char* name) { if (!value) throw std::invalid_argument(std::string(name) + " must be positive"); }
void require_output(double* output, size_t needed, size_t capacity) { if (capacity < needed || (needed && !output)) throw std::invalid_argument("output buffer is too small"); }
void require_output_i64(int64_t* output, size_t needed, size_t capacity) { if (capacity < needed || (needed && !output)) throw std::invalid_argument("output buffer is too small"); }
}

extern "C" {

void* coolbox_misc_timer_new() { return new Timer(); }
void coolbox_misc_timer_free(void* handle) { delete static_cast<Timer*>(handle); }
void coolbox_misc_timer_start(void* handle) { coolbox_jbridge::checked<Timer>(handle, "timer").start(); }
void coolbox_misc_timer_stop(void* handle) { coolbox_jbridge::checked<Timer>(handle, "timer").stop(); }
void coolbox_misc_timer_reset(void* handle) { coolbox_jbridge::checked<Timer>(handle, "timer").reset(); }
double coolbox_misc_timer_elapsed_ms(const void* handle) { return coolbox_jbridge::checked<Timer>(const_cast<void*>(handle), "timer").elapsed<std::chrono::duration<double, std::milli>>().count(); }
double coolbox_misc_timer_lap_ms(void* handle) { return coolbox_jbridge::checked<Timer>(handle, "timer").lap<std::chrono::duration<double, std::milli>>().count(); }
int coolbox_misc_timer_is_running(const void* handle) { return coolbox_jbridge::checked<Timer>(const_cast<void*>(handle), "timer").is_running() ? 1 : 0; }
void* coolbox_misc_stats_new(int concurrent) { return new StatsHandle(concurrent != 0); }
void coolbox_misc_stats_free(void* handle) { delete static_cast<StatsHandle*>(handle); }
void coolbox_misc_stats_record_ns(void* handle, int64_t ns) { coolbox_jbridge::checked<StatsHandle>(handle, "timing stats").record(ns); }
void coolbox_misc_stats_reset(void* handle) { coolbox_jbridge::checked<StatsHandle>(handle, "timing stats").reset(); }
uint64_t coolbox_misc_stats_count(const void* handle) { return coolbox_jbridge::checked<StatsHandle>(const_cast<void*>(handle), "timing stats").snapshot().count; }
int64_t coolbox_misc_stats_total_ns(const void* handle) { return coolbox_jbridge::checked<StatsHandle>(const_cast<void*>(handle), "timing stats").snapshot().total.count(); }
int64_t coolbox_misc_stats_min_ns(const void* handle) { return coolbox_jbridge::checked<StatsHandle>(const_cast<void*>(handle), "timing stats").snapshot().min.count(); }
int64_t coolbox_misc_stats_max_ns(const void* handle) { return coolbox_jbridge::checked<StatsHandle>(const_cast<void*>(handle), "timing stats").snapshot().max.count(); }
void* coolbox_misc_snapshot_new(uint64_t count, int64_t total_ns, int64_t min_ns, int64_t max_ns) { return new generics::timer::TimingSnapshot{count, std::chrono::nanoseconds(total_ns), std::chrono::nanoseconds(min_ns), std::chrono::nanoseconds(max_ns)}; }
void coolbox_misc_snapshot_free(void* handle) { delete static_cast<generics::timer::TimingSnapshot*>(handle); }
uint64_t coolbox_misc_snapshot_count(const void* handle) { return coolbox_jbridge::checked<generics::timer::TimingSnapshot>(const_cast<void*>(handle), "snapshot").count; }
int64_t coolbox_misc_snapshot_total_ns(const void* handle) { return coolbox_jbridge::checked<generics::timer::TimingSnapshot>(const_cast<void*>(handle), "snapshot").total.count(); }
int64_t coolbox_misc_snapshot_min_ns(const void* handle) { return coolbox_jbridge::checked<generics::timer::TimingSnapshot>(const_cast<void*>(handle), "snapshot").min.count(); }
int64_t coolbox_misc_snapshot_max_ns(const void* handle) { return coolbox_jbridge::checked<generics::timer::TimingSnapshot>(const_cast<void*>(handle), "snapshot").max.count(); }
double coolbox_misc_snapshot_mean_ns(const void* handle) { return coolbox_jbridge::checked<generics::timer::TimingSnapshot>(const_cast<void*>(handle), "snapshot").mean_ns(); }
void* coolbox_misc_scoped_timer_new(ScopedHandle::Callback callback, void* user) { if (!callback) return nullptr; return new ScopedHandle(callback, user); }
double coolbox_misc_scoped_timer_elapsed_ms(const void* handle) { auto& scoped = coolbox_jbridge::checked<ScopedHandle>(const_cast<void*>(handle), "scoped timer"); return scoped.timer ? scoped.timer->elapsed<std::chrono::duration<double, std::milli>>().count() : 0.0; }
double coolbox_misc_scoped_timer_close(void* handle) { auto& scoped = coolbox_jbridge::checked<ScopedHandle>(handle, "scoped timer"); double elapsed = scoped.timer ? scoped.timer->elapsed<std::chrono::duration<double, std::milli>>().count() : 0.0; scoped.timer.reset(); return elapsed; }
void coolbox_misc_scoped_timer_free(void* handle) { delete static_cast<ScopedHandle*>(handle); }

int coolbox_misc_make_regression(size_t n, size_t features, double noise, double bias, uint64_t seed, double* x, size_t x_capacity, double* y, size_t y_capacity, char* error, size_t error_capacity) {
    return coolbox_jbridge::invoke(error, error_capacity, [&] {
        require_count(n, "n_samples"); require_count(features, "n_features"); if (noise < 0.0) throw std::invalid_argument("noise must be non-negative");
        if (features > std::numeric_limits<size_t>::max() / n) throw std::invalid_argument("dataset size overflows");
        require_output(x, n * features, x_capacity); require_output(y, n, y_capacity);
        std::mt19937_64 engine(seed); std::normal_distribution<double> feature(0.0, 1.0); std::normal_distribution<double> noise_dist(0.0, noise == 0.0 ? 1.0 : noise); std::uniform_real_distribution<double> coefficient(-10.0, 10.0);
        std::vector<double> weights(features); for (double& w : weights) w = coefficient(engine);
        for (size_t r = 0; r < n; ++r) { double target = bias; for (size_t c = 0; c < features; ++c) { double v = feature(engine); x[r * features + c] = v; target += v * weights[c]; } y[r] = target + (noise == 0.0 ? 0.0 : noise_dist(engine)); }
    });
}
int coolbox_misc_make_classification(size_t n, size_t features, size_t classes, double sep, uint64_t seed, double* x, size_t x_capacity, int64_t* labels, size_t label_capacity, char* error, size_t error_capacity) {
    return coolbox_jbridge::invoke(error, error_capacity, [&] {
        require_count(n, "n_samples"); require_count(features, "n_features"); if (classes < 2) throw std::invalid_argument("n_classes must be at least 2"); if (classes > n) throw std::invalid_argument("n_classes cannot exceed n_samples"); if (sep <= 0.0) throw std::invalid_argument("class_sep must be positive");
        if (features > std::numeric_limits<size_t>::max() / n || features > std::numeric_limits<size_t>::max() / classes) throw std::invalid_argument("dataset size overflows");
        require_output(x, n * features, x_capacity); require_output_i64(labels, n, label_capacity);
        std::mt19937_64 engine(seed); std::normal_distribution<double> sample(0.0, 1.0); std::uniform_real_distribution<double> center_dist(-sep, sep); std::vector<double> centers(classes * features); for (double& center : centers) center = center_dist(engine);
        for (size_t k = 0; k < classes; ++k) centers[k * features] = sep * (2.0 * static_cast<double>(k) / static_cast<double>(classes - 1) - 1.0);
        std::vector<int64_t> order(n); for (size_t r = 0; r < n; ++r) order[r] = static_cast<int64_t>(r % classes); std::shuffle(order.begin(), order.end(), engine);
        for (size_t r = 0; r < n; ++r) { auto label = order[r]; labels[r] = label; for (size_t c = 0; c < features; ++c) x[r * features + c] = centers[static_cast<size_t>(label) * features + c] + sample(engine); }
    });
}
int coolbox_misc_make_blobs(size_t n, size_t centers_count, size_t features, double cluster_std, uint64_t seed, double* x, size_t x_capacity, int64_t* labels, size_t label_capacity, char* error, size_t error_capacity) {
    return coolbox_jbridge::invoke(error, error_capacity, [&] {
        require_count(n, "n_samples"); require_count(centers_count, "centers"); require_count(features, "n_features"); if (centers_count > n) throw std::invalid_argument("centers cannot exceed n_samples"); if (cluster_std < 0.0) throw std::invalid_argument("cluster_std must be non-negative");
        if (features > std::numeric_limits<size_t>::max() / n || features > std::numeric_limits<size_t>::max() / centers_count) throw std::invalid_argument("dataset size overflows");
        require_output(x, n * features, x_capacity); require_output_i64(labels, n, label_capacity);
        std::mt19937_64 engine(seed); std::uniform_real_distribution<double> center_dist(-10.0, 10.0); std::normal_distribution<double> offset(0.0, cluster_std == 0.0 ? 1.0 : cluster_std); std::vector<double> centers(centers_count * features); for (double& center : centers) center = center_dist(engine);
        std::vector<int64_t> order(n); for (size_t r = 0; r < n; ++r) order[r] = static_cast<int64_t>(r % centers_count); std::shuffle(order.begin(), order.end(), engine);
        for (size_t r = 0; r < n; ++r) { auto label = order[r]; labels[r] = label; for (size_t c = 0; c < features; ++c) x[r * features + c] = centers[static_cast<size_t>(label) * features + c] + (cluster_std == 0.0 ? 0.0 : offset(engine)); }
    });
}
char* coolbox_misc_fuzzer_function_name() { return coolbox_jbridge::dup_string("Fuzzer"); }
char* coolbox_misc_http_function_name() { return coolbox_jbridge::dup_string("Http"); }

}
