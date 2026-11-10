package io.coolbox.web;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

final class WebJson {
 private WebJson(){}
 static Object parse(String text){Parser p=new Parser(text);Object out=p.value();p.space();if(p.i!=text.length())throw new IllegalArgumentException("Unexpected trailing JSON data");return out;}
 @SuppressWarnings("unchecked") static Map<String,Object> object(String text){Object v=parse(text);if(!(v instanceof Map))throw new IllegalArgumentException("Expected JSON object");return (Map<String,Object>)v;}
 static final class Parser {
  final String s;int i;Parser(String s){this.s=s==null?"":s;}
  void space(){while(i<s.length()&&Character.isWhitespace(s.charAt(i)))i++;}
  Object value(){space();if(i>=s.length())throw bad();char c=s.charAt(i);if(c=='"')return string();if(c=='{')return object();if(c=='[')return array();if(s.startsWith("true",i)){i+=4;return true;}if(s.startsWith("false",i)){i+=5;return false;}if(s.startsWith("null",i)){i+=4;return null;}return number();}
  Map<String,Object> object(){i++;Map<String,Object> m=new LinkedHashMap<>();space();if(take('}'))return m;do{space();if(i>=s.length()||s.charAt(i)!='"')throw bad();String k=string();space();if(!take(':'))throw bad();m.put(k,value());space();if(take('}'))return m;if(!take(','))throw bad();}while(true);}
  List<Object> array(){i++;List<Object> a=new ArrayList<>();space();if(take(']'))return a;do{a.add(value());space();if(take(']'))return a;if(!take(','))throw bad();}while(true);}
  String string(){if(!take('"'))throw bad();StringBuilder b=new StringBuilder();while(i<s.length()){char c=s.charAt(i++);if(c=='"')return b.toString();if(c!='\\'){b.append(c);continue;}if(i>=s.length())throw bad();char e=s.charAt(i++);switch(e){case '"','\\','/'->b.append(e);case 'b'->b.append('\b');case 'f'->b.append('\f');case 'n'->b.append('\n');case 'r'->b.append('\r');case 't'->b.append('\t');case 'u'->{if(i+4>s.length())throw bad();b.append((char)Integer.parseInt(s.substring(i,i+4),16));i+=4;}default->throw bad();}}throw bad();}
  Number number(){int start=i;while(i<s.length()&&"-+0123456789.eE".indexOf(s.charAt(i))>=0)i++;if(start==i)throw bad();String n=s.substring(start,i);try{return n.contains(".")||n.contains("e")||n.contains("E")?Double.parseDouble(n):Long.parseLong(n);}catch(NumberFormatException ex){throw bad();}}
  boolean take(char c){if(i<s.length()&&s.charAt(i)==c){i++;return true;}return false;}IllegalArgumentException bad(){return new IllegalArgumentException("Invalid JSON at offset "+i);}
 }
}
