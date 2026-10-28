package io.coolbox.distributed;
import com.sun.jna.Pointer;import com.sun.jna.ptr.LongByReference;import io.coolbox.internal.ErrorBuffer;import io.coolbox.internal.Matrices;
/** Distributed K-Means training (Python: {@code distributed.DistributedKMeansTrainer}). */
public final class DistributedKMeansTrainer extends DistributedTrainer {
 public DistributedKMeansTrainer(DistributedContext context,int clusters,int maxIterations){super(create(context,clusters,maxIterations));}
 private static Pointer create(DistributedContext c,int k,int it){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dist_trainer_kmeans_new(c.handle(),TrainingStrategy.DATA_PARALLEL.ordinal(),k,it,e.pointer(),e.capacity()));}
 public double[][] centroids(){var r=new LongByReference();var c=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=N.coolbox_dist_trainer_centroids(handle(),r,c,e.pointer(),e.capacity());return Matrices.takeMatrix(NativeChecks.pointer(p,e),r.getValue(),c.getValue());}
}
