package io.coolbox.distributed;
import com.sun.jna.Pointer;import com.sun.jna.ptr.DoubleByReference;import io.coolbox.internal.ErrorBuffer;
/** Distributed neural network training (Python: {@code distributed.DistributedNeuralNetTrainer}). */
public final class DistributedNeuralNetTrainer extends DistributedTrainer {
 public DistributedNeuralNetTrainer(DistributedContext context,TrainingStrategy strategy,int inputDim,int[] hiddenDims,int outputDim,double learningRate){super(create(context,strategy,inputDim,hiddenDims,outputDim,learningRate));}
 private static Pointer create(DistributedContext c,TrainingStrategy s,int in,int[] h,int out,double lr){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dist_trainer_neural_new(c.handle(),s.ordinal(),in,h,h.length,out,lr,e.pointer(),e.capacity()));}
 public double localLoss(){var v=new DoubleByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_trainer_local_loss(handle(),v,e.pointer(),e.capacity()));return v.getValue();}
 public double globalLoss(){var v=new DoubleByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_trainer_global_loss(handle(),v,e.pointer(),e.capacity()));return v.getValue();}
}
