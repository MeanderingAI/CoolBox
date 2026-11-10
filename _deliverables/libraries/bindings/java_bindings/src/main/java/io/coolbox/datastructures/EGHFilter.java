package io.coolbox.datastructures;
/** EGH membership filter (Python: {@code data_structures.EGHFilter}). */
public final class EGHFilter extends Int64Filter { public EGHFilter(){this(1000,.01);} public EGHFilter(long expectedInsertions,double falsePositiveRate){super(1,expectedInsertions,falsePositiveRate);} }
