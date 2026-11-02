package io.coolbox.tracker;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.NativeLibraries;

interface TrackerNative extends Library {
    TrackerNative INSTANCE = NativeLibraries.bridge(TrackerNative.class);
    Pointer coolbox_tracker_kalman_new(double dt,double[] a,long ar,long ac,double[] c,long cr,long cc,double[] q,long qr,long qc,double[] r,long rr,long rc,double[] p,long pr,long pc,Pointer error,long capacity);
    void coolbox_tracker_kalman_free(Pointer filter);
    int coolbox_tracker_kalman_init(Pointer filter,double[] x,long n,Pointer error,long capacity);
    int coolbox_tracker_kalman_predict(Pointer filter,Pointer error,long capacity);
    int coolbox_tracker_kalman_update(Pointer filter,double[] y,long n,Pointer error,long capacity);
    Pointer coolbox_tracker_kalman_state(Pointer filter,LongByReference count,Pointer error,long capacity);
    Pointer coolbox_tracker_kalman_covariance(Pointer filter,LongByReference rows,LongByReference cols,Pointer error,long capacity);
}
