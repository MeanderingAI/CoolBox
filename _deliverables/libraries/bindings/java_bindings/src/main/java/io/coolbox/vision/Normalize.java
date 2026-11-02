package io.coolbox.vision;
/** Channel normalization transform (Python: {@code computer_vision.Normalize}). */
public final class Normalize extends Transform { public Normalize(float[] mean,float[] std){super(create(6,new int[0],new float[0],mean,std,0));} }

