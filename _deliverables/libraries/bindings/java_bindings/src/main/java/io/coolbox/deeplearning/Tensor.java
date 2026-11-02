package io.coolbox.deeplearning;

import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
import java.util.Arrays;

/** Multidimensional tensor (Python: {@code deep_learning.Tensor}). */
public final class Tensor extends NativeHandle {
    private static final DLNative N=DLNative.INSTANCE;
    public Tensor(long... shape){this(shape,null);}
    public Tensor(long[] shape,double[] data){super(create(shape,data),N::coolbox_dl_tensor_free);}
    private static com.sun.jna.Pointer create(long[] s,double[] d){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dl_tensor_new(s,s.length,d,d==null?0:d.length,e.pointer(),e.capacity()));}
    Tensor(com.sun.jna.Pointer p){super(p,N::coolbox_dl_tensor_free);}
    public static Tensor adopt(com.sun.jna.Pointer p){return new Tensor(p);}
    public long[] shape(){long[] s=new long[Math.toIntExact(N.coolbox_dl_tensor_dimensions(handle()))];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_tensor_shape(handle(),s,s.length,e.pointer(),e.capacity()));return s;}
    public long size(){return N.coolbox_dl_tensor_size(handle());}
    public double[] data(){double[] d=new double[Math.toIntExact(size())];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_tensor_data(handle(),d,d.length,e.pointer(),e.capacity()));return d;}
    public Tensor cloneTensor(){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_tensor_clone(handle(),e.pointer(),e.capacity())));}
    public Tensor reshape(long... shape){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_tensor_reshape(handle(),shape,shape.length,e.pointer(),e.capacity())));}
    public Tensor transpose(){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_tensor_transpose(handle(),e.pointer(),e.capacity())));}
    public Tensor add(Tensor other){return binary(other,0);} public Tensor subtract(Tensor other){return binary(other,1);} public Tensor multiply(Tensor other){return binary(other,2);} public Tensor divide(Tensor other){return binary(other,3);}
    private Tensor binary(Tensor t,int op){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_tensor_binary(handle(),t.handle(),op,e.pointer(),e.capacity())));}
    public Tensor multiply(double x){return scalar(x,false);} public Tensor divide(double x){return scalar(x,true);}
    private Tensor scalar(double x,boolean div){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_tensor_scalar(handle(),x,div?1:0,e.pointer(),e.capacity())));}
    public Tensor matmul(Tensor t){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_tensor_matmul(handle(),t.handle(),e.pointer(),e.capacity())));}
    public Tensor fill(double x){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_tensor_fill(handle(),x,e.pointer(),e.capacity()));return this;}
    public Tensor randomize(double min,double max){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_tensor_randomize(handle(),min,max,e.pointer(),e.capacity()));return this;}
    public double get(long index){var v=new com.sun.jna.ptr.DoubleByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_tensor_at(handle(),index,v,e.pointer(),e.capacity()));return v.getValue();}
    public void set(long index,double value){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_tensor_set(handle(),index,value,e.pointer(),e.capacity()));}
    @Override public String toString(){return "Tensor(shape="+Arrays.toString(shape())+", size="+size()+")";}
}


