package io.coolbox.json;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import io.coolbox.internal.NativeLibraries;
interface JsonNative extends Library {
 JsonNative INSTANCE=NativeLibraries.bridge(JsonNative.class);
 Pointer coolbox_json_encode(String[] keys,String[] values,long count,Pointer error,long capacity);
 Pointer coolbox_json_decode(String text,Pointer error,long capacity);
 void coolbox_json_map_free(Pointer map); long coolbox_json_map_size(Pointer map);
 Pointer coolbox_json_map_key(Pointer map,long index); Pointer coolbox_json_map_value(Pointer map,long index);
 Pointer coolbox_json_encode_array(String[] values,long count,Pointer error,long capacity);
 Pointer coolbox_json_decode_array(String text,Pointer error,long capacity);
 void coolbox_json_array_free(Pointer array); long coolbox_json_array_size(Pointer array);
 Pointer coolbox_json_array_value(Pointer array,long index);
}
