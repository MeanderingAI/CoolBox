package io.coolbox.datastructures;
import com.sun.jna.ptr.IntByReference; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.NativeHandle; import java.util.OptionalLong;
/** FIFO integer queue (Python: {@code data_structures.Int64FolioQueue}). */
public final class Int64FolioQueue extends NativeHandle {
 public Int64FolioQueue(){super(DataStructuresNative.INSTANCE.coolbox_ds_queue_new(),DataStructuresNative.INSTANCE::coolbox_ds_queue_free);} public void enqueue(long v){DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_queue_enqueue(handle(),v));} public void enqueueBatch(long... values){if(values.length>0)DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_queue_enqueue_batch(handle(),values,values.length));} public OptionalLong tryDequeue(){LongByReference v=new LongByReference();IntByReference f=new IntByReference();DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_queue_try_dequeue(handle(),v,f));return DataHandle.optional(v,f.getValue()!=0);} public long size(){return DataStructuresNative.INSTANCE.coolbox_ds_queue_size(handle());} public boolean isEmpty(){return DataStructuresNative.INSTANCE.coolbox_ds_queue_empty(handle())!=0;}
}
