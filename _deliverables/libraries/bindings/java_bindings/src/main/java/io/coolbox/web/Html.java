package io.coolbox.web;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.json.Json;
import java.util.Map;
/** HTML file and template helpers backed by {@code coolbox_http_html_file}. */
public final class Html {
 private Html(){}
 public static Response file(String root,String filename){return load(root,filename,Map.of(),false);}
 public static Response render(String root,String filename,Map<String,String> context){return load(root,filename,context,true);}
 private static Response load(String root,String filename,Map<String,String> context,boolean render){ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(WebNative.INSTANCE.coolbox_http_html_file(root,filename,Json.jsonEncode(context),render?1:0,e.pointer(),e.capacity()));String json;try{json=p.getString(0,"UTF-8");}finally{WebNative.INSTANCE.coolbox_c_free_string(p);}Map<String,Object> envelope=WebJson.object(json);Response response=Response.html(String.valueOf(envelope.getOrDefault("body","")),((Number)envelope.getOrDefault("status_code",200)).intValue());Object headers=envelope.get("headers");if(headers instanceof Map<?,?> h)h.forEach((k,v)->response.setHeader(String.valueOf(k),String.valueOf(v)));return response;}
}
