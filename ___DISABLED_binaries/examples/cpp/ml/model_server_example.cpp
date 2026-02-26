/**
 * Model Server Example
 * 
 * Demonstrates:
 * - ModelServer for ML model serving
 * - Prediction endpoints
 * - Model metadata
 * - Batch prediction
 */

// #include "networking/rest_api/model_server.h" (removed)
#include "dataformats/json/json.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace networking::rest_api;
using namespace networking::json;

// Simple linear model: y = w * x + b
class LinearModel : public MLModel {
private:
    double weight_ = 2.0;
    double bias_ = 1.0;
    
public:
    std::vector<double> predict(const std::vector<double>& input) override {
        std::vector<double> output;
        for (double x : input) {
            output.push_back(weight_ * x + bias_);
        }
        return output;
    }
    
    std::vector<std::vector<double>> predict_batch(
        const std::vector<std::vector<double>>& inputs) override {
        std::vector<std::vector<double>> outputs;
        for (const auto& input : inputs) {
            outputs.push_back(predict(input));
        }
        return outputs;
    }
    
    std::string metadata() const override {
        Builder metadata_builder;
        metadata_builder.add("model_type", "linear_regression")
                       .add("parameters", 2)
                       .add("weight", weight_)
                       .add("bias", bias_)
                       .add("equation", "y = 2.0 * x + 1.0");
        
        return metadata_builder.build().to_string();
    }
};

// Non-linear model: y = sin(x) * scale
class SineModel : public MLModel {
private:
    double scale_ = 10.0;
    
public:
    std::vector<double> predict(const std::vector<double>& input) override {
        std::vector<double> output;
        for (double x : input) {
            output.push_back(std::sin(x) * scale_);
        }
        return output;
    }
    
    std::vector<std::vector<double>> predict_batch(
        const std::vector<std::vector<double>>& inputs) override {
        std::vector<std::vector<double>> outputs;
        for (const auto& input : inputs) {
            outputs.push_back(predict(input));
        }
        return outputs;
    }
    
    std::string metadata() const override {
        Builder metadata_builder;
        metadata_builder.add("model_type", "sine_function")
                       .add("parameters", 1)
                       .add("scale", scale_)
                       .add("equation", "y = sin(x) * 10.0");
        
        return metadata_builder.build().to_string();
    }
};

int main() {
    std::cout << "=== Model Server Example ===" << std::endl;
    
    // Create model server with 4 threads
    // ModelServer removed for HTTP2-only build
    
    // Register models
    auto linear_model = std::make_shared<LinearModel>();
    auto sine_model = std::make_shared<SineModel>();
    
    server.register_model("linear", linear_model);
    server.register_model("sine", sine_model);
    
    server.start();
    
    // ========================================
    // Test 1: Get available models
    // ========================================
    std::cout << "\n1. Get available models:" << std::endl;
    
    Request req_models;
    req_models.set_method("GET");
    req_models.set_path("/models");
    Response res_models = server.handle_request(req_models);
    std::cout << res_models.body() << std::endl;
    
    // ========================================
    // Test 2: Get model metadata
    // ========================================
    std::cout << "\n2. Get model metadata:" << std::endl;
    
    Request req_meta_linear;
    req_meta_linear.set_method("GET");
    req_meta_linear.set_path("/models/linear/metadata");
    Response res_meta_linear = server.handle_request(req_meta_linear);
    std::cout << "Linear model: " << res_meta_linear.body() << std::endl;
    
    Request req_meta_sine;
    req_meta_sine.set_method("GET");
    req_meta_sine.set_path("/models/sine/metadata");
    Response res_meta_sine = server.handle_request(req_meta_sine);
    std::cout << "Sine model: " << res_meta_sine.body() << std::endl;
    
    // ========================================
    // Test 3: Single prediction
    // ========================================
    std::cout << "\n3. Single predictions:" << std::endl;
    
    // Linear model: y = 2 * x + 1
    Request req_pred_linear;
    req_pred_linear.set_method("POST");
    req_pred_linear.set_path("/models/linear/predict");
    req_pred_linear.set_body("{\"input\": [1.0, 2.0, 3.0, 4.0, 5.0]}");
    Response res_pred_linear = server.handle_request(req_pred_linear);
    std::cout << "Linear predictions: " << res_pred_linear.body() << std::endl;
    std::cout << "(Expected: [3.0, 5.0, 7.0, 9.0, 11.0])" << std::endl;
    
    // Sine model: y = sin(x) * 10
    Request req_pred_sine;
    req_pred_sine.set_method("POST");
    req_pred_sine.set_path("/models/sine/predict");
    req_pred_sine.set_body("{\"input\": [0.0, 1.5708, 3.1416]}");
    Response res_pred_sine = server.handle_request(req_pred_sine);
    std::cout << "Sine predictions: " << res_pred_sine.body() << std::endl;
    std::cout << "(Expected: ~[0.0, 10.0, 0.0] for 0, π/2, π)" << std::endl;
    
    // ========================================
    // Test 4: Batch prediction
    // ========================================
    std::cout << "\n4. Batch predictions:" << std::endl;
    
    Request req_batch;
    req_batch.set_method("POST");
    req_batch.set_path("/models/linear/predict_batch");
    req_batch.set_body(R"({
        "inputs": [
            [1.0, 2.0],
            [3.0, 4.0],
            [5.0, 6.0]
        ]
    })");
    Response res_batch = server.handle_request(req_batch);
    std::cout << "Batch predictions: " << res_batch.body() << std::endl;
    std::cout << "(Expected: [[3.0, 5.0], [7.0, 9.0], [11.0, 13.0]])" << std::endl;
    
    // ========================================
    // Test 5: Error handling
    // ========================================
    std::cout << "\n5. Error handling:" << std::endl;
    
    // Non-existent model
    Request req_error;
    req_error.set_method("POST");
    req_error.set_path("/models/nonexistent/predict");
    req_error.set_body("{\"input\": [1.0]}");
    Response res_error = server.handle_request(req_error);
    std::cout << "Non-existent model: " << res_error.body() << std::endl;
    
    // Invalid JSON
    Request req_invalid;
    req_invalid.set_method("POST");
    req_invalid.set_path("/models/linear/predict");
    req_invalid.set_body("invalid json");
    Response res_invalid = server.handle_request(req_invalid);
    std::cout << "Invalid JSON: " << res_invalid.body() << std::endl;
    
    server.stop();
    return 0;
}
