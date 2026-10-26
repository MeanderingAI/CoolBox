package io.coolbox.internal;

import com.sun.jna.Library;
import com.sun.jna.Pointer;

/** Core exports of {@code coolbox_java_bridge} (native/src/core_jbridge.cc). */
public interface BridgeCore extends Library {
    BridgeCore INSTANCE = NativeLibraries.bridge(BridgeCore.class);

    /** Releases memory the bridge allocated with malloc (strings and arrays returned to Java). */
    void coolbox_java_free(Pointer pointer);

    String coolbox_java_bridge_version();
}
