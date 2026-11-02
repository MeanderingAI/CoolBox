package io.coolbox.web;
import java.util.List;
import java.util.Map;
/** Form validation result equivalent to Python {@code rest_api.ValidationResult}. */
public record ValidationResult(boolean valid,Map<String,List<String>> errors,Map<String,Object> values,String rawJson) {
 static ValidationResult fromJson(String json){Map<String,Object> root=WebJson.object(json);boolean valid=Boolean.TRUE.equals(root.get("valid"));Map<String,List<String>> errors=new java.util.LinkedHashMap<>();Object e=root.get("errors");if(e instanceof Map<?,?> map)for(var entry:map.entrySet()){List<String> vals=new java.util.ArrayList<>();if(entry.getValue() instanceof List<?> list)for(Object item:list)vals.add(String.valueOf(item));errors.put(String.valueOf(entry.getKey()),List.copyOf(vals));}Map<String,Object> values=root.get("values") instanceof Map<?,?> map?stringKeyMap(map):Map.of();return new ValidationResult(valid,Map.copyOf(errors),values,json);}
 private static Map<String,Object> stringKeyMap(Map<?,?> input){Map<String,Object> out=new java.util.LinkedHashMap<>();input.forEach((k,v)->out.put(String.valueOf(k),v));return java.util.Collections.unmodifiableMap(out);}
 public String toJson(){return rawJson;}
}
