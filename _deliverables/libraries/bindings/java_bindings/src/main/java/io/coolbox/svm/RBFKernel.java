package io.coolbox.svm;

import io.coolbox.internal.ErrorBuffer;

/** Gaussian radial-basis kernel (Python: {@code ml_core.svm.RBFKernel}). */
public final class RBFKernel extends Kernel {
    public RBFKernel(double gamma) { super(create(gamma)); }
    private static com.sun.jna.Pointer create(double gamma) { var e = new ErrorBuffer(); return e.check(NATIVE.coolbox_svm_rbf_kernel_new(gamma, e.pointer(), e.capacity())); }
}
