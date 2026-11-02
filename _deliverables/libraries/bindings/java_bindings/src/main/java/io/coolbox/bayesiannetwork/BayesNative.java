package io.coolbox.bayesiannetwork;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.NativeLibraries;

interface BayesNative extends Library {
    BayesNative INSTANCE = NativeLibraries.bridge(BayesNative.class);
    Pointer coolbox_bayes_network_new(Pointer error, long capacity);
    void coolbox_bayes_network_free(Pointer network);
    int coolbox_bayes_add_node(Pointer network, String name, Pointer[] states, long count, IntByReference index, Pointer error, long capacity);
    int coolbox_bayes_add_edge(Pointer network, int parent, int child, Pointer error, long capacity);
    int coolbox_bayes_set_cpt(Pointer network, int node, double[] values, long rows, long cols, Pointer error, long capacity);
    int coolbox_bayes_joint_probability(Pointer network, int[] keys, int[] values, long count, DoubleByReference result, Pointer error, long capacity);
    int coolbox_bayes_infer(Pointer network, int queryNode, int queryState, int[] keys, int[] values, long count, DoubleByReference result, Pointer error, long capacity);
}
