#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <sstream>

namespace ml {
namespace networking {
namespace html {

// Web Component Definition
struct WebComponent {
    std::string name;           // Component tag name (e.g., "app-header")
    std::string template_html;  // HTML template
    std::string styles;         // CSS styles (scoped to component)
    std::string script;         // JavaScript code
    std::map<std::string, std::string> attributes;
    std::vector<std::string> slots;
    
    WebComponent() = default;
    WebComponent(const std::string& n) : name(n) {}
    
    std::string render() const;
    std::string to_custom_element() const;
};

// Web Component Builder - Fluent API
class WebComponentBuilder {
public:
    WebComponentBuilder(const std::string& name);
    
    WebComponentBuilder& template_html(const std::string& html);
    WebComponentBuilder& style(const std::string& css);
    WebComponentBuilder& script(const std::string& js);
    WebComponentBuilder& attribute(const std::string& name, const std::string& default_value = "");
    WebComponentBuilder& slot(const std::string& name);
    
    WebComponent build();
    
private:
    WebComponent component_;
};

// Component Registry
class ComponentRegistry {
public:
    static ComponentRegistry& instance();
    
    void register_component(const WebComponent& component);
    WebComponent* get_component(const std::string& name);
    std::vector<std::string> list_components() const;
    void clear();
    
private:
    ComponentRegistry() = default;
    std::map<std::string, WebComponent> components_;
};

// Web Component Bundler - Compiles all components into single HTML file
class ComponentBundler {
public:
    ComponentBundler();
    
    // Add component to bundle
    ComponentBundler& add_component(const WebComponent& component);
    ComponentBundler& add_component_from_registry(const std::string& name);
    
    // Add global styles/scripts
    ComponentBundler& add_global_style(const std::string& css);
    ComponentBundler& add_global_script(const std::string& js);
    
    // Set page metadata
    ComponentBundler& set_title(const std::string& title);
    ComponentBundler& set_meta(const std::string& name, const std::string& content);
    ComponentBundler& set_favicon(const std::string& href);
    
    // Set main HTML body content
    ComponentBundler& set_body_content(const std::string& html);
    
    // Bundle options
    ComponentBundler& minify(bool enable = true);
    ComponentBundler& inline_everything(bool enable = true);
    ComponentBundler& add_polyfills(bool enable = true);
    
    // Generate final bundled HTML
    std::string bundle() const;
    
    // Save to file
    bool save_to_file(const std::string& filepath) const;
    
private:
    std::vector<WebComponent> components_;
    std::vector<std::string> global_styles_;
    std::vector<std::string> global_scripts_;
    std::string title_;
    std::map<std::string, std::string> meta_tags_;
    std::string favicon_;
    std::string body_content_;
    bool minify_;
    bool inline_everything_;
    bool add_polyfills_;
    
    std::string minify_css(const std::string& css) const;
    std::string minify_js(const std::string& js) const;
    std::string minify_html(const std::string& html) const;
    std::string get_polyfills() const;
};

// Pre-built component library
namespace components {

// App Header Component
WebComponent create_app_header();

// Navigation Menu Component
WebComponent create_nav_menu();

// Card Component
WebComponent create_card();

// Button Component
WebComponent create_button();

// Form Input Component
WebComponent create_form_input();

// Modal Dialog Component
WebComponent create_modal();

// Toast Notification Component
WebComponent create_toast();

// Data Table Component
WebComponent create_data_table();

// Progress Bar Component
WebComponent create_progress_bar();

// Tab Container Component
WebComponent create_tabs();

// Dropdown Component
WebComponent create_dropdown();

// Accordion Component
WebComponent create_accordion();

// Footer Component
WebComponent create_footer();

} // namespace components

// API Documentation Generator
class ApiDocGenerator {
public:
    struct Endpoint {
        std::string method;      // GET, POST, PUT, DELETE, etc.
        std::string path;        // /api/users/{id}
        std::string description;
        std::string request_body;   // JSON schema or example
        std::string response_body;  // JSON schema or example
        std::vector<std::pair<std::string, std::string>> parameters; // name, description
        std::vector<std::string> response_codes; // 200, 404, etc.
    };

    struct Service {
        std::string name;
        std::string version;
        std::string base_url;
        std::string description;
        std::vector<Endpoint> endpoints;
    };

    ApiDocGenerator(const std::string& service_name, const std::string& version = "1.0");

    // Add endpoints
    ApiDocGenerator& add_endpoint(const std::string& method, const std::string& path, 
                                   const std::string& description);
    ApiDocGenerator& with_request_body(const std::string& json_example);
    ApiDocGenerator& with_response_body(const std::string& json_example);
    ApiDocGenerator& with_parameter(const std::string& name, const std::string& description);
    ApiDocGenerator& with_response_code(const std::string& code);

    // Set service metadata
    ApiDocGenerator& set_base_url(const std::string& url);
    ApiDocGenerator& set_description(const std::string& desc);

    // Generate documentation HTML
    std::string generate_html() const;
    std::string generate_markdown() const;
    std::string generate_json() const; // OpenAPI/Swagger format

private:
    Service service_;
    Endpoint* current_endpoint_ = nullptr;
};

// Utility functions
std::string escape_html(const std::string& text);
std::string escape_js(const std::string& text);
std::string generate_component_id();

} // namespace html
} // namespace networking
} // namespace ml
