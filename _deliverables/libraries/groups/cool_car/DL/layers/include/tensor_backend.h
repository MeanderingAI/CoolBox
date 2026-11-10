#ifndef COOLBOX_TENSOR_BACKEND_H
#define COOLBOX_TENSOR_BACKEND_H

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace ml {
namespace deep_learning {

enum class TensorBackend {
    CPU,
    VULKAN,
    AUTO
};

enum class TensorPrecision {
    AUTO,
    FP64,
    FP32,
    FP16,
    BF16
};

namespace detail {

enum class VulkanTensorOperation {
    ADD = 0,
    SUBTRACT = 1,
    MULTIPLY = 2,
    DIVIDE = 3,
    SCALAR_MULTIPLY = 4,
    SCALAR_DIVIDE = 5,
    MATMUL = 6,
    TRANSPOSE = 7,
    MATMUL_BIAS = 8,
    RELU = 9,
    RELU_BACKWARD = 10,
    ROW_SOFTMAX = 11,
    MASK_FILL = 12,
    LAYER_NORM = 13,
    LAYER_NORM_BACKWARD = 14,
    LAYER_NORM_GAMMA_GRADIENT = 15,
    ROW_SUM = 16,
    BROADCAST_ADD = 17,
    BROADCAST_MULTIPLY = 18,
    ADD_PREFIX = 19,
    SLICE_COLUMNS = 20,
    CONCAT_COLUMNS = 21,
    SQUARED_SUM = 22,
    ADAMW_UPDATE = 23
};

#ifdef COOLBOX_HAS_VULKAN_TENSOR
bool vulkan_tensor_available();
std::string vulkan_tensor_unavailable_reason();
std::string vulkan_tensor_device_name();
bool vulkan_tensor_precision_available(TensorPrecision precision);
#endif

inline std::atomic<TensorBackend> requested_tensor_backend{TensorBackend::CPU};
inline std::atomic<TensorBackend> active_tensor_backend{TensorBackend::CPU};
inline std::atomic<std::size_t> requested_cpu_threads{0};
inline std::atomic<TensorPrecision> requested_tensor_precision{
    TensorPrecision::AUTO};
inline std::atomic<TensorPrecision> active_tensor_precision{
    TensorPrecision::FP64};

} // namespace detail

inline bool tensor_backend_available(TensorBackend backend) {
    if (backend == TensorBackend::CPU || backend == TensorBackend::AUTO) {
        return true;
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    return detail::vulkan_tensor_available();
#else
    return false;
#endif
}

inline std::string tensor_backend_unavailable_reason(TensorBackend backend) {
    if (tensor_backend_available(backend)) {
        return {};
    }
    if (backend == TensorBackend::VULKAN) {
#ifdef COOLBOX_HAS_VULKAN_TENSOR
        return detail::vulkan_tensor_unavailable_reason();
#else
        return "Vulkan tensor support was not compiled into this build";
#endif
    }
    return "Unknown tensor backend";
}

inline void set_tensor_backend(TensorBackend backend) {
    if (backend == TensorBackend::VULKAN &&
        !tensor_backend_available(TensorBackend::VULKAN)) {
        throw std::runtime_error(
            "Vulkan tensor backend is unavailable: " +
            tensor_backend_unavailable_reason(TensorBackend::VULKAN));
    }
    detail::requested_tensor_backend.store(backend);
}

inline TensorBackend tensor_backend() {
    return detail::requested_tensor_backend.load();
}

inline TensorBackend active_tensor_backend() {
    return detail::active_tensor_backend.load();
}

inline void set_tensor_precision(TensorPrecision precision) {
    detail::requested_tensor_precision.store(precision);
}

inline TensorPrecision tensor_precision() {
    return detail::requested_tensor_precision.load();
}

inline TensorPrecision active_tensor_precision() {
    return detail::active_tensor_precision.load();
}

inline bool tensor_precision_available(TensorPrecision precision) {
    if (precision == TensorPrecision::AUTO ||
        precision == TensorPrecision::FP64) {
        return true;
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    return detail::vulkan_tensor_precision_available(precision);
#else
    return false;
#endif
}

inline void set_tensor_cpu_threads(std::size_t thread_count) {
    detail::requested_cpu_threads.store(thread_count);
}

inline std::size_t tensor_cpu_threads() {
    const std::size_t requested =
        detail::requested_cpu_threads.load();
    if (requested != 0) {
        return requested;
    }
    return std::max(
        std::size_t{1},
        static_cast<std::size_t>(
            std::thread::hardware_concurrency()));
}

inline std::string tensor_backend_device_name(TensorBackend backend) {
    if (backend == TensorBackend::CPU) {
        return "CPU";
    }
    if (backend == TensorBackend::AUTO) {
        backend = active_tensor_backend();
        if (backend == TensorBackend::CPU) {
            return "CPU";
        }
    }
#ifdef COOLBOX_HAS_VULKAN_TENSOR
    if (backend == TensorBackend::VULKAN &&
        detail::vulkan_tensor_available()) {
        return detail::vulkan_tensor_device_name();
    }
#endif
    return {};
}

namespace detail {

inline bool should_use_vulkan(std::size_t work_items) {
    const TensorBackend requested = requested_tensor_backend.load();
    bool use_vulkan = false;
    if (requested == TensorBackend::VULKAN) {
        use_vulkan = true;
    } else if (requested == TensorBackend::AUTO) {
        use_vulkan =
            work_items >= 4096 &&
            tensor_backend_available(TensorBackend::VULKAN);
    }
    active_tensor_backend.store(
        use_vulkan ? TensorBackend::VULKAN : TensorBackend::CPU);
    return use_vulkan;
}

template <typename Function>
void parallel_for(
    std::size_t task_count,
    std::size_t work_items,
    Function&& function) {
    constexpr std::size_t MIN_PARALLEL_WORK = 1U << 20;
    const std::size_t thread_count =
        std::min(task_count, tensor_cpu_threads());
    if (thread_count < 2 || work_items < MIN_PARALLEL_WORK) {
        function(0, task_count);
        return;
    }

    std::vector<std::thread> workers;
    workers.reserve(thread_count - 1);
    const std::size_t tasks_per_thread =
        (task_count + thread_count - 1) / thread_count;
    try {
        for (std::size_t worker = 1;
             worker < thread_count;
             ++worker) {
            const std::size_t begin = worker * tasks_per_thread;
            const std::size_t end =
                std::min(task_count, begin + tasks_per_thread);
            if (begin >= end) {
                break;
            }
            workers.emplace_back(
                [begin, end, &function]() {
                    function(begin, end);
                });
        }
    } catch (...) {
        for (std::thread& worker : workers) {
            worker.join();
        }
        throw;
    }
    function(0, std::min(task_count, tasks_per_thread));
    for (std::thread& worker : workers) {
        worker.join();
    }
}

} // namespace detail
} // namespace deep_learning
} // namespace ml

#endif
