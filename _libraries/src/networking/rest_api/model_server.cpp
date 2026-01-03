#include "networking/rest_api/model_server.h"
#include "networking/rest_api/server.h"
#include <sstream>

namespace networking {
namespace rest_api {

ModelServer::ModelServer(int port) : server_(port) {
    setup_health_endpoint();
}

void ModelServer::setup_prediction_endpoint(const std::string& model_name,
                                           std::function<std::vector<double>(const std::vector<double>&)> predict_fn) {
    std::string endpoint = "/api/v1/models/" + model_name + "/predict";
    
    server_.post(endpoint, [predict_fn](const nhh::Request& req) {
        nhh::Response resp;
        
        try {
            // Parse input from JSON body
            std::string body = req.body();
            
            // Simple parsing - expects {"features": [1.0, 2.0, 3.0]}
            // Extract numbers between brackets
            size_t start = body.find('[');
            size_t end = body.find(']');
            
            if (start == std::string::npos || end == std::string::npos) {
                resp.set_status(nhh::HttpStatus::BAD_REQUEST);
                resp.set_json("{\"error\": \"Invalid input format\"}");
                return resp;
            }
            
            std::string features_str = body.substr(start + 1, end - start - 1);
            std::vector<double> features;
            std::istringstream iss(features_str);
            std::string token;
            
            while (std::getline(iss, token, ',')) {
                features.push_back(std::stod(token));
            }
            
            // Make prediction
            std::vector<double> prediction = predict_fn(features);
            
            // Format response
            std::ostringstream json;
            json << "{\"prediction\": [";
            for (size_t i = 0; i < prediction.size(); ++i) {
                if (i > 0) json << ", ";
                json << prediction[i];
            }
            json << "]}";
            
            resp.set_status(nhh::HttpStatus::OK);
            resp.set_json(json.str());
            
        } catch (const std::exception& e) {
            resp.set_status(nhh::HttpStatus::INTERNAL_SERVER_ERROR);
            resp.set_json("{\"error\": \"" + std::string(e.what()) + "\"}");
        }
        
        return resp;
    });
}

void ModelServer::setup_batch_prediction_endpoint(const std::string& model_name,
                                                 std::function<std::vector<std::vector<double>>(const std::vector<std::vector<double>>&)> batch_predict_fn) {
    std::string endpoint = "/api/v1/models/" + model_name + "/batch_predict";
    
    server_.post(endpoint, [batch_predict_fn](const nhh::Request& req) {
        nhh::Response resp;
        
        try {
            // Parse batch input
            std::string body = req.body();
            
            // Simple batch parsing - expects {"batch": [[1,2], [3,4]]}
            std::vector<std::vector<double>> batch;
            
            // Extract outer brackets
            size_t outer_start = body.find("[[");
            size_t outer_end = body.rfind("]]");
            
            if (outer_start == std::string::npos || outer_end == std::string::npos) {
                resp.set_status(nhh::HttpStatus::BAD_REQUEST);
                resp.set_json("{\"error\": \"Invalid batch format\"}");
                return resp;
            }
            
            std::string batch_str = body.substr(outer_start + 2, outer_end - outer_start - 2);
            
            // Parse each row
            std::istringstream iss(batch_str);
            std::string row_str;
            
            while (std::getline(iss, row_str, ']')) {
                // Remove leading characters
                size_t start = row_str.find('[');
                if (start == std::string::npos) continue;
                
                std::string features_str = row_str.substr(start + 1);
                std::vector<double> features;
                std::istringstream row_iss(features_str);
                std::string val;
                
                while (std::getline(row_iss, val, ',')) {
                    try {
                        features.push_back(std::stod(val));
                    } catch (...) {}
                }
                
                if (!features.empty()) {
                    batch.push_back(features);
                }
            }
            
            // Make batch prediction
            auto predictions = batch_predict_fn(batch);
            
            // Format response
            std::ostringstream json;
            json << "{\"predictions\": [";
            for (size_t i = 0; i < predictions.size(); ++i) {
                if (i > 0) json << ", ";
                json << "[";
                for (size_t j = 0; j < predictions[i].size(); ++j) {
                    if (j > 0) json << ", ";
                    json << predictions[i][j];
                }
                json << "]";
            }
            json << "]}";
            
            resp.set_status(nhh::HttpStatus::OK);
            resp.set_json(json.str());
            
        } catch (const std::exception& e) {
            resp.set_status(nhh::HttpStatus::INTERNAL_SERVER_ERROR);
            resp.set_json("{\"error\": \"" + std::string(e.what()) + "\"}");
        }
        
        return resp;
    });
}

void ModelServer::setup_health_endpoint() {
    server_.get("/health", [](const nhh::Request& req) {
        nhh::Response resp;
        resp.set_status(nhh::HttpStatus::OK);
        resp.set_json("{\"status\": \"healthy\"}");
        return resp;
    });
}

void ModelServer::setup_info_endpoint(const std::string& model_name,
                                     const std::string& model_type,
                                     const std::string& version) {
    model_info_[model_name] = model_type;
    
    std::string endpoint = "/api/v1/models/" + model_name + "/info";
    
    server_.get(endpoint, [model_name, model_type, version](const nhh::Request& req) {
        nhh::Response resp;
        
        std::ostringstream json;
        json << "{"
             << "\"name\": \"" << model_name << "\", "
             << "\"type\": \"" << model_type << "\", "
             << "\"version\": \"" << version << "\", "
             << "\"status\": \"ready\""
             << "}";
        
        resp.set_status(nhh::HttpStatus::OK);
        resp.set_json(json.str());
        return resp;
    });
}

void ModelServer::start() {
    server_.start();
}

void ModelServer::stop() {
    server_.stop();
}

} // namespace rest_api
} // namespace ml
