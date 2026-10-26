package io.coolbox.kmeans;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.IntByReference;
import io.coolbox.internal.NativeLibraries;

interface KMeansNative extends Library {
    KMeansNative INSTANCE=NativeLibraries.bridge(KMeansNative.class);
    Pointer coolbox_kmeans_new(int clusters,int iterations,int init,int seed,Pointer error,long capacity);
    void coolbox_kmeans_free(Pointer model);
    int coolbox_kmeans_fit(Pointer model,double[] x,long rows,long cols,Pointer error,long capacity);
    Pointer coolbox_kmeans_predict(Pointer model,double[] x,long rows,long cols,LongByReference count,Pointer error,long capacity);
    Pointer coolbox_kmeans_fit_predict(Pointer model,double[] x,long rows,long cols,LongByReference count,Pointer error,long capacity);
    Pointer coolbox_kmeans_centroids(Pointer model,LongByReference rows,LongByReference cols,Pointer error,long capacity);
    Pointer coolbox_kmeans_labels(Pointer model,LongByReference count,Pointer error,long capacity);
    double coolbox_kmeans_inertia(Pointer model,Pointer error,long capacity);
    Pointer coolbox_kmeans_inertia_history(Pointer model,LongByReference count,Pointer error,long capacity);
    int coolbox_kmeans_n_iter(Pointer model,IntByReference out,Pointer error,long capacity);
    int coolbox_kmeans_is_fitted(Pointer model);
}
