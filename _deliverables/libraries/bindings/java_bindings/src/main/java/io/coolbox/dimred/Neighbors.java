package io.coolbox.dimred;

/** Neighbor indices and distances returned by KNN. */
public record Neighbors(int[][] indices,double[][] distances) { }
