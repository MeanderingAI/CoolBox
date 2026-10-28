package io.coolbox.datastructures;
import com.sun.jna.ptr.LongByReference; import io.coolbox.CoolBoxException; import io.coolbox.internal.NativeHandle;
/** Legacy integer heap (Python: {@code data_structures.LegacyInt64Heap}). */
public final class LegacyInt64Heap extends NativeHandle {
 public LegacyInt64Heap(){this(HeapKind.MAX);} public LegacyInt64Heap(HeapKind kind){super(DataStructuresNative.INSTANCE.coolbox_ds_legacy_heap_new(kind==HeapKind.MIN?1:0),DataStructuresNative.INSTANCE::coolbox_ds_legacy_heap_free);}
 public void push(long value){DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_legacy_heap_push(handle(),value));} public long pop(){long value=top();DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_legacy_heap_pop(handle()));return value;} public long top(){LongByReference v=new LongByReference();if(DataStructuresNative.INSTANCE.coolbox_ds_legacy_heap_top(handle(),v)==0)throw new CoolBoxException("heap is empty");return v.getValue();} public long size(){return DataStructuresNative.INSTANCE.coolbox_ds_legacy_heap_size(handle());} public boolean isEmpty(){return DataStructuresNative.INSTANCE.coolbox_ds_legacy_heap_empty(handle())!=0;} public void clear(){DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_legacy_heap_clear(handle()));}
}
