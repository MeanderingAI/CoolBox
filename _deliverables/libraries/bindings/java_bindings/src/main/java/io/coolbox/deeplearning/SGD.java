package io.coolbox.deeplearning;
/** SGD optimizer (Python: {@code deep_learning.SGD}). */
public final class SGD extends Optimizer { public SGD(double learningRate){this(learningRate,0.0);} public SGD(double learningRate,double momentum){super(0,learningRate,momentum,0,0);} }
