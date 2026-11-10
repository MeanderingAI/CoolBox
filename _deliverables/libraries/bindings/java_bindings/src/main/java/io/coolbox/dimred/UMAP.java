package io.coolbox.dimred;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

/** UMAP manifold embedding (Python: {@code ml_core.dimensionality_reduction.UMAP}). */
public final class UMAP extends NativeHandle {
    private static final DimredNative N=DimredNative.INSTANCE;
    public UMAP(){this(2,15,0.1,"euclidean",1.0,200,42);}
    public UMAP(int components,int neighbors,double minDistance,String metric,double learningRate,int epochs,int randomState){super(create(components,neighbors,minDistance,metric,learningRate,epochs,randomState),N::coolbox_dimred_umap_free);}
    private static Pointer create(int nc,int nn,double md,String metric,double lr,int epochs,int seed){var e=new ErrorBuffer();return e.check(N.coolbox_dimred_umap_new(nc,nn,md,metric,lr,epochs,seed,e.pointer(),e.capacity()));}
    public UMAP fit(double[][] x){int c=Matrices.columns(x);var e=new ErrorBuffer();e.check(N.coolbox_dimred_umap_fit(handle(),Matrices.flatten(x),x.length,c,e.pointer(),e.capacity()));return this;}
    public double[][] transform(double[][] x){return matrix(x,false);} public double[][] fitTransform(double[][] x){return matrix(x,true);}
    private double[][] matrix(double[][] x,boolean fit){int c=Matrices.columns(x);var r=new LongByReference();var co=new LongByReference();var e=new ErrorBuffer();Pointer p=fit?N.coolbox_dimred_umap_fit_transform(handle(),Matrices.flatten(x),x.length,c,r,co,e.pointer(),e.capacity()):N.coolbox_dimred_umap_transform(handle(),Matrices.flatten(x),x.length,c,r,co,e.pointer(),e.capacity());return DimredSupport.takeMatrix(p,r,co,e);}
    public double[][] embedding(){var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();return DimredSupport.takeMatrix(N.coolbox_dimred_umap_embedding(handle(),r,c,e.pointer(),e.capacity()),r,c,e);}
    public boolean isFitted(){return N.coolbox_dimred_umap_is_fitted(handle())!=0;} public int numberOfComponents(){return N.coolbox_dimred_umap_n_components(handle());} public int numberOfNeighbors(){return N.coolbox_dimred_umap_n_neighbors(handle());}
}
