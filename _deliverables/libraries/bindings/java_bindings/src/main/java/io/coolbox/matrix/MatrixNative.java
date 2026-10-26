package io.coolbox.matrix;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import io.coolbox.internal.NativeLibraries;
import com.sun.jna.ptr.DoubleByReference;

interface MatrixNative extends Library {
    MatrixNative INSTANCE = NativeLibraries.bridge(MatrixNative.class);
    Pointer coolbox_matrix_new(long rows, long columns, double[] values, long count, Pointer error, long errorCapacity);
    Pointer coolbox_matrix_identity(long size, Pointer error, long errorCapacity);
    void coolbox_matrix_free(Pointer matrix);
    long coolbox_matrix_rows(Pointer matrix);
    long coolbox_matrix_columns(Pointer matrix);
    int coolbox_matrix_copy_data(Pointer matrix, double[] out, long capacity, Pointer error, long errorCapacity);
    int coolbox_matrix_get(Pointer matrix, long row, long column, DoubleByReference value, Pointer error, long errorCapacity);
    int coolbox_matrix_set(Pointer matrix, long row, long column, double value, Pointer error, long errorCapacity);
    Pointer coolbox_matrix_binary(Pointer lhs, Pointer rhs, int operation, int optimization, Pointer error, long errorCapacity);
    Pointer coolbox_matrix_transpose(Pointer matrix, Pointer error, long errorCapacity);
    Pointer coolbox_matrix_rowwise_add(Pointer matrix, Pointer vector, int optimization, Pointer error, long errorCapacity);
    double coolbox_matrix_norm(Pointer matrix);
    Pointer coolbox_vector_new(double[] values, long size, Pointer error, long errorCapacity);
    void coolbox_vector_free(Pointer vector);
    long coolbox_vector_size(Pointer vector);
    int coolbox_vector_copy_data(Pointer vector, double[] out, long capacity, Pointer error, long errorCapacity);
    int coolbox_vector_get(Pointer vector, long index, DoubleByReference value, Pointer error, long errorCapacity);
    int coolbox_vector_set(Pointer vector, long index, double value, Pointer error, long errorCapacity);
    double coolbox_vector_norm(Pointer vector);
    int coolbox_vector_dot(Pointer lhs, Pointer rhs, int optimization, DoubleByReference out, Pointer error, long errorCapacity);
    Pointer coolbox_vector_add(Pointer lhs, Pointer rhs, int optimization, Pointer error, long errorCapacity);
    Pointer coolbox_vector_transpose(Pointer vector, Pointer error, long errorCapacity);
    String coolbox_cpu_optimization_name(int value);
    int coolbox_cpu_optimization_available(int value);
    void coolbox_matrix_set_cpu_threads(long count);
    long coolbox_matrix_cpu_threads();
    int coolbox_matrix_set_backend(int backend, Pointer error, long errorCapacity);
    int coolbox_matrix_get_backend();
    int coolbox_matrix_get_active_backend();
    String coolbox_matrix_backend_name(int backend);
    int coolbox_matrix_backend_available(int backend);
}
