package io.coolbox.ensemble;
public final class BoostTreeParameters {public int numEstimators=100;public double learningRate=.1;public int maxDepth=3;public BoostTreeParameters(){}public BoostTreeParameters(int n,double rate,int depth){numEstimators=n;learningRate=rate;maxDepth=depth;}}
