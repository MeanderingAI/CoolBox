package io.coolbox.opt;
/** Configuration (Python: {@code opt.OwaConfig}). */
public final class OwaConfig {public int mergeIterations=500;public double learningRate=.05,l2Regularization=1e-4;public boolean projectToSimplex=true;public OwaConfig(){}public OwaConfig mergeIterations(int v){mergeIterations=v;return this;}public OwaConfig learningRate(double v){learningRate=v;return this;}public OwaConfig l2Regularization(double v){l2Regularization=v;return this;}public OwaConfig projectToSimplex(boolean v){projectToSimplex=v;return this;}}
