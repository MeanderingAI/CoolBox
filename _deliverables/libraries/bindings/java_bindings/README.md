# CoolBox Java Bindings

This module provides Java 17+ bindings for CoolBox using JNA. It loads
`coolbox_java_bridge` for the numerical and model APIs and `coolbox_c_bindings`
for SQL, HTTP, forms, HTML and UUIDs. No JNI code generation is required.

## Available APIs

- Core metadata and capabilities through `CoolBoxClient`.
- Bandits, Bayesian networks, decision trees, ensembles, generalized linear
  models, SVMs, K-means, dimensionality reduction and hidden Markov models.
- Deep learning, distributed learning, NLP, computer vision, time series and
  Kalman filtering.
- Data structures, matrices, mathematics, optimization, finance, electronics,
  PDEs, graphics, synthetic data, timers and miscellaneous utilities.
- SQL, JSON, HTTP servers and routes, form validation, HTML rendering and UUIDs.

The Maven source tree is [src/main/java](src/main/java). The older Java/JNI
sources outside that directory are not part of this artifact. These bindings
cover the APIs implemented in that source tree; they do not promise complete
coverage of every C++ type or Python extension.

## Requirements

- A 64-bit JDK 17 or newer and Maven.
- CMake 3.21 or newer and a C++17 compiler targeting the JVM's architecture.
- Internet access on the first build to fetch Maven and native dependencies.

On Windows ARM64, the Maven `windows-aarch64` profile automatically selects
LLVM-MinGW. Install it with `winget install MartinStorsjo.LLVM-MinGW.UCRT`
and put its `bin` directory on `PATH`, including the
`aarch64-w64-mingw32-clang` and `aarch64-w64-mingw32-clang++` executables and
MinGW make. On Windows x64, use an installed Visual Studio C++ toolchain.

## Build

From the repository root:

```powershell
mvn -B -f _deliverables\libraries\bindings\java_bindings\pom.xml clean verify
```

Use `/` instead of `\` in shell paths on Linux or macOS.

Maven configures [native/CMakeLists.txt](native/CMakeLists.txt), builds both
native libraries and stages them and their Windows runtime dependencies in
`native/build/natives`. The Java JAR is produced in `target`. Native libraries
are **not embedded in the JAR**: distribute the native output directory
alongside it and keep its dependency libraries together.

For a Java-only rebuild against already-built native libraries:

```powershell
mvn -B -f _deliverables\libraries\bindings\java_bindings\pom.xml "-Dskip.native.build=true" test
```

For an isolated native build directory or another toolchain, override
`coolbox.native.build.dir` or `coolbox.native.cmake.args`. Quote Maven `-D`
arguments containing dots when invoking Maven from PowerShell.

## Use

```java
import io.coolbox.CoolBoxClient;

CoolBoxClient client = CoolBoxClient.createDefault();
System.out.println(client.getVersion());
System.out.println(client.describe());
```

Native-backed objects implementing `AutoCloseable` should be used with
try-with-resources:

```java
import io.coolbox.svm.LinearKernel;
import io.coolbox.svm.SVM;

try (var kernel = new LinearKernel(); var model = new SVM(kernel)) {
    model.fit(new double[][] {{-2}, {-1}, {1}, {2}},
              new double[] {-1, -1, 1, 1});
    System.out.println(model.predict(new double[] {1.5}));
}
```

Calls after `close()` fail with `IllegalStateException`. Native errors are
reported as `CoolBoxException`. Strings crossing either library boundary use
UTF-8, independently of JNA's default encoding.

## Native library discovery

Set `-Dcoolbox.native.dir=<absolute native directory>` on the application's
Java command, or set `COOLBOX_NATIVE_DIR`. The loader also supports
`coolbox.java.bridge.dir`, `coolbox.c.bindings.dir`, `COOLBOX_JAVA_BRIDGE_DIR`
and `COOLBOX_C_BINDINGS_DIR`, and searches standard build directories relative
to the current working directory. System properties take precedence over
environment variables. An unsuccessful load reports the searched directories.

The native compiler and JVM must agree on architecture. The JNA declarations
use Java `long` for native `size_t`, so 32-bit runtimes are not supported.

## Verification

`mvn test` runs behavioral tests against the real native libraries, including
an HTTP loopback request, model fitting and prediction, SQL operations and
data-structure operations. In addition,
[NativeBindingsTest](src/test/java/io/coolbox/NativeBindingsTest.java) discovers
the compiled JNA interfaces and checks that every declared function is exported
by the library actually backing that interface. This checks symbol availability,
not the correctness of every possible native argument combination.

Regression tests also cover UTF-8 transfer with a non-UTF-8 JNA default,
exactly-once handle release and use-after-close rejection. Surefire uses a
separate TCP channel so output from native algorithms does not corrupt the
test runner's protocol.
