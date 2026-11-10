package io.coolbox.uuid;
import com.sun.jna.Library;
import io.coolbox.internal.NativeLibraries;
interface UuidNative extends Library {
 UuidNative INSTANCE=NativeLibraries.cBindings(UuidNative.class);
 String coolbox_c_uuid_v1(); String coolbox_c_uuid_v2(int localIdentifier,int localDomain);
 String coolbox_c_uuid_v3(String namespaceUuid,String name); String coolbox_c_uuid_v4();
 String coolbox_c_uuid_v5(String namespaceUuid,String name); String coolbox_c_uuid_v6();
 String coolbox_c_uuid_v8(String customEntropyHex); String coolbox_c_guid();
}
