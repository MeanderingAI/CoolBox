package io.coolbox.vision;
/** Gaussian blur (Python: {@code computer_vision.GaussianBlur}). */
public final class GaussianBlur extends Transform { public GaussianBlur(int kernelSize,float sigma){super(create(12,new int[]{kernelSize},new float[]{sigma},new float[0],new float[0],0));} }

