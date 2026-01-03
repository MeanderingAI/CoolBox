# C++ REST API Examples

This directory contains C++ examples demonstrating the REST API and JSON libraries.

## Prerequisites

1. **Build the main project first:**
   ```bash
   cd ../..
   ./clean&build.sh
   ```

2. **Requirements:**
   - C++17 compatible compiler (g++, clang++)
   - CMake >= 3.12
   - Libraries: `libjson.so`, `librest_api.so` (built by main project)

## Examples

### 1. Basic Server (`basic_server_example.cpp`)
Demonstrates fundamental REST API server usage:
- Creating a server with thread pool
- Adding GET and POST routes
- Path parameters (`/users/:id`)
- JSON request/response handling
- Request parsing and response building

**Key concepts:**
- `Server` class with `get()` and `post()` methods
- `Request` and `Response` objects
- `Builder` for creating JSON responses
- `Parser` for reading JSON requests

### 2. JSON Library (`json_example.cpp`)
Comprehensive JSON library demonstration:
- Building JSON with `Builder` class
- Creating nested objects and arrays
- Parsing JSON strings with `Parser`
- Type-safe value access
- Working with arrays of objects
- Backward-compatible simple utilities

**Key concepts:**
- `Value`, `Object`, `Array` classes
- `Builder` fluent API
- `Parser` for string parsing
- Type conversions and validation

### 3. Threaded Server (`threaded_server_example.cpp`)
Shows advanced concurrency features:
- Thread pool configuration (8 threads)
- Asynchronous request handling
- Concurrent request processing
- Performance comparison (sync vs async)
- High-volume request handling

**Key concepts:**
- `handle_request_async()` for non-blocking requests
- Thread pool benefits for I/O-bound operations
- Callbacks for async response handling
- Performance benchmarking

### 4. Model Server (`model_server_example.cpp`)
ML model serving capabilities:
- `ModelServer` class for model hosting
- Model registration and discovery
- Single and batch predictions
- Model metadata endpoints
- Error handling for invalid requests

**Key concepts:**
- `MLModel` interface implementation
- Model registration with names
- `/models` endpoints for discovery
- `/models/:name/predict` for inference
- JSON-based prediction API

## Building and Running

### Build all examples:
```bash
make
```

### Build specific example:
```bash
make basic_server
make json_example
make threaded_server
make model_server
```

### Run all examples:
```bash
make run-all
```

### Run specific example:
```bash
make run-basic       # Basic server
make run-json        # JSON library
make run-threaded    # Threaded server
make run-model       # Model server
```

### Clean compiled binaries:
```bash
make clean
```

### Show help:
```bash
make help
```

## Example Output

### Basic Server
```
=== Basic Server Example ===
Server started on port 8080
Thread pool size: 4 threads

GET / => {"message": "Hello, World!"}
GET /users/42 => {"user_id": "42", "name": "User 42", "status": "active"}
POST /users => {"message": "User created", "id": "12345", "data": {...}}
```

### JSON Library
```
=== JSON Library Example ===

1. Building JSON with Builder:
{"id": 123, "name": "Alice", "email": "alice@example.com", "active": true}

2. Creating nested JSON:
{"name": "Bob", "age": 30, "address": {"street": "123 Main St", ...}}

...
```

### Threaded Server
```
=== Threaded Server Example ===

Test 1: Synchronous Requests (blocking)
Total time (sequential): 100ms

Test 2: Asynchronous Requests (concurrent)
Total time (concurrent): 50ms
(Should be ~50ms with parallelism vs ~150ms sequential)

Test 3: High Volume Requests
Processed 20 requests in 15ms
Average: 0.75ms per request
```

### Model Server
```
=== Model Server Example ===

1. Get available models:
{"models": ["linear", "sine"]}

2. Get model metadata:
Linear model: {"model_type": "linear_regression", "equation": "y = 2.0 * x + 1.0"}

3. Single predictions:
Linear predictions: {"predictions": [3.0, 5.0, 7.0, 9.0, 11.0]}
(Expected: [3.0, 5.0, 7.0, 9.0, 11.0])

...
```

## Library Features Demonstrated

### JSON Library (`include/json/json.h`)
- **Value**: Holds any JSON type (null, bool, number, string, array, object)
- **Object**: Key-value map for JSON objects
- **Array**: Vector of values for JSON arrays
- **Parser**: Recursive descent parser for JSON strings
- **Builder**: Fluent API for constructing JSON
- **simple**: Backward-compatible utilities

### REST API Library (`include/rest_api/server.h`)
- **Server**: HTTP server with routing and thread pool
- **Request**: HTTP request with path, method, headers, body
- **Response**: HTTP response with status, headers, body
- **ThreadPool**: Worker thread pool for async processing
- **ModelServer**: Specialized server for ML model serving
- **MLModel**: Interface for model implementations

## Thread Pool Configuration

The server uses a thread pool for handling requests concurrently:

```cpp
// Default: 4 threads
Server server(8080);

// Custom: 8 threads
Server server(8080, 8);

// Async request handling
server.handle_request_async(request, [](const Response& res) {
    std::cout << "Response: " << res.body() << std::endl;
});
```

**Benefits:**
- Concurrent request processing
- Reduced latency for I/O-bound operations
- Better CPU utilization
- Scalable to many simultaneous clients

## Error Handling

All examples include error handling:

```cpp
try {
    Parser parser(json_str);
    Value data = parser.parse();
    // Use data...
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

## Troubleshooting

### Library not found error:
```
error while loading shared libraries: libjson.so: cannot open shared object file
```

**Solution:** Make sure libraries are built and `LD_LIBRARY_PATH` is set (Makefile does this automatically):
```bash
export LD_LIBRARY_PATH=../../build/src/json:../../build/src/rest_api:$LD_LIBRARY_PATH
```

### Compilation errors:
```
fatal error: rest_api/server.h: No such file or directory
```

**Solution:** Build main project first:
```bash
cd ../.. && ./clean&build.sh
```

## Next Steps

- Modify examples to experiment with different features
- Create custom models for `ModelServer`
- Add authentication middleware
- Implement custom JSON serialization
- Test with real HTTP clients (curl, Python requests)

## Python Bindings

These C++ examples have Python equivalents. See:
- `../rest_api_example.py` - Python REST API usage
- `python_bindings/test_bindings.py` - Comprehensive tests

Both C++ and Python share the same underlying implementation!
