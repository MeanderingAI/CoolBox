package io.coolbox.svm;

import io.coolbox.internal.ErrorBuffer;

/** Linear SVM kernel (Python: {@code ml_core.svm.LinearKernel}). */
public final class LinearKernel extends Kernel {
    public LinearKernel() { super(create()); }
    private static com.sun.jna.Pointer create() { var e = new ErrorBuffer(); return e.check(NATIVE.coolbox_svm_linear_kernel_new(e.pointer(), e.capacity())); }
}
