# CoolBox Java Bindings

This module provides a plain Java extension scaffold for CoolBox.

## Current scope

- Maven-based Java package
- simple `CoolBoxClient` entry point
- version and metadata accessors
- ready for future JNI, REST, or generated bindings integration

## Build

```bash
mvn -f _libraries/java_bindings/pom.xml test package
```

## Use

```java
import io.coolbox.CoolBoxClient;

CoolBoxClient client = CoolBoxClient.createDefault();
System.out.println(client.getVersion());
System.out.println(client.describe());
```
