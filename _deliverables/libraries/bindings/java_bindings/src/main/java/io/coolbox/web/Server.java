package io.coolbox.web;
import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
import io.coolbox.json.Json;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/** Threaded HTTP server equivalent to Python {@code rest_api.Server}. */
public final class Server extends NativeHandle {
 private final int port;
 private final List<WebNative.RouteHandlerV2> callbacks=new ArrayList<>();
 private final Map<String,Handler> localHandlers=new ConcurrentHashMap<>();
 private volatile Thread serverThread;
 private volatile String corsOrigin;
 private final AtomicReference<RuntimeException> startupFailure=new AtomicReference<>();
 public Server(){this(8080);} public Server(int port){this(create(port),port);}
 private Server(Pointer p,int port){super(p,WebNative.INSTANCE::coolbox_http_server_destroy);this.port=port;}
 private static Pointer create(int port){ErrorBuffer e=new ErrorBuffer();return e.check(WebNative.INSTANCE.coolbox_http_server_create(port,e.pointer(),e.capacity()));}
 public int port(){return port;}
 public void get(String path,Handler h){addRoute(HttpMethod.GET,path,h);}public void post(String path,Handler h){addRoute(HttpMethod.POST,path,h);}public void put(String path,Handler h){addRoute(HttpMethod.PUT,path,h);}public void delete(String path,Handler h){addRoute(HttpMethod.DELETE,path,h);}public void patch(String path,Handler h){addRoute(HttpMethod.PATCH,path,h);}public void options(String path,Handler h){addRoute(HttpMethod.OPTIONS,path,h);}
 public synchronized void addRoute(HttpMethod method,String path,Handler handler){register(method.name(),path,handler,false);}
 public synchronized void registerHandler(String name,Handler handler){register(name,null,handler,true);}
 private void register(String key,String path,Handler handler,boolean named){if(handler==null)throw new IllegalArgumentException("handler is required");WebNative.RouteHandlerV2 callback=callback(handler);ErrorBuffer e=new ErrorBuffer();int ok=named?WebNative.INSTANCE.coolbox_http_server_register_handler_v2(handle(),key,callback,null,e.pointer(),e.capacity()):WebNative.INSTANCE.coolbox_http_server_add_route_v2(handle(),key,path,callback,null,e.pointer(),e.capacity());e.check(ok);callbacks.add(callback);if(!named)localHandlers.put(key+" "+path,handler);}
 private WebNative.RouteHandlerV2 callback(Handler handler){return (context,method,path,body,headersJson,pathKeys,pathValues,pathCount,queryKeys,queryValues,queryCount,status,headersOut)->{
  try{
   Map<String,String> headers=stringMap(headersJson);Map<String,String> paths=arrays(pathKeys,pathValues,pathCount),query=arrays(queryKeys,queryValues,queryCount);
   Response response=handler.handle(new Request(HttpMethod.valueOf(method),path,headers,body,query,paths));
   status.setValue(response.statusCode());Map<String,String> responseHeaders=new LinkedHashMap<>(response.headers());String origin=corsOrigin;if(origin!=null){responseHeaders.put("Access-Control-Allow-Origin",origin);responseHeaders.put("Access-Control-Allow-Methods","GET, POST, PUT, DELETE, PATCH, OPTIONS");responseHeaders.put("Access-Control-Allow-Headers","Content-Type, Authorization");}headersOut.setValue(WebNative.INSTANCE.coolbox_c_duplicate_string(Json.jsonEncode(responseHeaders)));
   return WebNative.INSTANCE.coolbox_c_duplicate_string(response.body()==null?"":response.body());
  }catch(Throwable error){status.setValue(500);headersOut.setValue(WebNative.INSTANCE.coolbox_c_duplicate_string("{}"));return WebNative.INSTANCE.coolbox_c_duplicate_string("Internal Server Error");}
 };}
 private static Map<String,String> stringMap(String json){if(json==null||json.isBlank())return Map.of();return Json.jsonDecode(json);}
 private static Map<String,String> arrays(Pointer keys,Pointer values,long count){Map<String,String> out=new LinkedHashMap<>();for(long i=0;i<count;i++)if(keys!=null&&values!=null)out.put(keys.getPointer(i*com.sun.jna.Native.POINTER_SIZE).getString(0,"UTF-8"),values.getPointer(i*com.sun.jna.Native.POINTER_SIZE).getString(0,"UTF-8"));return out;}
 public void enableCors(){enableCors("*");}
 public void enableCors(String origin){corsOrigin=java.util.Objects.requireNonNull(origin);}
 public void loadRoutes(String filename){ErrorBuffer e=new ErrorBuffer();e.check(WebNative.INSTANCE.coolbox_http_server_load_routes(handle(),filename,e.pointer(),e.capacity()));}
 public synchronized void start(){if(serverThread!=null&&serverThread.isAlive())throw new IllegalStateException("server is already running");startupFailure.set(null);serverThread=new Thread(()->{ErrorBuffer e=new ErrorBuffer();try{e.check(WebNative.INSTANCE.coolbox_http_server_start(handle(),e.pointer(),e.capacity()));}catch(RuntimeException ex){startupFailure.set(ex);}},"coolbox-http-"+port);serverThread.setDaemon(true);serverThread.start();long deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(5);while(System.nanoTime()<deadline){RuntimeException failure=startupFailure.get();if(failure!=null)throw failure;if(isRunning())return;if(!serverThread.isAlive()){failure=startupFailure.get();if(failure!=null)throw failure;throw new IllegalStateException("HTTP server stopped before starting");}try{Thread.sleep(5);}catch(InterruptedException ex){Thread.currentThread().interrupt();throw new IllegalStateException("Interrupted while starting server",ex);}}throw new IllegalStateException("Timed out waiting for HTTP server to start");}
 public void stop(){WebNative.INSTANCE.coolbox_http_server_stop(handle());}
 public boolean isRunning(){return WebNative.INSTANCE.coolbox_http_server_is_running(handle())!=0;}
 public Response handleRequest(Request request){Handler h=localHandlers.get(request.method().name()+" "+request.path());return h==null?new Response(404,"Not Found"):h.handle(request);}
 @Override public synchronized void close(){if(!isClosed()){stop();Thread t=serverThread;if(t!=null&&t!=Thread.currentThread()){try{t.join(5000);}catch(InterruptedException e){Thread.currentThread().interrupt();}if(t.isAlive())throw new IllegalStateException("HTTP server did not stop");}super.close();}}
}
