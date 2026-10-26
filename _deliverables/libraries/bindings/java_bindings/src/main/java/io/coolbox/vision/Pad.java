package io.coolbox.vision;
/** Border padding (Python: {@code computer_vision.Pad}). */
public final class Pad extends Transform { public Pad(int top,int bottom,int left,int right){this(top,bottom,left,right,0);} public Pad(int top,int bottom,int left,int right,float fillValue){super(create(13,new int[]{top,bottom,left,right},new float[]{fillValue},new float[0],new float[0],0));} }

