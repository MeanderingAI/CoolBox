package io.coolbox.svm;

import io.coolbox.internal.ErrorBuffer;

/** Polynomial SVM kernel (Python: {@code ml_core.svm.PolynomialKernel}). */
public final class PolynomialKernel extends Kernel {
    public PolynomialKernel(int degree, double gamma, double coef0) { super(create(degree, gamma, coef0)); }
    private static com.sun.jna.Pointer create(int degree, double gamma, double coef0) { var e = new ErrorBuffer(); return e.check(NATIVE.coolbox_svm_polynomial_kernel_new(degree, gamma, coef0, e.pointer(), e.capacity())); }
}
