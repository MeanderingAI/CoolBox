package io.coolbox.deeplearning;
/** Adam optimizer (Python: {@code deep_learning.Adam}). */
public final class Adam extends Optimizer { public Adam(){this(.001,.9,.999,1e-8);} public Adam(double learningRate,double beta1,double beta2,double epsilon){super(1,learningRate,beta1,beta2,epsilon);} }
