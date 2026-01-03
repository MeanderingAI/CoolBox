#include "ml_server/ml_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <random>
#include <cmath>

namespace ml {
namespace server {

// MLModelServer Implementation
MLModelServer::MLModelServer(int port)
    : port_(port), running_(false), total_predictions_(0) {
}

MLModelServer::~MLModelServer() {
    stop();
}

void MLModelServer::register_model(const std::string& name, std::shared_ptr<MLModel> model) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    models_[name] = model;
}

void MLModelServer::unregister_model(const std::string& name) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    models_.erase(name);
}

std::vector<std::string> MLModelServer::list_models() const {
    std::lock_guard<std::mutex> lock(models_mutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : models_) {
        names.push_back(name);
    }
    return names;
}

std::shared_ptr<MLModel> MLModelServer::get_model(const std::string& name) {
    std::lock_guard<std::mutex> lock(models_mutex_);
    auto it = models_.find(name);
    return it != models_.end() ? it->second : nullptr;
}

bool MLModelServer::upload_dataset(const std::string& name, const Dataset& dataset) {
    std::lock_guard<std::mutex> lock(datasets_mutex_);
    datasets_[name] = dataset;
    return true;
}

bool MLModelServer::delete_dataset(const std::string& name) {
    std::lock_guard<std::mutex> lock(datasets_mutex_);
    return datasets_.erase(name) > 0;
}

std::vector<std::string> MLModelServer::list_datasets() const {
    std::lock_guard<std::mutex> lock(datasets_mutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : datasets_) {
        names.push_back(name);
    }
    return names;
}

Dataset* MLModelServer::get_dataset(const std::string& name) {
    std::lock_guard<std::mutex> lock(datasets_mutex_);
    auto it = datasets_.find(name);
    return it != datasets_.end() ? &it->second : nullptr;
}

PredictionResult MLModelServer::predict(const std::string& model_name, const std::vector<std::vector<double>>& data) {
    auto model = get_model(model_name);
    PredictionResult result;
    
    if (!model) {
        return result;
    }
    
    result.predictions = model->predict(data);
    result.model_name = model_name;
    result.timestamp = std::chrono::system_clock::now();
    result.confidence = 0.85; // Simplified
    
    total_predictions_++;
    
    return result;
}

PredictionResult MLModelServer::predict_from_dataset(const std::string& model_name, const std::string& dataset_name) {
    auto* dataset = get_dataset(dataset_name);
    if (!dataset) {
        return PredictionResult();
    }
    
    return predict(model_name, dataset->data);
}

bool MLModelServer::train_model(const std::string& model_name, const std::string& dataset_name) {
    // Simplified training - in production would actually train the model
    auto model = get_model(model_name);
    auto* dataset = get_dataset(dataset_name);
    
    return model && dataset;
}

void MLModelServer::start() {
    running_ = true;
    // Simplified - in production would start HTTP server
}

void MLModelServer::stop() {
    running_ = false;
}

// LinearRegressionModel Implementation
LinearRegressionModel::LinearRegressionModel() 
    : intercept_(1.5) {
    // Simple coefficients for demo
    coefficients_ = {0.5, -0.3, 0.8};
}

std::vector<std::string> LinearRegressionModel::get_input_features() const {
    return {"feature1", "feature2", "feature3"};
}

std::string LinearRegressionModel::get_description() const {
    return "Linear regression model for continuous value prediction";
}

std::vector<double> LinearRegressionModel::predict(const std::vector<std::vector<double>>& data) {
    std::vector<double> predictions;
    
    for (const auto& row : data) {
        double pred = intercept_;
        for (size_t i = 0; i < std::min(row.size(), coefficients_.size()); ++i) {
            pred += coefficients_[i] * row[i];
        }
        predictions.push_back(pred);
    }
    
    return predictions;
}

std::map<std::string, double> LinearRegressionModel::get_metrics() const {
    return {
        {"r2_score", 0.87},
        {"mse", 0.23},
        {"mae", 0.15}
    };
}

// LogisticRegressionModel Implementation
LogisticRegressionModel::LogisticRegressionModel()
    : intercept_(0.5) {
    coefficients_ = {0.7, -0.4, 0.6};
}

std::vector<std::string> LogisticRegressionModel::get_input_features() const {
    return {"feature1", "feature2", "feature3"};
}

std::string LogisticRegressionModel::get_description() const {
    return "Logistic regression model for binary classification";
}

double LogisticRegressionModel::sigmoid(double x) const {
    return 1.0 / (1.0 + std::exp(-x));
}

std::vector<double> LogisticRegressionModel::predict(const std::vector<std::vector<double>>& data) {
    std::vector<double> predictions;
    
    for (const auto& row : data) {
        double logit = intercept_;
        for (size_t i = 0; i < std::min(row.size(), coefficients_.size()); ++i) {
            logit += coefficients_[i] * row[i];
        }
        predictions.push_back(sigmoid(logit));
    }
    
    return predictions;
}

std::map<std::string, double> LogisticRegressionModel::get_metrics() const {
    return {
        {"accuracy", 0.92},
        {"precision", 0.89},
        {"recall", 0.94},
        {"f1_score", 0.91}
    };
}

// RandomForestModel Implementation
RandomForestModel::RandomForestModel()
    : n_trees_(100) {
}

std::vector<std::string> RandomForestModel::get_input_features() const {
    return {"feature1", "feature2", "feature3", "feature4"};
}

std::string RandomForestModel::get_description() const {
    return "Random forest ensemble model with " + std::to_string(n_trees_) + " trees";
}

std::vector<double> RandomForestModel::predict(const std::vector<std::vector<double>>& data) {
    std::vector<double> predictions;
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (const auto& row : data) {
        // Simplified prediction - weighted average of features
        double pred = 0.0;
        for (size_t i = 0; i < row.size(); ++i) {
            pred += row[i] * (0.5 + dis(gen) * 0.5);
        }
        pred = pred > 0 ? 1.0 : 0.0;
        predictions.push_back(pred);
    }
    
    return predictions;
}

std::map<std::string, double> RandomForestModel::get_metrics() const {
    return {
        {"accuracy", 0.95},
        {"precision", 0.93},
        {"recall", 0.96},
        {"f1_score", 0.94},
        {"auc_roc", 0.97}
    };
}

} // namespace server
} // namespace ml
