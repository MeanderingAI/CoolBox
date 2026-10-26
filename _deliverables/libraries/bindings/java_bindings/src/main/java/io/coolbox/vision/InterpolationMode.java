package io.coolbox.vision;
/** Resampling mode (Python: {@code computer_vision.InterpolationMode}). */
public enum InterpolationMode { NEAREST(0), BILINEAR(1), BICUBIC(2); final int value; InterpolationMode(int v){value=v;} }
