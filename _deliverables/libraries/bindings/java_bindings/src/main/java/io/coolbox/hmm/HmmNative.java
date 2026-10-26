package io.coolbox.hmm;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.NativeLibraries;

interface HmmNative extends Library {
    HmmNative INSTANCE = NativeLibraries.bridge(HmmNative.class);
    Pointer coolbox_hmm_new(int states, int observations, Pointer error, long capacity);
    void coolbox_hmm_free(Pointer hmm);
    int coolbox_hmm_set_initial(Pointer hmm, double[] probabilities, long count, Pointer error, long capacity);
    int coolbox_hmm_set_transition(Pointer hmm, double[] values, long rows, long cols, Pointer error, long capacity);
    int coolbox_hmm_set_emission(Pointer hmm, double[] values, long rows, long cols, Pointer error, long capacity);
    Pointer coolbox_hmm_get_initial(Pointer hmm, LongByReference count, Pointer error, long capacity);
    Pointer coolbox_hmm_get_transition(Pointer hmm, LongByReference rows, LongByReference cols, Pointer error, long capacity);
    Pointer coolbox_hmm_get_emission(Pointer hmm, LongByReference rows, LongByReference cols, Pointer error, long capacity);
    double coolbox_hmm_log_likelihood(Pointer hmm, int[] observations, long count, Pointer error, long capacity);
    Pointer coolbox_hmm_viterbi(Pointer hmm, int[] observations, long length, LongByReference count, Pointer error, long capacity);
    int coolbox_hmm_train(Pointer hmm, int[] observations, long[] offsets, long sequences, long total, int iterations, double tolerance, double smoothing, int seed, Pointer error, long capacity);
}
