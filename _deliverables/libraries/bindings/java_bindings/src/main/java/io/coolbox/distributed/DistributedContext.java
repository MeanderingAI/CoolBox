package io.coolbox.distributed;
import com.sun.jna.Pointer;import com.sun.jna.ptr.LongByReference;import io.coolbox.internal.ErrorBuffer;import io.coolbox.internal.Matrices;import io.coolbox.internal.NativeHandle;import java.util.Arrays;
/** Communication context (Python: {@code distributed.DistributedContext}). */
public final class DistributedContext extends NativeHandle {
 private static final DistributedNative N=DistributedNative.INSTANCE;
 public DistributedContext(int worldSize,int rank){super(create(worldSize,rank),N::coolbox_dist_context_free);}
 private static Pointer create(int w,int r){if(w<=0||r<0||r>=w)throw new IllegalArgumentException("rank must be within a positive world size");ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dist_context_new(w,r,e.pointer(),e.capacity()));}
 public int rank(){return N.coolbox_dist_context_rank(handle());}public int worldSize(){return N.coolbox_dist_context_world_size(handle());}public boolean isMaster(){return N.coolbox_dist_context_is_master(handle())!=0;}
 public void send(Message message,int destination){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_context_send(handle(),message.handle(),destination,e.pointer(),e.capacity()));}
 public Message receive(){return receive(-1);}public Message receive(int sourceRank){ErrorBuffer e=new ErrorBuffer();return new Message(NativeChecks.pointer(N.coolbox_dist_context_receive(handle(),sourceRank,e.pointer(),e.capacity()),e));}
 public double[] allReduce(double[] values,ReduceOp op){var n=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=N.coolbox_dist_context_all_reduce(handle(),values,values.length,op.ordinal(),n,e.pointer(),e.capacity());return Matrices.takeDoubles(NativeChecks.pointer(p,e),n.getValue());}
 public double[] ringAllReduce(double[] values){var n=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=N.coolbox_dist_context_ring_all_reduce(handle(),values,values.length,n,e.pointer(),e.capacity());return Matrices.takeDoubles(NativeChecks.pointer(p,e),n.getValue());}
 public void broadcast(double[] data,int root){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_context_broadcast(handle(),data,data.length,root,e.pointer(),e.capacity()));}
 public double[] scatter(double[] data,int root){double[] out=new double[data.length];var n=new LongByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_context_scatter(handle(),data,data.length,out,out.length,n,root,e.pointer(),e.capacity()));return Arrays.copyOf(out,Math.toIntExact(n.getValue()));}
 public double[] gather(double[] data,int root){double[] out=new double[Math.multiplyExact(data.length,worldSize())];var n=new LongByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_context_gather(handle(),data,data.length,out,out.length,n,root,e.pointer(),e.capacity()));return Arrays.copyOf(out,Math.toIntExact(n.getValue()));}
 public void barrier(){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_context_barrier(handle(),e.pointer(),e.capacity()));}
}
final class NativeChecks{private NativeChecks(){}static Pointer pointer(Pointer p,ErrorBuffer e){return e.check(p);}}
