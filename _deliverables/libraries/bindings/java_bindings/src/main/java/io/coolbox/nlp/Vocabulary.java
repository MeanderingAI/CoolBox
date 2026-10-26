package io.coolbox.nlp;
import com.sun.jna.Pointer; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import io.coolbox.internal.NativeHandle; import java.util.List;
/** Token vocabulary (Python: {@code nlp.Vocabulary}). */
public final class Vocabulary extends NativeHandle {
 private static final NlpNative N=NlpNative.INSTANCE; public static final int PAD_IDX=0,UNK_IDX=1,BOS_IDX=2,EOS_IDX=3;
 public Vocabulary(){this(1,0);} public Vocabulary(int minFrequency,int maxSize){super(N.coolbox_nlp_vocabulary_new(minFrequency,maxSize),N::coolbox_nlp_vocabulary_free);}
 Vocabulary(Pointer h){super(h,N::coolbox_nlp_vocabulary_free);}
 public Vocabulary build(List<List<String>> docs){var d=NlpSupport.docs(docs);ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_nlp_vocabulary_build(handle(),d.strings(),d.offsets(),docs.size(),e.pointer(),e.capacity()));return this;}
 public Vocabulary buildFromTexts(List<String> texts){var a=NlpSupport.array(texts);ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_nlp_vocabulary_build_texts(handle(),a,texts.size(),e.pointer(),e.capacity()));return this;}
 public void addSpecialToken(String token){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_nlp_vocabulary_add_special(handle(),token,e.pointer(),e.capacity()));}
 public long tokenToIndex(String token){return N.coolbox_nlp_vocabulary_to_index(handle(),token);} public String indexToToken(long index){ErrorBuffer e=new ErrorBuffer();return Matrices.takeString(e.check(N.coolbox_nlp_vocabulary_to_token(handle(),index,e.pointer(),e.capacity())));}
 public boolean contains(String token){return N.coolbox_nlp_vocabulary_contains(handle(),token)!=0;} public long size(){return N.coolbox_nlp_vocabulary_size(handle());} public long frequency(String token){return N.coolbox_nlp_vocabulary_frequency(handle(),token);}
 public long[] encode(List<String> tokens){var a=NlpSupport.array(tokens);LongByReference c=new LongByReference();ErrorBuffer e=new ErrorBuffer();return NlpSupport.takeLongs(e.check(N.coolbox_nlp_vocabulary_encode(handle(),a,tokens.size(),c,e.pointer(),e.capacity())),c);}
 public List<String> decode(long[] indices){ErrorBuffer e=new ErrorBuffer();return NlpSupport.takeStrings(e.check(N.coolbox_nlp_vocabulary_decode(handle(),indices,indices.length,e.pointer(),e.capacity())));}
}
