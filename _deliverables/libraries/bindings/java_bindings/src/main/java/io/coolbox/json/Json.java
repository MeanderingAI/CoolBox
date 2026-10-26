package io.coolbox.json;

import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/** JSON string-map and string-array helpers equivalent to Python REST JSON utilities. */
public final class Json {
 private Json() {}
 public static String jsonEncode(Map<String,String> values){
  String[] keys=values.keySet().toArray(String[]::new), data=values.values().toArray(String[]::new);
  ErrorBuffer e=new ErrorBuffer(); return Matrices.takeString(e.check(JsonNative.INSTANCE.coolbox_json_encode(keys,data,keys.length,e.pointer(),e.capacity())));
 }
 public static Map<String,String> jsonDecode(String json){
  ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(JsonNative.INSTANCE.coolbox_json_decode(json,e.pointer(),e.capacity()));
  try{Map<String,String> out=new LinkedHashMap<>();for(long i=0;i<JsonNative.INSTANCE.coolbox_json_map_size(p);i++)out.put(Matrices.takeString(JsonNative.INSTANCE.coolbox_json_map_key(p,i)),Matrices.takeString(JsonNative.INSTANCE.coolbox_json_map_value(p,i)));return Map.copyOf(out);}finally{JsonNative.INSTANCE.coolbox_json_map_free(p);}
 }
 public static String jsonEncodeArray(List<String> values){String[] array=values.toArray(String[]::new);ErrorBuffer e=new ErrorBuffer();return Matrices.takeString(e.check(JsonNative.INSTANCE.coolbox_json_encode_array(array,array.length,e.pointer(),e.capacity())));}
 public static List<String> jsonDecodeArray(String json){ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(JsonNative.INSTANCE.coolbox_json_decode_array(json,e.pointer(),e.capacity()));try{List<String> out=new ArrayList<>();for(long i=0;i<JsonNative.INSTANCE.coolbox_json_array_size(p);i++)out.add(Matrices.takeString(JsonNative.INSTANCE.coolbox_json_array_value(p,i)));return List.copyOf(out);}finally{JsonNative.INSTANCE.coolbox_json_array_free(p);}}
}
