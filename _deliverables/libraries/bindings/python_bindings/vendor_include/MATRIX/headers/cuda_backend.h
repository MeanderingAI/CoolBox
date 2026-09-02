#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "matrix_dense.h"

#if defined(MYTRIX_ENABLE_CUDA) && defined(__CUDACC__)
#include <cuda_runtime.h>
#endif

namespace mytrix {
namespace cuda {

#if defined(MYTRIX_ENABLE_CUDA) && defined(__CUDACC__)
namespace detail {

inline void throw_cuda_error(cudaError_t error, const char* operation) {
    if (error != cudaSuccess) {
        throw std::runtime_error(std::string(operation) + ": " + cudaGetErrorString(error));
    }
}

__global__ void multiply_kernel_impl(const double* lhs, const double* rhs, double* out, int lhs_rows, int lhs_cols, int rhs_cols) {
    const int row = blockIdx.y * blockDim.y + threadIdx.y;
    const int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row >= lhs_rows || col >= rhs_cols) {
        return;
    }

    double sum = 0.0;
    for (int k = 0; k < lhs_cols; ++k) {
        const double left = lhs[row + k * lhs_rows];
        const double right = rhs[k + col * lhs_cols];
        sum += left * right;
    }

    out[row + col * lhs_rows] = sum;
}

__global__ void transpose_kernel_impl(const double* input, double* output, int rows, int cols) {
    const int row = blockIdx.y * blockDim.y + threadIdx.y;
    const int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row >= rows || col >= cols) {
        return;
    }

    output[col + row * cols] = input[row + col * rows];
}

__global__ void add_kernel_impl(const double* lhs, const double* rhs, double* out, int rows, int cols) {
    const int row = blockIdx.y * blockDim.y + threadIdx.y;
    const int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row >= rows || col >= cols) {
        return;
    }

    const int index = row + col * rows;
    out[index] = lhs[index] + rhs[index];
}

} // namespace detail
#endif

/**
 * @brief CUDA compute backend for matrix operations.
 *
 * Provides GPU acceleration on Windows/NVIDIA via CUDA.
 * Separate kernel methods for each operation.
 * Falls back to CPU for unsupported shapes or when CUDA is unavailable.
 *
 * When compiled with nvcc and MYTRIX_ENABLE_CUDA, these methods launch real CUDA kernels.
 * Otherwise they transparently fall back to the CPU path.
 */
class CudaBackend {
public:
    static bool is_available() {
#ifdef MYTRIX_ENABLE_CUDA
        return true;
#else
        return false;
#endif
    }

    /**
     * @brief Matrix multiplication kernel for CUDA.
     *
     * Optimal for large dense matrices on NVIDIA GPUs.
     * Uses CUBLAS or custom kernel implementation.
     */
    static DenseMatrix multiply_kernel(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.cols() != rhs.rows()) {
            throw std::invalid_argument("cuda multiply_kernel: incompatible dimensions");
        }
#if defined(MYTRIX_ENABLE_CUDA) && defined(__CUDACC__)
        const int lhs_rows = lhs.rows();
        const int lhs_cols = lhs.cols();
        const int rhs_cols = rhs.cols();

        const std::size_t lhs_bytes = static_cast<std::size_t>(lhs_rows) * static_cast<std::size_t>(lhs_cols) * sizeof(double);
        const std::size_t rhs_bytes = static_cast<std::size_t>(rhs.rows()) * static_cast<std::size_t>(rhs.cols()) * sizeof(double);
        const std::size_t out_bytes = static_cast<std::size_t>(lhs_rows) * static_cast<std::size_t>(rhs_cols) * sizeof(double);

        double* lhs_device = nullptr;
        double* rhs_device = nullptr;
        double* out_device = nullptr;

        detail::throw_cuda_error(cudaMalloc(&lhs_device, lhs_bytes), "cudaMalloc(lhs)");
        detail::throw_cuda_error(cudaMalloc(&rhs_device, rhs_bytes), "cudaMalloc(rhs)");
        detail::throw_cuda_error(cudaMalloc(&out_device, out_bytes), "cudaMalloc(out)");

        detail::throw_cuda_error(cudaMemcpy(lhs_device, lhs.data.data(), lhs_bytes, cudaMemcpyHostToDevice), "cudaMemcpy(lhs)");
        detail::throw_cuda_error(cudaMemcpy(rhs_device, rhs.data.data(), rhs_bytes, cudaMemcpyHostToDevice), "cudaMemcpy(rhs)");

        dim3 block(16, 16);
        dim3 grid((rhs_cols + block.x - 1) / block.x, (lhs_rows + block.y - 1) / block.y);
        multiply_kernel_impl<<<grid, block>>>(lhs_device, rhs_device, out_device, lhs_rows, lhs_cols, rhs_cols);
        detail::throw_cuda_error(cudaGetLastError(), "multiply_kernel launch");
        detail::throw_cuda_error(cudaDeviceSynchronize(), "multiply_kernel synchronize");

        DenseMatrix result(lhs_rows, rhs_cols);
        detail::throw_cuda_error(cudaMemcpy(result.data.data(), out_device, out_bytes, cudaMemcpyDeviceToHost), "cudaMemcpy(result)");

        cudaFree(lhs_device);
        cudaFree(rhs_device);
        cudaFree(out_device);
        return result;
#else
        DenseMatrix result(lhs.rows(), rhs.cols());
        result.data = lhs.data * rhs.data;
        return result;
#endif
    }

    /**
     * @brief Transpose kernel for CUDA.
     *
     * Optimized for coalesced memory access patterns on NVIDIA GPUs.
     */
    static DenseMatrix transpose_kernel(const DenseMatrix& mat) {
    #if defined(MYTRIX_ENABLE_CUDA) && defined(__CUDACC__)
        const int rows = mat.rows();
        const int cols = mat.cols();
        const std::size_t input_bytes = static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols) * sizeof(double);
        const std::size_t output_bytes = static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows) * sizeof(double);

        double* input_device = nullptr;
        double* output_device = nullptr;

        detail::throw_cuda_error(cudaMalloc(&input_device, input_bytes), "cudaMalloc(input)");
        detail::throw_cuda_error(cudaMalloc(&output_device, output_bytes), "cudaMalloc(output)");
        detail::throw_cuda_error(cudaMemcpy(input_device, mat.data.data(), input_bytes, cudaMemcpyHostToDevice), "cudaMemcpy(input)");

        dim3 block(16, 16);
        dim3 grid((cols + block.x - 1) / block.x, (rows + block.y - 1) / block.y);
        transpose_kernel_impl<<<grid, block>>>(input_device, output_device, rows, cols);
        detail::throw_cuda_error(cudaGetLastError(), "transpose_kernel launch");
        detail::throw_cuda_error(cudaDeviceSynchronize(), "transpose_kernel synchronize");

        DenseMatrix result(cols, rows);
        detail::throw_cuda_error(cudaMemcpy(result.data.data(), output_device, output_bytes, cudaMemcpyDeviceToHost), "cudaMemcpy(result)");

        cudaFree(input_device);
        cudaFree(output_device);
        return result;
    #else
        DenseMatrix result(mat.cols(), mat.rows());
        result.data = mat.data.transpose();
        return result;
    #endif
    }

    /**
     * @brief Element-wise addition kernel for CUDA.
     *
     * Parallelized element-wise operation across GPU threads.
     */
    static DenseMatrix add_kernel(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        if (lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols()) {
            throw std::invalid_argument("cuda add_kernel: dimension mismatch");
        }
#if defined(MYTRIX_ENABLE_CUDA) && defined(__CUDACC__)
        const int rows = lhs.rows();
        const int cols = lhs.cols();
        const std::size_t bytes = static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols) * sizeof(double);

        double* lhs_device = nullptr;
        double* rhs_device = nullptr;
        double* out_device = nullptr;

        detail::throw_cuda_error(cudaMalloc(&lhs_device, bytes), "cudaMalloc(lhs)");
        detail::throw_cuda_error(cudaMalloc(&rhs_device, bytes), "cudaMalloc(rhs)");
        detail::throw_cuda_error(cudaMalloc(&out_device, bytes), "cudaMalloc(out)");

        detail::throw_cuda_error(cudaMemcpy(lhs_device, lhs.data.data(), bytes, cudaMemcpyHostToDevice), "cudaMemcpy(lhs)");
        detail::throw_cuda_error(cudaMemcpy(rhs_device, rhs.data.data(), bytes, cudaMemcpyHostToDevice), "cudaMemcpy(rhs)");

        dim3 block(16, 16);
        dim3 grid((cols + block.x - 1) / block.x, (rows + block.y - 1) / block.y);
        add_kernel_impl<<<grid, block>>>(lhs_device, rhs_device, out_device, rows, cols);
        detail::throw_cuda_error(cudaGetLastError(), "add_kernel launch");
        detail::throw_cuda_error(cudaDeviceSynchronize(), "add_kernel synchronize");

        DenseMatrix result(rows, cols);
        detail::throw_cuda_error(cudaMemcpy(result.data.data(), out_device, bytes, cudaMemcpyDeviceToHost), "cudaMemcpy(result)");

        cudaFree(lhs_device);
        cudaFree(rhs_device);
        cudaFree(out_device);
        return result;
#else
        DenseMatrix result(lhs.rows(), lhs.cols());
        result.data = lhs.data + rhs.data;
        return result;
#endif
    }

    /**
     * @brief Public dispatch method for multiplication.
     * Routes through kernel with size-based threshold heuristic.
     */
    static DenseMatrix multiply(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        const int gpu_threshold = 256;
        if (lhs.rows() * rhs.cols() > gpu_threshold * gpu_threshold) {
            return multiply_kernel(lhs, rhs);
        }
        // CPU fallback for small matrices
        auto result = std::make_unique<DenseMatrix>(lhs.rows(), rhs.cols());
        result->data = lhs.data * rhs.data;
        return *result;
    }

    /**
     * @brief Public dispatch method for transpose.
     */
    static DenseMatrix transpose(const DenseMatrix& mat) {
        const int gpu_threshold = 256;
        if (mat.rows() * mat.cols() > gpu_threshold * gpu_threshold) {
            return transpose_kernel(mat);
        }
        auto result = std::make_unique<DenseMatrix>(mat.cols(), mat.rows());
        result->data = mat.data.transpose();
        return *result;
    }

    /**
     * @brief Public dispatch method for addition.
     */
    static DenseMatrix add(const DenseMatrix& lhs, const DenseMatrix& rhs) {
        const int gpu_threshold = 256;
        if (lhs.rows() * lhs.cols() > gpu_threshold * gpu_threshold) {
            return add_kernel(lhs, rhs);
        }
        auto result = std::make_unique<DenseMatrix>(lhs.rows(), lhs.cols());
        result->data = lhs.data + rhs.data;
        return *result;
    }
};

} // namespace cuda
} // namespace mytrix
