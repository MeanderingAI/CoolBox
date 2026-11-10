package io.coolbox.vision;
/** Random horizontal flip (Python: {@code computer_vision.RandomHorizontalFlip}). */
public final class RandomHorizontalFlip extends Transform { public RandomHorizontalFlip(){this(.5f,0);} public RandomHorizontalFlip(float probability,int seed){super(create(5,new int[]{seed},new float[]{probability},new float[0],new float[0],0));} }

