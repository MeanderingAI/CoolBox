# Scala Extension

## Location

- `_deliverables/libraries/bindings/scala_bindings`

## Surface

- Documentation namespace: `coolbox::scala`
- Maven package with `io.coolbox.CoolBoxScalaClient` as the primary entry point
- Uses JNA to bridge into the native C bindings
- Maven coordinates remain `io.coolbox:coolbox-scala-bindings`

## Installation

Build the package with Maven:

```bash
mvn -f _deliverables/libraries/bindings/scala_bindings/pom.xml test package
```

## Setup And Build

- Requires Java 17 according to the Maven compiler configuration.
- Uses Scala `2.13.16` via `scala-maven-plugin`.
- By default, the Maven build configures and builds `_deliverables/libraries/bindings/c_bindings` during the `generate-resources` phase.

Normal build:

```bash
mvn -f _deliverables/libraries/bindings/scala_bindings/pom.xml test package
```

If the native C bindings were already built and you want to skip rebuilding them:

```bash
mvn -f _deliverables/libraries/bindings/scala_bindings/pom.xml -Dskip.c.bindings.build=true test package
```

## Setup Notes

- The default native bindings directory is `_deliverables/libraries/bindings/c_bindings/build`.
- The test configuration passes that directory to the JVM as `coolbox.c.bindings.dir`.

## Packaging Notes

- Release page: https://github.com/MeanderingAI/CoolBox/releases
- The Scala bindings module is scaffolded in-repo and discoverable from the GUI extension browser.
- No dedicated Scala release artifact workflow is wired yet; packaging currently follows the same Maven/JNA pattern as Java at the source level.
