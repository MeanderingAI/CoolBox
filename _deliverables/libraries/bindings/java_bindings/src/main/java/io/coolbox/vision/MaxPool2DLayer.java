package io.coolbox.vision;
/** Python equivalent {@code computer_vision.MaxPool2DLayer}. */
public final class MaxPool2DLayer extends VisionLayer { public MaxPool2DLayer(int kernelSize){this(kernelSize,-1);} public MaxPool2DLayer(int kernelSize,int stride){super(1,new int[]{kernelSize,stride},0,0);} public int kernelSize(){return property(2);} public int stride(){return property(3);} }

