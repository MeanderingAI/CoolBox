#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>

namespace ml {
namespace server {

// ML Model interface
class MLModel {
public:
    virtual ~MLModel() = default;
    
    virtual std::string get_name() const = 0;
    virtual std::string get_type() const = 0;
    virtual std::vector<std::string> get_input_features() const = 0;
    virtual std::string get_description() const = 0;
    
    virtual std::vector<double> predict(const std::vector<std::vector<double>>& data) = 0;
    virtual std::map<std::string, double> get_metrics() const = 0;
};

// Dataset storage
struct Dataset {
    std::string name;
    std::vector<std::string> feature_names;
    std::vector<std::vector<double>> data;
    std::vector<double> labels;
    size_t rows() const { return data.size(); }
    size_t cols() const { return data.empty() ? 0 : data[0].size(); }
};

// Prediction result
struct PredictionResult {
    std::vector<double> predictions;
    double confidence;
    std::map<std::string, double> probabilities;
    std::string model_name;
    std::chrono::system_clock::time_point timestamp;
};

// ML Model Server
class MLModelServer {
public:
    MLModelServer(int port);
    ~MLModelServer();
    
    // Model management
    void register_model(const std::string& name, std::shared_ptr<MLModel> model);
    void unregister_model(const std::string& name);
    std::vector<std::string> list_models() const;
    std::shared_ptr<MLModel> get_model(const std::string& name);
    
    // Dataset management
    bool upload_dataset(const std::string& name, const Dataset& dataset);
    bool delete_dataset(const std::string& name);
    std::vector<std::string> list_datasets() const;
    Dataset* get_dataset(const std::string& name);
    
    // Predictions
    PredictionResult predict(const std::string& model_name, const std::vector<std::vector<double>>& data);
    PredictionResult predict_from_dataset(const std::string& model_name, const std::string& dataset_name);
    
    // Training (simplified)
    bool train_model(const std::string& model_name, const std::string& dataset_name);
    
    // Server control
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Statistics
    size_t get_total_predictions() const { return total_predictions_; }
    size_t get_total_datasets() const { return datasets_.size(); }
    size_t get_total_models() const { return models_.size(); }
    
private:
    int port_;
    bool running_;
    std::map<std::string, std::shared_ptr<MLModel>> models_;
    std::map<std::string, Dataset> datasets_;
    mutable std::mutex models_mutex_;
    mutable std::mutex datasets_mutex_;
    size_t total_predictions_;
    
    void handle_client(int client_fd);
    std::string generate_response(const std::string& endpoint, const std::string& body);
};

// Sample models
class LinearRegressionModel : public MLModel {
public:
    LinearRegressionModel();
    
    std::string get_name() const override { return "Linear Regression"; }
    std::string get_type() const override { return "Regression"; }
    std::vector<std::string> get_input_features() const override;
    std::string get_description() const override;
    
    std::vector<double> predict(const std::vector<std::vector<double>>& data) override;
    std::map<std::string, double> get_metrics() const override;
    
private:
    std::vector<double> coefficients_;
    double intercept_;
};

class LogisticRegressionModel : public MLModel {
public:
    LogisticRegressionModel();
    
    std::string get_name() const override { return "Logistic Regression"; }
    std::string get_type() const override { return "Classification"; }
    std::vector<std::string> get_input_features() const override;
    std::string get_description() const override;
    
    std::vector<double> predict(const std::vector<std::vector<double>>& data) override;
    std::map<std::string, double> get_metrics() const override;
    
private:
    std::vector<double> coefficients_;
    double intercept_;
    double sigmoid(double x) const;
};

class RandomForestModel : public MLModel {
public:
    RandomForestModel();
    
    std::string get_name() const override { return "Random Forest"; }
    std::string get_type() const override { return "Classification"; }
    std::vector<std::string> get_input_features() const override;
    std::string get_description() const override;
    
    std::vector<double> predict(const std::vector<std::vector<double>>& data) override;
    std::map<std::string, double> get_metrics() const override;
    
private:
    int n_trees_;
};

} // namespace server
} // namespace ml
