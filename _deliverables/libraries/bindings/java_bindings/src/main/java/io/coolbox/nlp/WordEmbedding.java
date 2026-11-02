package io.coolbox.nlp;
import com.sun.jna.Pointer; import com.sun.jna.StringArray; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import io.coolbox.internal.NativeHandle; import java.util.AbstractMap; import java.util.ArrayList; import java.util.List; import java.util.Map;
/** Word embedding table (Python: {@code nlp.WordEmbedding}). */
public final class WordEmbedding extends NativeHandle {
 private static final NlpNative N=NlpNative.INSTANCE; public WordEmbedding(int embeddingDim){super(N.coolbox_nlp_embedding_new(embeddingDim),N::coolbox_nlp_embedding_free);}
 public void randomInit(List<String> vocabulary){init(vocabulary,0);} public void xavierInit(List<String> vocabulary){init(vocabulary,1);} private void init(List<String> v,int mode){var a=NlpSupport.array(v);ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_nlp_embedding_init(handle(),a,v.size(),mode,e.pointer(),e.capacity()));}
 public double[] getEmbedding(String word){LongByReference c=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(N.coolbox_nlp_embedding_get(handle(),word,c,e.pointer(),e.capacity()));return Matrices.takeDoubles(p,c.getValue());}
 public boolean hasWord(String word){return N.coolbox_nlp_embedding_has(handle(),word)!=0;}
 public double[][] getEmbeddings(List<String> words){var a=NlpSupport.array(words);LongByReference r=new LongByReference(),c=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(N.coolbox_nlp_embedding_batch(handle(),a,words.size(),r,c,e.pointer(),e.capacity()));return Matrices.takeMatrix(p,r.getValue(),c.getValue());}
 public void updateEmbedding(String word,double[] values){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_nlp_embedding_update(handle(),word,values,values.length,e.pointer(),e.capacity()));}
 public double similarity(String first,String second){ErrorBuffer e=new ErrorBuffer();double x=N.coolbox_nlp_embedding_similarity(handle(),first,second,e.pointer(),e.capacity());e.throwIfSet();return x;}
 public List<Map.Entry<String,Double>> mostSimilar(String word){return mostSimilar(word,10);} public List<Map.Entry<String,Double>> mostSimilar(String word,int topK){ErrorBuffer e=new ErrorBuffer();List<String> lines=NlpSupport.takeStrings(e.check(N.coolbox_nlp_embedding_similar(handle(),word,topK,e.pointer(),e.capacity())));List<Map.Entry<String,Double>> out=new ArrayList<>();for(String line:lines){int i=line.lastIndexOf('\t');out.add(new AbstractMap.SimpleImmutableEntry<>(line.substring(0,i),Double.parseDouble(line.substring(i+1))));}return out;}
 public long vocabSize(){return N.coolbox_nlp_embedding_vocab_size(handle());} public long embeddingDim(){return N.coolbox_nlp_embedding_dim(handle());}
}
