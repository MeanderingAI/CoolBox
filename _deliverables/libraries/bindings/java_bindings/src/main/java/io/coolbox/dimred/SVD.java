package io.coolbox.dimred;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

/** Singular value decomposition (Python: {@code ml_core.dimensionality_reduction.SVD}). */
public final class SVD extends NativeHandle {
    private static final DimredNative N=DimredNative.INSTANCE;
    public SVD(){this(false);} public SVD(boolean computeFullMatrices){super(create(computeFullMatrices),N::coolbox_dimred_svd_free);}
    private static Pointer create(boolean full){var e=new ErrorBuffer();return e.check(N.coolbox_dimred_svd_new(full?1:0,e.pointer(),e.capacity()));}
    public SVD compute(double[][] x){int c=Matrices.columns(x);var e=new ErrorBuffer();e.check(N.coolbox_dimred_svd_compute(handle(),Matrices.flatten(x),x.length,c,e.pointer(),e.capacity()));return this;}
    public double[][] getU(){var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();return DimredSupport.takeMatrix(N.coolbox_dimred_svd_u(handle(),r,c,e.pointer(),e.capacity()),r,c,e);}
    public double[] getSingularValues(){var n=new LongByReference();var e=new ErrorBuffer();return DimredSupport.takeVector(N.coolbox_dimred_svd_values(handle(),n,e.pointer(),e.capacity()),n,e);}
    public double[][] getV(){var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();return DimredSupport.takeMatrix(N.coolbox_dimred_svd_v(handle(),r,c,e.pointer(),e.capacity()),r,c,e);}
    public double[][] getS(){var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();return DimredSupport.takeMatrix(N.coolbox_dimred_svd_s(handle(),r,c,e.pointer(),e.capacity()),r,c,e);}
    public double[][] reconstruct(){return reconstruct(0);} public double[][] reconstruct(int components){var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();return DimredSupport.takeMatrix(N.coolbox_dimred_svd_reconstruct(handle(),components,r,c,e.pointer(),e.capacity()),r,c,e);}
    public int rank(){return rank(-1);} public int rank(double tolerance){var e=new ErrorBuffer();int n=N.coolbox_dimred_svd_rank(handle(),tolerance,e.pointer(),e.capacity());e.throwIfSet();return n;}
    public double[] explainedVarianceRatio(){var n=new LongByReference();var e=new ErrorBuffer();return DimredSupport.takeVector(N.coolbox_dimred_svd_variance(handle(),n,e.pointer(),e.capacity()),n,e);}
    public boolean isComputed(){return N.coolbox_dimred_svd_is_computed(handle())!=0;}
}
