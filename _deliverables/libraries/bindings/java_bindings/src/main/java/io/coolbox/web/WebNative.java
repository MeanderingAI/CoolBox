package io.coolbox.web;
import com.sun.jna.Callback;
import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;
import io.coolbox.internal.NativeLibraries;
interface WebNative extends Library {
 WebNative INSTANCE=NativeLibraries.cBindings(WebNative.class);
 interface RouteHandlerV2 extends Callback { Pointer invoke(Pointer context,String method,String path,String body,String headersJson,Pointer pathKeys,Pointer pathValues,long pathCount,Pointer queryKeys,Pointer queryValues,long queryCount,IntByReference status,PointerByReference headers); }
 Pointer coolbox_c_duplicate_string(String value);void coolbox_c_free_string(Pointer value);
 Pointer coolbox_http_server_create(int port,Pointer error,long capacity);
 int coolbox_http_server_add_route_v2(Pointer server,String method,String path,RouteHandlerV2 handler,Pointer context,Pointer error,long capacity);
 int coolbox_http_server_register_handler_v2(Pointer server,String name,RouteHandlerV2 handler,Pointer context,Pointer error,long capacity);
 int coolbox_http_server_load_routes(Pointer server,String file,Pointer error,long capacity);
 int coolbox_http_server_start(Pointer server,Pointer error,long capacity);
 void coolbox_http_server_stop(Pointer server);int coolbox_http_server_is_running(Pointer server);void coolbox_http_server_destroy(Pointer server);
 Pointer coolbox_http_form_load(String file,Pointer error,long capacity);Pointer coolbox_http_form_validate(String schema,String body,String contentType,Pointer error,long capacity);
 Pointer coolbox_http_html_file(String root,String filename,String context,int render,Pointer error,long capacity);
}
