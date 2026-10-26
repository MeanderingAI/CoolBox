package io.coolbox.vision;
/** 2D convolution layer (Python: {@code computer_vision.Conv2DLayer}). */
public final class Conv2DLayer extends VisionLayer {
 public Conv2DLayer(int inChannels,int outChannels,int kernelSize){this(inChannels,outChannels,kernelSize,1,0);}public Conv2DLayer(int inChannels,int outChannels,int kernelSize,int stride,int padding){super(0,new int[]{inChannels,outChannels,kernelSize,stride,padding},0,0);}
 public int inChannels(){return property(0);}public int outChannels(){return property(1);}public int kernelSize(){return property(2);}public int stride(){return property(3);}public int padding(){return property(4);}
}
