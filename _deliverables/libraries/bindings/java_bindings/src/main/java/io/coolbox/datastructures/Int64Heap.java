package io.coolbox.datastructures;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.CoolBoxException; import io.coolbox.internal.NativeHandle;
/** Integer binary heap (Python: {@code data_structures.Int64Heap}). */
public final class Int64Heap extends NativeHandle {
 public Int64Heap(){this(HeapKind.MAX);} public Int64Heap(HeapKind kind){super(DataStructuresNative.INSTANCE.coolbox_ds_heap_new(kind==HeapKind.MIN?1:0),DataStructuresNative.INSTANCE::coolbox_ds_heap_free);}
 public void push(long value){DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_heap_push(handle(),value));} public long pop(){long value=top();DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_heap_pop(handle()));return value;} public long top(){LongByReference v=new LongByReference();if(DataStructuresNative.INSTANCE.coolbox_ds_heap_top(handle(),v)==0)throw new CoolBoxException("heap is empty");return v.getValue();} public long size(){return DataStructuresNative.INSTANCE.coolbox_ds_heap_size(handle());} public boolean isEmpty(){return DataStructuresNative.INSTANCE.coolbox_ds_heap_empty(handle())!=0;} public void clear(){DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_heap_clear(handle()));}
}
