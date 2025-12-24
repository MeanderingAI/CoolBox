#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <sstream>

namespace ml {
namespace networking {
namespace html {

// HTML attribute
struct HtmlAttribute {
    std::string name;
    std::string value;
    
    HtmlAttribute() = default;
    HtmlAttribute(const std::string& n, const std::string& v) 
        : name(n), value(v) {}
};

// HTML node types
enum class NodeType {
    ELEMENT,
    TEXT,
    COMMENT,
    DOCTYPE
};

// Forward declaration
class HtmlElement;

// HTML Node base class
class HtmlNode {
public:
    virtual ~HtmlNode() = default;
    virtual NodeType get_type() const = 0;
    virtual std::string to_string(int indent = 0) const = 0;
    virtual std::unique_ptr<HtmlNode> clone() const = 0;
};

// Text node
class TextNode : public HtmlNode {
public:
    explicit TextNode(const std::string& text) : text_(text) {}
    
    NodeType get_type() const override { return NodeType::TEXT; }
    std::string to_string(int indent = 0) const override;
    std::unique_ptr<HtmlNode> clone() const override;
    
    const std::string& get_text() const { return text_; }
    void set_text(const std::string& text) { text_ = text; }
    
private:
    std::string text_;
};

// Comment node
class CommentNode : public HtmlNode {
public:
    explicit CommentNode(const std::string& comment) : comment_(comment) {}
    
    NodeType get_type() const override { return NodeType::COMMENT; }
    std::string to_string(int indent = 0) const override;
    std::unique_ptr<HtmlNode> clone() const override;
    
    const std::string& get_comment() const { return comment_; }
    
private:
    std::string comment_;
};

// DOCTYPE node
class DoctypeNode : public HtmlNode {
public:
    explicit DoctypeNode(const std::string& doctype = "html") : doctype_(doctype) {}
    
    NodeType get_type() const override { return NodeType::DOCTYPE; }
    std::string to_string(int indent = 0) const override;
    std::unique_ptr<HtmlNode> clone() const override;
    
private:
    std::string doctype_;
};

// HTML Element
class HtmlElement : public HtmlNode {
public:
    explicit HtmlElement(const std::string& tag_name);
    ~HtmlElement() override = default;
    
    NodeType get_type() const override { return NodeType::ELEMENT; }
    std::string to_string(int indent = 0) const override;
    std::unique_ptr<HtmlNode> clone() const override;
    
    // Tag name
    const std::string& get_tag() const { return tag_name_; }
    void set_tag(const std::string& tag) { tag_name_ = tag; }
    
    // Attributes
    void set_attribute(const std::string& name, const std::string& value);
    std::string get_attribute(const std::string& name) const;
    bool has_attribute(const std::string& name) const;
    void remove_attribute(const std::string& name);
    const std::vector<HtmlAttribute>& get_attributes() const { return attributes_; }
    
    // Convenience methods
    void set_id(const std::string& id) { set_attribute("id", id); }
    void add_class(const std::string& class_name);
    void remove_class(const std::string& class_name);
    void set_style(const std::string& property, const std::string& value);
    
    // Children
    void add_child(std::unique_ptr<HtmlNode> child);
    void add_text(const std::string& text);
    void add_element(std::unique_ptr<HtmlElement> element);
    const std::vector<std::unique_ptr<HtmlNode>>& get_children() const { return children_; }
    size_t child_count() const { return children_.size(); }
    
    // Self-closing tags
    bool is_self_closing() const;
    void set_self_closing(bool self_closing) { self_closing_ = self_closing; }
    
    // Query
    std::vector<HtmlElement*> find_by_tag(const std::string& tag);
    std::vector<HtmlElement*> find_by_class(const std::string& class_name);
    HtmlElement* find_by_id(const std::string& id);
    
private:
    std::string tag_name_;
    std::vector<HtmlAttribute> attributes_;
    std::vector<std::unique_ptr<HtmlNode>> children_;
    bool self_closing_;
    
    void find_by_tag_recursive(const std::string& tag, std::vector<HtmlElement*>& results);
    void find_by_class_recursive(const std::string& class_name, std::vector<HtmlElement*>& results);
    HtmlElement* find_by_id_recursive(const std::string& id);
};

// HTML Document
class HtmlDocument {
public:
    HtmlDocument();
    ~HtmlDocument() = default;
    
    // Document structure
    void set_doctype(const std::string& doctype = "html");
    void set_root(std::unique_ptr<HtmlElement> root);
    HtmlElement* get_root() { return root_.get(); }
    const HtmlElement* get_root() const { return root_.get(); }
    
    // Convenience methods for common structure
    HtmlElement* get_head();
    HtmlElement* get_body();
    
    void set_title(const std::string& title);
    void add_meta(const std::string& name, const std::string& content);
    void add_stylesheet(const std::string& href);
    void add_script(const std::string& src);
    
    // Serialization
    std::string to_string() const;
    std::string to_string_pretty(int indent_size = 2) const;
    
private:
    std::unique_ptr<DoctypeNode> doctype_;
    std::unique_ptr<HtmlElement> root_;
};

// HTML Builder - fluent interface for building HTML
class HtmlBuilder {
public:
    HtmlBuilder(const std::string& tag);
    
    HtmlBuilder& attr(const std::string& name, const std::string& value);
    HtmlBuilder& id(const std::string& id);
    HtmlBuilder& class_name(const std::string& class_name);
    HtmlBuilder& style(const std::string& property, const std::string& value);
    HtmlBuilder& text(const std::string& text);
    HtmlBuilder& child(std::unique_ptr<HtmlElement> element);
    HtmlBuilder& child(const std::string& tag, std::function<void(HtmlBuilder&)> fn);
    
    std::unique_ptr<HtmlElement> build();
    
private:
    std::unique_ptr<HtmlElement> element_;
};

// HTML Parser - simple HTML parsing
class HtmlParser {
public:
    HtmlParser();
    
    std::unique_ptr<HtmlDocument> parse(const std::string& html);
    std::unique_ptr<HtmlElement> parse_fragment(const std::string& html);
    
private:
    std::string html_;
    size_t pos_;
    
    void skip_whitespace();
    char peek() const;
    char consume();
    bool consume_if(char c);
    std::string consume_until(char c);
    std::string consume_until(const std::string& chars);
    
    std::unique_ptr<HtmlNode> parse_node();
    std::unique_ptr<HtmlElement> parse_element();
    std::unique_ptr<TextNode> parse_text();
    std::unique_ptr<CommentNode> parse_comment();
    std::unique_ptr<DoctypeNode> parse_doctype();
    
    std::string parse_tag_name();
    std::vector<HtmlAttribute> parse_attributes();
    std::string parse_attribute_value();
};

// HTML utilities
class HtmlUtils {
public:
    // Escaping
    static std::string escape(const std::string& text);
    static std::string unescape(const std::string& html);
    
    // Attribute escaping
    static std::string escape_attribute(const std::string& text);
    
    // Text extraction
    static std::string extract_text(const HtmlElement& element);
    static std::string extract_text(const HtmlDocument& doc);
    
    // Minification
    static std::string minify(const std::string& html);
    
    // Pretty printing
    static std::string pretty_print(const std::string& html, int indent_size = 2);
    
    // Validation
    static bool is_valid_tag_name(const std::string& tag);
    static bool is_valid_attribute_name(const std::string& name);
    
    // Common self-closing tags
    static bool is_self_closing_tag(const std::string& tag);
};

// Template processor - simple variable substitution
class HtmlTemplate {
public:
    HtmlTemplate(const std::string& template_string);
    
    void set_variable(const std::string& name, const std::string& value);
    void set_variables(const std::map<std::string, std::string>& variables);
    
    std::string render() const;
    
private:
    std::string template_;
    std::map<std::string, std::string> variables_;
};

} // namespace html
} // namespace networking
} // namespace ml
