package io.coolbox.svm;

import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;

/** Base for SVM kernels (Python: {@code ml_core.svm.Kernel}). */
public abstract class Kernel extends NativeHandle {
    static final SvmNative NATIVE = SvmNative.INSTANCE;

    Kernel(com.sun.jna.Pointer handle) {
        super(handle, NATIVE::coolbox_svm_kernel_free);
    }

    /** Calculates the kernel similarity (Python: {@code Kernel.calculate}). */
    public final double calculate(double[] x, double[] y) {
        var out = new com.sun.jna.ptr.DoubleByReference();
        var error = new ErrorBuffer();
        error.check(NATIVE.coolbox_svm_kernel_calculate(handle(), x, x.length, y, y.length, out,
                error.pointer(), error.capacity()));
        return out.getValue();
    }
}
