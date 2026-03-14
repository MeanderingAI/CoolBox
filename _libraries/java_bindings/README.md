# CoolBox Java Bindings

This module provides Java bindings for CoolBox through the native C bindings.

## Current scope

- Maven-based Java package
- `CoolBoxClient` entry point backed by the native C bindings
- version and metadata accessors delegated to the linked CoolBox library
- automatic native build during Maven runs

## Build

```bash
mvn -f _libraries/java_bindings/pom.xml test package
```

During the Maven build, the project configures and builds the native C bindings in `_libraries/c_bindings/build`.

## Use

```java
import io.coolbox.CoolBoxClient;

CoolBoxClient client = CoolBoxClient.createDefault();
System.out.println(client.getVersion());
System.out.println(client.describe());
```
