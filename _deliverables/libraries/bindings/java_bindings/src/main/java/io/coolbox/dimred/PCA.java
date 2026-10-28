package io.coolbox.dimred;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

/** Principal component analysis (Python: {@code ml_core.dimensionality_reduction.PCA}). */
public final class PCA extends NativeHandle {
    private static final DimredNative N=DimredNative.INSTANCE;
    public PCA(){this(0,true,false);} public PCA(int components,boolean center,boolean scale){super(create(components,center,scale),N::coolbox_dimred_pca_free);}
    private static Pointer create(int n,boolean center,boolean scale){var e=new ErrorBuffer();return e.check(N.coolbox_dimred_pca_new(n,center?1:0,scale?1:0,e.pointer(),e.capacity()));}
    public PCA fit(double[][] x){int c=Matrices.columns(x);var e=new ErrorBuffer();e.check(N.coolbox_dimred_pca_fit(handle(),Matrices.flatten(x),x.length,c,e.pointer(),e.capacity()));return this;}
    public double[][] transform(double[][] x){return matrix(x,false,false);} public double[][] fitTransform(double[][] x){return matrix(x,true,false);} public double[][] inverseTransform(double[][] x){return matrix(x,false,true);}
    private double[][] matrix(double[][] x,boolean fit,boolean inverse){int c=Matrices.columns(x);var r=new LongByReference();var co=new LongByReference();var e=new ErrorBuffer();Pointer p=fit?N.coolbox_dimred_pca_fit_transform(handle(),Matrices.flatten(x),x.length,c,r,co,e.pointer(),e.capacity()):inverse?N.coolbox_dimred_pca_inverse(handle(),Matrices.flatten(x),x.length,c,r,co,e.pointer(),e.capacity()):N.coolbox_dimred_pca_transform(handle(),Matrices.flatten(x),x.length,c,r,co,e.pointer(),e.capacity());return DimredSupport.takeMatrix(p,r,co,e);}
    public double[][] components(){var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();return DimredSupport.takeMatrix(N.coolbox_dimred_pca_components(handle(),r,c,e.pointer(),e.capacity()),r,c,e);}
    public double[] explainedVariance(){var n=new LongByReference();var e=new ErrorBuffer();return vector(N.coolbox_dimred_pca_explained(handle(),n,e.pointer(),e.capacity()),n,e);}
    public double[] explainedVarianceRatio(){var n=new LongByReference();var e=new ErrorBuffer();return vector(N.coolbox_dimred_pca_ratio(handle(),n,e.pointer(),e.capacity()),n,e);}
    public double[] singularValues(){var n=new LongByReference();var e=new ErrorBuffer();return vector(N.coolbox_dimred_pca_singular(handle(),n,e.pointer(),e.capacity()),n,e);}
    public double[] mean(){var n=new LongByReference();var e=new ErrorBuffer();return vector(N.coolbox_dimred_pca_mean(handle(),n,e.pointer(),e.capacity()),n,e);}
    public double[] scale(){var n=new LongByReference();var e=new ErrorBuffer();return vector(N.coolbox_dimred_pca_scale(handle(),n,e.pointer(),e.capacity()),n,e);}
    private double[] vector(Pointer p,LongByReference n,ErrorBuffer e){e.throwIfSet();if(p==null)throw e.failure();return Matrices.takeDoubles(p,n.getValue());}
    public int numberOfComponents(){return N.coolbox_dimred_pca_n_components(handle());} public boolean isFitted(){return N.coolbox_dimred_pca_is_fitted(handle())!=0;}
}
