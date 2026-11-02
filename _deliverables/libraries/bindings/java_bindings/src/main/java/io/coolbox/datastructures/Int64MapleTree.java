package io.coolbox.datastructures;
import com.sun.jna.ptr.IntByReference; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.NativeHandle; import java.util.OptionalLong;
/** Integer Maple tree (Python: {@code data_structures.Int64MapleTree}). */
public final class Int64MapleTree extends NativeHandle {
 public Int64MapleTree(){super(DataStructuresNative.INSTANCE.coolbox_ds_maple_new(),DataStructuresNative.INSTANCE::coolbox_ds_maple_free);} public void insert(long k,long v){DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_maple_insert(handle(),k,v));} public OptionalLong get(long k){LongByReference v=new LongByReference();IntByReference f=new IntByReference();DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_maple_get(handle(),k,v,f));return DataHandle.optional(v,f.getValue()!=0);} public boolean remove(long k){IntByReference r=new IntByReference();DataHandle.ok(DataStructuresNative.INSTANCE.coolbox_ds_maple_remove(handle(),k,r));return DataHandle.changed(r);}
}
