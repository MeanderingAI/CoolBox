package io.coolbox.vision;
/** Random crop (Python: {@code computer_vision.RandomCrop}). */
public final class RandomCrop extends Transform { public RandomCrop(int height,int width){this(height,width,0);} public RandomCrop(int height,int width,int seed){super(create(2,new int[]{height,width,seed},new float[0],new float[0],new float[0],0));} }

