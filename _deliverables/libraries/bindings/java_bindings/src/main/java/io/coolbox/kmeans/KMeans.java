package io.coolbox.kmeans;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

/** K-means clustering (Python: {@code ml_core.kmeans.KMeans}). */
public final class KMeans extends NativeHandle {
    private static final KMeansNative N=KMeansNative.INSTANCE;
    public KMeans(){this(3,300,InitMethod.KMEANSPP,42);} public KMeans(int clusters,int maxIterations,InitMethod init,int randomState){super(create(clusters,maxIterations,init,randomState),N::coolbox_kmeans_free);}
    private static Pointer create(int k,int it,InitMethod init,int seed){var e=new ErrorBuffer();return e.check(N.coolbox_kmeans_new(k,it,init.ordinal(),seed,e.pointer(),e.capacity()));}
    public KMeans fit(double[][] x){int c=Matrices.columns(x);var e=new ErrorBuffer();e.check(N.coolbox_kmeans_fit(handle(),Matrices.flatten(x),x.length,c,e.pointer(),e.capacity()));return this;}
    public int[] predict(double[][] x){return labels(x,false);} public int[] fitPredict(double[][] x){return labels(x,true);}
    private int[] labels(double[][] x,boolean fit){int c=Matrices.columns(x);var n=new LongByReference();var e=new ErrorBuffer();Pointer p=fit?N.coolbox_kmeans_fit_predict(handle(),Matrices.flatten(x),x.length,c,n,e.pointer(),e.capacity()):N.coolbox_kmeans_predict(handle(),Matrices.flatten(x),x.length,c,n,e.pointer(),e.capacity());e.throwIfSet();if(p==null)throw e.failure();return Matrices.takeInts(p,n.getValue());}
    public double[][] centroids(){var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();Pointer p=N.coolbox_kmeans_centroids(handle(),r,c,e.pointer(),e.capacity());e.throwIfSet();return Matrices.takeMatrix(p,r.getValue(),c.getValue());}
    public int[] labels(){var n=new LongByReference();var e=new ErrorBuffer();Pointer p=N.coolbox_kmeans_labels(handle(),n,e.pointer(),e.capacity());e.throwIfSet();if(p==null)throw e.failure();return Matrices.takeInts(p,n.getValue());}
    public double inertia(){var e=new ErrorBuffer();double v=N.coolbox_kmeans_inertia(handle(),e.pointer(),e.capacity());e.throwIfSet();return v;}
    public double[] inertiaHistory(){var n=new LongByReference();var e=new ErrorBuffer();Pointer p=N.coolbox_kmeans_inertia_history(handle(),n,e.pointer(),e.capacity());e.throwIfSet();if(p==null)throw e.failure();return Matrices.takeDoubles(p,n.getValue());}
    public int numberOfIterations(){var out=new com.sun.jna.ptr.IntByReference();var e=new ErrorBuffer();e.check(N.coolbox_kmeans_n_iter(handle(),out,e.pointer(),e.capacity()));return out.getValue();} public boolean isFitted(){return N.coolbox_kmeans_is_fitted(handle())!=0;}
}
