package io.coolbox.deeplearning;
/** RMSprop optimizer (Python: {@code deep_learning.RMSprop}). */
public final class RMSprop extends Optimizer { public RMSprop(){this(.001,.9,1e-8);} public RMSprop(double learningRate,double decay,double epsilon){super(2,learningRate,decay,epsilon,0);} }
