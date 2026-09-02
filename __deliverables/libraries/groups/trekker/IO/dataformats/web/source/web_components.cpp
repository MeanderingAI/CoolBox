#include "IO/dataformats/web/web_components.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <iomanip>

namespace ml {
namespace networking {
namespace html {

// WebComponent Implementation
std::string WebComponent::render() const {
    return template_html;
}

std::string WebComponent::to_custom_element() const {
    std::stringstream ss;
    
    ss << "class " << name << " extends HTMLElement {\n";
    ss << "  constructor() {\n";
    ss << "    super();\n";
    ss << "    const shadow = this.attachShadow({ mode: 'open' });\n";
    
    // Add styles
    if (!styles.empty()) {
        ss << "    const style = document.createElement('style');\n";
        ss << "    style.textContent = `" << styles << "`;\n";
        ss << "    shadow.appendChild(style);\n";
    }
    
    // Add template
    ss << "    const template = document.createElement('template');\n";
    ss << "    template.innerHTML = `" << template_html << "`;\n";
    ss << "    shadow.appendChild(template.content.cloneNode(true));\n";
    
    // Add attributes as properties
    for (const auto& [name, default_val] : attributes) {
        ss << "    this._" << name << " = '" << default_val << "';\n";
    }
    
    ss << "  }\n\n";
    
    // Add attribute getters/setters
    for (const auto& [name, default_val] : attributes) {
        ss << "  get " << name << "() { return this._" << name << "; }\n";
        ss << "  set " << name << "(val) {\n";
        ss << "    this._" << name << " = val;\n";
        ss << "    this.setAttribute('" << name << "', val);\n";
        ss << "  }\n\n";
    }
    
    // Add observed attributes
    if (!attributes.empty()) {
        ss << "  static get observedAttributes() {\n";
        ss << "    return [";
        bool first = true;
        for (const auto& [name, _] : attributes) {
            if (!first) ss << ", ";
            ss << "'" << name << "'";
            first = false;
        }
        ss << "];\n";
        ss << "  }\n\n";
        
        ss << "  attributeChangedCallback(name, oldValue, newValue) {\n";
        ss << "    this['_' + name] = newValue;\n";
        ss << "    this.render();\n";
        ss << "  }\n\n";
    }
    
    // Add custom script
    if (!script.empty()) {
        ss << "  " << script << "\n";
    }
    
    // Add render method
    ss << "  render() {\n";
    ss << "    // Update shadow DOM based on current state\n";
    ss << "  }\n";
    
    ss << "}\n\n";
    ss << "customElements.define('" << name << "', " << name << ");\n";
    
    return ss.str();
}

// WebComponentBuilder Implementation
WebComponentBuilder::WebComponentBuilder(const std::string& name) {
    component_.name = name;
}

WebComponentBuilder& WebComponentBuilder::template_html(const std::string& html) {
    component_.template_html = html;
    return *this;
}

WebComponentBuilder& WebComponentBuilder::style(const std::string& css) {
    component_.styles = css;
    return *this;
}

WebComponentBuilder& WebComponentBuilder::script(const std::string& js) {
    component_.script = js;
    return *this;
}

WebComponentBuilder& WebComponentBuilder::attribute(const std::string& name, const std::string& default_value) {
    component_.attributes[name] = default_value;
    return *this;
}

WebComponentBuilder& WebComponentBuilder::slot(const std::string& name) {
    component_.slots.push_back(name);
    return *this;
}

WebComponent WebComponentBuilder::build() {
    return component_;
}

// ComponentRegistry Implementation
ComponentRegistry& ComponentRegistry::instance() {
    static ComponentRegistry registry;
    return registry;
}

void ComponentRegistry::register_component(const WebComponent& component) {
    components_[component.name] = component;
}

WebComponent* ComponentRegistry::get_component(const std::string& name) {
    auto it = components_.find(name);
    return it != components_.end() ? &it->second : nullptr;
}

std::vector<std::string> ComponentRegistry::list_components() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : components_) {
        names.push_back(name);
    }
    return names;
}

void ComponentRegistry::clear() {
    components_.clear();
}

// ComponentBundler Implementation
ComponentBundler::ComponentBundler() 
    : title_("Web Application")
    , minify_(false)
    , inline_everything_(true)
    , add_polyfills_(true) {
}

ComponentBundler& ComponentBundler::add_component(const WebComponent& component) {
    components_.push_back(component);
    return *this;
}

ComponentBundler& ComponentBundler::add_component_from_registry(const std::string& name) {
    auto* comp = ComponentRegistry::instance().get_component(name);
    if (comp) {
        components_.push_back(*comp);
    }
    return *this;
}

ComponentBundler& ComponentBundler::add_global_style(const std::string& css) {
    global_styles_.push_back(css);
    return *this;
}

ComponentBundler& ComponentBundler::add_global_script(const std::string& js) {
    global_scripts_.push_back(js);
    return *this;
}

ComponentBundler& ComponentBundler::set_title(const std::string& title) {
    title_ = title;
    return *this;
}

ComponentBundler& ComponentBundler::set_meta(const std::string& name, const std::string& content) {
    meta_tags_[name] = content;
    return *this;
}

ComponentBundler& ComponentBundler::set_favicon(const std::string& href) {
    favicon_ = href;
    return *this;
}

ComponentBundler& ComponentBundler::set_body_content(const std::string& html) {
    body_content_ = html;
    return *this;
}

ComponentBundler& ComponentBundler::minify(bool enable) {
    minify_ = enable;
    return *this;
}

ComponentBundler& ComponentBundler::inline_everything(bool enable) {
    inline_everything_ = enable;
    return *this;
}

ComponentBundler& ComponentBundler::add_polyfills(bool enable) {
    add_polyfills_ = enable;
    return *this;
}

std::string ComponentBundler::bundle() const {
    std::stringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"en\">\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    
    // Meta tags
    for (const auto& [name, content] : meta_tags_) {
        html << "  <meta name=\"" << name << "\" content=\"" << content << "\">\n";
    }
    
    // Title
    html << "  <title>" << title_ << "</title>\n";
    
    // Favicon
    if (!favicon_.empty()) {
        html << "  <link rel=\"icon\" href=\"" << favicon_ << "\">\n";
    }
    
    // Global styles
    if (!global_styles_.empty()) {
        html << "  <style>\n";
        for (const auto& css : global_styles_) {
            html << (minify_ ? minify_css(css) : css) << "\n";
        }
        html << "  </style>\n";
    }
    
    html << "</head>\n";
    html << "<body>\n";
    
    // Body content
    if (!body_content_.empty()) {
        html << "  " << (minify_ ? minify_html(body_content_) : body_content_) << "\n";
    }
    
    // Web Components polyfill
    if (add_polyfills_) {
        html << "  <script>\n" << get_polyfills() << "</script>\n";
    }
    
    // Component definitions
    if (!components_.empty()) {
        html << "  <script>\n";
        for (const auto& component : components_) {
            html << "    // Component: " << component.name << "\n";
            html << "    " << (minify_ ? minify_js(component.to_custom_element()) : component.to_custom_element()) << "\n\n";
        }
        html << "  </script>\n";
    }
    
    // Global scripts
    if (!global_scripts_.empty()) {
        html << "  <script>\n";
        for (const auto& js : global_scripts_) {
            html << (minify_ ? minify_js(js) : js) << "\n";
        }
        html << "  </script>\n";
    }
    
    html << "</body>\n";
    html << "</html>";
    
    return html.str();
}

bool ComponentBundler::save_to_file(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file) return false;
    
    file << bundle();
    return true;
}

std::string ComponentBundler::minify_css(const std::string& css) const {
    std::string result = css;
    // Remove comments
    size_t pos = 0;
    while ((pos = result.find("/*", pos)) != std::string::npos) {
        size_t end = result.find("*/", pos);
        if (end != std::string::npos) {
            result.erase(pos, end - pos + 2);
        } else {
            break;
        }
    }
    // Remove extra whitespace
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    return result;
}

std::string ComponentBundler::minify_js(const std::string& js) const {
    std::string result = js;
    // Basic minification - remove comments and extra whitespace
    // Remove single-line comments
    size_t pos = 0;
    while ((pos = result.find("//", pos)) != std::string::npos) {
        size_t end = result.find("\n", pos);
        if (end != std::string::npos) {
            result.erase(pos, end - pos);
        } else {
            break;
        }
    }
    return result;
}

std::string ComponentBundler::minify_html(const std::string& html) const {
    std::string result = html;
    // Remove extra whitespace between tags
    size_t pos = 0;
    while ((pos = result.find(">  <", pos)) != std::string::npos) {
        result.replace(pos, 4, "><");
    }
    return result;
}

std::string ComponentBundler::get_polyfills() const {
    return R"(
    // Web Components polyfill check
    if (!window.customElements) {
      console.warn('Web Components not supported in this browser');
    }
    )";
}

// Pre-built Components
namespace components {

WebComponent create_app_header() {
    return WebComponentBuilder("app-header")
        .template_html(R"(
            <header>
                <div class="logo">
                    <slot name="logo">App Logo</slot>
                </div>
                <nav>
                    <slot name="nav"></slot>
                </nav>
                <div class="actions">
                    <slot name="actions"></slot>
                </div>
            </header>
        )")
        .style(R"(
            header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                padding: 1rem 2rem;
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                color: white;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            }
            .logo {
                font-size: 1.5rem;
                font-weight: bold;
            }
            nav {
                display: flex;
                gap: 1rem;
            }
            .actions {
                display: flex;
                gap: 0.5rem;
            }
        )")
        .attribute("theme", "default")
        .build();
}

WebComponent create_nav_menu() {
    return WebComponentBuilder("nav-menu")
        .template_html(R"(
            <nav>
                <slot></slot>
            </nav>
        )")
        .style(R"(
            nav {
                display: flex;
                gap: 1rem;
            }
            ::slotted(a) {
                color: white;
                text-decoration: none;
                padding: 0.5rem 1rem;
                border-radius: 4px;
                transition: background 0.3s;
            }
            ::slotted(a:hover) {
                background: rgba(255,255,255,0.2);
            }
        )")
        .build();
}

WebComponent create_card() {
    return WebComponentBuilder("app-card")
        .template_html(R"(
            <div class="card">
                <div class="card-header">
                    <slot name="header"></slot>
                </div>
                <div class="card-body">
                    <slot></slot>
                </div>
                <div class="card-footer">
                    <slot name="footer"></slot>
                </div>
            </div>
        )")
        .style(R"(
            .card {
                background: white;
                border-radius: 8px;
                box-shadow: 0 2px 8px rgba(0,0,0,0.1);
                overflow: hidden;
                transition: transform 0.3s, box-shadow 0.3s;
            }
            .card:hover {
                transform: translateY(-4px);
                box-shadow: 0 4px 16px rgba(0,0,0,0.15);
            }
            .card-header {
                padding: 1rem;
                border-bottom: 1px solid #eee;
                font-weight: bold;
            }
            .card-body {
                padding: 1rem;
            }
            .card-footer {
                padding: 1rem;
                border-top: 1px solid #eee;
                background: #f9f9f9;
            }
        )")
        .attribute("elevation", "2")
        .build();
}

WebComponent create_button() {
    return WebComponentBuilder("app-button")
        .template_html(R"(
            <button class="btn">
                <slot></slot>
            </button>
        )")
        .style(R"(
            .btn {
                padding: 0.75rem 1.5rem;
                border: none;
                border-radius: 4px;
                font-size: 1rem;
                cursor: pointer;
                background: #667eea;
                color: white;
                transition: background 0.3s, transform 0.1s;
            }
            .btn:hover {
                background: #5568d3;
            }
            .btn:active {
                transform: scale(0.98);
            }
            .btn[disabled] {
                opacity: 0.5;
                cursor: not-allowed;
            }
        )")
        .attribute("variant", "primary")
        .attribute("disabled", "false")
        .build();
}

WebComponent create_form_input() {
    return WebComponentBuilder("form-input")
        .template_html(R"(
            <div class="input-group">
                <label></label>
                <input type="text" />
                <span class="error"></span>
            </div>
        )")
        .style(R"(
            .input-group {
                margin-bottom: 1rem;
            }
            label {
                display: block;
                margin-bottom: 0.5rem;
                font-weight: 500;
            }
            input {
                width: 100%;
                padding: 0.75rem;
                border: 1px solid #ddd;
                border-radius: 4px;
                font-size: 1rem;
                transition: border-color 0.3s;
            }
            input:focus {
                outline: none;
                border-color: #667eea;
            }
            .error {
                color: #e74c3c;
                font-size: 0.875rem;
                margin-top: 0.25rem;
            }
        )")
        .attribute("label", "")
        .attribute("type", "text")
        .attribute("placeholder", "")
        .attribute("required", "false")
        .build();
}

WebComponent create_modal() {
    return WebComponentBuilder("app-modal")
        .template_html(R"(
            <div class="modal-backdrop">
                <div class="modal">
                    <div class="modal-header">
                        <slot name="header">Modal Title</slot>
                        <button class="close">&times;</button>
                    </div>
                    <div class="modal-body">
                        <slot></slot>
                    </div>
                    <div class="modal-footer">
                        <slot name="footer"></slot>
                    </div>
                </div>
            </div>
        )")
        .style(R"(
            .modal-backdrop {
                position: fixed;
                top: 0;
                left: 0;
                width: 100%;
                height: 100%;
                background: rgba(0,0,0,0.5);
                display: flex;
                align-items: center;
                justify-content: center;
                z-index: 1000;
            }
            .modal {
                background: white;
                border-radius: 8px;
                max-width: 600px;
                width: 90%;
                max-height: 90vh;
                overflow: auto;
            }
            .modal-header {
                padding: 1rem;
                border-bottom: 1px solid #eee;
                display: flex;
                justify-content: space-between;
                align-items: center;
            }
            .close {
                background: none;
                border: none;
                font-size: 1.5rem;
                cursor: pointer;
            }
            .modal-body {
                padding: 1rem;
            }
            .modal-footer {
                padding: 1rem;
                border-top: 1px solid #eee;
                display: flex;
                justify-content: flex-end;
                gap: 0.5rem;
            }
        )")
        .attribute("open", "false")
        .build();
}

WebComponent create_toast() {
    return WebComponentBuilder("app-toast")
        .template_html(R"(
            <div class="toast">
                <span class="icon"></span>
                <div class="content">
                    <slot></slot>
                </div>
                <button class="close">&times;</button>
            </div>
        )")
        .style(R"(
            .toast {
                position: fixed;
                bottom: 2rem;
                right: 2rem;
                background: white;
                padding: 1rem;
                border-radius: 4px;
                box-shadow: 0 4px 12px rgba(0,0,0,0.15);
                display: flex;
                align-items: center;
                gap: 1rem;
                min-width: 300px;
                animation: slideIn 0.3s ease-out;
            }
            @keyframes slideIn {
                from {
                    transform: translateX(400px);
                    opacity: 0;
                }
                to {
                    transform: translateX(0);
                    opacity: 1;
                }
            }
            .close {
                background: none;
                border: none;
                cursor: pointer;
                font-size: 1.25rem;
            }
        )")
        .attribute("type", "info")
        .attribute("duration", "3000")
        .build();
}

WebComponent create_data_table() {
    return WebComponentBuilder("data-table")
        .template_html(R"(
            <div class="table-container">
                <table>
                    <thead>
                        <slot name="header"></slot>
                    </thead>
                    <tbody>
                        <slot></slot>
                    </tbody>
                </table>
            </div>
        )")
        .style(R"(
            .table-container {
                overflow-x: auto;
            }
            table {
                width: 100%;
                border-collapse: collapse;
            }
            ::slotted(th) {
                text-align: left;
                padding: 0.75rem;
                background: #f5f5f5;
                border-bottom: 2px solid #ddd;
            }
            ::slotted(td) {
                padding: 0.75rem;
                border-bottom: 1px solid #eee;
            }
            ::slotted(tr:hover) {
                background: #f9f9f9;
            }
        )")
        .attribute("striped", "false")
        .attribute("hover", "true")
        .build();
}

WebComponent create_progress_bar() {
    return WebComponentBuilder("progress-bar")
        .template_html(R"(
            <div class="progress-container">
                <div class="progress-bar">
                    <div class="progress-fill"></div>
                </div>
                <span class="progress-text"></span>
            </div>
        )")
        .style(R"(
            .progress-container {
                display: flex;
                align-items: center;
                gap: 1rem;
            }
            .progress-bar {
                flex: 1;
                height: 8px;
                background: #eee;
                border-radius: 4px;
                overflow: hidden;
            }
            .progress-fill {
                height: 100%;
                background: linear-gradient(90deg, #667eea, #764ba2);
                transition: width 0.3s ease;
            }
            .progress-text {
                font-weight: 500;
                min-width: 3rem;
                text-align: right;
            }
        )")
        .attribute("value", "0")
        .attribute("max", "100")
        .build();
}

WebComponent create_tabs() {
    return WebComponentBuilder("tab-container")
        .template_html(R"(
            <div class="tabs">
                <div class="tab-headers">
                    <slot name="headers"></slot>
                </div>
                <div class="tab-content">
                    <slot></slot>
                </div>
            </div>
        )")
        .style(R"(
            .tabs {
                border: 1px solid #ddd;
                border-radius: 4px;
                overflow: hidden;
            }
            .tab-headers {
                display: flex;
                background: #f5f5f5;
                border-bottom: 1px solid #ddd;
            }
            ::slotted([slot="headers"]) {
                padding: 1rem;
                cursor: pointer;
                transition: background 0.3s;
            }
            ::slotted([slot="headers"]:hover) {
                background: #e8e8e8;
            }
            ::slotted([slot="headers"].active) {
                background: white;
                border-bottom: 2px solid #667eea;
            }
            .tab-content {
                padding: 1rem;
            }
        )")
        .attribute("active", "0")
        .build();
}

WebComponent create_dropdown() {
    return WebComponentBuilder("app-dropdown")
        .template_html(R"(
            <div class="dropdown">
                <button class="dropdown-toggle">
                    <slot name="trigger">Select</slot>
                </button>
                <div class="dropdown-menu">
                    <slot></slot>
                </div>
            </div>
        )")
        .style(R"(
            .dropdown {
                position: relative;
                display: inline-block;
            }
            .dropdown-toggle {
                padding: 0.75rem 1rem;
                background: white;
                border: 1px solid #ddd;
                border-radius: 4px;
                cursor: pointer;
            }
            .dropdown-menu {
                position: absolute;
                top: 100%;
                left: 0;
                min-width: 200px;
                background: white;
                border: 1px solid #ddd;
                border-radius: 4px;
                box-shadow: 0 4px 12px rgba(0,0,0,0.1);
                display: none;
                z-index: 100;
            }
            .dropdown.open .dropdown-menu {
                display: block;
            }
        )")
        .attribute("open", "false")
        .build();
}

WebComponent create_accordion() {
    return WebComponentBuilder("app-accordion")
        .template_html(R"(
            <div class="accordion">
                <div class="accordion-header">
                    <slot name="header">Accordion Title</slot>
                    <span class="icon">▼</span>
                </div>
                <div class="accordion-content">
                    <slot></slot>
                </div>
            </div>
        )")
        .style(R"(
            .accordion {
                border: 1px solid #ddd;
                border-radius: 4px;
                margin-bottom: 0.5rem;
            }
            .accordion-header {
                padding: 1rem;
                background: #f5f5f5;
                cursor: pointer;
                display: flex;
                justify-content: space-between;
                align-items: center;
                user-select: none;
            }
            .accordion-header:hover {
                background: #e8e8e8;
            }
            .icon {
                transition: transform 0.3s;
            }
            .accordion.open .icon {
                transform: rotate(180deg);
            }
            .accordion-content {
                max-height: 0;
                overflow: hidden;
                transition: max-height 0.3s ease-out;
            }
            .accordion.open .accordion-content {
                max-height: 1000px;
                padding: 1rem;
            }
        )")
        .attribute("open", "false")
        .build();
}

WebComponent create_footer() {
    return WebComponentBuilder("app-footer")
        .template_html(R"(
            <footer>
                <div class="footer-content">
                    <div class="footer-section">
                        <slot name="left"></slot>
                    </div>
                    <div class="footer-section">
                        <slot name="center"></slot>
                    </div>
                    <div class="footer-section">
                        <slot name="right"></slot>
                    </div>
                </div>
                <div class="footer-bottom">
                    <slot name="copyright">&copy; 2025 All rights reserved</slot>
                </div>
            </footer>
        )")
        .style(R"(
            footer {
                background: #2c3e50;
                color: white;
                padding: 2rem;
                margin-top: 4rem;
            }
            .footer-content {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
                gap: 2rem;
                max-width: 1200px;
                margin: 0 auto;
            }
            .footer-section {
                padding: 1rem 0;
            }
            .footer-bottom {
                text-align: center;
                padding-top: 2rem;
                margin-top: 2rem;
                border-top: 1px solid rgba(255,255,255,0.1);
            }
        )")
        .build();
}

} // namespace components

// Utility functions
std::string escape_html(const std::string& text) {
    std::string result = text;
    size_t pos = 0;
    while ((pos = result.find("&", pos)) != std::string::npos) {
        result.replace(pos, 1, "&amp;");
        pos += 5;
    }
    pos = 0;
    while ((pos = result.find("<", pos)) != std::string::npos) {
        result.replace(pos, 1, "&lt;");
        pos += 4;
    }
    pos = 0;
    while ((pos = result.find(">", pos)) != std::string::npos) {
        result.replace(pos, 1, "&gt;");
        pos += 4;
    }
    return result;
}

std::string escape_js(const std::string& text) {
    std::string result = text;
    size_t pos = 0;
    while ((pos = result.find("\\", pos)) != std::string::npos) {
        result.replace(pos, 1, "\\\\");
        pos += 2;
    }
    pos = 0;
    while ((pos = result.find("\"", pos)) != std::string::npos) {
        result.replace(pos, 1, "\\\"");
        pos += 2;
    }
    return result;
}

std::string generate_component_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "comp-";
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

// ApiDocGenerator Implementation
ApiDocGenerator::ApiDocGenerator(const std::string& service_name, const std::string& version) {
    service_.name = service_name;
    service_.version = version;
    service_.base_url = "http://localhost:8080";
}

ApiDocGenerator& ApiDocGenerator::add_endpoint(const std::string& method, const std::string& path, 
                                                 const std::string& description) {
    Endpoint endpoint;
    endpoint.method = method;
    endpoint.path = path;
    endpoint.description = description;
    service_.endpoints.push_back(endpoint);
    current_endpoint_ = &service_.endpoints.back();
    return *this;
}

ApiDocGenerator& ApiDocGenerator::with_request_body(const std::string& json_example) {
    if (current_endpoint_) current_endpoint_->request_body = json_example;
    return *this;
}

ApiDocGenerator& ApiDocGenerator::with_response_body(const std::string& json_example) {
    if (current_endpoint_) current_endpoint_->response_body = json_example;
    return *this;
}

ApiDocGenerator& ApiDocGenerator::with_parameter(const std::string& name, const std::string& description) {
    if (current_endpoint_) current_endpoint_->parameters.push_back({name, description});
    return *this;
}

ApiDocGenerator& ApiDocGenerator::with_response_code(const std::string& code) {
    if (current_endpoint_) current_endpoint_->response_codes.push_back(code);
    return *this;
}

ApiDocGenerator& ApiDocGenerator::set_base_url(const std::string& url) {
    service_.base_url = url;
    return *this;
}

ApiDocGenerator& ApiDocGenerator::set_description(const std::string& desc) {
    service_.description = desc;
    return *this;
}

std::string ApiDocGenerator::generate_html() const {
    std::stringstream html;
    
    html << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" << service_.name << R"( API Documentation</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 100%);
            color: #e0e0e0;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        header {
            background: linear-gradient(135deg, #6366f1 0%, #8b5cf6 100%);
            padding: 40px;
            border-radius: 12px;
            margin-bottom: 30px;
            box-shadow: 0 8px 32px rgba(99, 102, 241, 0.3);
        }
        h1 { font-size: 36px; margin-bottom: 10px; }
        .version { opacity: 0.9; font-size: 14px; }
        .description { margin-top: 15px; opacity: 0.95; }
        .base-url {
            background: rgba(0, 0, 0, 0.2);
            padding: 10px 15px;
            border-radius: 6px;
            margin-top: 15px;
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
            font-size: 14px;
        }
        .endpoint {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 12px;
            padding: 25px;
            margin-bottom: 20px;
            box-shadow: 0 4px 16px rgba(0, 0, 0, 0.3);
        }
        .endpoint-header {
            display: flex;
            align-items: center;
            gap: 15px;
            margin-bottom: 15px;
        }
        .method {
            padding: 6px 12px;
            border-radius: 6px;
            font-weight: 700;
            font-size: 12px;
            letter-spacing: 0.5px;
        }
        .method-get { background: rgba(34, 197, 94, 0.2); color: #4ade80; }
        .method-post { background: rgba(251, 191, 36, 0.2); color: #fbbf24; }
        .method-put { background: rgba(59, 130, 246, 0.2); color: #60a5fa; }
        .method-delete { background: rgba(239, 68, 68, 0.2); color: #f87171; }
        .path {
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
            font-size: 16px;
            color: #a78bfa;
            font-weight: 600;
        }
        .endpoint-description { margin: 15px 0; color: #9ca3af; }
        .section-title {
            font-size: 14px;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            color: #a78bfa;
            margin: 20px 0 10px;
        }
        .code-block {
            background: rgba(0, 0, 0, 0.4);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            padding: 15px;
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
            font-size: 13px;
            overflow-x: auto;
            white-space: pre;
            color: #10b981;
        }
        .parameter {
            display: flex;
            gap: 10px;
            padding: 8px 0;
            border-bottom: 1px solid rgba(255, 255, 255, 0.05);
        }
        .parameter:last-child { border-bottom: none; }
        .param-name {
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
            font-weight: 600;
            color: #60a5fa;
            min-width: 150px;
        }
        .param-desc { color: #9ca3af; }
        .response-codes {
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
        }
        .response-code {
            padding: 4px 12px;
            border-radius: 4px;
            font-size: 12px;
            font-weight: 600;
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
        }
        .code-200 { background: rgba(34, 197, 94, 0.2); color: #4ade80; }
        .code-400 { background: rgba(251, 191, 36, 0.2); color: #fbbf24; }
        .code-404 { background: rgba(239, 68, 68, 0.2); color: #f87171; }
        .code-500 { background: rgba(239, 68, 68, 0.3); color: #f87171; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>📚 )" << service_.name << R"( API</h1>
            <div class="version">Version )" << service_.version << R"(</div>
            <div class="description">)" << service_.description << R"(</div>
            <div class="base-url">Base URL: )" << service_.base_url << R"(</div>
        </header>
)";
    
    for (const auto& endpoint : service_.endpoints) {
        html << R"(        <div class="endpoint">
            <div class="endpoint-header">
                <span class="method method-)" << endpoint.method << R"(">)" << endpoint.method << R"(</span>
                <span class="path">)" << endpoint.path << R"(</span>
            </div>
            <div class="endpoint-description">)" << endpoint.description << R"(</div>
)";
        
        if (!endpoint.parameters.empty()) {
            html << R"(            <div class="section-title">Parameters</div>
            <div>
)";
            for (const auto& [name, desc] : endpoint.parameters) {
                html << R"(                <div class="parameter">
                    <span class="param-name">)" << name << R"(</span>
                    <span class="param-desc">)" << desc << R"(</span>
                </div>
)";
            }
            html << R"(            </div>
)";
        }
        
        if (!endpoint.request_body.empty()) {
            html << R"(            <div class="section-title">Request Body</div>
            <div class="code-block">)" << escape_html(endpoint.request_body) << R"(</div>
)";
        }
        
        if (!endpoint.response_body.empty()) {
            html << R"(            <div class="section-title">Response Body</div>
            <div class="code-block">)" << escape_html(endpoint.response_body) << R"(</div>
)";
        }
        
        if (!endpoint.response_codes.empty()) {
            html << R"(            <div class="section-title">Response Codes</div>
            <div class="response-codes">
)";
            for (const auto& code : endpoint.response_codes) {
                std::string code_class = "code-" + code.substr(0, 3);
                html << R"(                <span class="response-code )" << code_class << R"(">)" << code << R"(</span>
)";
            }
            html << R"(            </div>
)";
        }
        
        html << R"(        </div>
)";
    }
    
    html << R"(    </div>
</body>
</html>)";
    
    return html.str();
}

std::string ApiDocGenerator::generate_markdown() const {
    std::stringstream md;
    md << "# " << service_.name << " API Documentation\n\n";
    md << "**Version:** " << service_.version << "\n\n";
    md << "**Base URL:** `" << service_.base_url << "`\n\n";
    if (!service_.description.empty()) {
        md << service_.description << "\n\n";
    }
    md << "## Endpoints\n\n";
    
    for (const auto& endpoint : service_.endpoints) {
        md << "### " << endpoint.method << " " << endpoint.path << "\n\n";
        md << endpoint.description << "\n\n";
        
        if (!endpoint.parameters.empty()) {
            md << "**Parameters:**\n\n";
            for (const auto& [name, desc] : endpoint.parameters) {
                md << "- `" << name << "`: " << desc << "\n";
            }
            md << "\n";
        }
        
        if (!endpoint.request_body.empty()) {
            md << "**Request Body:**\n```json\n" << endpoint.request_body << "\n```\n\n";
        }
        
        if (!endpoint.response_body.empty()) {
            md << "**Response Body:**\n```json\n" << endpoint.response_body << "\n```\n\n";
        }
        
        if (!endpoint.response_codes.empty()) {
            md << "**Response Codes:** ";
            for (size_t i = 0; i < endpoint.response_codes.size(); ++i) {
                md << "`" << endpoint.response_codes[i] << "`";
                if (i < endpoint.response_codes.size() - 1) md << ", ";
            }
            md << "\n\n";
        }
    }
    
    return md.str();
}

std::string ApiDocGenerator::generate_json() const {
    std::stringstream json;
    json << "{\n";
    json << "  \"openapi\": \"3.0.0\",\n";
    json << "  \"info\": {\n";
    json << "    \"title\": \"" << service_.name << "\",\n";
    json << "    \"version\": \"" << service_.version << "\",\n";
    json << "    \"description\": \"" << service_.description << "\"\n";
    json << "  },\n";
    json << "  \"servers\": [\n";
    json << "    {\"url\": \"" << service_.base_url << "\"}\n";
    json << "  ],\n";
    json << "  \"paths\": {\n";
    
    for (size_t i = 0; i < service_.endpoints.size(); ++i) {
        const auto& endpoint = service_.endpoints[i];
        json << "    \"" << endpoint.path << "\": {\n";
        json << "      \"" << endpoint.method << "\": {\n";
        json << "        \"summary\": \"" << endpoint.description << "\"\n";
        json << "      }\n";
        json << "    }";
        if (i < service_.endpoints.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  }\n";
    json << "}\n";
    return json.str();
}

} // namespace html
} // namespace networking
} // namespace ml
