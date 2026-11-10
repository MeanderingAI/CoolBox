package io.coolbox.glm;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.NativeLibraries;

interface GlmNative extends Library {
    GlmNative INSTANCE = NativeLibraries.bridge(GlmNative.class);

    Pointer coolbox_glm_linear_regression_new(int iterations, double learningRate, int fitType);

    void coolbox_glm_linear_regression_free(Pointer model);

    int coolbox_glm_linear_regression_fit(Pointer model, double[] x, long rows, long cols, double[] y,
                                          Pointer error, long errorCapacity);

    int coolbox_glm_linear_regression_predict(Pointer model, double[] sample, long cols,
                                              DoubleByReference prediction, Pointer error, long errorCapacity);

    int coolbox_glm_linear_regression_coefficients(Pointer model, double[] weights, long weightCapacity,
                                                   DoubleByReference intercept, Pointer error, long errorCapacity);

    long coolbox_glm_linear_regression_feature_count(Pointer model);
}
