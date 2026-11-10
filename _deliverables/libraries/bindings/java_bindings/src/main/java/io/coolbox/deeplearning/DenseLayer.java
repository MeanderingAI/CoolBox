package io.coolbox.deeplearning;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer;
/** Fully connected layer (Python: {@code deep_learning.DenseLayer}). */
public final class DenseLayer extends Layer {
    public DenseLayer(int inputSize,int outputSize){super(create(0,new long[]{inputSize,outputSize},0));}
    public double[][] weights(){return values(false);}
    public double[] bias(){double[][] matrix=values(true);double[] out=new double[matrix.length];for(int i=0;i<matrix.length;i++)out[i]=matrix[i][0];return out;}
    private double[][] values(boolean bias){var r=new LongByReference();var c=new LongByReference();ErrorBuffer e=new ErrorBuffer();e.check(DLNative.INSTANCE.coolbox_dl_dense_shape(handle(),bias?1:0,r,c,e.pointer(),e.capacity()));int rows=Math.toIntExact(r.getValue()),cols=Math.toIntExact(c.getValue());double[] v=new double[Math.multiplyExact(rows,cols)];e=new ErrorBuffer();e.check(DLNative.INSTANCE.coolbox_dl_dense_values(handle(),bias?1:0,v,v.length,e.pointer(),e.capacity()));return io.coolbox.internal.Matrices.unflatten(v,rows,cols);}
}
