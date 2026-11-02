package io.coolbox.decisiontree;
public enum SplitCriterion { GINI(0),ENTROPY(1);final int value;SplitCriterion(int v){value=v;} }
