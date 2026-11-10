package io.coolbox.dimred;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.PointerByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

/** Exact nearest-neighbor search (Python: {@code ml_core.dimensionality_reduction.KNN}). */
public final class KNN extends NativeHandle {
    private static final DimredNative N=DimredNative.INSTANCE;
    public KNN(){this(5,"euclidean");} public KNN(int k,String metric){super(create(k,metric),N::coolbox_dimred_knn_free);}
    private static Pointer create(int k,String metric){var e=new ErrorBuffer();return e.check(N.coolbox_dimred_knn_new(k,metric,e.pointer(),e.capacity()));}
    public KNN fit(double[][] x){int c=Matrices.columns(x);var e=new ErrorBuffer();e.check(N.coolbox_dimred_knn_fit(handle(),Matrices.flatten(x),x.length,c,e.pointer(),e.capacity()));return this;}
    public Neighbors kneighbors(double[][] query){return neighbors(query,false);} public Neighbors kneighbors(){return neighbors(new double[0][0],true);}
    private Neighbors neighbors(double[][] query,boolean training){int rows=training?0:query.length,cols=training?0:Matrices.columns(query);var i=new PointerByReference();var d=new PointerByReference();var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();e.check(N.coolbox_dimred_knn_neighbors(handle(),training?new double[0]:Matrices.flatten(query),rows,cols,training?1:0,i,d,r,c,e.pointer(),e.capacity()));int count=Math.toIntExact(r.getValue()*c.getValue());int[] rawIndices=Matrices.takeInts(i.getValue(),count); double[][] neighborDistances=Matrices.takeMatrix(d.getValue(),r.getValue(),c.getValue()); return new Neighbors(intMatrix(rawIndices,Math.toIntExact(r.getValue()),Math.toIntExact(c.getValue())),neighborDistances);}
    private static int[][] intMatrix(int[] values,int rows,int cols){int[][] out=new int[rows][cols];for(int r=0;r<rows;r++)System.arraycopy(values,r*cols,out[r],0,cols);return out;}
    public double[][] pairwiseDistances(double[][] x,double[][] y){int xc=Matrices.columns(x),yc=Matrices.columns(y);var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();Pointer p=N.coolbox_dimred_knn_pairwise(handle(),Matrices.flatten(x),x.length,xc,Matrices.flatten(y),y.length,yc,r,c,e.pointer(),e.capacity());return DimredSupport.takeMatrix(p,r,c,e);}
    public int k(){return N.coolbox_dimred_knn_get_k(handle());} public String metric(){var e=new ErrorBuffer();Pointer p=N.coolbox_dimred_knn_get_metric(handle(),e.pointer(),e.capacity());e.throwIfSet();return Matrices.takeString(p);} public boolean isFitted(){return N.coolbox_dimred_knn_is_fitted(handle())!=0;}
}
