package io.coolbox.glm;

/** Mirrors Python's {@code glm.LinearRegressionFitMethod} solver choice. */
public enum FitMethod {
    GRADIENT_DESCENT(0),
    CLOSED_FORM(1);

    final int nativeValue;

    FitMethod(int nativeValue) {
        this.nativeValue = nativeValue;
    }
}
