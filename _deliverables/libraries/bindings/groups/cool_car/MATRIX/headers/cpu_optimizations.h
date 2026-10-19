#pragma once

#include <cstddef>

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define MYTRIX_CPU_X86 1
#include <immintrin.h>
#endif

#if defined(__aarch64__)
#define MYTRIX_CPU_NEON 1
#include <arm_neon.h>
#endif

namespace mytrix {

enum class CpuOptimization {
    Auto,
    Scalar,
    Avx2Fma,
    Neon
};

struct CpuFeatures {
    bool avx2{false};
    bool fma{false};
    bool neon{false};
};

inline CpuFeatures detect_cpu_features() {
    CpuFeatures features;

#if defined(MYTRIX_CPU_X86) && (defined(__GNUC__) || defined(__clang__))
    __builtin_cpu_init();
    features.avx2 = __builtin_cpu_supports("avx2");
    features.fma = __builtin_cpu_supports("fma");
#endif

#if defined(MYTRIX_CPU_NEON)
    features.neon = true;
#endif

    return features;
}

inline const CpuFeatures& cpu_features() {
    static const CpuFeatures features = detect_cpu_features();
    return features;
}

inline bool cpu_optimization_available(CpuOptimization optimization) {
    switch (optimization) {
        case CpuOptimization::Auto:
        case CpuOptimization::Scalar:
            return true;
        case CpuOptimization::Avx2Fma:
            return cpu_features().avx2 && cpu_features().fma;
        case CpuOptimization::Neon:
            return cpu_features().neon;
        default:
            return false;
    }
}

inline CpuOptimization selected_cpu_optimization(CpuOptimization requested = CpuOptimization::Auto) {
    if (requested != CpuOptimization::Auto) {
        return cpu_optimization_available(requested) ? requested : CpuOptimization::Scalar;
    }
    if (cpu_optimization_available(CpuOptimization::Avx2Fma)) {
        return CpuOptimization::Avx2Fma;
    }
    if (cpu_optimization_available(CpuOptimization::Neon)) {
        return CpuOptimization::Neon;
    }
    return CpuOptimization::Scalar;
}

inline const char* cpu_optimization_name(CpuOptimization optimization) {
    switch (optimization) {
        case CpuOptimization::Auto: return "auto";
        case CpuOptimization::Scalar: return "scalar";
        case CpuOptimization::Avx2Fma: return "avx2_fma";
        case CpuOptimization::Neon: return "neon";
        default: return "unknown";
    }
}

namespace detail {

inline double dot_scalar(const double* lhs, const double* rhs, std::size_t count) {
    double result = 0.0;
    for (std::size_t index = 0; index < count; ++index) {
        result += lhs[index] * rhs[index];
    }
    return result;
}

inline void add_scalar(const double* lhs, const double* rhs, double* output, std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        output[index] = lhs[index] + rhs[index];
    }
}

#if defined(MYTRIX_CPU_X86) && (defined(__GNUC__) || defined(__clang__))
__attribute__((target("avx2,fma")))
inline double dot_avx2_fma(const double* lhs, const double* rhs, std::size_t count) {
    __m256d sum = _mm256_setzero_pd();
    std::size_t index = 0;
    for (; index + 4 <= count; index += 4) {
        const __m256d lhs_values = _mm256_loadu_pd(lhs + index);
        const __m256d rhs_values = _mm256_loadu_pd(rhs + index);
        sum = _mm256_fmadd_pd(lhs_values, rhs_values, sum);
    }

    alignas(32) double lanes[4];
    _mm256_store_pd(lanes, sum);
    double result = lanes[0] + lanes[1] + lanes[2] + lanes[3];
    for (; index < count; ++index) {
        result += lhs[index] * rhs[index];
    }
    return result;
}

__attribute__((target("avx2,fma")))
inline void add_avx2(const double* lhs, const double* rhs, double* output, std::size_t count) {
    std::size_t index = 0;
    for (; index + 4 <= count; index += 4) {
        const __m256d lhs_values = _mm256_loadu_pd(lhs + index);
        const __m256d rhs_values = _mm256_loadu_pd(rhs + index);
        _mm256_storeu_pd(output + index, _mm256_add_pd(lhs_values, rhs_values));
    }
    add_scalar(lhs + index, rhs + index, output + index, count - index);
}
#endif

#if defined(MYTRIX_CPU_NEON)
inline double dot_neon(const double* lhs, const double* rhs, std::size_t count) {
    float64x2_t sum = vdupq_n_f64(0.0);
    std::size_t index = 0;
    for (; index + 2 <= count; index += 2) {
        sum = vfmaq_f64(sum, vld1q_f64(lhs + index), vld1q_f64(rhs + index));
    }
    double result = vaddvq_f64(sum);
    for (; index < count; ++index) {
        result += lhs[index] * rhs[index];
    }
    return result;
}

inline void add_neon(const double* lhs, const double* rhs, double* output, std::size_t count) {
    std::size_t index = 0;
    for (; index + 2 <= count; index += 2) {
        vst1q_f64(output + index, vaddq_f64(vld1q_f64(lhs + index), vld1q_f64(rhs + index)));
    }
    add_scalar(lhs + index, rhs + index, output + index, count - index);
}
#endif

} // namespace detail

inline double optimized_dot(const double* lhs, const double* rhs, std::size_t count,
                            CpuOptimization optimization = CpuOptimization::Auto) {
    switch (selected_cpu_optimization(optimization)) {
#if defined(MYTRIX_CPU_X86) && (defined(__GNUC__) || defined(__clang__))
        case CpuOptimization::Avx2Fma:
            return detail::dot_avx2_fma(lhs, rhs, count);
#endif
#if defined(MYTRIX_CPU_NEON)
        case CpuOptimization::Neon:
            return detail::dot_neon(lhs, rhs, count);
#endif
        default:
            return detail::dot_scalar(lhs, rhs, count);
    }
}

inline void optimized_add(const double* lhs, const double* rhs, double* output, std::size_t count,
                          CpuOptimization optimization = CpuOptimization::Auto) {
    switch (selected_cpu_optimization(optimization)) {
#if defined(MYTRIX_CPU_X86) && (defined(__GNUC__) || defined(__clang__))
        case CpuOptimization::Avx2Fma:
            detail::add_avx2(lhs, rhs, output, count);
            return;
#endif
#if defined(MYTRIX_CPU_NEON)
        case CpuOptimization::Neon:
            detail::add_neon(lhs, rhs, output, count);
            return;
#endif
        default:
            detail::add_scalar(lhs, rhs, output, count);
    }
}

} // namespace mytrix
