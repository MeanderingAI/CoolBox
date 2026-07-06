#include "xml_screen_descriptor_parser.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace os_generics {
namespace xml_screen_descriptor {
namespace {

struct XmlNode {
    std::string name;
    std::unordered_map<std::string, std::string> attrs;
    std::vector<XmlNode> children;
};

std::string trim(const std::string& text) {
    std::size_t first = 0;
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first])) != 0) {
        ++first;
    }
    std::size_t last = text.size();
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1])) != 0) {
        --last;
    }
    return text.substr(first, last - first);
}

std::string lower_copy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool parse_bool(const std::string& value, bool fallback) {
    const std::string v = lower_copy(trim(value));
    if (v == "true" || v == "1" || v == "yes" || v == "on") {
        return true;
    }
    if (v == "false" || v == "0" || v == "no" || v == "off") {
        return false;
    }
    return fallback;
}

std::size_t parse_size_t(const std::string& value, std::size_t fallback) {
    try {
        std::size_t pos = 0;
        const auto parsed = std::stoul(trim(value), &pos, 10);
        if (pos == trim(value).size()) {
            return static_cast<std::size_t>(parsed);
        }
    } catch (...) {
    }
    return fallback;
}

std::vector<std::string> split_csv(const std::string& value) {
    std::vector<std::string> out;
    std::string token;
    std::stringstream ss(value);
    while (std::getline(ss, token, ',')) {
        const std::string t = trim(token);
        if (!t.empty()) {
            out.push_back(t);
        }
    }
    return out;
}

std::vector<bool> split_csv_bools(const std::string& value) {
    std::vector<bool> out;
    for (const auto& token : split_csv(value)) {
        out.push_back(parse_bool(token, false));
    }
    return out;
}

std::string attr_or(const XmlNode& node, const std::string& key, const std::string& fallback = "") {
    const auto it = node.attrs.find(key);
    if (it == node.attrs.end()) {
        return fallback;
    }
    return it->second;
}

::graphics::windows::PlatformStyle parse_platform(const std::string& value) {
    const std::string platform = lower_copy(value);
    if (platform == "mac" || platform == "macos") {
        return ::graphics::windows::PlatformStyle::MacOS;
    }
    if (platform == "linux") {
        return ::graphics::windows::PlatformStyle::Linux;
    }
    return ::graphics::windows::PlatformStyle::Windows;
}

::graphics::components::BorderStyle parse_border_style(const std::string& value) {
    const std::string style = lower_copy(trim(value));
    if (style == "solid") {
        return ::graphics::components::BorderStyle::Solid;
    }
    if (style == "dashed") {
        return ::graphics::components::BorderStyle::Dashed;
    }
    if (style == "double") {
        return ::graphics::components::BorderStyle::Double;
    }
    return ::graphics::components::BorderStyle::None;
}

bool parse_tag(const std::string& xml_text,
               std::size_t& cursor,
               std::string& tag_name,
               std::unordered_map<std::string, std::string>& attrs,
               bool& closing,
               bool& self_closing,
               std::vector<std::string>& errors) {
    const std::size_t open = xml_text.find('<', cursor);
    if (open == std::string::npos) {
        return false;
    }
    const std::size_t close = xml_text.find('>', open + 1);
    if (close == std::string::npos) {
        errors.push_back("Malformed XML: missing '>'");
        return false;
    }

    cursor = close + 1;
    std::string inside = trim(xml_text.substr(open + 1, close - open - 1));
    if (inside.empty()) {
        return true;
    }

    if (inside[0] == '!' || inside[0] == '?') {
        return true;
    }

    closing = false;
    self_closing = false;
    if (inside[0] == '/') {
        closing = true;
        inside = trim(inside.substr(1));
    }

    if (!inside.empty() && inside.back() == '/') {
        self_closing = true;
        inside = trim(inside.substr(0, inside.size() - 1));
    }

    attrs.clear();
    tag_name.clear();

    std::size_t p = 0;
    while (p < inside.size() && std::isspace(static_cast<unsigned char>(inside[p])) == 0) {
        tag_name.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(inside[p]))));
        ++p;
    }

    while (p < inside.size()) {
        while (p < inside.size() && std::isspace(static_cast<unsigned char>(inside[p])) != 0) {
            ++p;
        }
        if (p >= inside.size()) {
            break;
        }

        std::string key;
        while (p < inside.size() && inside[p] != '=' && std::isspace(static_cast<unsigned char>(inside[p])) == 0) {
            key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(inside[p]))));
            ++p;
        }
        while (p < inside.size() && std::isspace(static_cast<unsigned char>(inside[p])) != 0) {
            ++p;
        }
        if (p >= inside.size() || inside[p] != '=') {
            errors.push_back("Malformed XML attribute near tag: " + tag_name);
            break;
        }
        ++p;
        while (p < inside.size() && std::isspace(static_cast<unsigned char>(inside[p])) != 0) {
            ++p;
        }
        if (p >= inside.size()) {
            errors.push_back("Missing XML attribute value near tag: " + tag_name);
            break;
        }

        char quote = 0;
        if (inside[p] == '"' || inside[p] == '\'') {
            quote = inside[p++];
        }

        std::string value;
        while (p < inside.size()) {
            if (quote != 0) {
                if (inside[p] == quote) {
                    ++p;
                    break;
                }
            } else if (std::isspace(static_cast<unsigned char>(inside[p])) != 0) {
                break;
            }
            value.push_back(inside[p++]);
        }
        attrs[key] = value;
    }

    return true;
}

XmlNode parse_xml_tree(const std::string& xml_text, std::vector<std::string>& errors) {
    XmlNode root;
    root.name = "__root";
    std::vector<XmlNode*> stack;
    stack.push_back(&root);

    std::size_t cursor = 0;
    while (cursor < xml_text.size()) {
        std::string tag_name;
        std::unordered_map<std::string, std::string> attrs;
        bool closing = false;
        bool self_closing = false;
        const std::size_t previous_cursor = cursor;

        if (!parse_tag(xml_text, cursor, tag_name, attrs, closing, self_closing, errors)) {
            break;
        }
        if (cursor == previous_cursor) {
            break;
        }
        if (tag_name.empty()) {
            continue;
        }

        if (closing) {
            if (stack.size() <= 1U || stack.back()->name != tag_name) {
                errors.push_back("Mismatched closing tag: " + tag_name);
                continue;
            }
            stack.pop_back();
            continue;
        }

        XmlNode node;
        node.name = tag_name;
        node.attrs = std::move(attrs);

        stack.back()->children.push_back(std::move(node));
        XmlNode* inserted = &stack.back()->children.back();
        if (!self_closing) {
            stack.push_back(inserted);
        }
    }

    if (stack.size() != 1U) {
        errors.push_back("Malformed XML: unclosed tags");
    }

    return root;
}

::graphics::components::Component component_from_node(const XmlNode& node,
                                                      std::vector<std::string>& errors);

::graphics::components::ComponentHolder holder_from_node(const XmlNode& node,
                                                         std::vector<std::string>& errors);

::graphics::components::ComponentHolder tab_subview_from_node(const XmlNode& tab_node,
                                                              std::vector<std::string>& errors) {
    using namespace ::graphics::components;

    if (tab_node.children.empty()) {
        return ComponentHolder::vertical();
    }

    if (tab_node.children.size() == 1U) {
        const std::string only_name = lower_copy(tab_node.children.front().name);
        if (only_name == "layout" || only_name == "vertical" || only_name == "horizontal" || only_name == "grid") {
            return holder_from_node(tab_node.children.front(), errors);
        }
    }

    ComponentHolder holder = ComponentHolder::vertical();
    for (const auto& child : tab_node.children) {
        const std::string child_name = lower_copy(child.name);
        if (child_name == "menu" || child_name == "item") {
            continue;
        }
        holder.add_component(component_from_node(child, errors));
    }
    return holder;
}

::graphics::components::ComponentHolder holder_from_node(const XmlNode& node,
                                                         std::vector<std::string>& errors) {
    using namespace ::graphics::components;

    const std::string node_name = lower_copy(node.name);
    LayoutType layout_type = LayoutType::Vertical;
    std::size_t columns = parse_size_t(attr_or(node, "columns", "2"), 2U);
    std::size_t h_spacing = parse_size_t(attr_or(node, "hspacing", "3"), 3U);
    std::size_t v_spacing = parse_size_t(attr_or(node, "vspacing", "1"), 1U);

    if (node_name == "horizontal") {
        layout_type = LayoutType::Horizontal;
    } else if (node_name == "grid") {
        layout_type = LayoutType::Grid;
    }

    ComponentHolder holder = (layout_type == LayoutType::Horizontal)
        ? ComponentHolder::horizontal({}, h_spacing)
        : (layout_type == LayoutType::Grid)
            ? ComponentHolder::grid({}, columns, h_spacing, v_spacing)
            : ComponentHolder::vertical({}, v_spacing);

    for (const auto& child : node.children) {
        const std::string child_name = lower_copy(child.name);
        if (child_name == "menu" || child_name == "item") {
            continue;
        }
        holder.add_component(component_from_node(child, errors));
    }
    return holder;
}

::graphics::components::MenuModel menu_from_node(const XmlNode& node, std::vector<std::string>& errors) {
    ::graphics::components::MenuModel menu(attr_or(node, "title", "Menu"));
    for (const auto& child : node.children) {
        if (lower_copy(child.name) != "item") {
            errors.push_back("menu supports only <item> children; got <" + child.name + ">");
            continue;
        }
        const std::string label = attr_or(child, "label", "Item");
        const std::string shortcut = attr_or(child, "shortcut", "");
        const bool enabled = parse_bool(attr_or(child, "enabled", "true"), true);
        const bool checked = parse_bool(attr_or(child, "checked", "false"), false);
        const bool separator = parse_bool(attr_or(child, "separator", "false"), false);
        if (separator) {
            menu.add_item(::graphics::components::MenuItem::divider());
        } else {
            menu.add_item(::graphics::components::MenuItem::action(label, shortcut, enabled, checked));
        }
    }
    return menu;
}

::graphics::components::Component component_from_node(const XmlNode& node,
                                                      std::vector<std::string>& errors) {
    using namespace ::graphics::components;
    const std::string node_name = lower_copy(node.name);

    if (node_name == "button") {
        return Component::button(
            attr_or(node, "label", "Button"),
            parse_bool(attr_or(node, "enabled", "true"), true),
            parse_bool(attr_or(node, "pressed", "false"), false),
            parse_size_t(attr_or(node, "width", "0"), 0),
            parse_bool(attr_or(node, "round_top_left", attr_or(node, "round-top-left", "false")), false),
            parse_bool(attr_or(node, "round_top_right", attr_or(node, "round-top-right", "false")), false),
            parse_bool(attr_or(node, "round_bottom_right", attr_or(node, "round-bottom-right", "false")), false),
            parse_bool(attr_or(node, "round_bottom_left", attr_or(node, "round-bottom-left", "false")), false));
    }
    if (node_name == "text" || node_name == "text_view") {
        return Component::text_view(
            attr_or(node, "text", ""),
            parse_size_t(attr_or(node, "width", "32"), 32),
            parse_bool(attr_or(node, "wrap", attr_or(node, "wrap_text", "true")), true),
            parse_border_style(attr_or(node, "border_style", attr_or(node, "border-style", "none"))),
            parse_size_t(attr_or(node, "border_thickness", attr_or(node, "border-thickness", "0")), 0),
            parse_bool(attr_or(node, "line_numbers", attr_or(node, "line-numbers", "false")), false));
    }
    if (node_name == "editable_text" || node_name == "editable_text_view") {
        return Component::editable_text_view(
            attr_or(node, "text", ""),
            parse_size_t(attr_or(node, "cursor", "0"), 0),
            parse_bool(attr_or(node, "focused", "true"), true),
            parse_size_t(attr_or(node, "width", "32"), 32),
            parse_bool(attr_or(node, "wrap", attr_or(node, "wrap_text", "false")), false),
            parse_border_style(attr_or(node, "border_style", attr_or(node, "border-style", "none"))),
            parse_size_t(attr_or(node, "border_thickness", attr_or(node, "border-thickness", "0")), 0),
            parse_bool(attr_or(node, "line_numbers", attr_or(node, "line-numbers", "false")), false),
            parse_bool(attr_or(node, "resizable", "false"), false),
            parse_size_t(attr_or(node, "max_height", attr_or(node, "max-height", "1")), 1),
            parse_size_t(attr_or(node, "font_size", attr_or(node, "font-size", "14")), 14),
            split_csv(attr_or(node, "autocomplete", attr_or(node, "recommendations", ""))));
    }
    if (node_name == "radio") {
        return Component::radio_button(
            attr_or(node, "label", "Option"),
            parse_bool(attr_or(node, "selected", "false"), false),
            parse_bool(attr_or(node, "enabled", "true"), true));
    }
    if (node_name == "checkbox") {
        return Component::check_box(
            attr_or(node, "label", "Check"),
            parse_bool(attr_or(node, "checked", "false"), false),
            parse_bool(attr_or(node, "enabled", "true"), true));
    }
    if (node_name == "toolbar") {
        ToolbarModel toolbar(split_csv(attr_or(node, "actions", "")));
        return Component::toolbar(toolbar);
    }
    if (node_name == "dock_panel") {
        DockPanelModel dock(attr_or(node, "title", "Dock"), parse_bool(attr_or(node, "floating", "false"), false));
        return Component::dock_panel(dock);
    }
    if (node_name == "layer_list") {
        LayerListModel layers(split_csv(attr_or(node, "layers", "")), parse_size_t(attr_or(node, "selected", "0"), 0));
        return Component::layer_list(layers);
    }
    if (node_name == "property_inspector") {
        PropertyInspectorModel inspector;
        for (const auto& pair : split_csv(attr_or(node, "properties", ""))) {
            const std::size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                inspector.add_property(trim(pair.substr(0, eq)), trim(pair.substr(eq + 1)));
            }
        }
        return Component::property_inspector(inspector);
    }
    if (node_name == "radio_selector") {
        RadioSelectorModel model(split_csv(attr_or(node, "options", "")), parse_size_t(attr_or(node, "selected", "0"), 0));
        return Component::radio_selector(model);
    }
    if (node_name == "checkbox_group") {
        const auto options = split_csv(attr_or(node, "options", ""));
        auto checks = split_csv_bools(attr_or(node, "checked", ""));
        if (checks.size() < options.size()) {
            checks.resize(options.size(), false);
        }
        CheckboxGroupModel model(options, checks);
        return Component::checkbox_group(model);
    }
    if (node_name == "menu_bar") {
        MenuBarModel menu_bar;
        for (const auto& child : node.children) {
            if (lower_copy(child.name) == "menu") {
                menu_bar.add_menu(menu_from_node(child, errors));
            }
        }
        return Component::menu_bar(menu_bar);
    }
    if (node_name == "tabbed_view") {
        TabbedViewModel model;
        model.set_selected(parse_size_t(attr_or(node, "selected", "0"), 0));
        for (const auto& child : node.children) {
            if (lower_copy(child.name) != "tab") {
                errors.push_back("tabbed_view supports only <tab> children; got <" + child.name + ">");
                continue;
            }
            const std::string title = attr_or(child, "title", "Tab");
            model.add_tab(title, tab_subview_from_node(child, errors));
        }
        return Component::tabbed_view(model);
    }
    if (node_name == "dropdown") {
        MenuModel dropdown(attr_or(node, "title", "Dropdown"));
        for (const auto& child : node.children) {
            if (lower_copy(child.name) == "item") {
                const bool separator = parse_bool(attr_or(child, "separator", "false"), false);
                if (separator) {
                    dropdown.add_item(MenuItem::divider());
                } else {
                    dropdown.add_item(MenuItem::action(
                        attr_or(child, "label", "Item"),
                        attr_or(child, "shortcut", ""),
                        parse_bool(attr_or(child, "enabled", "true"), true),
                        parse_bool(attr_or(child, "checked", "false"), false)));
                }
            }
        }
        return Component::dropdown_menu(dropdown);
    }

    if (node_name == "layout" || node_name == "vertical" || node_name == "horizontal" || node_name == "grid") {
        return Component::layout_group(holder_from_node(node, errors), attr_or(node, "label", ""));
    }

    errors.push_back("Unknown component tag: <" + node.name + ">. Falling back to text_view.");
    return Component::text_view("Unsupported tag: " + node.name, 48);
}

} // namespace

ParseResult XmlScreenDescriptorParser::parse_string(const std::string& xml_text) const {
    ParseResult result;
    if (trim(xml_text).empty()) {
        result.errors.push_back("XML input is empty.");
        return result;
    }

    std::vector<std::string> errors;
    const XmlNode tree = parse_xml_tree(xml_text, errors);
    if (!errors.empty()) {
        result.errors = std::move(errors);
        return result;
    }

    const XmlNode* screen = nullptr;
    for (const auto& child : tree.children) {
        if (lower_copy(child.name) == "screen") {
            screen = &child;
            break;
        }
    }

    if (screen == nullptr) {
        result.errors.push_back("Missing <screen> root element.");
        return result;
    }

    result.descriptor.title = attr_or(*screen, "title", result.descriptor.title);
    result.descriptor.width = parse_size_t(attr_or(*screen, "width", "960"), 960);
    result.descriptor.height = parse_size_t(attr_or(*screen, "height", "640"), 640);
    result.descriptor.platform = parse_platform(attr_or(*screen, "platform", "windows"));

    if (screen->children.empty()) {
        result.errors.push_back("<screen> has no child layout/components.");
        return result;
    }

    const XmlNode* layout_node = nullptr;
    if (screen->children.size() == 1U) {
        const auto only_name = lower_copy(screen->children.front().name);
        if (only_name == "layout" || only_name == "vertical" || only_name == "horizontal" || only_name == "grid") {
            layout_node = &screen->children.front();
        }
    }

    if (layout_node != nullptr) {
        result.descriptor.layout = holder_from_node(*layout_node, result.errors);
    } else {
        XmlNode synthetic_layout;
        synthetic_layout.name = attr_or(*screen, "layout", "vertical");
        synthetic_layout.attrs = screen->attrs;
        synthetic_layout.children = screen->children;
        result.descriptor.layout = holder_from_node(synthetic_layout, result.errors);
    }

    return result;
}

ParseResult XmlScreenDescriptorParser::parse_file(const std::string& path) const {
    std::ifstream in(path);
    if (!in) {
        ParseResult result;
        result.errors.push_back("Failed to open XML file: " + path);
        return result;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return parse_string(buffer.str());
}

} // namespace xml_screen_descriptor
} // namespace os_generics
