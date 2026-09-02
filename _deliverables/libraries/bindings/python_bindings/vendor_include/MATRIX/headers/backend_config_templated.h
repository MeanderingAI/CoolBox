#pragma once

#include <atomic>
#include <string>
#include <type_traits>

// ============================================================================
// Platform Detection (compile-time)
// ============================================================================

#ifdef __APPLE__
    #define MYTRIX_PLATFORM_APPLE 1
#elif defined(_WIN32) || defined(_WIN64)
    #define MYTRIX_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define MYTRIX_PLATFORM_LINUX 1
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

namespace mytrix {

enum class ComputeBackend {
    CPU,
    BOOST,
    EIGEN,
    GPU_AUTO,
    GPU_OPENCL,
    GPU_CUDA,
    GPU_METAL
};

struct OperationOptions {
    ComputeBackend backend{ComputeBackend::GPU_AUTO};
    bool boost{false};
};

namespace metal {
    class MetalBackend;
}

namespace cuda {
    class CudaBackend;
}

namespace opencl {
    class OpenCLBackend;
}

// ============================================================================
// Template Traits for Backend Resolution
// ============================================================================

/**
 * @brief Template specialization to resolve backend type at compile time.
 *
 * Based on platform detection and compile-time feature flags, maps
 * ComputeBackend enum values to their actual backend class implementations.
 */
template <ComputeBackend B, typename = void>
struct BackendResolver;

// Metal Backend Resolution (macOS)
#ifdef MYTRIX_ENABLE_METAL
template <>
struct BackendResolver<ComputeBackend::GPU_METAL> {
    using type = metal::MetalBackend;
    static constexpr bool available = true;
};
#else
template <>
struct BackendResolver<ComputeBackend::GPU_METAL> {
    // Placeholder when Metal not available
    struct Disabled {};
    using type = Disabled;
    static constexpr bool available = false;
};
#endif

// CUDA Backend Resolution (Windows/NVIDIA)
#ifdef MYTRIX_ENABLE_CUDA
template <>
struct BackendResolver<ComputeBackend::GPU_CUDA> {
    using type = cuda::CudaBackend;
    static constexpr bool available = true;
};
#else
template <>
struct BackendResolver<ComputeBackend::GPU_CUDA> {
    struct Disabled {};
    using type = Disabled;
    static constexpr bool available = false;
};
#endif

// OpenCL Backend Resolution (Linux/cross-platform)
#ifdef MYTRIX_ENABLE_OPENCL
template <>
struct BackendResolver<ComputeBackend::GPU_OPENCL> {
    using type = opencl::OpenCLBackend;
    static constexpr bool available = true;
};
#else
template <>
struct BackendResolver<ComputeBackend::GPU_OPENCL> {
    struct Disabled {};
    using type = Disabled;
    static constexpr bool available = false;
};
#endif

// CPU Backend always available
template <>
struct BackendResolver<ComputeBackend::CPU> {
    struct CPUBackend {}; // Marker, actual CPU fallback is inline
    using type = CPUBackend;
    static constexpr bool available = true;
};

template <>
struct BackendResolver<ComputeBackend::BOOST> {
    struct BoostBackend {};
    using type = BoostBackend;
    static constexpr bool available = true;
};

template <>
struct BackendResolver<ComputeBackend::EIGEN> {
    struct EigenBackend {};
    using type = EigenBackend;
    static constexpr bool available = true;
};

/**
 * @brief Default platform-preferred backend based on host.
 *
 * Compile-time decision:
 * - macOS: GPU_METAL (via __APPLE__)
 * - Windows: GPU_CUDA (via _WIN32)
 * - Linux: GPU_OPENCL (via __linux__)
 * Falls back to GPU_AUTO if preferred not available.
 */
template <typename = void>
struct DefaultPlatformBackend {
    static constexpr ComputeBackend value() {
#ifdef MYTRIX_PLATFORM_APPLE
    #ifdef MYTRIX_ENABLE_METAL
        return ComputeBackend::GPU_METAL;
    #endif
#elif MYTRIX_PLATFORM_WINDOWS
    #ifdef MYTRIX_ENABLE_CUDA
        return ComputeBackend::GPU_CUDA;
    #endif
#elif MYTRIX_PLATFORM_LINUX
    #ifdef MYTRIX_ENABLE_OPENCL
        return ComputeBackend::GPU_OPENCL;
    #endif
#endif
        // Fallback to AUTO if preferred unavailable
        return ComputeBackend::GPU_AUTO;
    }
};

// ============================================================================
// Dispatch Traits with SFINAE
// ============================================================================

/**
 * @brief Operation dispatcher using SFINAE and template specialization.
 *
 * Compile-time dispatch routes matrix operations to appropriate backend.
 * Each backend specialization includes separate kernel methods.
 *
 * Usage:
 *   auto result = OperationDispatcher<ComputeBackend::GPU_METAL>::multiply(a, b);
 */

// Forward declaration of DenseMatrix (avoid circular includes)
class DenseMatrix;

template <ComputeBackend B, typename = void>
struct OperationDispatcher;

// Metal Backend Operations (separate kernel methods)
#ifdef MYTRIX_ENABLE_METAL
template <>
struct OperationDispatcher<ComputeBackend::GPU_METAL> {
    static DenseMatrix multiply(const DenseMatrix& lhs, const DenseMatrix& rhs);
    static DenseMatrix transpose(const DenseMatrix& mat);
    static DenseMatrix add(const DenseMatrix& lhs, const DenseMatrix& rhs);
};
#endif

// CUDA Backend Operations (separate kernel methods)
#ifdef MYTRIX_ENABLE_CUDA
template <>
struct OperationDispatcher<ComputeBackend::GPU_CUDA> {
    static DenseMatrix multiply(const DenseMatrix& lhs, const DenseMatrix& rhs);
    static DenseMatrix transpose(const DenseMatrix& mat);
    static DenseMatrix add(const DenseMatrix& lhs, const DenseMatrix& rhs);
};
#endif

// OpenCL Backend Operations (separate kernel methods)
#ifdef MYTRIX_ENABLE_OPENCL
template <>
struct OperationDispatcher<ComputeBackend::GPU_OPENCL> {
    static DenseMatrix multiply(const DenseMatrix& lhs, const DenseMatrix& rhs);
    static DenseMatrix transpose(const DenseMatrix& mat);
    static DenseMatrix add(const DenseMatrix& lhs, const DenseMatrix& rhs);
};
#endif

// ============================================================================
// Runtime Backend Configuration (wrapper around template logic)
// ============================================================================

class BackendConfig {
public:
    static void set_requested_backend(ComputeBackend backend) {
        requested_backend_.store(backend, std::memory_order_relaxed);
    }

    static void set_boost_enabled(bool enabled) {
        boost_enabled_.store(enabled, std::memory_order_relaxed);
    }

    static ComputeBackend requested_backend() {
        return requested_backend_.load(std::memory_order_relaxed);
    }

    static ComputeBackend active_backend() {
        return active_backend_.load(std::memory_order_relaxed);
    }

    static bool boost_enabled() {
        return boost_enabled_.load(std::memory_order_relaxed);
    }

    static void set_active_backend(ComputeBackend backend) {
        active_backend_.store(backend, std::memory_order_relaxed);
    }

    /**
     * @brief Resolve backend at runtime, respecting template specialization.
     *
     * Uses BackendResolver trait to check compile-time availability of backends.
     * Follows fallback chain: Requested → GPU_AUTO → CPU
     */
    static ComputeBackend resolve_backend() {
        return resolve_backend(requested_backend(), boost_enabled());
    }

    static ComputeBackend resolve_backend(ComputeBackend requested, bool boost) {
        if (requested == ComputeBackend::BOOST || requested == ComputeBackend::EIGEN) {
            return requested;
        }

        if (boost && (requested == ComputeBackend::CPU || requested == ComputeBackend::GPU_AUTO)) {
            return ComputeBackend::BOOST;
        }

        switch (requested) {
            case ComputeBackend::CPU:
                return ComputeBackend::CPU;

            case ComputeBackend::BOOST:
                return ComputeBackend::BOOST;

            case ComputeBackend::EIGEN:
                return ComputeBackend::EIGEN;
                
            case ComputeBackend::GPU_AUTO: {
                // Use compile-time platform preference
                const auto preferred = DefaultPlatformBackend<>::value();
                if (is_backend_available(preferred)) {
                    return preferred;
                }
                // Fallback chain: Metal → CUDA → OpenCL → CPU
                if (is_backend_available(ComputeBackend::GPU_METAL)) return ComputeBackend::GPU_METAL;
                if (is_backend_available(ComputeBackend::GPU_CUDA)) return ComputeBackend::GPU_CUDA;
                if (is_backend_available(ComputeBackend::GPU_OPENCL)) return ComputeBackend::GPU_OPENCL;
                return ComputeBackend::CPU;
            }
                
            case ComputeBackend::GPU_METAL:
                return is_backend_available(ComputeBackend::GPU_METAL) ? ComputeBackend::GPU_METAL : ComputeBackend::CPU;
                
            case ComputeBackend::GPU_CUDA:
                return is_backend_available(ComputeBackend::GPU_CUDA) ? ComputeBackend::GPU_CUDA : ComputeBackend::CPU;
                
            case ComputeBackend::GPU_OPENCL:
                return is_backend_available(ComputeBackend::GPU_OPENCL) ? ComputeBackend::GPU_OPENCL : ComputeBackend::CPU;
                
            default:
                return ComputeBackend::CPU;
        }
    }

    /**
     * @brief Check if backend is available using compile-time template trait.
     */
    static bool is_backend_available(ComputeBackend backend) {
        switch (backend) {
            case ComputeBackend::CPU:
                return true;

            case ComputeBackend::BOOST:
            case ComputeBackend::EIGEN:
                return true;
                
            case ComputeBackend::GPU_METAL:
                return BackendResolver<ComputeBackend::GPU_METAL>::available;
                
            case ComputeBackend::GPU_CUDA:
                return BackendResolver<ComputeBackend::GPU_CUDA>::available;
                
            case ComputeBackend::GPU_OPENCL:
                return BackendResolver<ComputeBackend::GPU_OPENCL>::available;
                
            case ComputeBackend::GPU_AUTO:
                return true; // AUTO always available as fallback
                
            default:
                return false;
        }
    }

    static const char* backend_name(ComputeBackend backend) {
        switch (backend) {
            case ComputeBackend::CPU: return "cpu";
            case ComputeBackend::BOOST: return "boost";
            case ComputeBackend::EIGEN: return "eigen";
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
    inline static std::atomic<bool> boost_enabled_{false};
};

// ============================================================================
// Public API
// ============================================================================

inline void set_backend(ComputeBackend backend) {
    BackendConfig::set_requested_backend(backend);
}

inline void set_boost_enabled(bool enabled) {
    BackendConfig::set_boost_enabled(enabled);
}

inline ComputeBackend requested_backend() {
    return BackendConfig::requested_backend();
}

inline ComputeBackend active_backend() {
    return BackendConfig::active_backend();
}

inline bool boost_enabled() {
    return BackendConfig::boost_enabled();
}

inline std::string backend_name(ComputeBackend backend) {
    return BackendConfig::backend_name(backend);
}

} // namespace mytrix
