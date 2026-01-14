#include "networking/document/web_components.h"
#include "networking/document/html_processor.h"
#include <iostream>
#include <string>
#include <iomanip>

using namespace ml::networking::html;

int main() {
    std::cout << "=== Web Components & Bundler Demo ===\n\n";
    
    // Register all pre-built components
    ComponentRegistry& registry = ComponentRegistry::instance();
    registry.register_component(components::create_app_header());
    registry.register_component(components::create_nav_menu());
    registry.register_component(components::create_card());
    registry.register_component(components::create_button());
    registry.register_component(components::create_form_input());
    registry.register_component(components::create_modal());
    registry.register_component(components::create_toast());
    registry.register_component(components::create_data_table());
    registry.register_component(components::create_progress_bar());
    registry.register_component(components::create_tabs());
    registry.register_component(components::create_dropdown());
    registry.register_component(components::create_accordion());
    registry.register_component(components::create_footer());
    
    std::cout << "✓ Registered " << registry.list_components().size() << " components:\n";
    for (const auto& name : registry.list_components()) {
        std::cout << "  - " << name << "\n";
    }
    
    // Create a complete web application
    ComponentBundler bundler;
    
    bundler
        .set_title("ToolBox Dashboard - Web Components Demo")
        .set_meta("description", "A modern dashboard built with custom web components")
        .set_meta("author", "ToolBox Framework")
        .set_favicon("data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>🛠️</text></svg>")
        
        // Add global styles
        .add_global_style(R"(
            * {
                margin: 0;
                padding: 0;
                box-sizing: border-box;
            }
            body {
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
                background: #f5f7fa;
                color: #333;
                line-height: 1.6;
            }
            .container {
                max-width: 1200px;
                margin: 0 auto;
                padding: 2rem;
            }
            .grid {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
                gap: 2rem;
                margin: 2rem 0;
            }
            h1, h2, h3 {
                margin-bottom: 1rem;
                color: #2c3e50;
            }
            .hero {
                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                color: white;
                padding: 4rem 2rem;
                text-align: center;
                margin-bottom: 3rem;
                border-radius: 8px;
            }
            .hero h1 {
                color: white;
                font-size: 3rem;
                margin-bottom: 1rem;
            }
            .hero p {
                font-size: 1.25rem;
                opacity: 0.9;
            }
        )")
        
        // Add all components to the bundle
        .add_component_from_registry("app-header")
        .add_component_from_registry("nav-menu")
        .add_component_from_registry("app-card")
        .add_component_from_registry("app-button")
        .add_component_from_registry("form-input")
        .add_component_from_registry("app-modal")
        .add_component_from_registry("app-toast")
        .add_component_from_registry("data-table")
        .add_component_from_registry("progress-bar")
        .add_component_from_registry("tab-container")
        .add_component_from_registry("app-dropdown")
        .add_component_from_registry("app-accordion")
        .add_component_from_registry("app-footer")
        
        // Set body content with component usage
        .set_body_content(R"(
            <!-- Application Header -->
            <app-header>
                <span slot="logo">🛠️ ToolBox</span>
                <nav-menu slot="nav">
                    <a href="#home">Home</a>
                    <a href="#components">Components</a>
                    <a href="#docs">Docs</a>
                    <a href="#about">About</a>
                </nav-menu>
                <div slot="actions">
                    <app-button>Sign In</app-button>
                </div>
            </app-header>

            <div class="container">
                <!-- Hero Section -->
                <div class="hero">
                    <h1>Welcome to ToolBox</h1>
                    <p>Build modern web applications with reusable components</p>
                    <br>
                    <app-button>Get Started</app-button>
                </div>

                <!-- Feature Cards -->
                <h2>Component Library</h2>
                <div class="grid">
                    <app-card>
                        <h3 slot="header">📦 Cards</h3>
                        Beautiful card components with headers, body, and footer sections.
                        <div slot="footer">
                            <app-button>Learn More</app-button>
                        </div>
                    </app-card>

                    <app-card>
                        <h3 slot="header">🎨 Forms</h3>
                        <form-input label="Name" placeholder="Enter your name"></form-input>
                        <form-input label="Email" type="email" placeholder="your@email.com"></form-input>
                        <div slot="footer">
                            <app-button>Submit</app-button>
                        </div>
                    </app-card>

                    <app-card>
                        <h3 slot="header">📊 Progress</h3>
                        <progress-bar value="75" max="100"></progress-bar>
                        <br><br>
                        <progress-bar value="50" max="100"></progress-bar>
                        <br><br>
                        <progress-bar value="30" max="100"></progress-bar>
                    </app-card>
                </div>

                <!-- Accordion Section -->
                <h2>Frequently Asked Questions</h2>
                <app-accordion>
                    <div slot="header">What is ToolBox?</div>
                    ToolBox is a comprehensive C++ framework for building modern web applications with custom web components.
                </app-accordion>
                
                <app-accordion>
                    <div slot="header">How do I get started?</div>
                    Simply include the header files, register your components, and use the bundler to compile everything into a single HTML file.
                </app-accordion>
                
                <app-accordion>
                    <div slot="header">Is it production ready?</div>
                    Yes! ToolBox includes a full suite of components, bundler, and web server integration.
                </app-accordion>

                <!-- Data Table -->
                <h2>Performance Metrics</h2>
                <data-table>
                    <tr slot="header">
                        <th>Component</th>
                        <th>Size (KB)</th>
                        <th>Load Time (ms)</th>
                        <th>Status</th>
                    </tr>
                    <tr>
                        <td>app-header</td>
                        <td>2.3</td>
                        <td>12</td>
                        <td>✓</td>
                    </tr>
                    <tr>
                        <td>app-card</td>
                        <td>1.8</td>
                        <td>8</td>
                        <td>✓</td>
                    </tr>
                    <tr>
                        <td>app-button</td>
                        <td>0.9</td>
                        <td>5</td>
                        <td>✓</td>
                    </tr>
                    <tr>
                        <td>form-input</td>
                        <td>1.5</td>
                        <td>7</td>
                        <td>✓</td>
                    </tr>
                </data-table>
            </div>

            <!-- Footer -->
            <app-footer>
                <div slot="left">
                    <h3>ToolBox</h3>
                    <p>Modern C++ Web Framework</p>
                </div>
                <div slot="center">
                    <h3>Resources</h3>
                    <p><a href="#" style="color: white;">Documentation</a></p>
                    <p><a href="#" style="color: white;">GitHub</a></p>
                    <p><a href="#" style="color: white;">Examples</a></p>
                </div>
                <div slot="right">
                    <h3>Contact</h3>
                    <p>support@toolbox.dev</p>
                </div>
                <div slot="copyright">© 2025 ToolBox Framework. Built with ❤️</div>
            </app-footer>
        )")
        
        // Add interactivity
        .add_global_script(R"(
            // Initialize app
            document.addEventListener('DOMContentLoaded', () => {
                console.log('ToolBox Dashboard loaded!');
                
                // Add accordion interactivity
                document.querySelectorAll('app-accordion').forEach(accordion => {
                    const header = accordion.shadowRoot.querySelector('.accordion-header');
                    header.addEventListener('click', () => {
                        accordion.classList.toggle('open');
                    });
                });
                
                // Add button click handlers
                document.querySelectorAll('app-button').forEach(btn => {
                    btn.addEventListener('click', () => {
                        console.log('Button clicked:', btn.textContent);
                    });
                });
                
                // Simulate progress bar animation
                const progressBars = document.querySelectorAll('progress-bar');
                progressBars.forEach((bar, index) => {
                    const fill = bar.shadowRoot.querySelector('.progress-fill');
                    const text = bar.shadowRoot.querySelector('.progress-text');
                    const value = bar.getAttribute('value') || 0;
                    fill.style.width = value + '%';
                    text.textContent = value + '%';
                });
            });
        )")
        
        .minify(false)  // Keep readable for demo
        .inline_everything(true)
        .add_polyfills(true);
    
    // Generate bundled HTML
    std::cout << "\n✓ Building bundle...\n";
    std::string bundled_html = bundler.bundle();
    
    std::cout << "✓ Bundle generated: " << bundled_html.size() << " bytes\n";
    
    // Save to file
    std::string output_file = "webapp_bundle.html";
    if (bundler.save_to_file(output_file)) {
        std::cout << "✓ Saved to: " << output_file << "\n";
    } else {
        std::cout << "✗ Failed to save bundle\n";
    }
    
    // Create a minified version
    std::cout << "\n✓ Creating minified version...\n";
    ComponentBundler minified_bundler;
    minified_bundler
        .set_title("ToolBox Dashboard")
        .set_body_content(R"(
            <app-header>
                <span slot="logo">🛠️ ToolBox</span>
            </app-header>
            <div class="container">
                <h1>Minified Build</h1>
                <app-card>
                    <h3 slot="header">Optimized</h3>
                    This is the minified production build.
                </app-card>
            </div>
        )")
        .add_component_from_registry("app-header")
        .add_component_from_registry("app-card")
        .minify(true)
        .inline_everything(true);
    
    std::string minified_html = minified_bundler.bundle();
    std::cout << "✓ Minified bundle: " << minified_html.size() << " bytes\n";
    
    if (minified_bundler.save_to_file("webapp_bundle.min.html")) {
        std::cout << "✓ Saved to: webapp_bundle.min.html\n";
    }
    
    // Calculate compression ratio
    double ratio = (1.0 - (double)minified_html.size() / bundled_html.size()) * 100;
    std::cout << "✓ Size reduction: " << std::fixed << std::setprecision(1) << ratio << "%\n";
    
    // Create a custom component example
    std::cout << "\n=== Custom Component Example ===\n";
    WebComponent custom = WebComponentBuilder("user-profile")
        .template_html(R"(
            <div class="profile">
                <img class="avatar" />
                <h3 class="name"></h3>
                <p class="bio"></p>
                <app-button>Follow</app-button>
            </div>
        )")
        .style(R"(
            .profile { text-align: center; padding: 2rem; }
            .avatar { width: 100px; height: 100px; border-radius: 50%; }
            .name { margin: 1rem 0; }
            .bio { color: #666; margin-bottom: 1rem; }
        )")
        .attribute("username", "")
        .attribute("avatar", "")
        .attribute("bio", "")
        .build();
    
    registry.register_component(custom);
    std::cout << "✓ Created custom component: user-profile\n";
    
    // Bundle with custom component
    ComponentBundler custom_bundler;
    custom_bundler
        .set_title("User Profile")
        .set_body_content(R"(
            <user-profile 
                username="John Doe" 
                avatar="/avatar.jpg" 
                bio="Software Engineer | Open Source Contributor">
            </user-profile>
        )")
        .add_component_from_registry("user-profile")
        .add_component_from_registry("app-button");
    
    if (custom_bundler.save_to_file("profile_bundle.html")) {
        std::cout << "✓ Saved custom component demo to: profile_bundle.html\n";
    }
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "✓ Components registered: " << registry.list_components().size() << "\n";
    std::cout << "✓ Bundles created: 3\n";
    std::cout << "  - webapp_bundle.html (full demo)\n";
    std::cout << "  - webapp_bundle.min.html (minified)\n";
    std::cout << "  - profile_bundle.html (custom component)\n";
    std::cout << "\nOpen these files in a browser to see the components in action!\n";
    
    return 0;
}
