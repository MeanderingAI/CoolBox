# Shared Libraries Usage Examples

This document provides usage examples for all shared libraries in the ToolBox project, in C++, Python, and JavaScript (via WASM/Emscripten where available).

---

## advanced_logging

### C++ Example
```cpp
#include "advanced_logging/advanced_logging.h"

int main() {
    advanced_logging::Logger logger("mylog.log");
    logger.info("Hello from C++!");
    logger.warn("This is a warning");
    logger.error("This is an error");
    return 0;
}
```

### Python Example
```python
from ml_toolbox import advanced_logging

logger = advanced_logging.Logger("mylog.log")
logger.info("Hello from Python!")
logger.warn("This is a warning")
logger.error("This is an error")
```

### JavaScript Example (WASM/Emscripten)
```js
// Assuming advanced_logging.js and .wasm are loaded
createAdvancedLoggingModule().then(Module => {
    const Logger = Module.Logger;
    const Level = Module.Level;
    const logger = new Logger(); // logs to console by default
    logger.info("Hello from JS!");
    logger.warn("This is a warning");
    logger.error("This is an error");
});
```

---

## [Add more libraries here as you expose them]

- For each library, provide C++, Python, and JS usage if bindings exist.
- For JS, ensure the WASM/JS build is available and loaded in the browser.
- For Python, ensure the python_bindings package is installed.

---

*This document is auto-generated. Update as new bindings are added.*
