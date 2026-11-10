package io.coolbox.distributed;
import com.sun.jna.Pointer;import com.sun.jna.ptr.LongByReference;import io.coolbox.internal.ErrorBuffer;import io.coolbox.internal.Matrices;import io.coolbox.internal.NativeHandle;
/** Balanced dataset partitioner (Python: {@code distributed.DataPartitioner}). */
public final class DataPartitioner extends NativeHandle {
 private static final DistributedNative N=DistributedNative.INSTANCE;
 public DataPartitioner(long totalSize,int worldSize){super(create(totalSize,worldSize),N::coolbox_dist_partitioner_free);}
 private static Pointer create(long n,int w){if(w<=0)throw new IllegalArgumentException("worldSize must be positive");ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dist_partitioner_new(n,w,e.pointer(),e.capacity()));}
 public Partition getPartition(int rank){var s=new LongByReference();var end=new LongByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_partitioner_partition(handle(),rank,s,end,e.pointer(),e.capacity()));return new Partition(s.getValue(),end.getValue());}
 public long[] getIndices(int rank){var n=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=N.coolbox_dist_partitioner_indices(handle(),rank,n,e.pointer(),e.capacity());return Matrices.takeLongs(NativeChecks.pointer(p,e),n.getValue());}
 public long partitionSize(int rank){return N.coolbox_dist_partitioner_size(handle(),rank);}
 public record Partition(long start,long end){}
}
