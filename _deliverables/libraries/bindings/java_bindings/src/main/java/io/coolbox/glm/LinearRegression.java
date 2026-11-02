package io.coolbox.glm;

import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

import java.util.Arrays;

/** Ordinary least squares regression (Python: {@code ml_core.glm.LinearRegression}). */
public final class LinearRegression extends NativeHandle implements GLM {
    private static final GlmNative NATIVE = GlmNative.INSTANCE;

    public LinearRegression() {
        this(10000, 0.01, FitMethod.GRADIENT_DESCENT);
    }

    public LinearRegression(LinearRegressionFitMethod method) {
        this(method.getNumIterations(), method.getLearningRate(), method.getType());
    }

    public LinearRegression(int iterations, double learningRate, FitMethod method) {
        super(NATIVE.coolbox_glm_linear_regression_new(iterations, learningRate, method.nativeValue),
                NATIVE::coolbox_glm_linear_regression_free);
    }

    @Override
    public LinearRegression fit(double[][] x, double[] y) {
        if (x.length != y.length) {
            throw new IllegalArgumentException("x and y must have the same number of rows");
        }
        int cols = Matrices.columns(x);
        ErrorBuffer err = new ErrorBuffer();
        err.check(NATIVE.coolbox_glm_linear_regression_fit(handle(), Matrices.flatten(x), x.length, cols, y,
                err.pointer(), err.capacity()));
        return this;
    }

    @Override
    public double predict(double[] sample) {
        DoubleByReference out = new DoubleByReference();
        ErrorBuffer err = new ErrorBuffer();
        err.check(NATIVE.coolbox_glm_linear_regression_predict(handle(), sample, sample.length, out,
                err.pointer(), err.capacity()));
        return out.getValue();
    }

    public double[] predict(double[][] samples) {
        double[] out = new double[samples.length];
        for (int i = 0; i < samples.length; i++) {
            out[i] = predict(samples[i]);
        }
        return out;
    }

    public double[] coefficients() {
        return Arrays.copyOf(readCoefficients(null), featureCount());
    }

    public double intercept() {
        DoubleByReference intercept = new DoubleByReference();
        readCoefficients(intercept);
        return intercept.getValue();
    }

    private int featureCount() {
        return (int) NATIVE.coolbox_glm_linear_regression_feature_count(handle());
    }

    private double[] readCoefficients(DoubleByReference interceptOut) {
        double[] weights = new double[Math.max(featureCount(), 1)];
        DoubleByReference intercept = interceptOut != null ? interceptOut : new DoubleByReference();
        ErrorBuffer err = new ErrorBuffer();
        err.check(NATIVE.coolbox_glm_linear_regression_coefficients(handle(), weights, weights.length, intercept,
                err.pointer(), err.capacity()));
        return weights;
    }
}



