package io.coolbox.nlp;
import com.sun.jna.Pointer; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import java.util.List; import java.util.Set;
/** Text preprocessing and tokenization (Python: {@code nlp.TextProcessor}). */
public final class TextProcessor {
 private static final NlpNative N=NlpNative.INSTANCE; private TextProcessor(){}
 public static String toLowercase(String s){return Matrices.takeString(N.coolbox_nlp_lowercase(s));} public static String removePunctuation(String s){return Matrices.takeString(N.coolbox_nlp_remove_punctuation(s));} public static String removeNumbers(String s){return Matrices.takeString(N.coolbox_nlp_remove_numbers(s));} public static String removeExtraWhitespace(String s){return Matrices.takeString(N.coolbox_nlp_normalize_whitespace(s));}
 public static String strip(String s){return list(N.coolbox_nlp_strip(s)).get(0);}
 private static List<String> list(Pointer p){return NlpSupport.takeStrings(p);}
 public static List<String> tokenize(String text){return tokenize(text," ");} public static List<String> tokenize(String text,String delimiter){ErrorBuffer e=new ErrorBuffer();return list(e.check(N.coolbox_nlp_tokenize_full(text,delimiter,0,e.pointer(),e.capacity())));}
 public static List<String> wordTokenize(String text){ErrorBuffer e=new ErrorBuffer();return list(e.check(N.coolbox_nlp_tokenize_full(text," ",1,e.pointer(),e.capacity())));} public static List<String> sentenceTokenize(String text){ErrorBuffer e=new ErrorBuffer();return list(e.check(N.coolbox_nlp_tokenize_full(text," ",2,e.pointer(),e.capacity())));}
 public static List<String> generateNgrams(List<String> tokens,int n){var a=NlpSupport.array(tokens);ErrorBuffer e=new ErrorBuffer();return list(e.check(N.coolbox_nlp_ngrams(a,tokens.size(),n,e.pointer(),e.capacity())));}
 public static String stem(String word){ErrorBuffer e=new ErrorBuffer();return Matrices.takeString(e.check(N.coolbox_nlp_stem(word,e.pointer(),e.capacity())));}
 public static List<String> stemTokens(List<String> tokens){var a=NlpSupport.array(tokens);ErrorBuffer e=new ErrorBuffer();return list(e.check(N.coolbox_nlp_stem_tokens(a,tokens.size(),e.pointer(),e.capacity())));}
 public static List<String> defaultStopWords(){return list(N.coolbox_nlp_default_stop_words());}
 public static List<String> removeStopWords(List<String> tokens){return removeStopWords(tokens,defaultStopWords());} public static List<String> removeStopWords(List<String> tokens,Set<String> stopWords){return removeStopWords(tokens,List.copyOf(stopWords));} private static List<String> removeStopWords(List<String> t,List<String> s){var a=NlpSupport.array(t);var b=NlpSupport.array(s);ErrorBuffer e=new ErrorBuffer();return list(e.check(N.coolbox_nlp_remove_stop_words(a,t.size(),b,s.size(),e.pointer(),e.capacity())));}
 public static List<String> process(String text){return process(text,true,true,false,true,false);} public static List<String> process(String text,boolean lowercase,boolean removePunctuation,boolean removeNumbers,boolean removeStops,boolean stemming){ErrorBuffer e=new ErrorBuffer();return list(e.check(N.coolbox_nlp_process(text,lowercase?1:0,removePunctuation?1:0,removeNumbers?1:0,removeStops?1:0,stemming?1:0,e.pointer(),e.capacity())));}
}


