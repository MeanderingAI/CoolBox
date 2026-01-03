# REST API Module

A lightweight C++ REST API framework for serving machine learning models with Python bindings.

## Features

- **HTTP Methods**: GET, POST, PUT, DELETE, PATCH, OPTIONS
- **Routing**: Pattern-based routing with path parameters (e.g., `/api/:id`)
- **Query Parameters**: Automatic parsing of URL query strings
- **JSON Support**: Built-in JSON encoding/decoding utilities
- **Model Serving**: Specialized `ModelServer` class for ML model deployment
- **CORS Support**: Enable cross-origin requests
- **Middleware**: Support for custom middleware functions

## Quick Start

### Basic Server

```python
from ml_toolbox import rest_api

# Create a server
server = rest_api.Server(port=8080)

# Define a route
def hello_handler(req):
    response = rest_api.Response()
    response.set_json('{"message": "Hello, World!"}')
    return response

server.get("/hello", hello_handler)

# Start the server (for testing, use handle_request)
req = rest_api.Request(rest_api.HttpMethod.GET, "/hello", {}, "")
resp = server.handle_request(req)
print(resp.body())  # {"message": "Hello, World!"}
```

### Model Server

```python
from ml_toolbox import rest_api, deep_learning as dl

# Create a model server
model_server = rest_api.ModelServer(port=8080)

# Define a prediction function
def predict(features):
    # Your model prediction logic
    return [sum(features) * 0.5]

# Setup endpoints
model_server.setup_prediction_endpoint("my_model", predict)
model_server.setup_info_endpoint("my_model", "LinearModel", "1.0.0")

# Test it
server = model_server.get_server()
req = rest_api.Request(
    rest_api.HttpMethod.POST,
    "/api/v1/models/my_model/predict",
    {"Content-Type": "application/json"},
    '{"features": [1.0, 2.0, 3.0]}'
)
resp = server.handle_request(req)
print(resp.body())  # {"prediction": [3.0]}
```

## API Reference

### Server Class

```python
server = rest_api.Server(port=8080)

# Register routes
server.get(pattern, handler)
server.post(pattern, handler)
server.put(pattern, handler)
server.delete(pattern, handler)
server.patch(pattern, handler)

# Enable CORS
server.enable_cors("*")

# Handle requests
response = server.handle_request(request)

# Start/stop (mock implementation)
server.start()
server.stop()
```

### Request Class

```python
request = rest_api.Request(method, path, headers, body)

# Access request data
request.method()           # HttpMethod enum
request.path()             # "/api/users/123"
request.body()             # Request body string
request.headers()          # Dict of headers
request.query_params()     # Dict of query parameters
request.path_params()      # Dict of path parameters

# Convenience methods
request.get_header("Content-Type", default="")
request.get_query_param("page", default="1")
request.get_path_param("id", default="")
```

### Response Class

```python
response = rest_api.Response()

# Set response data
response.set_status(200)
response.set_body("Hello")
response.set_header("Content-Type", "text/plain")
response.set_json('{"key": "value"}')

# Get response data
response.status_code()
response.body()
response.headers()
response.to_string()  # Full HTTP response
```

### ModelServer Class

```python
model_server = rest_api.ModelServer(port=8080)

# Setup endpoints
model_server.setup_prediction_endpoint(model_name, predict_fn)
model_server.setup_batch_prediction_endpoint(model_name, batch_predict_fn)
model_server.setup_health_endpoint()
model_server.setup_info_endpoint(model_name, model_type, version)

# Access underlying server
server = model_server.get_server()
```

### JSON Utilities

```python
# Encode/decode dictionaries
json_str = rest_api.json_encode({"key": "value"})
data = rest_api.json_decode(json_str)

# Encode/decode arrays
json_array = rest_api.json_encode_array(["a", "b", "c"])
array = rest_api.json_decode_array(json_array)
```

## Path Parameters

Use `:param` syntax for dynamic path segments:

```python
server.get("/users/:id", handler)
server.post("/api/:version/models/:name", handler)

# In handler:
def handler(req):
    user_id = req.get_path_param("id")
    version = req.get_path_param("version")
    name = req.get_path_param("name")
    # ...
```

## Query Parameters

Automatically parsed from URL:

```python
# URL: /search?q=test&limit=10

def handler(req):
    query = req.get_query_param("q")
    limit = req.get_query_param("limit", "20")
    # ...
```

## Standard Endpoints (ModelServer)

When using `ModelServer`, these endpoints are automatically available:

- `GET /health` - Health check
- `GET /api/v1/models/:name/info` - Model information
- `POST /api/v1/models/:name/predict` - Single prediction
- `POST /api/v1/models/:name/batch_predict` - Batch prediction

### Prediction Request Format

Single prediction:
```json
{
  "features": [1.0, 2.0, 3.0]
}
```

Batch prediction:
```json
{
  "batch": [
    [1.0, 2.0],
    [3.0, 4.0],
    [5.0, 6.0]
  ]
}
```

## CORS Support

Enable CORS for cross-origin requests:

```python
server.enable_cors("*")  # Allow all origins
server.enable_cors("https://example.com")  # Specific origin
```

## HTTP Status Codes

Use the `HttpStatus` enum:

```python
response.set_status(rest_api.HttpStatus.OK)                    # 200
response.set_status(rest_api.HttpStatus.CREATED)               # 201
response.set_status(rest_api.HttpStatus.BAD_REQUEST)           # 400
response.set_status(rest_api.HttpStatus.NOT_FOUND)             # 404
response.set_status(rest_api.HttpStatus.INTERNAL_SERVER_ERROR) # 500
```

## Complete Example

See [examples/rest_api_example.py](../examples/rest_api_example.py) for comprehensive examples including:
- Basic server with routes
- Model serving
- Neural network REST API
- CORS configuration
- JSON utilities

## Production Deployment

This module provides a programmatic API for testing and development. For production:

1. **Use with Flask/FastAPI**: Wrap the handlers in a Python web framework
2. **C++ HTTP Server**: Integrate with cpp-httplib or similar for native HTTP
3. **Container Deployment**: Deploy as microservices in Docker/Kubernetes
4. **API Gateway**: Put behind nginx, Kong, or cloud API gateway

## Limitations

- This is a mock server implementation for Python bindings
- `start()` and `stop()` are placeholder methods
- Use `handle_request()` for programmatic testing
- For real HTTP serving, integrate with a proper HTTP server library

## Architecture

```
rest_api::Server
├── Routes (pattern matching)
├── Handlers (user-defined functions)
├── Middleware (optional)
└── Request/Response objects

rest_api::ModelServer (extends Server)
├── Prediction endpoints
├── Batch prediction
├── Health checks
└── Model metadata
```

## Thread Safety

The current implementation is not thread-safe. For concurrent requests:
- Use request handling in separate processes
- Add mutex protection for shared state
- Consider using a thread-safe HTTP server library

## Future Enhancements

- [ ] WebSocket support
- [ ] Streaming responses
- [ ] File upload handling
- [ ] Session management
- [ ] Authentication middleware
- [ ] Rate limiting
- [ ] Request logging
- [ ] OpenAPI/Swagger documentation generation
