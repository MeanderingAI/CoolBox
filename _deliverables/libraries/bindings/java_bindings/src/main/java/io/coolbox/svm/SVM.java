package io.coolbox.svm;

import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

/** Support-vector classifier (Python: {@code ml_core.svm.SVM}). */
public final class SVM extends NativeHandle {
    private static final SvmNative NATIVE = SvmNative.INSTANCE;
    private final Kernel kernel;

    public SVM(Kernel kernel) {
        super(create(kernel), NATIVE::coolbox_svm_free);
        this.kernel = kernel;
    }

    private static com.sun.jna.Pointer create(Kernel kernel) {
        if (kernel == null) throw new IllegalArgumentException("kernel must not be null");
        var error = new ErrorBuffer();
        return error.check(NATIVE.coolbox_svm_new(kernel.handle(), error.pointer(), error.capacity()));
    }

    /** Trains the classifier (Python: {@code SVM.fit}). */
    public SVM fit(double[][] x, double[] y) {
        if (x.length != y.length) throw new IllegalArgumentException("X and y must have the same number of rows");
        int cols = Matrices.columns(x);
        var error = new ErrorBuffer();
        error.check(NATIVE.coolbox_svm_fit(handle(), Matrices.flatten(x), x.length, cols, y, y.length,
                error.pointer(), error.capacity()));
        return this;
    }

    /** Predicts a class label (Python: {@code SVM.predict}). */
    public double predict(double[] sample) {
        var out = new DoubleByReference();
        var error = new ErrorBuffer();
        error.check(NATIVE.coolbox_svm_predict(handle(), sample, sample.length, out, error.pointer(), error.capacity()));
        return out.getValue();
    }
}
