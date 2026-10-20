#pragma once

#include <cstddef>
#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>
#include <exception>
#include <mutex>

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define MYTRIX_CPU_X86 1
#include <immintrin.h>
#endif

#if defined(__aarch64__)
#define MYTRIX_CPU_NEON 1
#include <arm_neon.h>
#endif

namespace mytrix {

inline std::atomic<std::size_t> cpu_thread_limit{0};

inline void set_cpu_threads(std::size_t count) {
    cpu_thread_limit.store(count, std::memory_order_relaxed);
}

inline std::size_t cpu_threads() {
    const auto requested = cpu_thread_limit.load(std::memory_order_relaxed);
    return requested ? requested : std::max(1u, std::thread::hardware_concurrency());
}

template<class Function>
inline void parallel_matrix_rows(std::size_t rows, std::size_t columns, std::size_t inner,
                                 Function function) {
    auto workers = std::min(cpu_threads(), rows / 32 + (rows % 32 != 0));
    // Amortize thread startup and repeated Eigen packing with enough work per worker.
    constexpr long double work_per_worker = 64 * 1024 * 1024;
    const auto useful_workers = static_cast<long double>(rows) * columns * inner / work_per_worker;
    if (useful_workers < workers)
        workers = static_cast<std::size_t>(useful_workers);
    if (workers <= 1) {
        function(0, rows);
        return;
    }
    std::vector<std::thread> threads;
    threads.reserve(workers - 1);
    std::exception_ptr failure;
    std::mutex failure_mutex;
    const auto run = [&](std::size_t first, std::size_t last) {
        try { function(first, last); }
        catch (...) {
            std::lock_guard<std::mutex> lock(failure_mutex);
            if (!failure) failure = std::current_exception();
        }
    };
    try {
        for (std::size_t worker = 1; worker < workers; ++worker)
            threads.emplace_back([&, worker] { run(rows * worker / workers, rows * (worker + 1) / workers); });
    } catch (...) {
        for (auto& thread : threads) thread.join();
        throw;
    }
    run(0, rows / workers);
    for (auto& thread : threads) thread.join();
    if (failure) std::rethrow_exception(failure);
}

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

#if defined(MYTRIX_CPU_X86) && (defined(__GNUC__) || defined(__clang__))
__attribute__((target("avx2,fma")))
inline std::size_t gemm_avx2_block(const double* lhs, const double* rhs, double* output,
                                 std::size_t begin, std::size_t end, std::size_t inner,
                                 std::size_t columns, std::size_t jj, std::size_t jend,
                                 std::size_t kk, std::size_t kend) {
    std::size_t i = begin;
    for (; i + 4 <= end; i += 4) {
        std::size_t j = jj;
        for (; j + 8 <= jend; j += 8) {
            __m256d c00 = _mm256_loadu_pd(output + i * columns + j);
            __m256d c01 = _mm256_loadu_pd(output + i * columns + j + 4);
            __m256d c10 = _mm256_loadu_pd(output + (i + 1) * columns + j);
            __m256d c11 = _mm256_loadu_pd(output + (i + 1) * columns + j + 4);
            __m256d c20 = _mm256_loadu_pd(output + (i + 2) * columns + j);
            __m256d c21 = _mm256_loadu_pd(output + (i + 2) * columns + j + 4);
            __m256d c30 = _mm256_loadu_pd(output + (i + 3) * columns + j);
            __m256d c31 = _mm256_loadu_pd(output + (i + 3) * columns + j + 4);
            for (std::size_t k = kk; k < kend; ++k) {
                const auto b0 = _mm256_loadu_pd(rhs + k * columns + j);
                const auto b1 = _mm256_loadu_pd(rhs + k * columns + j + 4);
                const auto a0 = _mm256_set1_pd(lhs[i * inner + k]);
                const auto a1 = _mm256_set1_pd(lhs[(i + 1) * inner + k]);
                const auto a2 = _mm256_set1_pd(lhs[(i + 2) * inner + k]);
                const auto a3 = _mm256_set1_pd(lhs[(i + 3) * inner + k]);
                c00 = _mm256_fmadd_pd(a0, b0, c00); c01 = _mm256_fmadd_pd(a0, b1, c01);
                c10 = _mm256_fmadd_pd(a1, b0, c10); c11 = _mm256_fmadd_pd(a1, b1, c11);
                c20 = _mm256_fmadd_pd(a2, b0, c20); c21 = _mm256_fmadd_pd(a2, b1, c21);
                c30 = _mm256_fmadd_pd(a3, b0, c30); c31 = _mm256_fmadd_pd(a3, b1, c31);
            }
            _mm256_storeu_pd(output + i * columns + j, c00);
            _mm256_storeu_pd(output + i * columns + j + 4, c01);
            _mm256_storeu_pd(output + (i + 1) * columns + j, c10);
            _mm256_storeu_pd(output + (i + 1) * columns + j + 4, c11);
            _mm256_storeu_pd(output + (i + 2) * columns + j, c20);
            _mm256_storeu_pd(output + (i + 2) * columns + j + 4, c21);
            _mm256_storeu_pd(output + (i + 3) * columns + j, c30);
            _mm256_storeu_pd(output + (i + 3) * columns + j + 4, c31);
        }
        for (; j < jend; ++j)
            for (std::size_t k = kk; k < kend; ++k)
                for (std::size_t r = 0; r < 4; ++r)
                    output[(i + r) * columns + j] += lhs[(i + r) * inner + k] * rhs[k * columns + j];
    }
    return i;
}
#endif

inline void gemm_rows(const double* lhs, const double* rhs, double* output,
                     std::size_t begin, std::size_t end, std::size_t inner, std::size_t columns,
                     CpuOptimization optimization) {
    constexpr std::size_t block = 128;
    for (std::size_t jj = 0; jj < columns; jj += block)
        for (std::size_t kk = 0; kk < inner; kk += block) {
            const auto jend = std::min(columns, jj + block);
            const auto kend = std::min(inner, kk + block);
            std::size_t i = begin;
#if defined(MYTRIX_CPU_X86) && (defined(__GNUC__) || defined(__clang__))
            if (optimization == CpuOptimization::Avx2Fma)
                i = gemm_avx2_block(lhs, rhs, output, begin, end, inner, columns, jj, jend, kk, kend);
#endif
#if defined(MYTRIX_CPU_NEON)
            if (optimization == CpuOptimization::Neon) {
                for (; i + 4 <= end; i += 4) {
                    std::size_t j = jj;
                    for (; j + 8 <= jend; j += 8) {
                        float64x2_t c00 = vld1q_f64(output + i * columns + j);
                        float64x2_t c01 = vld1q_f64(output + i * columns + j + 2);
                        float64x2_t c02 = vld1q_f64(output + i * columns + j + 4);
                        float64x2_t c03 = vld1q_f64(output + i * columns + j + 6);
                        float64x2_t c10 = vld1q_f64(output + (i + 1) * columns + j);
                        float64x2_t c11 = vld1q_f64(output + (i + 1) * columns + j + 2);
                        float64x2_t c12 = vld1q_f64(output + (i + 1) * columns + j + 4);
                        float64x2_t c13 = vld1q_f64(output + (i + 1) * columns + j + 6);
                        float64x2_t c20 = vld1q_f64(output + (i + 2) * columns + j);
                        float64x2_t c21 = vld1q_f64(output + (i + 2) * columns + j + 2);
                        float64x2_t c22 = vld1q_f64(output + (i + 2) * columns + j + 4);
                        float64x2_t c23 = vld1q_f64(output + (i + 2) * columns + j + 6);
                        float64x2_t c30 = vld1q_f64(output + (i + 3) * columns + j);
                        float64x2_t c31 = vld1q_f64(output + (i + 3) * columns + j + 2);
                        float64x2_t c32 = vld1q_f64(output + (i + 3) * columns + j + 4);
                        float64x2_t c33 = vld1q_f64(output + (i + 3) * columns + j + 6);
                        for (std::size_t k = kk; k < kend; ++k) {
                            const auto b0 = vld1q_f64(rhs + k * columns + j);
                            const auto b1 = vld1q_f64(rhs + k * columns + j + 2);
                            const auto b2 = vld1q_f64(rhs + k * columns + j + 4);
                            const auto b3 = vld1q_f64(rhs + k * columns + j + 6);
                            const double a0 = lhs[i * inner + k], a1 = lhs[(i + 1) * inner + k];
                            const double a2 = lhs[(i + 2) * inner + k], a3 = lhs[(i + 3) * inner + k];
                            c00 = vfmaq_n_f64(c00, b0, a0); c01 = vfmaq_n_f64(c01, b1, a0);
                            c02 = vfmaq_n_f64(c02, b2, a0); c03 = vfmaq_n_f64(c03, b3, a0);
                            c10 = vfmaq_n_f64(c10, b0, a1); c11 = vfmaq_n_f64(c11, b1, a1);
                            c12 = vfmaq_n_f64(c12, b2, a1); c13 = vfmaq_n_f64(c13, b3, a1);
                            c20 = vfmaq_n_f64(c20, b0, a2); c21 = vfmaq_n_f64(c21, b1, a2);
                            c22 = vfmaq_n_f64(c22, b2, a2); c23 = vfmaq_n_f64(c23, b3, a2);
                            c30 = vfmaq_n_f64(c30, b0, a3); c31 = vfmaq_n_f64(c31, b1, a3);
                            c32 = vfmaq_n_f64(c32, b2, a3); c33 = vfmaq_n_f64(c33, b3, a3);
                        }
                        vst1q_f64(output + i * columns + j, c00); vst1q_f64(output + i * columns + j + 2, c01);
                        vst1q_f64(output + i * columns + j + 4, c02); vst1q_f64(output + i * columns + j + 6, c03);
                        vst1q_f64(output + (i + 1) * columns + j, c10); vst1q_f64(output + (i + 1) * columns + j + 2, c11);
                        vst1q_f64(output + (i + 1) * columns + j + 4, c12); vst1q_f64(output + (i + 1) * columns + j + 6, c13);
                        vst1q_f64(output + (i + 2) * columns + j, c20); vst1q_f64(output + (i + 2) * columns + j + 2, c21);
                        vst1q_f64(output + (i + 2) * columns + j + 4, c22); vst1q_f64(output + (i + 2) * columns + j + 6, c23);
                        vst1q_f64(output + (i + 3) * columns + j, c30); vst1q_f64(output + (i + 3) * columns + j + 2, c31);
                        vst1q_f64(output + (i + 3) * columns + j + 4, c32); vst1q_f64(output + (i + 3) * columns + j + 6, c33);
                    }
                    for (; j < jend; ++j)
                        for (std::size_t k = kk; k < kend; ++k)
                            for (std::size_t r = 0; r < 4; ++r)
                                output[(i + r) * columns + j] += lhs[(i + r) * inner + k] * rhs[k * columns + j];
                }
            }
#else
            (void)optimization;
#endif
            for (; i < end; ++i)
                for (std::size_t k = kk; k < kend; ++k) {
                    const double a = lhs[i * inner + k];
                    for (std::size_t j = jj; j < jend; ++j)
                        output[i * columns + j] += a * rhs[k * columns + j];
                }
        }
}

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

// Row-major, non-aliasing buffers; output is fully overwritten, including tails.
inline void optimized_multiply(const double* lhs, const double* rhs, double* output,
                               std::size_t rows, std::size_t inner, std::size_t columns,
                               CpuOptimization optimization = CpuOptimization::Auto) {
    std::fill_n(output, rows * columns, 0.0);
    const auto selected = selected_cpu_optimization(optimization);
    parallel_matrix_rows(rows, columns, inner, [&](std::size_t first, std::size_t last) {
        detail::gemm_rows(lhs, rhs, output, first, last, inner, columns, selected);
    });
}

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
