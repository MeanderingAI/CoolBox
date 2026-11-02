package io.coolbox.web;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.json.Json;
import java.util.Map;
/** Schema-backed form validator equivalent to Python {@code rest_api.FormSchema}. */
public final class FormSchema {
 private final String definition;
 public FormSchema(String definition){ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(WebNative.INSTANCE.coolbox_http_form_validate(definition,"{}","application/json",e.pointer(),e.capacity()));WebNative.INSTANCE.coolbox_c_free_string(p);this.definition=definition;}
 public static FormSchema fromFile(String filename){ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(WebNative.INSTANCE.coolbox_http_form_load(filename,e.pointer(),e.capacity()));return new FormSchema(take(p));}
 public String definition(){return definition;}
 public ValidationResult validate(String body){return validate(body,"application/json");}
 public ValidationResult validate(String body,String contentType){ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(WebNative.INSTANCE.coolbox_http_form_validate(definition,body,contentType,e.pointer(),e.capacity()));return ValidationResult.fromJson(take(p));}
 public ValidationResult validateRequest(Request request){return validate(request.body(),request.getHeader("Content-Type","application/json"));}
 private static String take(Pointer p){try{return p.getString(0,"UTF-8");}finally{WebNative.INSTANCE.coolbox_c_free_string(p);}}
}
