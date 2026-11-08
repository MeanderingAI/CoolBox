#ifndef COOLBOX_TENSOR_BACKEND_H
#define COOLBOX_TENSOR_BACKEND_H

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace ml {
namespace deep_learning {

enum class TensorBackend {
    CPU,
    VULKAN,
    AUTO
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
    TRANSPOSE = 7
};

#ifdef COOLBOX_HAS_VULKAN_TENSOR
bool vulkan_tensor_available();
std::string vulkan_tensor_unavailable_reason();
std::string vulkan_tensor_device_name();
#endif

inline std::atomic<TensorBackend> requested_tensor_backend{TensorBackend::CPU};
inline std::atomic<TensorBackend> active_tensor_backend{TensorBackend::CPU};

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

} // namespace detail
} // namespace deep_learning
} // namespace ml

#endif
