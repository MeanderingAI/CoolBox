package io.coolbox.datastructures;
/** Bloom membership filter (Python: {@code data_structures.BloomFilter}). */
public final class BloomFilter extends Int64Filter { public BloomFilter(){this(1000,.01);} public BloomFilter(long expectedInsertions,double falsePositiveRate){super(0,expectedInsertions,falsePositiveRate);} }
