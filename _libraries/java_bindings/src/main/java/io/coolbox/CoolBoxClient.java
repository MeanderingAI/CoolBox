package io.coolbox;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;

import java.time.Instant;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
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

    public List<String> getCapabilities() {
        int capabilityCount = NATIVE.coolbox_c_capability_count();
        List<String> capabilities = new ArrayList<>(capabilityCount);
        for (int i = 0; i < capabilityCount; i++) {
            capabilities.add(NATIVE.coolbox_c_capability_at(i));
        }
        return List.copyOf(capabilities);
    }

    public Instant generatedAt() {
        return Instant.now();
    }

    public boolean isReady() {
        return NATIVE.coolbox_c_is_ready() != 0;
    }

    private static CoolBoxNative loadNativeBindings() {
        for (Path directory : nativeSearchDirectories()) {
            NativeLibrary.addSearchPath("coolbox_c_bindings", directory.toString());
        }

        try {
            return Native.load("coolbox_c_bindings", CoolBoxNative.class);
        } catch (UnsatisfiedLinkError error) {
            throw new IllegalStateException(
                    "Unable to load native CoolBox C bindings. Set coolbox.c.bindings.dir or COOLBOX_C_BINDINGS_DIR to the build directory.",
                    error
            );
        }
    }

    private static List<Path> nativeSearchDirectories() {
        List<Path> directories = new ArrayList<>();
        addIfDirectory(directories, System.getProperty("coolbox.c.bindings.dir"));
        addIfDirectory(directories, System.getenv("COOLBOX_C_BINDINGS_DIR"));

        Path userDir = Paths.get("").toAbsolutePath().normalize();
        addIfDirectory(directories, userDir.resolve("_libraries/c_bindings/build"));
        addIfDirectory(directories, userDir.resolve("../c_bindings/build"));

        return directories;
    }

    private static void addIfDirectory(List<Path> directories, String candidate) {
        if (candidate == null || candidate.isBlank()) {
            return;
        }

        addIfDirectory(directories, Paths.get(candidate));
    }

    private static void addIfDirectory(List<Path> directories, Path candidate) {
        Path normalized = candidate.toAbsolutePath().normalize();
        if (Files.isDirectory(normalized) && !directories.contains(normalized)) {
            directories.add(normalized);
        }
    }

    private interface CoolBoxNative extends Library {
        String coolbox_c_version();
        String coolbox_c_describe();
        int coolbox_c_capability_count();
        String coolbox_c_capability_at(int index);
        int coolbox_c_is_ready();
    }
}
