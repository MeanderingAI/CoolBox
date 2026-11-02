package io.coolbox.misc;
import io.coolbox.internal.Matrices;
/** Python placeholder surface for Fuzzer. */
public final class Fuzzer { private Fuzzer(){} public static String functionName(){ return Matrices.takeString(MiscNative.INSTANCE.coolbox_misc_fuzzer_function_name()); } }
