package io.coolbox.vision;
/** Center crop (Python: {@code computer_vision.CenterCrop}). */
public final class CenterCrop extends Transform { public CenterCrop(int height,int width){super(create(1,new int[]{height,width},new float[0],new float[0],new float[0],0));} }

