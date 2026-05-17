# Java Extension

## Location

- `_libraries/java_bindings`

## Surface

- Documentation namespace: `coolbox::java`
- Maven package with `io.coolbox.CoolBoxClient` as the primary entry point
- Uses JNA to bridge into the native C bindings
- Maven coordinates remain `io.coolbox:coolbox-java-bindings`

## Installation

Build the package with Maven:

```bash
mvn -f _libraries/java_bindings/pom.xml test package
```

## Setup And Build

- Requires Java 17 according to the Maven compiler configuration.
- By default, the Maven build configures and builds `_libraries/c_bindings` during the `generate-resources` phase.

Normal build:

```bash
mvn -f _libraries/java_bindings/pom.xml test package
```

If the native C bindings were already built and you want to skip rebuilding them:

```bash
mvn -f _libraries/java_bindings/pom.xml -Dskip.c.bindings.build=true test package
```

## Setup Notes

- The default native bindings directory is `_libraries/c_bindings/build`.
- The test configuration passes that directory to the JVM as `coolbox.c.bindings.dir`.

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- Release workflows publish `java-extension-*` artifacts.
- The Java packaging flow restores the prebuilt C extension archive and repackages the native payload for Java consumers.