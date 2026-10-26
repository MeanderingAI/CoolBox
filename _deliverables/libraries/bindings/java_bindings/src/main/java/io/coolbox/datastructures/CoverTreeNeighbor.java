package io.coolbox.datastructures;
/** Search result returned by a cover-tree nearest-neighbor query. */
public record CoverTreeNeighbor(long index,double distance,double[] item) { }
