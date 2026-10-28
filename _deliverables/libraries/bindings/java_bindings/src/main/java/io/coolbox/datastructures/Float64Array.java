package io.coolbox.datastructures;
import com.sun.jna.ptr.DoubleByReference; import io.coolbox.CoolBoxException; import io.coolbox.internal.NativeHandle; import java.util.Arrays;
/** Multidimensional float64 array (Python: {@code data_structures.Float64Array}). */
public final class Float64Array extends NativeHandle {
 private final long[] shape;
 public Float64Array(long... shape){super(create(validate(shape)),DataStructuresNative.INSTANCE::coolbox_ds_array_free);this.shape=shape.clone();}
 private static long[] validate(long[] s){if(s.length==0)throw new IllegalArgumentException("shape must not be empty");for(long x:s)if(x<=0)throw new IllegalArgumentException("shape dimensions must be positive");return s;} private static com.sun.jna.Pointer create(long[] s){return DataStructuresNative.INSTANCE.coolbox_ds_array_new(s.length,s);}
 public int rank(){return Math.toIntExact(DataStructuresNative.INSTANCE.coolbox_ds_array_rank(handle()));} public long size(){return DataStructuresNative.INSTANCE.coolbox_ds_array_size(handle());} public long[] shape(){return shape.clone();}
 public double get(long... indices){checkRank(indices);DoubleByReference v=new DoubleByReference();DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_array_get(handle(),indices,indices.length,v));return v.getValue();} public void set(double value,long... indices){checkRank(indices);DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_array_set(handle(),indices,indices.length,value));} private void checkRank(long[] i){if(i.length!=shape.length)throw new IllegalArgumentException("index rank does not match array rank");}
}
