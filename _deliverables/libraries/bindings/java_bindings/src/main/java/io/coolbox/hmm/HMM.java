package io.coolbox.hmm;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;
import java.util.Arrays;

/** Discrete hidden Markov model (Python: {@code ml_core.hmm.HMM}). */
public final class HMM extends NativeHandle {
    private static final HmmNative NATIVE = HmmNative.INSTANCE;
    public HMM(int states, int observations) { super(create(states, observations), NATIVE::coolbox_hmm_free); }
    private static Pointer create(int states,int observations) { var e=new ErrorBuffer(); return e.check(NATIVE.coolbox_hmm_new(states,observations,e.pointer(),e.capacity())); }
    /** Sets the initial state distribution (Python: {@code set_initial_probabilities}). */
    public HMM setInitialProbabilities(double[] probabilities) { var e=new ErrorBuffer(); e.check(NATIVE.coolbox_hmm_set_initial(handle(),probabilities,probabilities.length,e.pointer(),e.capacity())); return this; }
    /** Sets the state transition matrix (Python: {@code set_transition_matrix}). */
    public HMM setTransitionMatrix(double[][] matrix) { int c=Matrices.columns(matrix); var e=new ErrorBuffer(); e.check(NATIVE.coolbox_hmm_set_transition(handle(),Matrices.flatten(matrix),matrix.length,c,e.pointer(),e.capacity())); return this; }
    /** Sets the emission matrix (Python: {@code set_emission_matrix}). */
    public HMM setEmissionMatrix(double[][] matrix) { int c=Matrices.columns(matrix); var e=new ErrorBuffer(); e.check(NATIVE.coolbox_hmm_set_emission(handle(),Matrices.flatten(matrix),matrix.length,c,e.pointer(),e.capacity())); return this; }
    public double[] initialProbabilities() { var n=new LongByReference(); var e=new ErrorBuffer(); Pointer p=NATIVE.coolbox_hmm_get_initial(handle(),n,e.pointer(),e.capacity()); e.throwIfSet(); return Matrices.takeDoubles(p,n.getValue()); }
    public double[][] transitionMatrix() { var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();Pointer p=NATIVE.coolbox_hmm_get_transition(handle(),r,c,e.pointer(),e.capacity());e.throwIfSet();return Matrices.takeMatrix(p,r.getValue(),c.getValue()); }
    public double[][] emissionMatrix() { var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();Pointer p=NATIVE.coolbox_hmm_get_emission(handle(),r,c,e.pointer(),e.capacity());e.throwIfSet();return Matrices.takeMatrix(p,r.getValue(),c.getValue()); }
    /** Calculates the observation-sequence log likelihood (Python: {@code log_likelihood}). */
    public double logLikelihood(int[] observations) { var e=new ErrorBuffer(); double value=NATIVE.coolbox_hmm_log_likelihood(handle(),observations,observations.length,e.pointer(),e.capacity());e.throwIfSet();return value; }
    /** Returns the Viterbi most-likely state sequence (Python: {@code get_most_likely_states}). */
    public int[] mostLikelyStates(int[] observations) { var n=new LongByReference();var e=new ErrorBuffer();Pointer p=NATIVE.coolbox_hmm_viterbi(handle(),observations,observations.length,n,e.pointer(),e.capacity());e.throwIfSet();return Matrices.takeInts(p,n.getValue()); }
    /** Fits parameters with Baum–Welch (Python: {@code train}). */
    public HMM train(int[][] sequences) { return train(sequences,100,1e-6,0,0); }
    public HMM train(int[][] sequences,int maxIterations,double tolerance,double smoothingFactor,int seed) {
        int total=Arrays.stream(sequences).mapToInt(a->a.length).sum(); int[] values=new int[total]; long[] offsets=new long[sequences.length+1]; int at=0;
        for(int i=0;i<sequences.length;i++){ offsets[i]=at;System.arraycopy(sequences[i],0,values,at,sequences[i].length);at+=sequences[i].length; } offsets[sequences.length]=at;
        var e=new ErrorBuffer();e.check(NATIVE.coolbox_hmm_train(handle(),values,offsets,sequences.length,total,maxIterations,tolerance,smoothingFactor,seed,e.pointer(),e.capacity()));return this;
    }
}
