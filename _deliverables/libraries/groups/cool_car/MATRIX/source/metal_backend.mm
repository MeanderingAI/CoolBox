#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "../headers/metal_backend.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

namespace mytrix {
namespace metal {
namespace detail {

#if defined(MYTRIX_ENABLE_METAL) && defined(__APPLE__) && defined(__OBJC__)

namespace {

static std::string read_shader_text() {
    const char* shader_path = MYTRIX_METAL_SHADER_PATH;
    std::ifstream input(shader_path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

static NSString* shader_source() {
    static NSString* source = nil;
    static std::once_flag once_token;
    std::call_once(once_token, [] {
        const std::string shader_text = read_shader_text();
        if (!shader_text.empty()) {
            source = [[NSString alloc] initWithUTF8String:shader_text.c_str()];
        }
        if (source == nil) {
            source = @"#include <metal_stdlib>\nusing namespace metal;\n";
        }
    });
    return source;
}

static std::vector<double> flatten_row_major(const DenseMatrix& matrix) {
    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(matrix.rows() * matrix.cols()));
    for (int row = 0; row < matrix.rows(); ++row) {
        for (int col = 0; col < matrix.cols(); ++col) {
            values.push_back(matrix.data(row, col));
        }
    }
    return values;
}

static DenseMatrix make_matrix_from_row_major(int rows, int cols, const std::vector<double>& values) {
    DenseMatrix result(rows, cols);
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            result.data(row, col) = values[static_cast<std::size_t>(row * cols + col)];
        }
    }
    return result;
}

static id<MTLComputePipelineState> pipeline(id<MTLDevice> device, NSString* name) {
    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:shader_source() options:nil error:&error];
    if (library == nil) {
        return nil;
    }

    id<MTLFunction> function = [library newFunctionWithName:name];
    if (function == nil) {
        return nil;
    }

    id<MTLComputePipelineState> state = [device newComputePipelineStateWithFunction:function error:&error];
    return state;
}

} // namespace

bool runtime_available() {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    return device != nil;
}

std::unique_ptr<DenseMatrix> dispatch_add(const DenseMatrix& lhs, const DenseMatrix& rhs) {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
        return nullptr;
    }

    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (queue == nil) {
        return nullptr;
    }

    id<MTLComputePipelineState> state = pipeline(device, @"matrix_add");
    if (state == nil) {
        return nullptr;
    }

    const std::vector<double> lhs_values = flatten_row_major(lhs);
    const std::vector<double> rhs_values = flatten_row_major(rhs);
    std::vector<double> out_values(lhs_values.size(), 0.0);

    id<MTLBuffer> lhs_buffer = [device newBufferWithBytes:lhs_values.data()
                                                   length:lhs_values.size() * sizeof(double)
                                                  options:MTLResourceStorageModeShared];
    id<MTLBuffer> rhs_buffer = [device newBufferWithBytes:rhs_values.data()
                                                   length:rhs_values.size() * sizeof(double)
                                                  options:MTLResourceStorageModeShared];
    id<MTLBuffer> out_buffer = [device newBufferWithLength:out_values.size() * sizeof(double)
                                                   options:MTLResourceStorageModeShared];

    id<MTLCommandBuffer> command_buffer = [queue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [command_buffer computeCommandEncoder];
    [encoder setComputePipelineState:state];
    [encoder setBuffer:lhs_buffer offset:0 atIndex:0];
    [encoder setBuffer:rhs_buffer offset:0 atIndex:1];
    [encoder setBuffer:out_buffer offset:0 atIndex:2];

    const NSUInteger element_count = static_cast<NSUInteger>(out_values.size());
    const NSUInteger thread_width = std::max<NSUInteger>(1, state.threadExecutionWidth);
    const NSUInteger threads_per_group = std::min<NSUInteger>(thread_width, element_count == 0 ? 1 : element_count);
    const MTLSize grid = MTLSizeMake(element_count, 1, 1);
    const MTLSize group = MTLSizeMake(threads_per_group, 1, 1);
    [encoder dispatchThreads:grid threadsPerThreadgroup:group];
    [encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];

    std::copy_n(static_cast<const double*>(out_buffer.contents), out_values.size(), out_values.begin());
    return std::make_unique<DenseMatrix>(make_matrix_from_row_major(lhs.rows(), lhs.cols(), out_values));
}

std::unique_ptr<DenseMatrix> dispatch_transpose(const DenseMatrix& mat) {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
        return nullptr;
    }

    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (queue == nil) {
        return nullptr;
    }

    id<MTLComputePipelineState> state = pipeline(device, @"matrix_transpose");
    if (state == nil) {
        return nullptr;
    }

    const std::vector<double> in_values = flatten_row_major(mat);
    std::vector<double> out_values(static_cast<std::size_t>(mat.rows() * mat.cols()), 0.0);
    const uint32_t rows = static_cast<uint32_t>(mat.rows());
    const uint32_t cols = static_cast<uint32_t>(mat.cols());

    id<MTLBuffer> in_buffer = [device newBufferWithBytes:in_values.data()
                                                  length:in_values.size() * sizeof(double)
                                                 options:MTLResourceStorageModeShared];
    id<MTLBuffer> out_buffer = [device newBufferWithLength:out_values.size() * sizeof(double)
                                                   options:MTLResourceStorageModeShared];
    id<MTLBuffer> rows_buffer = [device newBufferWithBytes:&rows length:sizeof(rows) options:MTLResourceStorageModeShared];
    id<MTLBuffer> cols_buffer = [device newBufferWithBytes:&cols length:sizeof(cols) options:MTLResourceStorageModeShared];

    id<MTLCommandBuffer> command_buffer = [queue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [command_buffer computeCommandEncoder];
    [encoder setComputePipelineState:state];
    [encoder setBuffer:in_buffer offset:0 atIndex:0];
    [encoder setBuffer:out_buffer offset:0 atIndex:1];
    [encoder setBuffer:rows_buffer offset:0 atIndex:2];
    [encoder setBuffer:cols_buffer offset:0 atIndex:3];

    const NSUInteger thread_width = std::max<NSUInteger>(1, state.threadExecutionWidth);
    const NSUInteger thread_height = std::max<NSUInteger>(1, state.maxTotalThreadsPerThreadgroup / thread_width);
    const MTLSize grid = MTLSizeMake(static_cast<NSUInteger>(cols), static_cast<NSUInteger>(rows), 1);
    const MTLSize group = MTLSizeMake(thread_width, thread_height, 1);
    [encoder dispatchThreads:grid threadsPerThreadgroup:group];
    [encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];

    std::copy_n(static_cast<const double*>(out_buffer.contents), out_values.size(), out_values.begin());
    return std::make_unique<DenseMatrix>(make_matrix_from_row_major(mat.cols(), mat.rows(), out_values));
}

std::unique_ptr<DenseMatrix> dispatch_multiply(const DenseMatrix& lhs, const DenseMatrix& rhs) {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) {
        return nullptr;
    }

    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (queue == nil) {
        return nullptr;
    }

    id<MTLComputePipelineState> state = pipeline(device, @"matrix_multiply");
    if (state == nil) {
        return nullptr;
    }

    const std::vector<double> lhs_values = flatten_row_major(lhs);
    const std::vector<double> rhs_values = flatten_row_major(rhs);
    std::vector<double> out_values(static_cast<std::size_t>(lhs.rows() * rhs.cols()), 0.0);
    const uint32_t lhs_rows = static_cast<uint32_t>(lhs.rows());
    const uint32_t lhs_cols = static_cast<uint32_t>(lhs.cols());
    const uint32_t rhs_cols = static_cast<uint32_t>(rhs.cols());

    id<MTLBuffer> lhs_buffer = [device newBufferWithBytes:lhs_values.data()
                                                   length:lhs_values.size() * sizeof(double)
                                                  options:MTLResourceStorageModeShared];
    id<MTLBuffer> rhs_buffer = [device newBufferWithBytes:rhs_values.data()
                                                   length:rhs_values.size() * sizeof(double)
                                                  options:MTLResourceStorageModeShared];
    id<MTLBuffer> out_buffer = [device newBufferWithLength:out_values.size() * sizeof(double)
                                                   options:MTLResourceStorageModeShared];
    id<MTLBuffer> lhs_rows_buffer = [device newBufferWithBytes:&lhs_rows length:sizeof(lhs_rows) options:MTLResourceStorageModeShared];
    id<MTLBuffer> lhs_cols_buffer = [device newBufferWithBytes:&lhs_cols length:sizeof(lhs_cols) options:MTLResourceStorageModeShared];
    id<MTLBuffer> rhs_cols_buffer = [device newBufferWithBytes:&rhs_cols length:sizeof(rhs_cols) options:MTLResourceStorageModeShared];

    id<MTLCommandBuffer> command_buffer = [queue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [command_buffer computeCommandEncoder];
    [encoder setComputePipelineState:state];
    [encoder setBuffer:lhs_buffer offset:0 atIndex:0];
    [encoder setBuffer:rhs_buffer offset:0 atIndex:1];
    [encoder setBuffer:out_buffer offset:0 atIndex:2];
    [encoder setBuffer:lhs_rows_buffer offset:0 atIndex:3];
    [encoder setBuffer:lhs_cols_buffer offset:0 atIndex:4];
    [encoder setBuffer:rhs_cols_buffer offset:0 atIndex:5];

    const NSUInteger thread_width = std::max<NSUInteger>(1, state.threadExecutionWidth);
    const NSUInteger thread_height = std::max<NSUInteger>(1, state.maxTotalThreadsPerThreadgroup / thread_width);
    const MTLSize grid = MTLSizeMake(static_cast<NSUInteger>(rhs.cols()), static_cast<NSUInteger>(lhs.rows()), 1);
    const MTLSize group = MTLSizeMake(thread_width, thread_height, 1);
    [encoder dispatchThreads:grid threadsPerThreadgroup:group];
    [encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];

    std::copy_n(static_cast<const double*>(out_buffer.contents), out_values.size(), out_values.begin());
    return std::make_unique<DenseMatrix>(make_matrix_from_row_major(lhs.rows(), rhs.cols(), out_values));
}

#endif

} // namespace detail
} // namespace metal
} // namespace mytrix