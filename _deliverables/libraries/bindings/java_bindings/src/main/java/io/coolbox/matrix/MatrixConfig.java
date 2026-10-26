package io.coolbox.matrix;

/** Process-wide matrix backend and CPU configuration (Python: {@code matrix} module functions). */
public final class MatrixConfig {
    private MatrixConfig() { }
    public static String optimizationName(CpuOptimization value) { return MatrixNative.INSTANCE.coolbox_cpu_optimization_name(value.ordinal()); }
    public static boolean optimizationAvailable(CpuOptimization value) { return MatrixNative.INSTANCE.coolbox_cpu_optimization_available(value.ordinal()) != 0; }
    public static void setCpuThreads(long count) { if (count < 0) throw new IllegalArgumentException("thread count cannot be negative"); MatrixNative.INSTANCE.coolbox_matrix_set_cpu_threads(count); }
    public static long cpuThreads() { return MatrixNative.INSTANCE.coolbox_matrix_cpu_threads(); }
    public static void setBackend(MatrixBackend backend) { var e = new io.coolbox.internal.ErrorBuffer(); e.check(MatrixNative.INSTANCE.coolbox_matrix_set_backend(backend.ordinal(), e.pointer(), e.capacity())); }
    public static MatrixBackend backend() { return MatrixBackend.values()[MatrixNative.INSTANCE.coolbox_matrix_get_backend()]; }
    public static MatrixBackend activeBackend() { return MatrixBackend.values()[MatrixNative.INSTANCE.coolbox_matrix_get_active_backend()]; }
    public static String backendName(MatrixBackend backend) { return MatrixNative.INSTANCE.coolbox_matrix_backend_name(backend.ordinal()); }
    public static boolean backendAvailable(MatrixBackend backend) { return MatrixNative.INSTANCE.coolbox_matrix_backend_available(backend.ordinal()) != 0; }
}
