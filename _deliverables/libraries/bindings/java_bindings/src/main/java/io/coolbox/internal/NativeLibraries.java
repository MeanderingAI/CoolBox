package io.coolbox.internal;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Platform;

import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Locates and loads the CoolBox native libraries.
 *
 * <p>Two shared libraries back the Java API:
 * <ul>
 *   <li>{@code coolbox_java_bridge} - ML, data structures, matrices, optimisation, simulation, SQL helpers.</li>
 *   <li>{@code coolbox_c_bindings} - SQL database access, HTTP server, forms/HTML and UUIDs.</li>
 * </ul>
 * Both are produced by {@code native/CMakeLists.txt} into a single directory. The directory is resolved from
 * (in order) the system properties {@code coolbox.native.dir}, {@code coolbox.java.bridge.dir},
 * {@code coolbox.c.bindings.dir}, the environment variables {@code COOLBOX_NATIVE_DIR},
 * {@code COOLBOX_JAVA_BRIDGE_DIR}, {@code COOLBOX_C_BINDINGS_DIR}, and finally well-known build folders relative
 * to the working directory.
 */
public final class NativeLibraries {
    public static final String BRIDGE = "coolbox_java_bridge";
    public static final String C_BINDINGS = "coolbox_c_bindings";

    private NativeLibraries() {
    }

    /** Loads a JNA interface backed by {@code coolbox_java_bridge}. */
    public static <T extends Library> T bridge(Class<T> api) {
        return load(BRIDGE, api);
    }

    /** Loads a JNA interface backed by {@code coolbox_c_bindings}. */
    public static <T extends Library> T cBindings(Class<T> api) {
        return load(C_BINDINGS, api);
    }

    public static <T extends Library> T load(String library, Class<T> api) {
        Path resolved = resolve(library);
        Map<String, Object> options = Map.of(Library.OPTION_STRING_ENCODING, StandardCharsets.UTF_8.name());
        try {
            if (resolved != null) {
                return Native.load(resolved.toString(), api, options);
            }
            return Native.load(library, api, options);
        } catch (UnsatisfiedLinkError error) {
            throw new IllegalStateException("Unable to load native library '" + library + "'. Build it with "
                    + "'mvn generate-resources' (or cmake -S native -B native/build) and point coolbox.native.dir / "
                    + "COOLBOX_NATIVE_DIR at the directory containing it. Searched: " + searchDirectories(), error);
        }
    }

    /** Returns the absolute path of the native library file, or {@code null} if it is not in a search directory. */
    public static Path resolve(String library) {
        for (Path directory : searchDirectories()) {
            for (String fileName : candidateFileNames(library)) {
                Path candidate = directory.resolve(fileName);
                if (Files.isRegularFile(candidate)) {
                    return candidate;
                }
            }
        }
        return null;
    }

    public static List<Path> searchDirectories() {
        List<Path> directories = new ArrayList<>();
        for (String property : new String[] {"coolbox.native.dir", "coolbox.java.bridge.dir", "coolbox.c.bindings.dir"}) {
            addIfDirectory(directories, System.getProperty(property));
        }
        for (String variable : new String[] {"COOLBOX_NATIVE_DIR", "COOLBOX_JAVA_BRIDGE_DIR", "COOLBOX_C_BINDINGS_DIR"}) {
            addIfDirectory(directories, System.getenv(variable));
        }
        Path userDir = Paths.get("").toAbsolutePath().normalize();
        addIfDirectory(directories, userDir.resolve("native/build/natives"));
        addIfDirectory(directories, userDir.resolve("_deliverables/libraries/bindings/java_bindings/native/build/natives"));
        addIfDirectory(directories, userDir.resolve("../c_bindings/build"));
        return directories;
    }

    private static List<String> candidateFileNames(String library) {
        List<String> names = new ArrayList<>();
        names.add(System.mapLibraryName(library));
        if (Platform.isWindows()) {
            names.add("lib" + library + ".dll");
            names.add(library + ".dll");
        } else {
            names.add(Platform.isMac() ? library + ".dylib" : library + ".so");
        }
        return names;
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
        for (String config : new String[] {"Release", "RelWithDebInfo", "Debug", "MinSizeRel"}) {
            Path configured = normalized.resolve(config);
            if (Files.isDirectory(configured) && !directories.contains(configured)) {
                directories.add(configured);
            }
        }
    }
}
