package io.coolbox.nlp;
import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import java.util.List; import java.util.Set;
/** NLP numerical helpers (Python: {@code nlp} module functions). */
public final class NlpFunctions {
 private static final NlpNative N=NlpNative.INSTANCE; private NlpFunctions(){}
 public static double cosineSimilarity(double[] a,double[] b){ErrorBuffer e=new ErrorBuffer();double r=N.coolbox_nlp_cosine(a,a.length,b,b.length,e.pointer(),e.capacity());e.throwIfSet();return r;}
 public static double jaccardSimilarity(Set<String> a,Set<String> b){var x=NlpSupport.array(List.copyOf(a));var y=NlpSupport.array(List.copyOf(b));ErrorBuffer e=new ErrorBuffer();double r=N.coolbox_nlp_jaccard(x,a.size(),y,b.size(),e.pointer(),e.capacity());e.throwIfSet();return r;}
 public static long levenshteinDistance(String a,String b){return N.coolbox_nlp_levenshtein(a,b);}
 public static double[][] createPositionalEncoding(int maxLength,int embeddingDim){LongByReference r=new LongByReference(),c=new LongByReference();ErrorBuffer e=new ErrorBuffer();return Matrices.takeMatrix(e.check(N.coolbox_nlp_positional(maxLength,embeddingDim,r,c,e.pointer(),e.capacity())),r.getValue(),c.getValue());}
 public static double[] averageEmbeddings(double[][] embeddings){return pool(embeddings,false);} public static double[] maxPoolingEmbeddings(double[][] embeddings){return pool(embeddings,true);}
 private static double[] pool(double[][] m,boolean max){if(m.length==0)throw new IllegalArgumentException("embeddings must not be empty");int cols=Matrices.columns(m);LongByReference c=new LongByReference();ErrorBuffer e=new ErrorBuffer();double[] out=Matrices.takeDoubles(e.check(N.coolbox_nlp_pool(Matrices.flatten(m),m.length,cols,max?1:0,c,e.pointer(),e.capacity())),c.getValue());return out;}
}
