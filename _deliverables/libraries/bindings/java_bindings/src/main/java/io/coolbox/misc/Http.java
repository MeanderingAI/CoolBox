package io.coolbox.misc;
import io.coolbox.internal.Matrices;
/** Python placeholder surface for HTTP. */
public final class Http { private Http(){} public static String functionName(){ return Matrices.takeString(MiscNative.INSTANCE.coolbox_misc_http_function_name()); } }
