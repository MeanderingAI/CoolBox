package io.coolbox;

import com.sun.jna.Library;
import io.coolbox.internal.NativeLibraries;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

/** Java entry point backed by the native CoolBox C bindings. */
public final class CoolBoxClient {
    private static final CoolBoxNative NATIVE = loadNativeBindings();

    private final String endpoint;

    private CoolBoxClient(String endpoint) {
        this.endpoint = endpoint;
    }

    public static CoolBoxClient createDefault() {
        return new CoolBoxClient("local://coolbox");
    }

    public static CoolBoxClient forEndpoint(String endpoint) {
        return new CoolBoxClient(endpoint);
    }

    public String getEndpoint() {
        return endpoint;
    }

    public String getVersion() {
        return NATIVE.coolbox_c_version();
    }

    public String describe() {
        return NATIVE.coolbox_c_describe();
    }

    public int getCapabilityCount() {
        return NATIVE.coolbox_c_capability_count();
    }

    public String getCapabilityAt(int index) {
        if (index < 0 || index >= getCapabilityCount()) {
            return "";
        }
        return NATIVE.coolbox_c_capability_at(index);
    }

    public List<String> getCapabilities() {
        int capabilityCount = getCapabilityCount();
        List<String> capabilities = new ArrayList<>(capabilityCount);
        for (int i = 0; i < capabilityCount; i++) {
            capabilities.add(getCapabilityAt(i));
        }
        return List.copyOf(capabilities);
    }

    public Instant generatedAt() {
        return Instant.now();
    }

    public boolean isReady() {
        return NATIVE.coolbox_c_is_ready() != 0;
    }

    public String uuidV1() {
        return NATIVE.coolbox_c_uuid_v1();
    }

    public String uuidV2(long localIdentifier, int localDomain) {
        return NATIVE.coolbox_c_uuid_v2((int) localIdentifier, localDomain);
    }

    public String uuidV3(String namespaceUuid, String name) {
        return NATIVE.coolbox_c_uuid_v3(namespaceUuid, name);
    }

    public String uuidV4() {
        return NATIVE.coolbox_c_uuid_v4();
    }

    public String uuidV5(String namespaceUuid, String name) {
        return NATIVE.coolbox_c_uuid_v5(namespaceUuid, name);
    }

    public String uuidV6() {
        return NATIVE.coolbox_c_uuid_v6();
    }

    public String uuidV8(String customEntropyHex) {
        return NATIVE.coolbox_c_uuid_v8(customEntropyHex);
    }

    public String guid() {
        return NATIVE.coolbox_c_guid();
    }

    private static CoolBoxNative loadNativeBindings() {
        return NativeLibraries.cBindings(CoolBoxNative.class);
    }

    private interface CoolBoxNative extends Library {
        String coolbox_c_version();
        String coolbox_c_describe();
        int coolbox_c_capability_count();
        String coolbox_c_capability_at(int index);
        int coolbox_c_is_ready();
        String coolbox_c_uuid_v1();
        String coolbox_c_uuid_v2(int localIdentifier, int localDomain);
        String coolbox_c_uuid_v3(String namespaceUuid, String name);
        String coolbox_c_uuid_v4();
        String coolbox_c_uuid_v5(String namespaceUuid, String name);
        String coolbox_c_uuid_v6();
        String coolbox_c_uuid_v8(String customEntropyHex);
        String coolbox_c_guid();
    }
}
