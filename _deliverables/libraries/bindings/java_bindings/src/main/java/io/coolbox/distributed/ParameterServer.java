package io.coolbox.distributed;
import com.sun.jna.Pointer;import com.sun.jna.ptr.LongByReference;import io.coolbox.internal.ErrorBuffer;import io.coolbox.internal.Matrices;import io.coolbox.internal.NativeHandle;
/** Central parameter store (Python: {@code distributed.ParameterServer}). */
public final class ParameterServer extends NativeHandle {
 private static final DistributedNative N=DistributedNative.INSTANCE;
 public ParameterServer(int worldSize){super(create(worldSize),N::coolbox_dist_parameter_server_free);}
 private static Pointer create(int w){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dist_parameter_server_new(w,e.pointer(),e.capacity()));}
 public void setParameters(String key,double[] values){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_server_set(handle(),key,values,values.length,e.pointer(),e.capacity()));}
 public double[] getParameters(String key){var n=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=N.coolbox_dist_server_get(handle(),key,n,e.pointer(),e.capacity());return Matrices.takeDoubles(NativeChecks.pointer(p,e),n.getValue());}
 public void updateParameters(String key,double[] gradients,double learningRate){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_server_update(handle(),key,gradients,gradients.length,learningRate,e.pointer(),e.capacity()));}
 public void accumulateGradient(String key,double[] gradient,int workerRank){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_server_accumulate(handle(),key,gradient,gradient.length,workerRank,e.pointer(),e.capacity()));}
 public void applyGradients(String key,double learningRate){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_server_apply(handle(),key,learningRate,e.pointer(),e.capacity()));}
 public void clearGradients(String key){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_server_clear(handle(),key,e.pointer(),e.capacity()));}
}
