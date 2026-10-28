package io.coolbox.web;
import java.util.LinkedHashMap;
import java.util.Map;
/** HTTP response equivalent to Python {@code rest_api.Response}. */
public final class Response {
 private int statusCode;private String body;private final Map<String,String> headers=new LinkedHashMap<>();
 public Response(){this(200,"");}public Response(int statusCode,String body){this.statusCode=statusCode;this.body=body;}
 public int statusCode(){return statusCode;}public String body(){return body;}public Map<String,String> headers(){return Map.copyOf(headers);}
 public Response setStatus(int code){statusCode=code;return this;}public Response setBody(String value){body=value;return this;}public Response setHeader(String key,String value){headers.put(key,value);return this;}public Response setJson(String json){body=json;headers.put("Content-Type","application/json");return this;}
 public String toString(){return "HTTP/1.1 "+statusCode+"\r\n"+headers.entrySet().stream().map(e->e.getKey()+": "+e.getValue()+"\r\n").reduce("",String::concat)+"\r\n"+body;}
 public static Response html(String content){return html(content,200);}public static Response html(String content,int status){return new Response(status,content).setHeader("Content-Type","text/html; charset=utf-8");}
 public static Response htmlFile(String root,String filename){return Html.file(root,filename);}public static Response render(String root,String filename,Map<String,String> context){return Html.render(root,filename,context);}
}
