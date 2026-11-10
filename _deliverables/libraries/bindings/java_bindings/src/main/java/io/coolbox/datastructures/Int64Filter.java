package io.coolbox.datastructures;
import io.coolbox.internal.NativeHandle;
abstract class Int64Filter extends NativeHandle {
 Int64Filter(int kind,long expected,double rate){super(DataStructuresNative.INSTANCE.coolbox_ds_filter_new(kind,expected,rate),DataStructuresNative.INSTANCE::coolbox_ds_filter_free);}
 public void insert(long value){DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_filter_insert(handle(),value));} public boolean mightContain(long value){return DataStructuresNative.INSTANCE.coolbox_ds_filter_contains(handle(),value)!=0;} public long insertCount(){return DataStructuresNative.INSTANCE.coolbox_ds_filter_insert_count(handle());} public long approximateCount(){return DataStructuresNative.INSTANCE.coolbox_ds_filter_approximate_count(handle());} public double estimatedRate(){return DataStructuresNative.INSTANCE.coolbox_ds_filter_estimated_rate(handle());} public void clear(){DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_filter_clear(handle()));}
}
