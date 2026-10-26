package io.coolbox.deeplearning;
/** Dropout layer (Python: {@code deep_learning.DropoutLayer}). */
public final class DropoutLayer extends Layer {
    public DropoutLayer(double rate){super(create(5,new long[0],rate));}
}
