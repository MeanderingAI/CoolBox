package io.coolbox.svm;

import io.coolbox.internal.ErrorBuffer;

/** Hyperbolic-tangent SVM kernel (Python: {@code ml_core.svm.SigmoidKernel}). */
public final class SigmoidKernel extends Kernel {
    public SigmoidKernel(double gamma, double coef0) { super(create(gamma, coef0)); }
    private static com.sun.jna.Pointer create(double gamma, double coef0) { var e = new ErrorBuffer(); return e.check(NATIVE.coolbox_svm_sigmoid_kernel_new(gamma, coef0, e.pointer(), e.capacity())); }
}
