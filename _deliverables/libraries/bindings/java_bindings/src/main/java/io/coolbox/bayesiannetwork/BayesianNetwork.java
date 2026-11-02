package io.coolbox.bayesiannetwork;

import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.DoubleByReference;
import com.sun.jna.ptr.IntByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;
import java.nio.charset.StandardCharsets;
import java.util.Map;

/** Discrete Bayesian network (Python: {@code ml_core.bayesian_network.BayesianNetwork}). */
public final class BayesianNetwork extends NativeHandle {
    private static final BayesNative NATIVE = BayesNative.INSTANCE;
    public BayesianNetwork() { super(create(), NATIVE::coolbox_bayes_network_free); }
    private static Pointer create() { var e = new ErrorBuffer(); return e.check(NATIVE.coolbox_bayes_network_new(e.pointer(), e.capacity())); }

    /** Adds a named variable and returns its node index (Python: {@code add_node}). */
    public int addNode(String name, java.util.List<String> states) {
        try (var stateMemory = new StringArray(states)) {
            var out = new IntByReference(); var e = new ErrorBuffer();
            e.check(NATIVE.coolbox_bayes_add_node(handle(), name, stateMemory.pointers, states.size(), out, e.pointer(), e.capacity()));
            return out.getValue();
        }
    }
    /** Adds a directed dependency (Python: {@code add_edge}). */
    public BayesianNetwork addEdge(int parentIndex, int childIndex) {
        var e = new ErrorBuffer(); e.check(NATIVE.coolbox_bayes_add_edge(handle(), parentIndex, childIndex, e.pointer(), e.capacity())); return this;
    }
    /** Sets the conditional probability table (Python: {@code set_cpt}). */
    public BayesianNetwork setCpt(int nodeIndex, double[][] cpt) {
        int cols = Matrices.columns(cpt); var e = new ErrorBuffer();
        e.check(NATIVE.coolbox_bayes_set_cpt(handle(), nodeIndex, Matrices.flatten(cpt), cpt.length, cols, e.pointer(), e.capacity())); return this;
    }
    /** Calculates a complete assignment's joint probability (Python: {@code calculate_joint_probability}). */
    public double calculateJointProbability(Map<Integer, Integer> assignment) { return evaluate(assignment, false, 0, 0); }
    /** Calculates a conditional probability (Python: {@code infer}). */
    public double infer(int queryNodeIndex, int queryStateIndex, Map<Integer, Integer> evidence) { return evaluate(evidence, true, queryNodeIndex, queryStateIndex); }
    private double evaluate(Map<Integer, Integer> assignment, boolean infer, int query, int state) {
        int n = assignment.size(), i = 0; int[] keys = new int[n], values = new int[n];
        for (var entry : assignment.entrySet()) { keys[i] = entry.getKey(); values[i++] = entry.getValue(); }
        var out = new DoubleByReference(); var e = new ErrorBuffer();
        int status = infer ? NATIVE.coolbox_bayes_infer(handle(), query, state, keys, values, n, out, e.pointer(), e.capacity())
                : NATIVE.coolbox_bayes_joint_probability(handle(), keys, values, n, out, e.pointer(), e.capacity());
        e.check(status); return out.getValue();
    }
    private static final class StringArray implements AutoCloseable {
        final Memory block; final Pointer[] pointers;
        StringArray(java.util.List<String> strings) {
            pointers = new Pointer[strings.size()]; block = new Memory(Math.max(1, strings.size()) * Native.POINTER_SIZE);
            for (int i=0;i<strings.size();i++) { byte[] bytes=(strings.get(i)+"\0").getBytes(StandardCharsets.UTF_8); Memory text=new Memory(bytes.length); text.write(0,bytes,0,bytes.length); pointers[i]=text; block.setPointer((long)i*Native.POINTER_SIZE,text); }
        }
        @Override public void close() { }
    }
}
