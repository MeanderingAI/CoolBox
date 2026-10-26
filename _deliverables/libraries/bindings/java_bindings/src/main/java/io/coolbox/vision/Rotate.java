package io.coolbox.vision;
/** Rotation transform (Python: {@code computer_vision.Rotate}). */
public final class Rotate extends Transform { public Rotate(float angleDegrees){this(angleDegrees,InterpolationMode.BILINEAR);} public Rotate(float angleDegrees,InterpolationMode mode){super(create(8,new int[0],new float[]{angleDegrees},new float[0],new float[0],mode.value));} }

