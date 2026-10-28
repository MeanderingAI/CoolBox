package io.coolbox.glm;
/** Common fitting and prediction surface (Python: {@code glm.GLM}). */
public interface GLM {GLM fit(double[][] x,double[] y);double predict(double[] sample);}
