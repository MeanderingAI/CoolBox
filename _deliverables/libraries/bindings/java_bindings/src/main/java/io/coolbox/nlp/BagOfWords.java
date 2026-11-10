package io.coolbox.nlp;
import com.sun.jna.Pointer; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import io.coolbox.internal.NativeHandle; import java.util.List;
/** Bag-of-words vectorizer (Python: {@code nlp.BagOfWords}). */
public final class BagOfWords extends NativeHandle {
 private static final NlpNative N=NlpNative.INSTANCE; public BagOfWords(){this(null);} public BagOfWords(Vocabulary vocabulary){super(N.coolbox_nlp_bag_new(vocabulary==null?null:vocabulary.handle()),N::coolbox_nlp_bag_free);}
 public BagOfWords fit(List<List<String>> docs){var d=NlpSupport.docs(docs);ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_nlp_bag_fit(handle(),d.strings(),d.offsets(),docs.size(),e.pointer(),e.capacity()));return this;}
 public double[] transform(List<String> tokens){var a=NlpSupport.array(tokens);LongByReference c=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(N.coolbox_nlp_bag_transform(handle(),a,tokens.size(),c,e.pointer(),e.capacity()));return Matrices.takeDoubles(p,c.getValue());}
 public double[][] transformBatch(List<List<String>> docs){var d=NlpSupport.docs(docs);LongByReference r=new LongByReference(),c=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(N.coolbox_nlp_bag_batch(handle(),d.strings(),d.offsets(),docs.size(),r,c,e.pointer(),e.capacity()));return Matrices.takeMatrix(p,r.getValue(),c.getValue());}
 public Vocabulary vocabulary(){return new Vocabulary(N.coolbox_nlp_bag_vocabulary(handle()));}
}
