package io.coolbox;

import com.sun.jna.Library;
import org.junit.jupiter.api.DynamicTest;
import org.junit.jupiter.api.TestFactory;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.Proxy;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;
import java.util.stream.Stream;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;

class NativeBindingsTest {
    @TestFactory
    Stream<DynamicTest> allDeclaredFunctionsAreExported() throws Exception {
        Path classes = Path.of(CoolBoxClient.class.getProtectionDomain().getCodeSource().getLocation().toURI());
        Map<Class<?>, Object> libraries = new LinkedHashMap<>();
        Set<Class<?>> interfaces = new LinkedHashSet<>();
        try (Stream<Path> files = Files.walk(classes.resolve("io").resolve("coolbox"))) {
            for (Path file : files.filter(path -> path.toString().endsWith(".class")).sorted().toList()) {
                String relative = classes.relativize(file).toString();
                String name = relative.substring(0, relative.length() - ".class".length())
                        .replace('\\', '.').replace('/', '.');
                Class<?> type = Class.forName(name, false, CoolBoxClient.class.getClassLoader());
                if (type.isInterface() && Library.class.isAssignableFrom(type)) {
                    interfaces.add(type);
                }
                for (Field field : type.getDeclaredFields()) {
                    if (Modifier.isStatic(field.getModifiers()) && Library.class.isAssignableFrom(field.getType())) {
                        field.setAccessible(true);
                        libraries.put(field.getType(), field.get(null));
                    }
                }
            }
        }
        assertFalse(libraries.isEmpty(), "No JNA interfaces discovered");
        assertEquals(interfaces, libraries.keySet(), "Every JNA interface must have a loaded library");
        return libraries.entrySet().stream().flatMap(entry -> {
            Library.Handler handler = (Library.Handler) Proxy.getInvocationHandler(entry.getValue());
            return Stream.of(entry.getKey().getDeclaredMethods())
                    .filter(method -> !Modifier.isStatic(method.getModifiers()) && !method.isDefault())
                    .map(method -> exportedFunction(handler, method));
        });
    }

    private static DynamicTest exportedFunction(Library.Handler handler, Method method) {
        return DynamicTest.dynamicTest(method.getDeclaringClass().getName() + "." + method.getName(),
                () -> handler.getNativeLibrary().getFunction(method.getName()));
    }
}
