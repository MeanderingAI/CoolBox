package io.coolbox.vision;
/** Python equivalent {@code computer_vision.AvgPool2DLayer}. */
public final class AvgPool2DLayer extends VisionLayer { public AvgPool2DLayer(int kernelSize){this(kernelSize,-1);} public AvgPool2DLayer(int kernelSize,int stride){super(2,new int[]{kernelSize,stride},0,0);} public int kernelSize(){return property(2);} public int stride(){return property(3);} }

