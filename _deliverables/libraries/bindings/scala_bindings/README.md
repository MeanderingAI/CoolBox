# CoolBox Scala Bindings

This module provides Scala bindings for CoolBox through the native C bindings.

## Current scope

- Maven-based Scala package
- `CoolBoxScalaClient` entry point backed by the native C bindings
- version, capability, and UUID accessors delegated to the linked CoolBox library
- automatic native build during Maven runs

## Build

```bash
mvn -f _deliverables/libraries/bindings/scala_bindings/pom.xml test package
```

During the Maven build, the project configures and builds the native C bindings in `_deliverables/libraries/bindings/c_bindings/build`.

## Use

```scala
import io.coolbox.CoolBoxScalaClient

val client = CoolBoxScalaClient.createDefault()
println(client.getVersion)
println(client.describe)
println(client.getCapabilities.mkString(", "))
```
