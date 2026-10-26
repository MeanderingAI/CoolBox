package io.coolbox.vision;
/** Resize transform (Python: {@code computer_vision.Resize}). */
public final class Resize extends Transform { public Resize(int height,int width){this(height,width,InterpolationMode.BILINEAR);} public Resize(int height,int width,InterpolationMode mode){super(create(0,new int[]{height,width},new float[0],new float[0],new float[0],mode.value));} }

