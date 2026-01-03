#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "networking/html/html_processor.h"

using namespace ml::networking::html;

// Helper function to write HTML to file
void write_html_file(const std::string& filename, const std::string& html) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << html;
        file.close();
        std::cout << "✓ Created: " << filename << "\n";
    } else {
        std::cerr << "✗ Failed to create: " << filename << "\n";
    }
}

// Generate CSS stylesheet
std::string generate_stylesheet() {
    return R"(/* Modern Website Stylesheet */
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
    line-height: 1.6;
    color: #333;
    background-color: #f8f9fa;
}

.container {
    max-width: 1200px;
    margin: 0 auto;
    padding: 0 20px;
}

/* Header Styles */
header {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    padding: 1rem 0;
    box-shadow: 0 2px 10px rgba(0,0,0,0.1);
}

header .container {
    display: flex;
    justify-content: space-between;
    align-items: center;
}

.logo {
    font-size: 1.5rem;
    font-weight: bold;
}

nav ul {
    display: flex;
    list-style: none;
    gap: 2rem;
}

nav a {
    color: white;
    text-decoration: none;
    transition: opacity 0.3s;
}

nav a:hover {
    opacity: 0.8;
}

/* Hero Section */
.hero {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    padding: 6rem 0;
    text-align: center;
}

.hero h1 {
    font-size: 3rem;
    margin-bottom: 1rem;
}

.hero p {
    font-size: 1.25rem;
    margin-bottom: 2rem;
    opacity: 0.9;
}

.btn {
    display: inline-block;
    padding: 0.75rem 2rem;
    background: white;
    color: #667eea;
    text-decoration: none;
    border-radius: 5px;
    font-weight: bold;
    transition: transform 0.3s, box-shadow 0.3s;
}

.btn:hover {
    transform: translateY(-2px);
    box-shadow: 0 5px 15px rgba(0,0,0,0.2);
}

/* Features Section */
.features {
    padding: 4rem 0;
    background: white;
}

.features h2 {
    text-align: center;
    font-size: 2.5rem;
    margin-bottom: 3rem;
    color: #667eea;
}

.feature-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 2rem;
}

.feature-card {
    padding: 2rem;
    background: #f8f9fa;
    border-radius: 10px;
    text-align: center;
    transition: transform 0.3s, box-shadow 0.3s;
}

.feature-card:hover {
    transform: translateY(-5px);
    box-shadow: 0 5px 20px rgba(0,0,0,0.1);
}

.feature-icon {
    font-size: 3rem;
    margin-bottom: 1rem;
}

.feature-card h3 {
    color: #667eea;
    margin-bottom: 1rem;
}

/* About Section */
.about {
    padding: 4rem 0;
    background: #f8f9fa;
}

.about h2 {
    font-size: 2.5rem;
    margin-bottom: 2rem;
    color: #667eea;
}

.about-content {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 3rem;
    align-items: center;
}

.about-text p {
    margin-bottom: 1rem;
    font-size: 1.1rem;
}

.about-image {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    height: 300px;
    border-radius: 10px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: white;
    font-size: 2rem;
}

/* Contact Form */
.contact {
    padding: 4rem 0;
    background: white;
}

.contact h2 {
    text-align: center;
    font-size: 2.5rem;
    margin-bottom: 3rem;
    color: #667eea;
}

.contact-form {
    max-width: 600px;
    margin: 0 auto;
}

.form-group {
    margin-bottom: 1.5rem;
}

.form-group label {
    display: block;
    margin-bottom: 0.5rem;
    font-weight: bold;
    color: #333;
}

.form-group input,
.form-group textarea {
    width: 100%;
    padding: 0.75rem;
    border: 2px solid #e9ecef;
    border-radius: 5px;
    font-size: 1rem;
    transition: border-color 0.3s;
}

.form-group input:focus,
.form-group textarea:focus {
    outline: none;
    border-color: #667eea;
}

.form-group textarea {
    resize: vertical;
    min-height: 150px;
}

.btn-submit {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    color: white;
    border: none;
    padding: 1rem 2rem;
    font-size: 1rem;
    border-radius: 5px;
    cursor: pointer;
    transition: transform 0.3s, box-shadow 0.3s;
}

.btn-submit:hover {
    transform: translateY(-2px);
    box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
}

/* Footer */
footer {
    background: #2d3748;
    color: white;
    padding: 3rem 0 1rem;
}

.footer-content {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
    gap: 2rem;
    margin-bottom: 2rem;
}

.footer-section h3 {
    margin-bottom: 1rem;
    color: #667eea;
}

.footer-section ul {
    list-style: none;
}

.footer-section li {
    margin-bottom: 0.5rem;
}

.footer-section a {
    color: #cbd5e0;
    text-decoration: none;
    transition: color 0.3s;
}

.footer-section a:hover {
    color: white;
}

.footer-bottom {
    text-align: center;
    padding-top: 2rem;
    border-top: 1px solid #4a5568;
    color: #cbd5e0;
}

@media (max-width: 768px) {
    .hero h1 {
        font-size: 2rem;
    }
    
    .about-content {
        grid-template-columns: 1fr;
    }
    
    nav ul {
        flex-direction: column;
        gap: 1rem;
    }
}
)";
}

// Build navigation menu
std::unique_ptr<HtmlElement> build_nav() {
    return HtmlBuilder("nav")
        .child("ul", [](HtmlBuilder& ul) {
            ul.child("li", [](HtmlBuilder& li) {
                li.child("a", [](HtmlBuilder& a) {
                    a.attr("href", "#home").text("Home");
                });
            })
            .child("li", [](HtmlBuilder& li) {
                li.child("a", [](HtmlBuilder& a) {
                    a.attr("href", "#features").text("Features");
                });
            })
            .child("li", [](HtmlBuilder& li) {
                li.child("a", [](HtmlBuilder& a) {
                    a.attr("href", "#about").text("About");
                });
            })
            .child("li", [](HtmlBuilder& li) {
                li.child("a", [](HtmlBuilder& a) {
                    a.attr("href", "#contact").text("Contact");
                });
            });
        })
        .build();
}

// Build header
std::unique_ptr<HtmlElement> build_header() {
    return HtmlBuilder("header")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("div", [](HtmlBuilder& logo) {
                    logo.class_name("logo").text("🚀 TechCorp");
                })
                .child(build_nav());
        })
        .build();
}

// Build hero section
std::unique_ptr<HtmlElement> build_hero() {
    return HtmlBuilder("section")
        .class_name("hero")
        .id("home")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("h1", [](HtmlBuilder& h1) {
                    h1.text("Welcome to the Future");
                })
                .child("p", [](HtmlBuilder& p) {
                    p.text("Build amazing web applications with our powerful HTML processing library");
                })
                .child("a", [](HtmlBuilder& a) {
                    a.class_name("btn")
                        .attr("href", "#features")
                        .text("Get Started");
                });
        })
        .build();
}

// Build feature card
std::unique_ptr<HtmlElement> build_feature_card(const std::string& icon, 
                                                 const std::string& title, 
                                                 const std::string& description) {
    return HtmlBuilder("div")
        .class_name("feature-card")
        .child("div", [&icon](HtmlBuilder& div) {
            div.class_name("feature-icon").text(icon);
        })
        .child("h3", [&title](HtmlBuilder& h3) {
            h3.text(title);
        })
        .child("p", [&description](HtmlBuilder& p) {
            p.text(description);
        })
        .build();
}

// Build features section
std::unique_ptr<HtmlElement> build_features() {
    return HtmlBuilder("section")
        .class_name("features")
        .id("features")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("h2", [](HtmlBuilder& h2) {
                    h2.text("Powerful Features");
                })
                .child("div", [](HtmlBuilder& grid) {
                    grid.class_name("feature-grid")
                        .child(build_feature_card("⚡", "Lightning Fast", 
                            "Optimized performance with minimal overhead for rapid HTML generation."))
                        .child(build_feature_card("🛡️", "Type Safe", 
                            "Built with C++17 for compile-time safety and runtime performance."))
                        .child(build_feature_card("🎨", "Fluent API", 
                            "Intuitive builder pattern makes HTML construction elegant and readable."))
                        .child(build_feature_card("🔍", "Query Support", 
                            "Find elements by tag, class, or ID with DOM-like query methods."))
                        .child(build_feature_card("🔒", "Secure", 
                            "Automatic HTML escaping prevents XSS vulnerabilities."))
                        .child(build_feature_card("📦", "Batteries Included", 
                            "Parser, builder, templates, and utilities all in one library."));
                });
        })
        .build();
}

// Build about section
std::unique_ptr<HtmlElement> build_about() {
    return HtmlBuilder("section")
        .class_name("about")
        .id("about")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("h2", [](HtmlBuilder& h2) {
                    h2.text("About Our Technology");
                })
                .child("div", [](HtmlBuilder& content) {
                    content.class_name("about-content")
                        .child("div", [](HtmlBuilder& text) {
                            text.class_name("about-text")
                                .child("p", [](HtmlBuilder& p) {
                                    p.text("Our HTML processing library is designed for modern C++ applications that need to generate, manipulate, or parse HTML content.");
                                })
                                .child("p", [](HtmlBuilder& p) {
                                    p.text("Whether you're building a web server, generating reports, or creating documentation, our library provides the tools you need with a clean, type-safe API.");
                                })
                                .child("p", [](HtmlBuilder& p) {
                                    p.text("Built with performance and security in mind, it's the perfect choice for mission-critical applications.");
                                });
                        })
                        .child("div", [](HtmlBuilder& image) {
                            image.class_name("about-image")
                                .text("📊");
                        });
                });
        })
        .build();
}

// Build contact form
std::unique_ptr<HtmlElement> build_contact() {
    return HtmlBuilder("section")
        .class_name("contact")
        .id("contact")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("h2", [](HtmlBuilder& h2) {
                    h2.text("Get In Touch");
                })
                .child("form", [](HtmlBuilder& form) {
                    form.class_name("contact-form")
                        .attr("action", "#")
                        .attr("method", "post")
                        .child("div", [](HtmlBuilder& group) {
                            group.class_name("form-group")
                                .child("label", [](HtmlBuilder& label) {
                                    label.attr("for", "name").text("Name");
                                })
                                .child("input", [](HtmlBuilder& input) {
                                    input.attr("type", "text")
                                        .attr("id", "name")
                                        .attr("name", "name")
                                        .attr("required", "required");
                                });
                        })
                        .child("div", [](HtmlBuilder& group) {
                            group.class_name("form-group")
                                .child("label", [](HtmlBuilder& label) {
                                    label.attr("for", "email").text("Email");
                                })
                                .child("input", [](HtmlBuilder& input) {
                                    input.attr("type", "email")
                                        .attr("id", "email")
                                        .attr("name", "email")
                                        .attr("required", "required");
                                });
                        })
                        .child("div", [](HtmlBuilder& group) {
                            group.class_name("form-group")
                                .child("label", [](HtmlBuilder& label) {
                                    label.attr("for", "message").text("Message");
                                })
                                .child("textarea", [](HtmlBuilder& textarea) {
                                    textarea.attr("id", "message")
                                        .attr("name", "message")
                                        .attr("required", "required");
                                });
                        })
                        .child("button", [](HtmlBuilder& button) {
                            button.attr("type", "submit")
                                .class_name("btn-submit")
                                .text("Send Message");
                        });
                });
        })
        .build();
}

// Build footer
std::unique_ptr<HtmlElement> build_footer() {
    return HtmlBuilder("footer")
        .child("div", [](HtmlBuilder& container) {
            container.class_name("container")
                .child("div", [](HtmlBuilder& content) {
                    content.class_name("footer-content")
                        .child("div", [](HtmlBuilder& section) {
                            section.class_name("footer-section")
                                .child("h3", [](HtmlBuilder& h3) {
                                    h3.text("Company");
                                })
                                .child("ul", [](HtmlBuilder& ul) {
                                    ul.child("li", [](HtmlBuilder& li) {
                                        li.child("a", [](HtmlBuilder& a) {
                                            a.attr("href", "#").text("About Us");
                                        });
                                    })
                                    .child("li", [](HtmlBuilder& li) {
                                        li.child("a", [](HtmlBuilder& a) {
                                            a.attr("href", "#").text("Careers");
                                        });
                                    })
                                    .child("li", [](HtmlBuilder& li) {
                                        li.child("a", [](HtmlBuilder& a) {
                                            a.attr("href", "#").text("Press");
                                        });
                                    });
                                });
                        })
                        .child("div", [](HtmlBuilder& section) {
                            section.class_name("footer-section")
                                .child("h3", [](HtmlBuilder& h3) {
                                    h3.text("Resources");
                                })
                                .child("ul", [](HtmlBuilder& ul) {
                                    ul.child("li", [](HtmlBuilder& li) {
                                        li.child("a", [](HtmlBuilder& a) {
                                            a.attr("href", "#").text("Documentation");
                                        });
                                    })
                                    .child("li", [](HtmlBuilder& li) {
                                        li.child("a", [](HtmlBuilder& a) {
                                            a.attr("href", "#").text("API Reference");
                                        });
                                    })
                                    .child("li", [](HtmlBuilder& li) {
                                        li.child("a", [](HtmlBuilder& a) {
                                            a.attr("href", "#").text("Examples");
                                        });
                                    });
                                });
                        })
                        .child("div", [](HtmlBuilder& section) {
                            section.class_name("footer-section")
                                .child("h3", [](HtmlBuilder& h3) {
                                    h3.text("Support");
                                })
                                .child("ul", [](HtmlBuilder& ul) {
                                    ul.child("li", [](HtmlBuilder& li) {
                                        li.child("a", [](HtmlBuilder& a) {
                                            a.attr("href", "#").text("Help Center");
                                        });
                                    })
                                    .child("li", [](HtmlBuilder& li) {
                                        li.child("a", [](HtmlBuilder& a) {
                                            a.attr("href", "#").text("Community");
                                        });
                                    })
                                    .child("li", [](HtmlBuilder& li) {
                                        li.child("a", [](HtmlBuilder& a) {
                                            a.attr("href", "#").text("Contact");
                                        });
                                    });
                                });
                        });
                })
                .child("div", [](HtmlBuilder& bottom) {
                    bottom.class_name("footer-bottom")
                        .child("p", [](HtmlBuilder& p) {
                            p.text("© 2025 TechCorp. Built with HTML Processor Library.");
                        });
                });
        })
        .build();
}

// Build complete website
void build_website(HtmlDocument& doc) {
    // Set metadata
    doc.set_title("TechCorp - Modern Web Solutions");
    doc.add_meta("charset", "UTF-8");
    doc.add_meta("viewport", "width=device-width, initial-scale=1.0");
    doc.add_meta("description", "Build amazing web applications with our powerful HTML processing library");
    doc.add_stylesheet("style.css");
    
    // Build body content
    auto body = doc.get_body();
    if (body) {
        body->add_element(build_header());
        body->add_element(build_hero());
        body->add_element(build_features());
        body->add_element(build_about());
        body->add_element(build_contact());
        body->add_element(build_footer());
    }
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║       Website Generator Demo                      ║\n";
    std::cout << "║       Complete Website with HTML Library          ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Generating website files...\n\n";
    
    // Generate CSS
    write_html_file("style.css", generate_stylesheet());
    
    // Generate HTML
    HtmlDocument website;
    build_website(website);
    write_html_file("index.html", website.to_string());
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Website Generated Successfully!     ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    std::cout << "Files created:\n";
    std::cout << "  📄 index.html  - Main HTML file\n";
    std::cout << "  🎨 style.css   - Stylesheet\n\n";
    
    std::cout << "To view the website:\n";
    std::cout << "  1. Open index.html in your web browser\n";
    std::cout << "  2. Or run: open index.html (macOS)\n";
    std::cout << "  3. Or run: xdg-open index.html (Linux)\n\n";
    
    std::cout << "Website features:\n";
    std::cout << "  ✓ Responsive design\n";
    std::cout << "  ✓ Modern gradient header\n";
    std::cout << "  ✓ Hero section with CTA\n";
    std::cout << "  ✓ Feature cards grid\n";
    std::cout << "  ✓ About section\n";
    std::cout << "  ✓ Contact form\n";
    std::cout << "  ✓ Footer with links\n\n";
    
    std::cout << "HTML Statistics:\n";
    std::string html = website.to_string();
    std::cout << "  Total size: " << html.length() << " bytes\n";
    std::cout << "  Lines: " << std::count(html.begin(), html.end(), '\n') << "\n\n";
    
    return 0;
}
