package io.coolbox.vision;
/** Random rotation (Python: {@code computer_vision.RandomRotation}). */
public final class RandomRotation extends Transform { public RandomRotation(float minAngle,float maxAngle){this(minAngle,maxAngle,InterpolationMode.BILINEAR,0);} public RandomRotation(float minAngle,float maxAngle,InterpolationMode mode,int seed){super(create(9,new int[]{seed},new float[]{minAngle,maxAngle},new float[0],new float[0],mode.value));} }

