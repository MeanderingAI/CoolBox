#include <iostream>
#include <iomanip>
#include "networking/document/html_processor.h"

using namespace ml::networking::html;

void demo_html_builder() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   HTML Builder Demo                   ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    // Build a simple HTML element
    auto div = HtmlBuilder("div")
        .id("container")
        .class_name("main-content")
        .class_name("centered")
        .style("background-color", "#f0f0f0")
        .style("padding", "20px")
        .text("Hello, HTML!")
        .build();
    
    std::cout << "Simple div with attributes and text:\n";
    std::cout << div->to_string(0) << "\n";
    
    // Build nested elements
    auto card = HtmlBuilder("div")
        .class_name("card")
        .child("h2", [](HtmlBuilder& h2) {
            h2.text("Card Title");
        })
        .child("p", [](HtmlBuilder& p) {
            p.text("This is the card content.");
        })
        .child("button", [](HtmlBuilder& btn) {
            btn.class_name("btn")
                .class_name("btn-primary")
                .text("Click Me");
        })
        .build();
    
    std::cout << "\nNested card structure:\n";
    std::cout << card->to_string(0) << "\n";
}

void demo_html_document() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   HTML Document Demo                  ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    HtmlDocument doc;
    doc.set_title("My Web Page");
    doc.add_meta("charset", "UTF-8");
    doc.add_meta("viewport", "width=device-width, initial-scale=1.0");
    doc.add_stylesheet("styles.css");
    doc.add_script("app.js");
    
    // Add content to body
    auto body = doc.get_body();
    if (body) {
        auto header = HtmlBuilder("header")
            .child("h1", [](HtmlBuilder& h1) {
                h1.text("Welcome to My Site");
            })
            .child("nav", [](HtmlBuilder& nav) {
                nav.child("a", [](HtmlBuilder& a) {
                    a.attr("href", "/home").text("Home");
                });
                nav.child("a", [](HtmlBuilder& a) {
                    a.attr("href", "/about").text("About");
                });
                nav.child("a", [](HtmlBuilder& a) {
                    a.attr("href", "/contact").text("Contact");
                });
            })
            .build();
        
        body->add_element(std::move(header));
        
        auto main = HtmlBuilder("main")
            .child("section", [](HtmlBuilder& section) {
                section.class_name("intro")
                    .child("h2", [](HtmlBuilder& h2) {
                        h2.text("Introduction");
                    })
                    .child("p", [](HtmlBuilder& p) {
                        p.text("This is a sample web page built with the HTML processor library.");
                    });
            })
            .build();
        
        body->add_element(std::move(main));
    }
    
    std::cout << "Complete HTML document:\n";
    std::cout << "─────────────────────────────────────────\n";
    std::cout << doc.to_string() << "\n";
}

void demo_html_manipulation() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   HTML Manipulation Demo              ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    // Create an element
    auto div = std::make_unique<HtmlElement>("div");
    div->set_id("main");
    div->add_class("container");
    div->add_class("fluid");
    
    std::cout << "Initial element:\n";
    std::cout << div->to_string(0) << "\n\n";
    
    // Add more attributes
    div->set_attribute("data-section", "hero");
    div->set_style("margin", "0 auto");
    
    std::cout << "After adding attributes and styles:\n";
    std::cout << div->to_string(0) << "\n\n";
    
    // Remove a class
    div->remove_class("fluid");
    
    std::cout << "After removing 'fluid' class:\n";
    std::cout << div->to_string(0) << "\n\n";
    
    // Add child elements
    auto h1 = std::make_unique<HtmlElement>("h1");
    h1->add_text("Main Title");
    div->add_element(std::move(h1));
    
    auto p = std::make_unique<HtmlElement>("p");
    p->add_text("This is a paragraph.");
    div->add_element(std::move(p));
    
    std::cout << "After adding children:\n";
    std::cout << div->to_string(0) << "\n";
}

void demo_html_queries() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   HTML Query Demo                     ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    // Build a document with multiple elements
    auto root = HtmlBuilder("div")
        .id("root")
        .child("header", [](HtmlBuilder& header) {
            header.class_name("site-header")
                .child("h1", [](HtmlBuilder& h1) {
                    h1.id("title").text("Site Title");
                });
        })
        .child("main", [](HtmlBuilder& main) {
            main.child("article", [](HtmlBuilder& article) {
                article.class_name("post")
                    .child("h2", [](HtmlBuilder& h2) {
                        h2.text("Article 1");
                    })
                    .child("p", [](HtmlBuilder& p) {
                        p.text("Content 1");
                    });
            })
            .child("article", [](HtmlBuilder& article) {
                article.class_name("post")
                    .child("h2", [](HtmlBuilder& h2) {
                        h2.text("Article 2");
                    })
                    .child("p", [](HtmlBuilder& p) {
                        p.text("Content 2");
                    });
            });
        })
        .build();
    
    std::cout << "Document structure:\n";
    std::cout << root->to_string(0) << "\n\n";
    
    // Find by tag
    auto articles = root->find_by_tag("article");
    std::cout << "Found " << articles.size() << " <article> elements\n";
    
    auto h2s = root->find_by_tag("h2");
    std::cout << "Found " << h2s.size() << " <h2> elements:\n";
    for (auto* h2 : h2s) {
        std::cout << "  - " << HtmlUtils::extract_text(*h2) << "\n";
    }
    
    // Find by class
    auto posts = root->find_by_class("post");
    std::cout << "\nFound " << posts.size() << " elements with class 'post'\n";
    
    // Find by ID
    auto title = root->find_by_id("title");
    if (title) {
        std::cout << "\nFound element with id 'title': " 
                  << HtmlUtils::extract_text(*title) << "\n";
    }
}

void demo_html_escaping() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   HTML Escaping Demo                  ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    std::string raw_text = "<script>alert('XSS');</script>";
    std::string escaped = HtmlUtils::escape(raw_text);
    
    std::cout << "Raw text:     " << raw_text << "\n";
    std::cout << "Escaped text: " << escaped << "\n";
    std::cout << "Unescaped:    " << HtmlUtils::unescape(escaped) << "\n\n";
    
    // Build element with potentially dangerous content
    auto div = HtmlBuilder("div")
        .text(raw_text)  // Will be properly escaped in output
        .build();
    
    std::cout << "Element with escaped content:\n";
    std::cout << div->to_string(0) << "\n";
}

void demo_html_template() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   HTML Template Demo                  ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    std::string template_html = R"(
<div class="user-card">
    <h3>{{name}}</h3>
    <p>Email: {{email}}</p>
    <p>Role: {{role}}</p>
    <p>Status: {{status}}</p>
</div>
)";
    
    HtmlTemplate tmpl(template_html);
    
    // Render with variables
    tmpl.set_variable("name", "John Doe");
    tmpl.set_variable("email", "john@example.com");
    tmpl.set_variable("role", "Developer");
    tmpl.set_variable("status", "Active");
    
    std::cout << "Template:\n" << template_html << "\n";
    std::cout << "Rendered output:\n";
    std::cout << "─────────────────────────────────────────\n";
    std::cout << tmpl.render() << "\n";
    
    // Render with different data
    std::map<std::string, std::string> user2 = {
        {"name", "Jane Smith"},
        {"email", "jane@example.com"},
        {"role", "Designer"},
        {"status", "Away"}
    };
    
    tmpl.set_variables(user2);
    std::cout << "\nWith different data:\n";
    std::cout << "─────────────────────────────────────────\n";
    std::cout << tmpl.render() << "\n";
}

void demo_html_parser() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   HTML Parser Demo                    ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    std::string html = R"(
<div class="container">
    <h1>Hello World</h1>
    <p>This is a <strong>test</strong> paragraph.</p>
    <ul>
        <li>Item 1</li>
        <li>Item 2</li>
        <li>Item 3</li>
    </ul>
</div>
)";
    
    std::cout << "Input HTML:\n";
    std::cout << html << "\n\n";
    
    HtmlParser parser;
    auto element = parser.parse_fragment(html);
    
    if (element) {
        std::cout << "Parsed and re-rendered:\n";
        std::cout << "─────────────────────────────────────────\n";
        std::cout << element->to_string(0) << "\n\n";
        
        // Extract text content
        std::string text = HtmlUtils::extract_text(*element);
        std::cout << "Extracted text: " << text << "\n";
    }
}

void demo_html_utilities() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   HTML Utilities Demo                 ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    // Self-closing tags
    std::cout << "Self-closing tags:\n";
    std::vector<std::string> tags = {"img", "br", "input", "div", "span"};
    for (const auto& tag : tags) {
        std::cout << "  " << std::setw(8) << std::left << tag << ": " 
                  << (HtmlUtils::is_self_closing_tag(tag) ? "yes" : "no") << "\n";
    }
    
    // Tag name validation
    std::cout << "\nTag name validation:\n";
    std::vector<std::string> test_tags = {"div", "my-tag", "tag_1", "123", "tag space"};
    for (const auto& tag : test_tags) {
        std::cout << "  " << std::setw(12) << std::left << ("'" + tag + "'") << ": " 
                  << (HtmlUtils::is_valid_tag_name(tag) ? "valid" : "invalid") << "\n";
    }
    
    // Minification
    std::string html = R"(
    <div class="example">
        <h1>Title</h1>
        <p>Paragraph with    multiple    spaces</p>
    </div>
    )";
    
    std::cout << "\nMinification:\n";
    std::cout << "Original: " << html.length() << " bytes\n";
    std::string minified = HtmlUtils::minify(html);
    std::cout << "Minified: " << minified.length() << " bytes\n";
    std::cout << "Result: " << minified << "\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║       HTML Processing Library Demo                ║\n";
    std::cout << "║       Build, Parse, and Manipulate HTML           ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    demo_html_builder();
    demo_html_document();
    demo_html_manipulation();
    demo_html_queries();
    demo_html_escaping();
    demo_html_template();
    demo_html_parser();
    demo_html_utilities();
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Demo Complete!                      ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    return 0;
}
