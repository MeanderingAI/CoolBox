package io.coolbox.distributed;
import com.sun.jna.Pointer;import com.sun.jna.ptr.IntByReference;import io.coolbox.internal.ErrorBuffer;import io.coolbox.internal.Matrices;import io.coolbox.internal.NativeHandle;import java.util.LinkedHashMap;import java.util.Map;
/** Distributed payload (Python: {@code distributed.Message}). */
public final class Message extends NativeHandle {
 private static final DistributedNative N=DistributedNative.INSTANCE;
 Message(Pointer p){super(p,N::coolbox_dist_message_free);}
 public Message(){this(MessageType.DATA,-1,-1);}public Message(MessageType type,int sourceRank,int destinationRank){super(create(type,sourceRank,destinationRank),N::coolbox_dist_message_free);}
 private static Pointer create(MessageType t,int s,int d){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dist_message_new(t.ordinal(),s,d,e.pointer(),e.capacity()));}
 public MessageType type(){return MessageType.values()[read(0)];}public int sourceRank(){return read(1);}public int destinationRank(){return read(2);}
 public Message type(MessageType value){return properties(value,sourceRank(),destinationRank());}public Message sourceRank(int value){return properties(type(),value,destinationRank());}public Message destinationRank(int value){return properties(type(),sourceRank(),value);}private Message properties(MessageType type,int src,int dst){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_message_set_properties(handle(),type.ordinal(),src,dst,e.pointer(),e.capacity()));return this;}
 private int read(int f){var t=new IntByReference();var s=new IntByReference();var d=new IntByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_message_properties(handle(),t,s,d,e.pointer(),e.capacity()));return f==0?t.getValue():f==1?s.getValue():d.getValue();}
 public double[] data(){double[] v=new double[Math.toIntExact(N.coolbox_dist_message_data_size(handle()))];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_message_get_data(handle(),v,v.length,e.pointer(),e.capacity()));return v;}public Message data(double[] values){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_message_set_data(handle(),values,values.length,e.pointer(),e.capacity()));return this;}
 public Message metadata(String key,String value){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dist_message_set_metadata(handle(),key,value,e.pointer(),e.capacity()));return this;}
 public Map<String,String> metadata(){Map<String,String> result=new LinkedHashMap<>();long n=N.coolbox_dist_message_metadata_count(handle());for(long i=0;i<n;i++){ErrorBuffer e=new ErrorBuffer();String k=Matrices.takeString(N.coolbox_dist_message_metadata_key(handle(),i,e.pointer(),e.capacity()));e.throwIfSet();e=new ErrorBuffer();String v=Matrices.takeString(N.coolbox_dist_message_metadata_value(handle(),k,e.pointer(),e.capacity()));e.throwIfSet();result.put(k,v);}return result;}
}

