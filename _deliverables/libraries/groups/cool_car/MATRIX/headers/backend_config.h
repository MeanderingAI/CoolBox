#pragma once

#include <atomic>
#include <string>

namespace mytrix {

enum class ComputeBackend {
    CPU,
    GPU_AUTO,
    GPU_OPENCL,
    GPU_CUDA,
    GPU_METAL
};

class BackendConfig {
public:
    static void set_requested_backend(ComputeBackend backend) {
        requested_backend_.store(backend, std::memory_order_relaxed);
    }

    static ComputeBackend requested_backend() {
        return requested_backend_.load(std::memory_order_relaxed);
    }

    static ComputeBackend active_backend() {
        return active_backend_.load(std::memory_order_relaxed);
    }

    static void set_active_backend(ComputeBackend backend) {
        active_backend_.store(backend, std::memory_order_relaxed);
    }

    static ComputeBackend resolve_backend() {
        const ComputeBackend requested = requested_backend();
        switch (requested) {
            case ComputeBackend::CPU:
                return ComputeBackend::CPU;
            case ComputeBackend::GPU_AUTO:
                if (is_backend_enabled(ComputeBackend::GPU_METAL)) return ComputeBackend::GPU_METAL;
                if (is_backend_enabled(ComputeBackend::GPU_CUDA)) return ComputeBackend::GPU_CUDA;
                if (is_backend_enabled(ComputeBackend::GPU_OPENCL)) return ComputeBackend::GPU_OPENCL;
                return ComputeBackend::CPU;
            case ComputeBackend::GPU_OPENCL:
                return is_backend_enabled(ComputeBackend::GPU_OPENCL) ? ComputeBackend::GPU_OPENCL : ComputeBackend::CPU;
            case ComputeBackend::GPU_CUDA:
                return is_backend_enabled(ComputeBackend::GPU_CUDA) ? ComputeBackend::GPU_CUDA : ComputeBackend::CPU;
            case ComputeBackend::GPU_METAL:
                return is_backend_enabled(ComputeBackend::GPU_METAL) ? ComputeBackend::GPU_METAL : ComputeBackend::CPU;
            default:
                return ComputeBackend::CPU;
        }
    }

    static bool is_backend_enabled(ComputeBackend backend) {
        switch (backend) {
            case ComputeBackend::CPU:
                return true;
            case ComputeBackend::GPU_AUTO:
                return true;
            case ComputeBackend::GPU_OPENCL:
#ifdef MYTRIX_ENABLE_OPENCL
                return true;
#else
                return false;
#endif
            case ComputeBackend::GPU_CUDA:
#ifdef MYTRIX_ENABLE_CUDA
                return true;
#else
                return false;
#endif
            case ComputeBackend::GPU_METAL:
#ifdef MYTRIX_ENABLE_METAL
                return true;
#else
                return false;
#endif
            default:
                return false;
        }
    }

    static const char* backend_name(ComputeBackend backend) {
        switch (backend) {
            case ComputeBackend::CPU: return "cpu";
            case ComputeBackend::GPU_AUTO: return "gpu_auto";
            case ComputeBackend::GPU_OPENCL: return "gpu_opencl";
            case ComputeBackend::GPU_CUDA: return "gpu_cuda";
            case ComputeBackend::GPU_METAL: return "gpu_metal";
            default: return "unknown";
        }
    }

private:
    inline static std::atomic<ComputeBackend> requested_backend_{ComputeBackend::CPU};
    inline static std::atomic<ComputeBackend> active_backend_{ComputeBackend::CPU};
};

inline void set_backend(ComputeBackend backend) {
    BackendConfig::set_requested_backend(backend);
}

inline ComputeBackend requested_backend() {
    return BackendConfig::requested_backend();
}

inline ComputeBackend active_backend() {
    return BackendConfig::active_backend();
}

inline std::string backend_name(ComputeBackend backend) {
    return BackendConfig::backend_name(backend);
}

} // namespace mytrix
