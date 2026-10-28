package io.coolbox.datastructures;
/** Concurrent integer hash map (Python: {@code data_structures.Int64ConcurrentMap}). */
public final class Int64ConcurrentMap extends Int64MapBase { public Int64ConcurrentMap(){this(16);} public Int64ConcurrentMap(int initialCapacity){super(true,initialCapacity);} }
