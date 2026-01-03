#!/usr/bin/env python3
"""
REST API Model Server Example

This example demonstrates how to serve machine learning models via REST API.
"""

import sys
import os

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'build'))

try:
    import ml_core
    from ml_core import rest_api
    from ml_core import deep_learning as dl
except ImportError as e:
    print(f"Error importing ml_core: {e}")
    print("\nPlease build the Python bindings first:")
    print("  cd python_bindings && ./build.sh")
    sys.exit(1)

import json


def print_separator(title=""):
    """Print a formatted separator."""
    if title:
        print(f"\n{'='*70}")
        print(f"  {title}")
        print(f"{'='*70}\n")
    else:
        print(f"\n{'-'*70}\n")


def basic_server_example():
    """Basic REST API server example."""
    print_separator("1. Basic REST API Server")
    
    # Create a server
    server = rest_api.Server(port=8080)
    
    # Define a simple GET endpoint
    def hello_handler(req):
        response = rest_api.Response()
        response.set_status(200)
        response.set_json('{"message": "Hello, World!"}')
        return response
    
    server.get("/hello", hello_handler)
    
    # Define a POST endpoint with path parameters
    def echo_handler(req):
        response = rest_api.Response()
        name = req.get_path_param("name", "Guest")
        body = req.body()
        
        result = f'{{"name": "{name}", "received": "{body}"}}'
        response.set_json(result)
        return response
    
    server.post("/echo/:name", echo_handler)
    
    print(f"✓ Server created on port {server.port()}")
    print(f"  Registered routes:")
    print(f"    GET  /hello")
    print(f"    POST /echo/:name")
    
    # Test the endpoints
    print(f"\nTesting endpoints:")
    
    # Test GET /hello
    req1 = rest_api.Request(
        rest_api.HttpMethod.GET,
        "/hello",
        {},
        ""
    )
    resp1 = server.handle_request(req1)
    print(f"  GET /hello -> {resp1.status_code()}: {resp1.body()}")
    
    # Test POST /echo/John
    req2 = rest_api.Request(
        rest_api.HttpMethod.POST,
        "/echo/John",
        {"Content-Type": "application/json"},
        '{"data": "test"}'
    )
    resp2 = server.handle_request(req2)
    print(f"  POST /echo/John -> {resp2.status_code()}: {resp2.body()}")
    
    return server


def model_server_example():
    """ML Model serving example."""
    print_separator("2. ML Model Server")
    
    # Create a model server
    model_server = rest_api.ModelServer(port=8081)
    
    print(f"✓ Model server created on port 8081")
    
    # Define a simple prediction function
    def simple_predictor(features):
        """Simple linear model: y = sum(features) * 0.5"""
        total = sum(features)
        return [total * 0.5]
    
    # Setup prediction endpoint
    model_server.setup_prediction_endpoint("linear_model", simple_predictor)
    
    # Define batch prediction
    def batch_predictor(batch):
        """Batch prediction"""
        return [simple_predictor(features) for features in batch]
    
    model_server.setup_batch_prediction_endpoint("linear_model", batch_predictor)
    
    # Setup model info
    model_server.setup_info_endpoint("linear_model", "LinearRegression", "1.0.0")
    
    print(f"  Registered endpoints:")
    print(f"    GET  /health")
    print(f"    GET  /api/v1/models/linear_model/info")
    print(f"    POST /api/v1/models/linear_model/predict")
    print(f"    POST /api/v1/models/linear_model/batch_predict")
    
    # Test the endpoints
    print(f"\nTesting endpoints:")
    
    # Get the underlying server
    server = model_server.get_server()
    
    # Test health endpoint
    health_req = rest_api.Request(
        rest_api.HttpMethod.GET,
        "/health",
        {},
        ""
    )
    health_resp = server.handle_request(health_req)
    print(f"  GET /health -> {health_resp.status_code()}: {health_resp.body()}")
    
    # Test model info
    info_req = rest_api.Request(
        rest_api.HttpMethod.GET,
        "/api/v1/models/linear_model/info",
        {},
        ""
    )
    info_resp = server.handle_request(info_req)
    print(f"  GET /api/v1/models/linear_model/info -> {info_resp.status_code()}")
    print(f"      {info_resp.body()}")
    
    # Test prediction
    predict_req = rest_api.Request(
        rest_api.HttpMethod.POST,
        "/api/v1/models/linear_model/predict",
        {"Content-Type": "application/json"},
        '{"features": [1.0, 2.0, 3.0]}'
    )
    predict_resp = server.handle_request(predict_req)
    print(f"  POST /api/v1/models/linear_model/predict")
    print(f"      Input: [1.0, 2.0, 3.0]")
    print(f"      Response: {predict_resp.body()}")
    
    # Test batch prediction
    batch_req = rest_api.Request(
        rest_api.HttpMethod.POST,
        "/api/v1/models/linear_model/batch_predict",
        {"Content-Type": "application/json"},
        '{"batch": [[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]]}'
    )
    batch_resp = server.handle_request(batch_req)
    print(f"  POST /api/v1/models/linear_model/batch_predict")
    print(f"      Input: [[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]]")
    print(f"      Response: {batch_resp.body()}")
    
    return model_server


def neural_network_server_example():
    """Serve a neural network via REST API."""
    print_separator("3. Neural Network REST API")
    
    # Create a binary classifier
    print("Creating a binary classifier...")
    model = dl.binary_classifier(input_dim=4, hidden_dims=[8, 4])
    print("✓ Model created")
    
    # Create model server
    model_server = rest_api.ModelServer(port=8082)
    
    # Define prediction function using the neural network
    def nn_predictor(features):
        """Predict using neural network"""
        # In a real scenario, you'd call model.predict(features)
        # For this example, we'll simulate a prediction
        score = sum(features) / len(features)
        prediction = 1.0 if score > 0.5 else 0.0
        confidence = abs(score - 0.5) * 2
        return [prediction, confidence]
    
    # Setup endpoints
    model_server.setup_prediction_endpoint("iris_classifier", nn_predictor)
    model_server.setup_info_endpoint("iris_classifier", "NeuralNetwork", "1.0.0")
    
    print(f"✓ Neural network model server ready on port 8082")
    print(f"  Model: Binary classifier (4 -> 8 -> 4 -> 1)")
    
    # Test prediction
    server = model_server.get_server()
    
    test_req = rest_api.Request(
        rest_api.HttpMethod.POST,
        "/api/v1/models/iris_classifier/predict",
        {"Content-Type": "application/json"},
        '{"features": [5.1, 3.5, 1.4, 0.2]}'
    )
    
    test_resp = server.handle_request(test_req)
    print(f"\nTest prediction:")
    print(f"  Input: [5.1, 3.5, 1.4, 0.2]")
    print(f"  Response: {test_resp.body()}")
    
    return model_server


def cors_example():
    """Example with CORS enabled."""
    print_separator("4. CORS Support")
    
    server = rest_api.Server(port=8083)
    server.enable_cors("*")
    
    def api_handler(req):
        response = rest_api.Response()
        response.set_json('{"data": "CORS enabled"}')
        return response
    
    server.get("/api/data", api_handler)
    
    print("✓ Server with CORS enabled")
    
    # Test request
    req = rest_api.Request(
        rest_api.HttpMethod.GET,
        "/api/data",
        {},
        ""
    )
    resp = server.handle_request(req)
    
    print(f"\nResponse headers:")
    headers = resp.headers()
    for key, value in headers.items():
        if "Access-Control" in key:
            print(f"  {key}: {value}")
    
    return server


def json_utilities_example():
    """Demonstrate JSON utilities."""
    print_separator("5. JSON Utilities")
    
    # Encode dictionary to JSON
    data = {"name": "John", "age": "30", "city": "NYC"}
    json_str = rest_api.json_encode(data)
    print(f"Dictionary: {data}")
    print(f"JSON: {json_str}")
    
    # Decode JSON to dictionary
    decoded = rest_api.json_decode(json_str)
    print(f"Decoded: {decoded}")
    
    # Array encoding/decoding
    array = ["apple", "banana", "cherry"]
    json_array = rest_api.json_encode_array(array)
    print(f"\nArray: {array}")
    print(f"JSON Array: {json_array}")
    
    decoded_array = rest_api.json_decode_array(json_array)
    print(f"Decoded Array: {decoded_array}")


def main():
    """Main demonstration function."""
    print("\n" + "="*70)
    print("  REST API MODEL SERVER EXAMPLES")
    print("="*70)
    print("\nDemonstrating how to serve ML models via REST API")
    
    try:
        # Run examples
        server1 = basic_server_example()
        model_server1 = model_server_example()
        model_server2 = neural_network_server_example()
        server2 = cors_example()
        json_utilities_example()
        
        print_separator("Summary")
        print("✓ Successfully demonstrated REST API functionality")
        print("\nFeatures:")
        print("  • HTTP methods: GET, POST, PUT, DELETE, PATCH")
        print("  • Path parameters: /api/:param")
        print("  • Query parameters: /api?key=value")
        print("  • JSON request/response handling")
        print("  • Model serving endpoints")
        print("  • Batch prediction support")
        print("  • Health checks and model info")
        print("  • CORS support")
        print("\nUse Cases:")
        print("  • Serve ML models as REST APIs")
        print("  • Create microservices for predictions")
        print("  • Build model serving infrastructure")
        print("  • Deploy models for web/mobile apps")
        
        print("\nNote: This is a programmatic API for testing.")
        print("For production, consider using with a proper HTTP server like:")
        print("  • Flask/FastAPI wrapper")
        print("  • C++ HTTP server (e.g., cpp-httplib)")
        print("  • Microservice deployment platform")
        
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    print("\n" + "="*70 + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
