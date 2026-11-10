package io.coolbox.misc;
import com.sun.jna.Library; import com.sun.jna.Pointer; import io.coolbox.internal.NativeLibraries; import io.coolbox.internal.BridgeCore; import io.coolbox.internal.Matrices;
interface MiscNative extends Library { MiscNative INSTANCE=NativeLibraries.bridge(MiscNative.class); Pointer coolbox_misc_fuzzer_function_name(); Pointer coolbox_misc_http_function_name(); }
