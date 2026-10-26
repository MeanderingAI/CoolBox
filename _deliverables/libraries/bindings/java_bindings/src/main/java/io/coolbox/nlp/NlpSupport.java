package io.coolbox.nlp;
import com.sun.jna.Pointer; import com.sun.jna.StringArray; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import java.util.ArrayList; import java.util.List;
final class NlpSupport {
 private NlpSupport(){}
 static StringArray array(List<String> values){return new StringArray(values.toArray(new String[0]));}
 static StringArray array(String[] values){return new StringArray(values);}
 static Encoded docs(List<List<String>> docs){List<String> flat=new ArrayList<>();long[] offsets=new long[docs.size()+1];for(int i=0;i<docs.size();i++){flat.addAll(docs.get(i));offsets[i+1]=flat.size();}return new Encoded(array(flat),offsets);}
 static List<String> takeStrings(Pointer p){if(p==null)return List.of();NlpNative n=NlpNative.INSTANCE;try{List<String> out=new ArrayList<>();long c=n.coolbox_nlp_string_list_size(p);for(int i=0;i<c;i++){ErrorBuffer e=new ErrorBuffer();out.add(Matrices.takeString(e.check(n.coolbox_nlp_string_list_at(p,i,e.pointer(),e.capacity()))));}return out;}finally{n.coolbox_nlp_string_list_free(p);}}
 static long[] takeLongs(Pointer p,LongByReference count){return Matrices.takeLongs(p,count.getValue());}
 static double[][] takeMatrix(Pointer p,LongByReference rows,LongByReference cols){return Matrices.takeMatrix(p,rows.getValue(),cols.getValue());}
 record Encoded(StringArray strings,long[] offsets){}
}
