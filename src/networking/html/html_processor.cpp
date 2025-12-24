#include "networking/html/html_processor.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <set>

namespace ml {
namespace networking {
namespace html {

// TextNode implementation
std::string TextNode::to_string(int indent) const {
    return text_;
}

std::unique_ptr<HtmlNode> TextNode::clone() const {
    return std::make_unique<TextNode>(text_);
}

// CommentNode implementation
std::string CommentNode::to_string(int indent) const {
    return "<!-- " + comment_ + " -->";
}

std::unique_ptr<HtmlNode> CommentNode::clone() const {
    return std::make_unique<CommentNode>(comment_);
}

// DoctypeNode implementation
std::string DoctypeNode::to_string(int indent) const {
    return "<!DOCTYPE " + doctype_ + ">";
}

std::unique_ptr<HtmlNode> DoctypeNode::clone() const {
    return std::make_unique<DoctypeNode>(doctype_);
}

// HtmlElement implementation
HtmlElement::HtmlElement(const std::string& tag_name)
    : tag_name_(tag_name)
    , self_closing_(false) {
    self_closing_ = is_self_closing();
}

std::string HtmlElement::to_string(int indent) const {
    std::stringstream ss;
    std::string indent_str(indent, ' ');
    
    ss << indent_str << "<" << tag_name_;
    
    // Attributes
    for (const auto& attr : attributes_) {
        ss << " " << attr.name << "=\"" << HtmlUtils::escape_attribute(attr.value) << "\"";
    }
    
    if (self_closing_) {
        ss << " />";
    } else {
        ss << ">";
        
        // Children
        if (!children_.empty()) {
            bool has_element_children = false;
            for (const auto& child : children_) {
                if (child->get_type() == NodeType::ELEMENT) {
                    has_element_children = true;
                    break;
                }
            }
            
            if (has_element_children) {
                ss << "\n";
                for (const auto& child : children_) {
                    ss << child->to_string(indent + 2);
                    if (child->get_type() == NodeType::ELEMENT) {
                        ss << "\n";
                    }
                }
                ss << indent_str;
            } else {
                for (const auto& child : children_) {
                    ss << child->to_string(0);
                }
            }
        }
        
        ss << "</" << tag_name_ << ">";
    }
    
    return ss.str();
}

std::unique_ptr<HtmlNode> HtmlElement::clone() const {
    auto element = std::make_unique<HtmlElement>(tag_name_);
    element->attributes_ = attributes_;
    element->self_closing_ = self_closing_;
    
    for (const auto& child : children_) {
        element->children_.push_back(child->clone());
    }
    
    return element;
}

void HtmlElement::set_attribute(const std::string& name, const std::string& value) {
    for (auto& attr : attributes_) {
        if (attr.name == name) {
            attr.value = value;
            return;
        }
    }
    attributes_.emplace_back(name, value);
}

std::string HtmlElement::get_attribute(const std::string& name) const {
    for (const auto& attr : attributes_) {
        if (attr.name == name) {
            return attr.value;
        }
    }
    return "";
}

bool HtmlElement::has_attribute(const std::string& name) const {
    for (const auto& attr : attributes_) {
        if (attr.name == name) {
            return true;
        }
    }
    return false;
}

void HtmlElement::remove_attribute(const std::string& name) {
    attributes_.erase(
        std::remove_if(attributes_.begin(), attributes_.end(),
            [&name](const HtmlAttribute& attr) { return attr.name == name; }),
        attributes_.end()
    );
}

void HtmlElement::add_class(const std::string& class_name) {
    std::string classes = get_attribute("class");
    if (!classes.empty()) {
        classes += " ";
    }
    classes += class_name;
    set_attribute("class", classes);
}

void HtmlElement::remove_class(const std::string& class_name) {
    std::string classes = get_attribute("class");
    size_t pos = classes.find(class_name);
    if (pos != std::string::npos) {
        classes.erase(pos, class_name.length());
        // Clean up extra spaces
        while (classes.find("  ") != std::string::npos) {
            classes.replace(classes.find("  "), 2, " ");
        }
        if (!classes.empty() && classes[0] == ' ') {
            classes.erase(0, 1);
        }
        if (!classes.empty() && classes.back() == ' ') {
            classes.pop_back();
        }
        set_attribute("class", classes);
    }
}

void HtmlElement::set_style(const std::string& property, const std::string& value) {
    std::string style = get_attribute("style");
    if (!style.empty() && style.back() != ';') {
        style += "; ";
    }
    style += property + ": " + value + ";";
    set_attribute("style", style);
}

void HtmlElement::add_child(std::unique_ptr<HtmlNode> child) {
    children_.push_back(std::move(child));
}

void HtmlElement::add_text(const std::string& text) {
    children_.push_back(std::make_unique<TextNode>(text));
}

void HtmlElement::add_element(std::unique_ptr<HtmlElement> element) {
    children_.push_back(std::move(element));
}

bool HtmlElement::is_self_closing() const {
    return HtmlUtils::is_self_closing_tag(tag_name_);
}

std::vector<HtmlElement*> HtmlElement::find_by_tag(const std::string& tag) {
    std::vector<HtmlElement*> results;
    find_by_tag_recursive(tag, results);
    return results;
}

std::vector<HtmlElement*> HtmlElement::find_by_class(const std::string& class_name) {
    std::vector<HtmlElement*> results;
    find_by_class_recursive(class_name, results);
    return results;
}

HtmlElement* HtmlElement::find_by_id(const std::string& id) {
    return find_by_id_recursive(id);
}

void HtmlElement::find_by_tag_recursive(const std::string& tag, std::vector<HtmlElement*>& results) {
    if (tag_name_ == tag) {
        results.push_back(this);
    }
    
    for (auto& child : children_) {
        if (child->get_type() == NodeType::ELEMENT) {
            static_cast<HtmlElement*>(child.get())->find_by_tag_recursive(tag, results);
        }
    }
}

void HtmlElement::find_by_class_recursive(const std::string& class_name, std::vector<HtmlElement*>& results) {
    std::string classes = get_attribute("class");
    if (classes.find(class_name) != std::string::npos) {
        results.push_back(this);
    }
    
    for (auto& child : children_) {
        if (child->get_type() == NodeType::ELEMENT) {
            static_cast<HtmlElement*>(child.get())->find_by_class_recursive(class_name, results);
        }
    }
}

HtmlElement* HtmlElement::find_by_id_recursive(const std::string& id) {
    if (get_attribute("id") == id) {
        return this;
    }
    
    for (auto& child : children_) {
        if (child->get_type() == NodeType::ELEMENT) {
            HtmlElement* result = static_cast<HtmlElement*>(child.get())->find_by_id_recursive(id);
            if (result) {
                return result;
            }
        }
    }
    
    return nullptr;
}

// HtmlDocument implementation
HtmlDocument::HtmlDocument()
    : doctype_(std::make_unique<DoctypeNode>("html"))
    , root_(std::make_unique<HtmlElement>("html")) {
}

void HtmlDocument::set_doctype(const std::string& doctype) {
    doctype_ = std::make_unique<DoctypeNode>(doctype);
}

void HtmlDocument::set_root(std::unique_ptr<HtmlElement> root) {
    root_ = std::move(root);
}

HtmlElement* HtmlDocument::get_head() {
    if (!root_) return nullptr;
    
    auto heads = root_->find_by_tag("head");
    if (!heads.empty()) {
        return heads[0];
    }
    
    // Create head if it doesn't exist
    auto head = std::make_unique<HtmlElement>("head");
    HtmlElement* head_ptr = head.get();
    root_->add_element(std::move(head));
    return head_ptr;
}

HtmlElement* HtmlDocument::get_body() {
    if (!root_) return nullptr;
    
    auto bodies = root_->find_by_tag("body");
    if (!bodies.empty()) {
        return bodies[0];
    }
    
    // Create body if it doesn't exist
    auto body = std::make_unique<HtmlElement>("body");
    HtmlElement* body_ptr = body.get();
    root_->add_element(std::move(body));
    return body_ptr;
}

void HtmlDocument::set_title(const std::string& title) {
    auto head = get_head();
    if (!head) return;
    
    auto titles = head->find_by_tag("title");
    if (!titles.empty()) {
        // Update existing title
        titles[0]->add_text(title);
    } else {
        // Create new title
        auto title_elem = std::make_unique<HtmlElement>("title");
        title_elem->add_text(title);
        head->add_element(std::move(title_elem));
    }
}

void HtmlDocument::add_meta(const std::string& name, const std::string& content) {
    auto head = get_head();
    if (!head) return;
    
    auto meta = std::make_unique<HtmlElement>("meta");
    meta->set_attribute("name", name);
    meta->set_attribute("content", content);
    head->add_element(std::move(meta));
}

void HtmlDocument::add_stylesheet(const std::string& href) {
    auto head = get_head();
    if (!head) return;
    
    auto link = std::make_unique<HtmlElement>("link");
    link->set_attribute("rel", "stylesheet");
    link->set_attribute("href", href);
    head->add_element(std::move(link));
}

void HtmlDocument::add_script(const std::string& src) {
    auto head = get_head();
    if (!head) return;
    
    auto script = std::make_unique<HtmlElement>("script");
    script->set_attribute("src", src);
    head->add_element(std::move(script));
}

std::string HtmlDocument::to_string() const {
    std::stringstream ss;
    ss << doctype_->to_string() << "\n";
    ss << root_->to_string(0);
    return ss.str();
}

std::string HtmlDocument::to_string_pretty(int indent_size) const {
    return to_string();  // Already formatted
}

// HtmlBuilder implementation
HtmlBuilder::HtmlBuilder(const std::string& tag)
    : element_(std::make_unique<HtmlElement>(tag)) {
}

HtmlBuilder& HtmlBuilder::attr(const std::string& name, const std::string& value) {
    element_->set_attribute(name, value);
    return *this;
}

HtmlBuilder& HtmlBuilder::id(const std::string& id) {
    element_->set_id(id);
    return *this;
}

HtmlBuilder& HtmlBuilder::class_name(const std::string& class_name) {
    element_->add_class(class_name);
    return *this;
}

HtmlBuilder& HtmlBuilder::style(const std::string& property, const std::string& value) {
    element_->set_style(property, value);
    return *this;
}

HtmlBuilder& HtmlBuilder::text(const std::string& text) {
    element_->add_text(text);
    return *this;
}

HtmlBuilder& HtmlBuilder::child(std::unique_ptr<HtmlElement> element) {
    element_->add_element(std::move(element));
    return *this;
}

HtmlBuilder& HtmlBuilder::child(const std::string& tag, std::function<void(HtmlBuilder&)> fn) {
    HtmlBuilder builder(tag);
    fn(builder);
    element_->add_element(builder.build());
    return *this;
}

std::unique_ptr<HtmlElement> HtmlBuilder::build() {
    return std::move(element_);
}

// HtmlParser implementation
HtmlParser::HtmlParser() : pos_(0) {
}

std::unique_ptr<HtmlDocument> HtmlParser::parse(const std::string& html) {
    html_ = html;
    pos_ = 0;
    
    auto doc = std::make_unique<HtmlDocument>();
    
    // Simple parsing - just look for root element
    skip_whitespace();
    
    // Skip doctype if present
    if (html_.substr(pos_, 9) == "<!DOCTYPE") {
        consume_until('>');
        consume();
    }
    
    skip_whitespace();
    
    // Parse root element
    if (peek() == '<') {
        auto root = parse_element();
        if (root) {
            doc->set_root(std::move(root));
        }
    }
    
    return doc;
}

std::unique_ptr<HtmlElement> HtmlParser::parse_fragment(const std::string& html) {
    html_ = html;
    pos_ = 0;
    
    skip_whitespace();
    
    if (peek() == '<') {
        return parse_element();
    }
    
    return nullptr;
}

void HtmlParser::skip_whitespace() {
    while (pos_ < html_.length() && std::isspace(html_[pos_])) {
        pos_++;
    }
}

char HtmlParser::peek() const {
    return pos_ < html_.length() ? html_[pos_] : '\0';
}

char HtmlParser::consume() {
    return pos_ < html_.length() ? html_[pos_++] : '\0';
}

bool HtmlParser::consume_if(char c) {
    if (peek() == c) {
        consume();
        return true;
    }
    return false;
}

std::string HtmlParser::consume_until(char c) {
    std::string result;
    while (pos_ < html_.length() && html_[pos_] != c) {
        result += html_[pos_++];
    }
    return result;
}

std::string HtmlParser::consume_until(const std::string& chars) {
    std::string result;
    while (pos_ < html_.length() && chars.find(html_[pos_]) == std::string::npos) {
        result += html_[pos_++];
    }
    return result;
}

std::unique_ptr<HtmlNode> HtmlParser::parse_node() {
    skip_whitespace();
    
    if (peek() == '<') {
        consume();
        if (peek() == '!') {
            consume();
            if (peek() == '-') {
                return parse_comment();
            }
        } else if (peek() != '/') {
            pos_--;  // Put back '<'
            return parse_element();
        }
    } else {
        return parse_text();
    }
    
    return nullptr;
}

std::unique_ptr<HtmlElement> HtmlParser::parse_element() {
    if (!consume_if('<')) {
        return nullptr;
    }
    
    std::string tag = parse_tag_name();
    if (tag.empty()) {
        return nullptr;
    }
    
    auto element = std::make_unique<HtmlElement>(tag);
    
    skip_whitespace();
    
    // Parse attributes
    auto attrs = parse_attributes();
    for (const auto& attr : attrs) {
        element->set_attribute(attr.name, attr.value);
    }
    
    skip_whitespace();
    
    // Self-closing tag
    if (consume_if('/')) {
        consume_if('>');
        return element;
    }
    
    consume_if('>');
    
    // Parse children until closing tag
    while (pos_ < html_.length()) {
        skip_whitespace();
        
        if (peek() == '<' && pos_ + 1 < html_.length() && html_[pos_ + 1] == '/') {
            // Closing tag
            consume();  // '<'
            consume();  // '/'
            consume_until('>');
            consume();  // '>'
            break;
        }
        
        auto child = parse_node();
        if (child) {
            element->add_child(std::move(child));
        } else {
            break;
        }
    }
    
    return element;
}

std::unique_ptr<TextNode> HtmlParser::parse_text() {
    std::string text = consume_until('<');
    if (!text.empty()) {
        return std::make_unique<TextNode>(HtmlUtils::unescape(text));
    }
    return nullptr;
}

std::unique_ptr<CommentNode> HtmlParser::parse_comment() {
    consume();  // second '-'
    std::string comment = consume_until('-');
    consume();  // '-'
    consume();  // '-'
    consume();  // '>'
    return std::make_unique<CommentNode>(comment);
}

std::unique_ptr<DoctypeNode> HtmlParser::parse_doctype() {
    return std::make_unique<DoctypeNode>("html");
}

std::string HtmlParser::parse_tag_name() {
    std::string tag;
    while (pos_ < html_.length() && 
           (std::isalnum(html_[pos_]) || html_[pos_] == '-' || html_[pos_] == '_')) {
        tag += html_[pos_++];
    }
    return tag;
}

std::vector<HtmlAttribute> HtmlParser::parse_attributes() {
    std::vector<HtmlAttribute> attrs;
    
    while (pos_ < html_.length() && peek() != '>' && peek() != '/') {
        skip_whitespace();
        
        std::string name = consume_until(" =>/");
        if (name.empty()) break;
        
        skip_whitespace();
        
        std::string value;
        if (consume_if('=')) {
            skip_whitespace();
            value = parse_attribute_value();
        }
        
        attrs.emplace_back(name, value);
        skip_whitespace();
    }
    
    return attrs;
}

std::string HtmlParser::parse_attribute_value() {
    std::string value;
    
    if (consume_if('"')) {
        value = consume_until('"');
        consume_if('"');
    } else if (consume_if('\'')) {
        value = consume_until('\'');
        consume_if('\'');
    } else {
        value = consume_until(" >/");
    }
    
    return HtmlUtils::unescape(value);
}

// HtmlUtils implementation
std::string HtmlUtils::escape(const std::string& text) {
    std::string result;
    for (char c : text) {
        switch (c) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c;
        }
    }
    return result;
}

std::string HtmlUtils::unescape(const std::string& html) {
    std::string result;
    size_t pos = 0;
    
    while (pos < html.length()) {
        if (html[pos] == '&') {
            size_t end = html.find(';', pos);
            if (end != std::string::npos) {
                std::string entity = html.substr(pos + 1, end - pos - 1);
                if (entity == "lt") result += '<';
                else if (entity == "gt") result += '>';
                else if (entity == "amp") result += '&';
                else if (entity == "quot") result += '"';
                else if (entity == "#39") result += '\'';
                else result += html.substr(pos, end - pos + 1);
                pos = end + 1;
            } else {
                result += html[pos++];
            }
        } else {
            result += html[pos++];
        }
    }
    
    return result;
}

std::string HtmlUtils::escape_attribute(const std::string& text) {
    return escape(text);
}

std::string HtmlUtils::extract_text(const HtmlElement& element) {
    std::string text;
    
    for (const auto& child : element.get_children()) {
        if (child->get_type() == NodeType::TEXT) {
            text += static_cast<const TextNode*>(child.get())->get_text();
        } else if (child->get_type() == NodeType::ELEMENT) {
            text += extract_text(*static_cast<const HtmlElement*>(child.get()));
        }
    }
    
    return text;
}

std::string HtmlUtils::extract_text(const HtmlDocument& doc) {
    if (doc.get_root()) {
        return extract_text(*doc.get_root());
    }
    return "";
}

std::string HtmlUtils::minify(const std::string& html) {
    std::string result;
    bool in_tag = false;
    bool prev_space = false;
    
    for (char c : html) {
        if (c == '<') {
            in_tag = true;
            result += c;
        } else if (c == '>') {
            in_tag = false;
            result += c;
        } else if (std::isspace(c)) {
            if (!prev_space && !in_tag) {
                result += ' ';
                prev_space = true;
            }
        } else {
            result += c;
            prev_space = false;
        }
    }
    
    return result;
}

std::string HtmlUtils::pretty_print(const std::string& html, int indent_size) {
    // Simple implementation - just add newlines after tags
    return html;
}

bool HtmlUtils::is_valid_tag_name(const std::string& tag) {
    if (tag.empty()) return false;
    
    for (char c : tag) {
        if (!std::isalnum(c) && c != '-' && c != '_') {
            return false;
        }
    }
    
    return true;
}

bool HtmlUtils::is_valid_attribute_name(const std::string& name) {
    return is_valid_tag_name(name);
}

bool HtmlUtils::is_self_closing_tag(const std::string& tag) {
    static const std::set<std::string> self_closing = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr"
    };
    
    return self_closing.find(tag) != self_closing.end();
}

// HtmlTemplate implementation
HtmlTemplate::HtmlTemplate(const std::string& template_string)
    : template_(template_string) {
}

void HtmlTemplate::set_variable(const std::string& name, const std::string& value) {
    variables_[name] = value;
}

void HtmlTemplate::set_variables(const std::map<std::string, std::string>& variables) {
    for (const auto& [name, value] : variables) {
        variables_[name] = value;
    }
}

std::string HtmlTemplate::render() const {
    std::string result = template_;
    
    for (const auto& [name, value] : variables_) {
        std::string placeholder = "{{" + name + "}}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
    
    return result;
}

} // namespace html
} // namespace networking
} // namespace ml
