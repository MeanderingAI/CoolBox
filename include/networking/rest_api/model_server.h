#ifndef ML_REST_API_MODEL_SERVER_H
#define ML_REST_API_MODEL_SERVER_H

#include "server.h"
#include <memory>
#include <string>
#include <vector>

namespace ml {
namespace rest_api {

// ML Model serving interface
class ModelServer {
public:
    ModelServer(int port = 8080);
    ~ModelServer() = default;
    
    // Register models
    template<typename ModelType>
    void register_model(const std::string& name, std::shared_ptr<ModelType> model);
    
    // Model endpoints
    void setup_prediction_endpoint(const std::string& model_name, 
                                   std::function<std::vector<double>(const std::vector<double>&)> predict_fn);
    
    void setup_batch_prediction_endpoint(const std::string& model_name,
                                        std::function<std::vector<std::vector<double>>(const std::vector<std::vector<double>>&)> batch_predict_fn);
    
    // Health check endpoint
    void setup_health_endpoint();
    
    // Model info endpoint
    void setup_info_endpoint(const std::string& model_name,
                           const std::string& model_type,
                           const std::string& version);
    
    // Start the server
    void start();
    void stop();
    
    ml::rest_api::Server& get_server() { return server_; }
    
private:
    ml::rest_api::Server server_;
    std::map<std::string, std::string> model_info_;
};

} // namespace rest_api
} // namespace ml

#endif // ML_REST_API_MODEL_SERVER_H
