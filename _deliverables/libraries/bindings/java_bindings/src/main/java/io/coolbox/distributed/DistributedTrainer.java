package io.coolbox.distributed;
import com.sun.jna.Pointer;import com.sun.jna.ptr.LongByReference;import io.coolbox.internal.ErrorBuffer;import io.coolbox.internal.Matrices;import io.coolbox.internal.NativeHandle;import java.util.List;
/** Abstract distributed trainer interface (Python: {@code distributed.DistributedTrainer}). */
public abstract class DistributedTrainer extends NativeHandle {
 static final DistributedNative N=DistributedNative.INSTANCE;
 protected DistributedTrainer(Pointer p){super(p,N::coolbox_dist_trainer_free);}
 public void trainEpoch(double[][] data,double[][] labels){if(data.length!=labels.length)throw new IllegalArgumentException("data and labels must have equal row counts");int xc=Matrices.columns(data),yc=Matrices.columns(labels);ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_trainer_epoch(handle(),Matrices.flatten(data),data.length,xc,Matrices.flatten(labels),labels.length,yc,e.pointer(),e.capacity()));}
 public double[] parameters(){var n=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=N.coolbox_dist_trainer_parameters(handle(),n,e.pointer(),e.capacity());return Matrices.takeDoubles(NativeChecks.pointer(p,e),n.getValue());}
 public void setParameters(double[] values){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_trainer_set_parameters(handle(),values,values.length,e.pointer(),e.capacity()));}
 public double[] predict(double[] sample){var n=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=N.coolbox_dist_trainer_predict(handle(),sample,sample.length,n,e.pointer(),e.capacity());return Matrices.takeDoubles(NativeChecks.pointer(p,e),n.getValue());}
 public void synchronizeModel(){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_trainer_sync(handle(),e.pointer(),e.capacity()));}
 public double[] aggregateGradients(double[] gradient){return aggregateGradients(gradient,AggregationMethod.SYNCHRONOUS);}public double[] aggregateGradients(double[] gradient,AggregationMethod method){var n=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=N.coolbox_dist_trainer_aggregate(handle(),gradient,gradient.length,method.ordinal(),n,e.pointer(),e.capacity());return Matrices.takeDoubles(NativeChecks.pointer(p,e),n.getValue());}
}
