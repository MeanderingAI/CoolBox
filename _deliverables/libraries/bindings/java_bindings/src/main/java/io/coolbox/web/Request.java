package io.coolbox.web;
import java.util.Map;
/** Immutable request data equivalent to Python {@code rest_api.Request}. */
public final class Request {
 private final HttpMethod method;private final String path,body;private final Map<String,String> headers,queryParams,pathParams;
 public Request(HttpMethod method,String path,Map<String,String> headers,String body){this(method,path,headers,body,Map.of(),Map.of());}
 Request(HttpMethod method,String path,Map<String,String> headers,String body,Map<String,String> query,Map<String,String> pathParams){this.method=method;this.path=path;this.headers=Map.copyOf(headers);this.body=body;this.queryParams=Map.copyOf(query);this.pathParams=Map.copyOf(pathParams);}
 public HttpMethod method(){return method;}public String path(){return path;}public String body(){return body;}public Map<String,String> headers(){return headers;}public Map<String,String> queryParams(){return queryParams;}public Map<String,String> pathParams(){return pathParams;}
 public String getHeader(String key){return getHeader(key,"");}public String getHeader(String key,String fallback){return lookup(headers,key,fallback);}public String getQueryParam(String key){return getQueryParam(key,"");}public String getQueryParam(String key,String fallback){return queryParams.getOrDefault(key,fallback);}public String getPathParam(String key){return getPathParam(key,"");}public String getPathParam(String key,String fallback){return pathParams.getOrDefault(key,fallback);}
 private static String lookup(Map<String,String> values,String key,String fallback){return values.entrySet().stream().filter(e->e.getKey().equalsIgnoreCase(key)).map(Map.Entry::getValue).findFirst().orElse(fallback);}
}
