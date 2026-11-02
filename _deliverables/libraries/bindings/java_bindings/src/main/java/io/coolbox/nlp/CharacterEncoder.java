package io.coolbox.nlp;
import com.sun.jna.Pointer; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import io.coolbox.internal.NativeHandle; import java.util.List;
/** Character-level text encoder (Python: {@code nlp.CharacterEncoder}). */
public final class CharacterEncoder extends NativeHandle {
 private static final NlpNative N=NlpNative.INSTANCE; public CharacterEncoder(){super(N.coolbox_nlp_character_new(),N::coolbox_nlp_character_free);}
 public CharacterEncoder fit(List<String> texts){var a=NlpSupport.array(texts);ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_nlp_character_fit(handle(),a,texts.size(),e.pointer(),e.capacity()));return this;}
 public long[] encode(String text){LongByReference c=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(N.coolbox_nlp_character_encode(handle(),text,c,e.pointer(),e.capacity()));return Matrices.takeLongs(p,c.getValue());}
 public String decode(long[] indices){ErrorBuffer e=new ErrorBuffer();return Matrices.takeString(e.check(N.coolbox_nlp_character_decode(handle(),indices,indices.length,e.pointer(),e.capacity())));}
 public long vocabSize(){return N.coolbox_nlp_character_vocab_size(handle());}
}
