package io.coolbox.svm;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.NativeLibraries;

interface SvmNative extends Library {
    SvmNative INSTANCE = NativeLibraries.bridge(SvmNative.class);
    Pointer coolbox_svm_linear_kernel_new(Pointer error, long capacity);
    Pointer coolbox_svm_rbf_kernel_new(double gamma, Pointer error, long capacity);
    Pointer coolbox_svm_polynomial_kernel_new(int degree, double gamma, double coef0, Pointer error, long capacity);
    Pointer coolbox_svm_sigmoid_kernel_new(double gamma, double coef0, Pointer error, long capacity);
    void coolbox_svm_kernel_free(Pointer kernel);
    int coolbox_svm_kernel_calculate(Pointer kernel, double[] x, long nx, double[] y, long ny, DoubleByReference out, Pointer error, long capacity);
    Pointer coolbox_svm_new(Pointer kernel, Pointer error, long capacity);
    void coolbox_svm_free(Pointer svm);
    int coolbox_svm_fit(Pointer svm, double[] x, long rows, long cols, double[] y, long ny, Pointer error, long capacity);
    int coolbox_svm_predict(Pointer svm, double[] sample, long count, DoubleByReference out, Pointer error, long capacity);
}
