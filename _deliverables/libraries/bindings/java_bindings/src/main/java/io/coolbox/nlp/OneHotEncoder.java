package io.coolbox.nlp;
import com.sun.jna.Pointer; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import io.coolbox.internal.NativeHandle; import java.util.List;
/** One-hot categorical encoder (Python: {@code nlp.OneHotEncoder}). */
public final class OneHotEncoder extends NativeHandle {
 private static final NlpNative N=NlpNative.INSTANCE; public OneHotEncoder(){super(N.coolbox_nlp_onehot_new(),N::coolbox_nlp_onehot_free);}
 public OneHotEncoder fit(List<String> vocabulary){var a=NlpSupport.array(vocabulary);ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_nlp_onehot_fit(handle(),a,vocabulary.size(),e.pointer(),e.capacity()));return this;}
 public double[] encode(String word){LongByReference c=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(N.coolbox_nlp_onehot_encode(handle(),word,c,e.pointer(),e.capacity()));return Matrices.takeDoubles(p,c.getValue());}
 public double[][] encodeBatch(List<String> words){var a=NlpSupport.array(words);LongByReference r=new LongByReference(),c=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(N.coolbox_nlp_onehot_batch(handle(),a,words.size(),r,c,e.pointer(),e.capacity()));return Matrices.takeMatrix(p,r.getValue(),c.getValue());}
 public long vocabSize(){return N.coolbox_nlp_onehot_vocab_size(handle());}
}
