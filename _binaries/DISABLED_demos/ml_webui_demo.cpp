#include "networking/document/web_components.h"
#include "ml_server/ml_server.h"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <random>

using namespace ml::networking::html;
using namespace ml::server;

class MLWebUI {
public:
    MLWebUI(int http_port, MLModelServer* ml_server) 
        : http_port_(http_port), ml_server_(ml_server), running_(false) {}
    
    void start() {
        running_ = true;
        
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(http_port_);
        
        bind(server_fd_, (struct sockaddr*)&address, sizeof(address));
        listen(server_fd_, 10);
        
        std::cout << "✓ ML Web UI running on http://localhost:" << http_port_ << "\n\n";
        
        while (running_) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd < 0) continue;
            
            handle_request(client_fd);
            close(client_fd);
        }
    }
    
private:
    int http_port_;
    int server_fd_;
    bool running_;
    MLModelServer* ml_server_;
    
    void handle_request(int client_fd) {
        char buffer[4096] = {0};
        read(client_fd, buffer, sizeof(buffer) - 1);
        
        std::string request(buffer);
        size_t path_start = request.find(" ") + 1;
        size_t path_end = request.find(" ", path_start);
        std::string path = request.substr(path_start, path_end - path_start);
        
        size_t query_pos = path.find('?');
        if (query_pos != std::string::npos) {
            path = path.substr(0, query_pos);
        }
        
        std::string response;
        if (path == "/" || path == "/dashboard") {
            response = generate_dashboard();
        } else if (path == "/models") {
            response = generate_models_page();
        } else if (path == "/datasets") {
            response = generate_datasets_page();
        } else if (path == "/predict") {
            response = generate_prediction_page();
        } else {
            response = generate_dashboard();
        }
        
        std::string http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: " + std::to_string(response.length()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + response;
        
        write(client_fd, http_response.c_str(), http_response.length());
    }
    
    std::string generate_dashboard() {
        ComponentBundler bundler;
        
        std::stringstream stats;
        stats << R"(
            <div class="stats-grid">
                <app-card>
                    <div slot="header">📊 Total Models</div>
                    <div class="stat-value">)" << ml_server_->get_total_models() << R"(</div>
                    <div class="stat-label">Registered Models</div>
                </app-card>
                <app-card>
                    <div slot="header">💾 Datasets</div>
                    <div class="stat-value">)" << ml_server_->get_total_datasets() << R"(</div>
                    <div class="stat-label">Uploaded Datasets</div>
                </app-card>
                <app-card>
                    <div slot="header">🎯 Predictions</div>
                    <div class="stat-value">)" << ml_server_->get_total_predictions() << R"(</div>
                    <div class="stat-label">Total Predictions Made</div>
                </app-card>
            </div>
        )";
        
        return bundler
            .set_title("ML Model Server - Dashboard")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body {
                    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
                    background: #f5f7fa;
                }
                .container {
                    max-width: 1400px;
                    margin: 0 auto;
                    padding: 2rem;
                }
                .stats-grid {
                    display: grid;
                    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
                    gap: 2rem;
                    margin: 2rem 0;
                }
                .stat-value {
                    font-size: 3rem;
                    font-weight: bold;
                    color: #667eea;
                    text-align: center;
                    margin: 1rem 0;
                }
                .stat-label {
                    text-align: center;
                    color: #666;
                }
                .hero {
                    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                    color: white;
                    padding: 3rem;
                    border-radius: 12px;
                    text-align: center;
                    margin-bottom: 2rem;
                }
                .hero h1 {
                    color: white;
                    font-size: 2.5rem;
                    margin-bottom: 1rem;
                }
                .grid-2 {
                    display: grid;
                    grid-template-columns: repeat(2, 1fr);
                    gap: 2rem;
                    margin: 2rem 0;
                }
            )")
            .set_body_content(R"(
                <app-header>
                    <span slot="logo">🤖 ML Model Server</span>
                    <nav-menu slot="nav">
                        <a href="/dashboard">Dashboard</a>
                        <a href="/models">Models</a>
                        <a href="/datasets">Datasets</a>
                        <a href="/predict">Predict</a>
                    </nav-menu>
                </app-header>

                <div class="container">
                    <div class="hero">
                        <h1>Welcome to ML Model Server</h1>
                        <p>Manage, deploy, and serve machine learning models with ease</p>
                    </div>

                    )" + stats.str() + R"(

                    <div class="grid-2">
                        <app-card>
                            <h3 slot="header">📈 Recent Activity</h3>
                            <p>✓ Model 'Random Forest' registered</p>
                            <p>✓ Dataset 'iris.csv' uploaded (150 samples)</p>
                            <p>✓ 23 predictions made in last hour</p>
                            <p>✓ Model 'Linear Regression' trained</p>
                        </app-card>

                        <app-card>
                            <h3 slot="header">⚡ Quick Actions</h3>
                            <app-button>Upload Dataset</app-button>
                            <br><br>
                            <app-button>Register Model</app-button>
                            <br><br>
                            <app-button>Make Prediction</app-button>
                        </app-card>
                    </div>
                </div>

                <app-footer>
                    <div slot="center">
                        <p>ML Model Server - Powered by ToolBox C++ Framework</p>
                    </div>
                    <div slot="copyright">© 2025 ToolBox ML Server</div>
                </app-footer>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .add_component_from_registry("app-card")
            .add_component_from_registry("app-button")
            .add_component_from_registry("app-footer")
            .minify(true)
            .bundle();
    }
    
    std::string generate_models_page() {
        ComponentBundler bundler;
        
        std::stringstream models_html;
        auto model_names = ml_server_->list_models();
        
        for (const auto& name : model_names) {
            auto model = ml_server_->get_model(name);
            if (model) {
                auto metrics = model->get_metrics();
                std::stringstream metrics_str;
                for (const auto& [key, value] : metrics) {
                    metrics_str << "<p><strong>" << key << ":</strong> " 
                               << std::fixed << std::setprecision(3) << value << "</p>";
                }
                
                models_html << R"(
                    <app-card>
                        <h3 slot="header">)" << model->get_name() << R"(</h3>
                        <p><strong>Type:</strong> )" << model->get_type() << R"(</p>
                        <p><strong>Description:</strong> )" << model->get_description() << R"(</p>
                        <br>
                        <h4>Metrics:</h4>
                        )" << metrics_str.str() << R"(
                        <div slot="footer">
                            <app-button>Use Model</app-button>
                        </div>
                    </app-card>
                )";
            }
        }
        
        return bundler
            .set_title("ML Models")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #f5f7fa; }
                .container { max-width: 1400px; margin: 0 auto; padding: 2rem; }
                .models-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(350px, 1fr)); gap: 2rem; margin: 2rem 0; }
                h1 { color: #2c3e50; margin-bottom: 1rem; }
                h4 { margin-top: 1rem; color: #667eea; }
            )")
            .set_body_content(R"(
                <app-header>
                    <span slot="logo">🤖 ML Model Server</span>
                    <nav-menu slot="nav">
                        <a href="/dashboard">Dashboard</a>
                        <a href="/models">Models</a>
                        <a href="/datasets">Datasets</a>
                        <a href="/predict">Predict</a>
                    </nav-menu>
                </app-header>

                <div class="container">
                    <h1>Registered Models</h1>
                    <p>Manage and deploy machine learning models</p>

                    <div class="models-grid">
                        )" + models_html.str() + R"(
                    </div>
                </div>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .add_component_from_registry("app-card")
            .add_component_from_registry("app-button")
            .minify(true)
            .bundle();
    }
    
    std::string generate_datasets_page() {
        ComponentBundler bundler;
        
        std::stringstream datasets_html;
        auto dataset_names = ml_server_->list_datasets();
        
        for (const auto& name : dataset_names) {
            auto* dataset = ml_server_->get_dataset(name);
            if (dataset) {
                datasets_html << R"(
                    <app-card>
                        <h3 slot="header">📊 )" << name << R"(</h3>
                        <p><strong>Samples:</strong> )" << dataset->rows() << R"(</p>
                        <p><strong>Features:</strong> )" << dataset->cols() << R"(</p>
                        <p><strong>Has Labels:</strong> )" << (dataset->labels.empty() ? "No" : "Yes") << R"(</p>
                        <div slot="footer">
                            <app-button>View Data</app-button>
                            <app-button>Delete</app-button>
                        </div>
                    </app-card>
                )";
            }
        }
        
        return bundler
            .set_title("Datasets")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #f5f7fa; }
                .container { max-width: 1400px; margin: 0 auto; padding: 2rem; }
                .datasets-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 2rem; margin: 2rem 0; }
                h1 { color: #2c3e50; margin-bottom: 1rem; }
            )")
            .set_body_content(R"(
                <app-header>
                    <span slot="logo">🤖 ML Model Server</span>
                    <nav-menu slot="nav">
                        <a href="/dashboard">Dashboard</a>
                        <a href="/models">Models</a>
                        <a href="/datasets">Datasets</a>
                        <a href="/predict">Predict</a>
                    </nav-menu>
                </app-header>

                <div class="container">
                    <h1>Datasets</h1>
                    <p>Upload and manage your training datasets</p>

                    <app-card>
                        <h3 slot="header">Upload New Dataset</h3>
                        <form-input label="Dataset Name" placeholder="e.g., iris.csv"></form-input>
                        <form-input label="File" type="file"></form-input>
                        <div slot="footer">
                            <app-button>Upload Dataset</app-button>
                        </div>
                    </app-card>

                    <h2 style="margin-top: 2rem;">Existing Datasets</h2>
                    <div class="datasets-grid">
                        )" + datasets_html.str() + R"(
                    </div>
                </div>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .add_component_from_registry("app-card")
            .add_component_from_registry("app-button")
            .add_component_from_registry("form-input")
            .minify(true)
            .bundle();
    }
    
    std::string generate_prediction_page() {
        ComponentBundler bundler;
        
        std::stringstream models_options;
        for (const auto& name : ml_server_->list_models()) {
            models_options << "<option value='" << name << "'>" << name << "</option>";
        }
        
        std::stringstream datasets_options;
        for (const auto& name : ml_server_->list_datasets()) {
            datasets_options << "<option value='" << name << "'>" << name << "</option>";
        }
        
        return bundler
            .set_title("Make Predictions")
            .add_global_style(R"(
                * { margin: 0; padding: 0; box-sizing: border-box; }
                body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; background: #f5f7fa; }
                .container { max-width: 1200px; margin: 0 auto; padding: 2rem; }
                .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 2rem; margin: 2rem 0; }
                h1, h2 { color: #2c3e50; margin-bottom: 1rem; }
                select { width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; font-size: 1rem; }
                .results { background: #f9f9f9; padding: 1rem; border-radius: 4px; margin-top: 1rem; }
            )")
            .set_body_content(R"(
                <app-header>
                    <span slot="logo">🤖 ML Model Server</span>
                    <nav-menu slot="nav">
                        <a href="/dashboard">Dashboard</a>
                        <a href="/models">Models</a>
                        <a href="/datasets">Datasets</a>
                        <a href="/predict">Predict</a>
                    </nav-menu>
                </app-header>

                <div class="container">
                    <h1>Make Predictions</h1>
                    <p>Run inference on your machine learning models</p>

                    <div class="grid-2">
                        <app-card>
                            <h3 slot="header">Configure Prediction</h3>
                            <label><strong>Select Model</strong></label>
                            <select id="model-select">
                                <option value="">Choose a model...</option>
                                )" + models_options.str() + R"(
                            </select>
                            <br><br>
                            <label><strong>Select Dataset</strong></label>
                            <select id="dataset-select">
                                <option value="">Choose a dataset...</option>
                                )" + datasets_options.str() + R"(
                            </select>
                            <br><br>
                            <label><strong>Or Enter Data Manually</strong></label>
                            <form-input label="Feature 1" placeholder="0.5"></form-input>
                            <form-input label="Feature 2" placeholder="1.2"></form-input>
                            <form-input label="Feature 3" placeholder="-0.3"></form-input>
                            <div slot="footer">
                                <app-button>Run Prediction</app-button>
                            </div>
                        </app-card>

                        <app-card>
                            <h3 slot="header">Prediction Results</h3>
                            <div class="results">
                                <p><strong>Status:</strong> Ready</p>
                                <p><em>Configure and run a prediction to see results here</em></p>
                            </div>
                            <br>
                            <h4>Sample Results:</h4>
                            <progress-bar value="85" max="100"></progress-bar>
                            <p>Confidence: 85%</p>
                            <br>
                            <data-table>
                                <tr slot="header">
                                    <th>Sample</th>
                                    <th>Prediction</th>
                                    <th>Probability</th>
                                </tr>
                                <tr><td>1</td><td>Class A</td><td>0.89</td></tr>
                                <tr><td>2</td><td>Class B</td><td>0.92</td></tr>
                                <tr><td>3</td><td>Class A</td><td>0.76</td></tr>
                            </data-table>
                        </app-card>
                    </div>

                    <app-accordion>
                        <div slot="header">📊 Model Performance Metrics</div>
                        <div>
                            <h4>Accuracy Metrics:</h4>
                            <progress-bar value="92" max="100"></progress-bar>
                            <p>Accuracy: 92%</p>
                            <br>
                            <progress-bar value="89" max="100"></progress-bar>
                            <p>Precision: 89%</p>
                            <br>
                            <progress-bar value="94" max="100"></progress-bar>
                            <p>Recall: 94%</p>
                        </div>
                    </app-accordion>
                </div>

                <app-footer>
                    <div slot="copyright">© 2025 ToolBox ML Server</div>
                </app-footer>
            )")
            .add_component_from_registry("app-header")
            .add_component_from_registry("nav-menu")
            .add_component_from_registry("app-card")
            .add_component_from_registry("app-button")
            .add_component_from_registry("form-input")
            .add_component_from_registry("progress-bar")
            .add_component_from_registry("data-table")
            .add_component_from_registry("app-accordion")
            .add_component_from_registry("app-footer")
            .add_global_script(R"(
                document.addEventListener('DOMContentLoaded', () => {
                    // Animate progress bars
                    document.querySelectorAll('progress-bar').forEach(bar => {
                        const fill = bar.shadowRoot.querySelector('.progress-fill');
                        const text = bar.shadowRoot.querySelector('.progress-text');
                        const value = bar.getAttribute('value') || 0;
                        fill.style.width = value + '%';
                        text.textContent = value + '%';
                    });
                    
                    // Add accordion handlers
                    document.querySelectorAll('app-accordion').forEach(acc => {
                        const header = acc.shadowRoot.querySelector('.accordion-header');
                        header.addEventListener('click', () => {
                            acc.classList.toggle('open');
                        });
                    });
                    
                    // Add button handlers
                    document.querySelectorAll('app-button').forEach(btn => {
                        btn.addEventListener('click', () => {
                            console.log('Button clicked:', btn.textContent);
                            alert('Prediction initiated! Results would appear in real deployment.');
                        });
                    });
                });
            )")
            .minify(true)
            .bundle();
    }
};

int main() {
    std::cout << "=== ML Model Server Web UI Demo ===\n\n";
    
    // Register web components
    ComponentRegistry& registry = ComponentRegistry::instance();
    registry.register_component(components::create_app_header());
    registry.register_component(components::create_nav_menu());
    registry.register_component(components::create_card());
    registry.register_component(components::create_button());
    registry.register_component(components::create_form_input());
    registry.register_component(components::create_progress_bar());
    registry.register_component(components::create_data_table());
    registry.register_component(components::create_accordion());
    registry.register_component(components::create_footer());
    
    // Create ML server
    MLModelServer ml_server(8082);
    ml_server.start();
    
    // Register sample models
    std::cout << "✓ Registering ML models...\n";
    ml_server.register_model("linear_regression", std::make_shared<LinearRegressionModel>());
    ml_server.register_model("logistic_regression", std::make_shared<LogisticRegressionModel>());
    ml_server.register_model("random_forest", std::make_shared<RandomForestModel>());
    
    // Create sample datasets
    std::cout << "✓ Creating sample datasets...\n";
    Dataset iris;
    iris.name = "iris.csv";
    iris.feature_names = {"sepal_length", "sepal_width", "petal_length", "petal_width"};
    iris.data = {{5.1, 3.5, 1.4, 0.2}, {4.9, 3.0, 1.4, 0.2}, {6.7, 3.1, 4.4, 1.4}};
    iris.labels = {0, 0, 1};
    ml_server.upload_dataset("iris.csv", iris);
    
    Dataset boston;
    boston.name = "boston_housing.csv";
    boston.feature_names = {"rooms", "age", "distance"};
    boston.data = {{6.0, 65.0, 4.0}, {7.5, 45.0, 3.5}, {5.5, 80.0, 6.0}};
    boston.labels = {250.0, 350.0, 180.0};
    ml_server.upload_dataset("boston_housing.csv", boston);
    
    // Run some predictions
    std::cout << "✓ Running sample predictions...\n";
    std::vector<std::vector<double>> test_data = {{5.0, 3.0, 1.5, 0.3}};
    ml_server.predict("random_forest", test_data);
    ml_server.predict("logistic_regression", test_data);
    
    std::cout << "\n✓ ML Server Status:\n";
    std::cout << "  Models: " << ml_server.get_total_models() << "\n";
    std::cout << "  Datasets: " << ml_server.get_total_datasets() << "\n";
    std::cout << "  Predictions: " << ml_server.get_total_predictions() << "\n";
    
    // Create web UI
    std::cout << "\n✓ Starting Web UI on http://localhost:8082\n";
    MLWebUI web_ui(8082, &ml_server);
    
    std::cout << "\nAvailable Pages:\n";
    std::cout << "  - http://localhost:8082/dashboard (Overview)\n";
    std::cout << "  - http://localhost:8082/models (Model Management)\n";
    std::cout << "  - http://localhost:8082/datasets (Data Management)\n";
    std::cout << "  - http://localhost:8082/predict (Run Predictions)\n";
    std::cout << "\nOpen in your browser!\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    web_ui.start();
    
    return 0;
}
