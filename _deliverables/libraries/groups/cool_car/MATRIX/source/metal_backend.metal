#include <metal_stdlib>
using namespace metal;

kernel void matrix_add(const device double* lhs [[buffer(0)]],
                       const device double* rhs [[buffer(1)]],
                       device double* out [[buffer(2)]],
                       uint gid [[thread_position_in_grid]]) {
    out[gid] = lhs[gid] + rhs[gid];
}

kernel void matrix_transpose(const device double* input [[buffer(0)]],
                             device double* output [[buffer(1)]],
                             constant uint& rows [[buffer(2)]],
                             constant uint& cols [[buffer(3)]],
                             uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= cols || gid.y >= rows) {
        return;
    }
    const uint src_index = gid.y * cols + gid.x;
    const uint dst_index = gid.x * rows + gid.y;
    output[dst_index] = input[src_index];
}

kernel void matrix_multiply(const device double* lhs [[buffer(0)]],
                            const device double* rhs [[buffer(1)]],
                            device double* out [[buffer(2)]],
                            constant uint& lhs_rows [[buffer(3)]],
                            constant uint& lhs_cols [[buffer(4)]],
                            constant uint& rhs_cols [[buffer(5)]],
                            uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= rhs_cols || gid.y >= lhs_rows) {
        return;
    }

    double value = 0.0;
    const uint row = gid.y;
    const uint col = gid.x;
    for (uint k = 0; k < lhs_cols; ++k) {
        value += lhs[row * lhs_cols + k] * rhs[k * rhs_cols + col];
    }
    out[row * rhs_cols + col] = value;
}