**Java Binding Guide**

- **Purpose:** Public overview of the Java package and its native dependency flow.
- **Location:** `_libraries/java_bindings`

## Surface

- Documentation namespace: `coolbox::java`.
- Maven package with `io.coolbox.CoolBoxClient` as the primary entry point.
- Uses JNA to bridge into the native C bindings.
- Maven coordinates remain `io.coolbox:coolbox-java-bindings`.

## Build

Normal build:

```bash
mvn -f _libraries/java_bindings/pom.xml test package
```

Skip rebuilding the native C bindings when they already exist:

```bash
mvn -f _libraries/java_bindings/pom.xml -Dskip.c.bindings.build=true test package
```

## Notes

- Java 17 is required by the Maven compiler configuration.
- The default native bindings directory is `_libraries/c_bindings/build`.
- Test runs pass the native directory to the JVM as `coolbox.c.bindings.dir`.

## Packaging

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `java-extension-*` artifacts.
- The Java packaging flow restores the prebuilt C extension archive and repackages the native payload for Java consumers.