#include "components.hpp"
#include "file_editor.hpp"
#include "full_application_window.hpp"
#include "ios_chrome.hpp"
#include "xml_screen_descriptor_parser.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#ifndef WINDOWS_LEAN_AND_MEAN
#define WINDOWS_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

namespace {

struct Options {
    std::string xml_path = "_deliverables/apps/screen_builder/app_layout.xml";
    bool force_cli = false;
};

std::string trim(const std::string& input) {
    std::size_t begin = 0;
    while (begin < input.size() && std::isspace(static_cast<unsigned char>(input[begin])) != 0) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
        --end;
    }

    return input.substr(begin, end - begin);
}

bool parse_int(const std::string& value, int& out) {
    try {
        std::size_t pos = 0;
        out = std::stoi(value, &pos);
        return pos == value.size();
    } catch (...) {
        return false;
    }
}

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string xml_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default: out.push_back(ch); break;
        }
    }
    return out;
}

Options parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "--xml" || arg == "-x") && i + 1 < argc) {
            options.xml_path = argv[++i];
            continue;
        }
        if (arg == "--cli") {
            options.force_cli = true;
            continue;
        }
        if (arg == "--help" || arg == "-h") {
            std::cout << "screen_builder\n";
            std::cout << "Usage: screen_builder [--xml <path>] [--cli]\n";
            std::exit(0);
        }
        std::cerr << "Unknown argument: " << arg << "\n";
        std::exit(2);
    }
    return options;
}

void print_help() {
    std::cout << "Commands:\n";
    std::cout << "  help                                Show this help\n";
    std::cout << "  view [start] [count]                Show XML lines\n";
    std::cout << "  set <line> <text>                   Replace line text\n";
    std::cout << "  insert <line> <text>                Insert line before index\n";
    std::cout << "  delete <line>                       Delete line\n";
    std::cout << "  save                                Save XML to disk\n";
    std::cout << "  reload                              Reload XML from disk\n";
    std::cout << "  preview                             Open preview window\n";
    std::cout << "  quit                                Exit\n";
}

void print_lines(const app_assets::file_editor::TextFileEditor& editor, int start, int count) {
    const auto lines = editor.view_lines(start, count);
    if (lines.empty()) {
        std::cout << "(empty)\n";
        return;
    }
    for (const auto& line : lines) {
        std::cout << line << "\n";
    }
}

void collect_components_recursive(
    const graphics::components::ComponentHolder& holder,
    std::vector<const graphics::components::Component*>& out_components) {
    for (const auto& component : holder.components()) {
        out_components.push_back(&component);
        if (component.type() == graphics::components::ComponentType::LayoutGroup) {
            const auto* nested = component.layout_group();
            if (nested != nullptr) {
                collect_components_recursive(*nested, out_components);
            }
        }
    }
}

struct Button {
    int x0;
    int y0;
    int x1;
    int y1;
    std::string label;
};

bool point_in_button(const Button& button, int x, int y) {
    return x >= button.x0 && x <= button.x1 && y >= button.y0 && y <= button.y1;
}

#if defined(_WIN32)
bool key_edge_down(int vk, std::array<bool, 256>& previous) {
    const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool edge = down && !previous[static_cast<std::size_t>(vk)];
    previous[static_cast<std::size_t>(vk)] = down;
    return edge;
}

char vk_to_char(int vk, bool shift_down) {
    if (vk >= 'A' && vk <= 'Z') {
        return static_cast<char>(shift_down ? vk : (vk - 'A' + 'a'));
    }
    if (vk >= '0' && vk <= '9') {
        static const char shifted_digits[] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '('};
        return shift_down ? shifted_digits[vk - '0'] : static_cast<char>(vk);
    }
    switch (vk) {
        case VK_SPACE: return ' ';
        case VK_OEM_MINUS: return shift_down ? '_' : '-';
        case VK_OEM_PLUS: return shift_down ? '+' : '=';
        case VK_OEM_COMMA: return shift_down ? '<' : ',';
        case VK_OEM_PERIOD: return shift_down ? '>' : '.';
        case VK_OEM_1: return shift_down ? ':' : ';';
        case VK_OEM_2: return shift_down ? '?' : '/';
        case VK_OEM_3: return shift_down ? '~' : '`';
        case VK_OEM_4: return shift_down ? '{' : '[';
        case VK_OEM_5: return shift_down ? '|' : '\\';
        case VK_OEM_6: return shift_down ? '}' : ']';
        case VK_OEM_7: return shift_down ? '"' : '\'';
        default: return '\0';
    }
}

bool apply_text_box_input(std::string& text, std::array<bool, 256>& previous) {
    bool changed = false;
    const bool shift_down = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    if (key_edge_down(VK_BACK, previous)) {
        if (!text.empty()) {
            text.pop_back();
            changed = true;
        }
    }

    for (int vk = 'A'; vk <= 'Z'; ++vk) {
        if (key_edge_down(vk, previous)) {
            text.push_back(vk_to_char(vk, shift_down));
            changed = true;
        }
    }
    for (int vk = '0'; vk <= '9'; ++vk) {
        if (key_edge_down(vk, previous)) {
            text.push_back(vk_to_char(vk, shift_down));
            changed = true;
        }
    }

    const int punctuation_keys[] = {
        VK_SPACE, VK_OEM_MINUS, VK_OEM_PLUS, VK_OEM_COMMA, VK_OEM_PERIOD,
        VK_OEM_1, VK_OEM_2, VK_OEM_3, VK_OEM_4, VK_OEM_5, VK_OEM_6, VK_OEM_7
    };
    for (const int vk : punctuation_keys) {
        if (key_edge_down(vk, previous)) {
            const char ch = vk_to_char(vk, shift_down);
            if (ch != '\0') {
                text.push_back(ch);
                changed = true;
            }
        }
    }

    return changed;
}
#endif

void run_preview_window(const std::string& xml_text,
                        const std::string& source_path,
                        const os_generics::xml_screen_descriptor::ParseResult& parsed) {
    using namespace graphics::full_application_window;

    WindowConfig config("screen_builder preview", 1100, 760, true, true);
    if (parsed.ok()) {
        config.title = parsed.descriptor.title + " [screen_builder preview]";
        config.width = static_cast<int>(std::max<std::size_t>(700U, parsed.descriptor.width));
        config.height = static_cast<int>(std::max<std::size_t>(520U, parsed.descriptor.height));
    }

    FullApplicationWindow window(config);
    std::vector<std::string> toolbar_actions;
    std::vector<Button> toolbar_buttons;
    std::string selected_toolbar_action;
    std::string toolbar_status;
    bool prev_left = false;
    bool needs_redraw = true;

    if (parsed.ok()) {
        window.set_platform_style(parsed.descriptor.platform);

        std::vector<const graphics::components::Component*> all_components;
        collect_components_recursive(parsed.descriptor.layout, all_components);
        for (const auto* component : all_components) {
            if (component != nullptr && component->type() == graphics::components::ComponentType::MenuBar) {
                const auto* model = component->menu_bar_model();
                if (model != nullptr) {
                    window.set_menu_bar(*model);
                }
            }
            if (component != nullptr && component->type() == graphics::components::ComponentType::Toolbar) {
                const auto* model = component->toolbar_model();
                if (model != nullptr && !model->actions.empty()) {
                    toolbar_actions = model->actions;
                }
            }
        }
    }

    RenderHooks hooks;
    hooks.on_render = [&](const RenderEvent&) {
        int w = 0;
        int h = 0;
        if (!window.client_size(w, h)) {
            return;
        }

        window.clear_background(22, 26, 34);
        window.draw_text_line(12, 10, "XML: " + source_path);

        int y = 36;
        if (!parsed.ok()) {
            window.draw_text_line(12, y, "Parse errors:");
            y += 24;
            for (std::size_t i = 0; i < parsed.errors.size(); ++i) {
                window.draw_text_line(14, y, "- " + parsed.errors[i]);
                y += 20;
                if (y > h - 20) {
                    break;
                }
            }
            return;
        }

        const auto meta = "screen title='" + parsed.descriptor.title + "'  size=" +
            std::to_string(parsed.descriptor.width) + "x" + std::to_string(parsed.descriptor.height);
        window.draw_text_line(12, y, meta);
        y += 20;

        toolbar_buttons.clear();
        if (!toolbar_actions.empty()) {
            window.draw_text_line(12, y, "Toolbar actions (interactive preview):");
            y += 20;

            int x = 12;
            int row_top = y;
            for (const auto& action : toolbar_actions) {
                const int button_width = std::max(96, static_cast<int>(action.size()) * 8 + 20);
                if (x + button_width > w - 12) {
                    x = 12;
                    row_top += 34;
                }

                Button button{x, row_top, x + button_width, row_top + 26, action};
                const bool active = (selected_toolbar_action == action);
                window.fill_rect(button.x0, button.y0, button.x1, button.y1,
                                 active ? 58 : 70,
                                 active ? 108 : 82,
                                 active ? 158 : 96);
                window.draw_text_line(button.x0 + 9, button.y0 + 7, action);
                toolbar_buttons.push_back(button);
                x = button.x1 + 8;
                y = std::max(y, row_top + 34);
            }

            if (!toolbar_status.empty()) {
                window.draw_text_line(12, y, toolbar_status);
                y += 20;
            }
        }

        window.draw_text_line(12, y, "Rendered component layout:");
        y += 18;
        window.fill_rect(10, y, w - 10, y + 2, 56, 68, 86);
        y += 10;

        const auto layout_lines = parsed.descriptor.layout.render();
        for (const auto& line : layout_lines) {
            window.draw_text_line(14, y, line);
            y += 18;
            if (y > h - 18) {
                break;
            }
        }
    };

    window.set_render_hooks(std::move(hooks));
    if (!window.create()) {
        std::cerr << "Failed to create preview window.\n";
        return;
    }

    window.show();
    window.request_redraw();
    while (window.is_open()) {
        if (!window.pump_events()) {
            break;
        }

        PointerState pointer{};
        if (window.query_pointer_state(pointer)) {
            const bool left = pointer.left_button_down;
            const bool click = left && !prev_left;
            if (click && pointer.inside) {
                for (const auto& button : toolbar_buttons) {
                    if (point_in_button(button, pointer.x, pointer.y)) {
                        selected_toolbar_action = button.label;
                        toolbar_status = "Clicked toolbar action: " + button.label;
                        needs_redraw = true;
                        break;
                    }
                }
            }
            prev_left = left;
        }

        if (needs_redraw) {
            window.request_redraw();
            needs_redraw = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

std::vector<std::string> discover_example_paths(const std::string& fallback_path) {
    std::vector<std::string> paths;
    const std::filesystem::path examples_dir =
        std::filesystem::path("_deliverables") / "apps" / "screen_builder" / "examples";
    if (std::filesystem::exists(examples_dir) && std::filesystem::is_directory(examples_dir)) {
        for (const auto& entry : std::filesystem::directory_iterator(examples_dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto ext = lower_copy(entry.path().extension().string());
            if (ext == ".xml") {
                paths.push_back(entry.path().generic_string());
            }
        }
    }

    std::sort(paths.begin(), paths.end());
    if (paths.empty()) {
        paths.push_back(fallback_path);
    }
    return paths;
}

int run_gui_editor(const Options& options, app_assets::file_editor::TextFileEditor& editor) {
    using namespace graphics::full_application_window;

    os_generics::xml_screen_descriptor::XmlScreenDescriptorParser parser;
    std::string status = "Loaded: " + options.xml_path;
    int scroll_line = 1;
    bool prev_left = false;
    bool needs_redraw = true;
    bool dropdown_open = false;

    const auto example_paths = discover_example_paths(options.xml_path);
    std::size_t selected_example = 0;
    for (std::size_t i = 0; i < example_paths.size(); ++i) {
        if (example_paths[i] == options.xml_path) {
            selected_example = i;
            break;
        }
    }

    app_assets::file_editor::TextFileEditor example_editor;
    if (!example_editor.load(example_paths[selected_example])) {
        example_editor.load(options.xml_path);
    }
    auto full_display_parse = parser.parse_string(example_editor.text());

    WindowConfig config("screen_builder", 1280, 820, true, true);
    FullApplicationWindow window(config);

    std::vector<std::string> component_names = {
        "Button", "TextView", "EditableTextView", "RadioButton", "CheckBox", "LayoutGroup", "MenuBar", "DropdownMenu", "Toolbar", "DockPanel", "LayerList", "PropertyInspector", "FileTree", "RadioSelector", "CheckboxGroup", "ColorPicker", "TabbedView"
    };
    int selected_component = 0;

    enum class BuilderTab {
        IndividualComponents,
        FullDisplay,
    };
    static BuilderTab active_tab = BuilderTab::IndividualComponents;

    static int component_splitter = 360;
    static bool dragging_component_splitter = false;

    static std::string custom_text = "";
    static int bg_r = 80, bg_g = 120, bg_b = 180;
    static int fg_r = 0, fg_g = 0, fg_b = 0;
    static int font_size = 16;
    static int font_type = 0; // 0: default, 1: monospace, 2: sans
    static bool bg_picker_open = false;
    static bool fg_picker_open = false;
    static bool font_size_dropdown_open = false;
    static bool font_family_dropdown_open = false;
    static int border_style = 0; // 0 none, 1 solid, 2 dashed, 3 double
    static int border_thickness = 0;
    static bool show_line_numbers = false;
    static bool border_style_dropdown_open = false;
    static bool border_thickness_dropdown_open = false;
    static bool round_top_left = false;
    static bool round_top_right = false;
    static bool round_bottom_right = false;
    static bool round_bottom_left = false;
    static bool radio_selected = true;
    static bool checkbox_checked = true;
    static bool radio_selector_second_selected = true;
    static bool checkbox_group_first_checked = true;
    static bool checkbox_group_second_checked = false;
    static bool radio_enabled = true;
    static bool checkbox_enabled = true;
    static bool radio_selector_enabled = true;
    static bool checkbox_group_enabled = true;
    static bool text_box_focused = false;
#if defined(_WIN32)
    static std::array<bool, 256> previous_key_state{};
#endif

    struct Swatch { int r; int g; int b; };
    const std::vector<Swatch> swatches = {
        {20, 24, 30}, {56, 68, 86}, {80, 120, 180}, {180, 80, 80}, {80, 160, 110},
        {220, 220, 220}, {255, 255, 255}, {255, 196, 92}, {136, 86, 246}, {25, 25, 25}
    };
    const std::vector<int>& font_sizes = app_assets::ios_chrome::typography::standard_font_sizes();
    const std::vector<std::string>& font_labels = app_assets::ios_chrome::typography::standard_font_families();
    const std::vector<std::string> border_style_labels = {"None", "Solid", "Dashed", "Double"};
    const std::vector<int> border_thicknesses = {0, 1, 2, 3, 4};

    RenderHooks hooks;
    hooks.on_render = [&](const RenderEvent&) {
        int w = 0;
        int h = 0;
        if (!window.client_size(w, h)) {
            return;
        }

        if (component_splitter < 220) component_splitter = 220;
        if (component_splitter > w - 320) component_splitter = w - 320;

        const auto fill_outline = [&](int x0, int y0, int x1, int y1,
                                      int outer_r, int outer_g, int outer_b,
                                      int inner_r, int inner_g, int inner_b) {
            window.fill_rect(x0, y0, x1, y1, outer_r, outer_g, outer_b);
            window.fill_rect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, inner_r, inner_g, inner_b);
        };
        const auto draw_card = [&](int x0, int y0, int x1, int y1,
                                   const std::string& title,
                                   int accent_r, int accent_g, int accent_b) {
            window.draw_rounded_rect(x0, y0, x1, y1, 10, 58, 68, 84, false);
            window.draw_rounded_rect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, 9, 24, 28, 36, true);
            window.draw_rounded_rect(x0 + 1, y0 + 1, x1 - 1, y0 + 28, 9, 31, 38, 50, true);
            window.fill_rect(x0 + 1, y0 + 28, x1 - 1, y0 + 30, accent_r, accent_g, accent_b);
            window.draw_text_line(x0 + 12, y0 + 36, title, 236, 240, 247);
        };
        const auto draw_chip = [&](int x0, int y0, int x1, int y1,
                                   const std::string& label,
                                   int fill_r, int fill_g, int fill_b,
                                   int border_r, int border_g, int border_b,
                                   int text_r, int text_g, int text_b) {
            window.draw_rounded_rect(x0, y0, x1, y1, 8, border_r, border_g, border_b, false);
            window.draw_rounded_rect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, 7, fill_r, fill_g, fill_b, true);
            window.draw_text_line(x0 + 10, y0 + 8, label, text_r, text_g, text_b);
        };
        const auto draw_input_box = [&](int x0, int y0, int x1, int y1,
                                        int border_r, int border_g, int border_b,
                                        int fill_r, int fill_g, int fill_b) {
            window.draw_rounded_rect(x0, y0, x1, y1, 6, border_r, border_g, border_b, false);
            window.draw_rounded_rect(x0 + 1, y0 + 1, x1 - 1, y1 - 1, 5, fill_r, fill_g, fill_b, true);
        };

        window.clear_background(15, 18, 24);
        window.fill_rect(0, 0, w, 44, 18, 23, 31);
        window.fill_rect(0, 44, w, 46, 69, 120, 196);
        window.draw_text_line(12, 12, "screen_builder", 244, 247, 252);
        window.draw_text_line(132, 12, "component explorer and XML preview workspace", 150, 164, 186);

        const int tab_separator_y = 88; // Move the separator below the tabs
        const int tab_top = 52; // Adjust tab top position
        const int tab_bottom = 84; // Adjust tab bottom position
        const int content_top = tab_bottom + 12; // Define content top position
        const int component_splitter = 200; // Example splitter position
        const int controls_card_bottom = std::max(content_top + 246, (h / 2)); // Define controls card bottom

        const Button individual_tab_button{12, tab_top, 228, tab_bottom, "Individual components"};
        const Button full_display_tab_button{228, tab_top, 392, tab_bottom, "Full display"};
        const bool individual_active = active_tab == BuilderTab::IndividualComponents;
        const bool full_display_active = active_tab == BuilderTab::FullDisplay;

        if (individual_active) {
            draw_card(10, tab_separator_y + 10, component_splitter - 10, h - 12, "Components", 83, 128, 206);
        } else {
            draw_card(10, tab_separator_y + 10, w - 10, h - 12, "XML Example", 83, 128, 206);
        }

        if (individual_active) {
            window.fill_rect(component_splitter - (dragging_component_splitter ? 3 : 1), content_top,
                             component_splitter + (dragging_component_splitter ? 3 : 1), h,
                             dragging_component_splitter ? 104 : 58,
                             dragging_component_splitter ? 146 : 68,
                             dragging_component_splitter ? 224 : 84);

            draw_card(10, content_top, component_splitter - 10, h - 12, "Components", 83, 128, 206);
            int comp_y = content_top + 52;
            for (size_t i = 0; i < component_names.size(); ++i) {
                bool selected = (static_cast<int>(i) == selected_component);
                const bool alternate = (i % 2U) == 1U;
                if (!selected) {
                    window.fill_rect(14, comp_y - 6, component_splitter - 14, comp_y + 16,
                                     alternate ? 26 : 24,
                                     alternate ? 32 : 29,
                                     alternate ? 42 : 38);
                }
                if (selected) {
                    fill_outline(14, comp_y - 6, component_splitter - 14, comp_y + 16, 92, 134, 214, 42, 55, 76);
                    window.fill_rect(14, comp_y - 6, 18, comp_y + 16, 96, 150, 230);
                }
                window.draw_text_line(28, comp_y, component_names[i],
                                      selected ? 244 : 216,
                                      selected ? 247 : 224,
                                      selected ? 252 : 236);
                comp_y += 24;
                if (comp_y > h - 18) break;
            }
        } else {
            const auto& parsed_example = full_display_parse;
            const Button preview_button{12, 108, 220, 134, "Open preview window"};
            draw_chip(preview_button.x0, preview_button.y0, preview_button.x1, preview_button.y1,
                      preview_button.label, 45, 60, 82, 90, 131, 214, 242, 246, 252);

            if (parsed_example.ok()) {
                draw_chip(238, 108, 530, 134,
                                      "Preview ready: " + parsed_example.descriptor.title + " (" +
                                      std::to_string(parsed_example.descriptor.width) + "x" +
                                      std::to_string(parsed_example.descriptor.height) + ")",
                          30, 44, 36, 67, 125, 92, 214, 238, 220);
            } else if (!parsed_example.errors.empty()) {
                draw_chip(238, 108, 530, 134,
                          "Preview has parse errors; open the window to inspect them.",
                          52, 35, 35, 132, 80, 80, 248, 222, 222);
            } else {
                draw_chip(238, 108, 530, 134, "Preview state unavailable.",
                          40, 43, 50, 77, 86, 98, 220, 226, 235);
            }

            const int xml_card_top = content_top + 50;
            draw_card(10, xml_card_top, w - 10, h - 12, "XML Example", 83, 128, 206);
            int y = xml_card_top + 52;
            const int xml_inner_px = std::max(220, w - 38);
            const std::size_t xml_chars = static_cast<std::size_t>(std::max(24, xml_inner_px / 8));
            const auto xml_wrapped = graphics::components::Component::text_view(example_editor.text(), xml_chars, true).render();
            for (const auto& line : xml_wrapped) {
                window.draw_text_line(24, y, line, 218, 224, 235);
                y += 18;
                if (y > h - 18) {
                    break;
                }
            }
            return;
        }

        int right_x = component_splitter + 12;
    draw_card(component_splitter + 10, content_top, w - 10, h - 12, "Example & XML", 83, 128, 206);
    window.fill_rect(component_splitter + 12, controls_card_bottom + 8, w - 12, controls_card_bottom + 10, 66, 78, 96);
    int prev_y = content_top + 52;
        std::string cname = component_names[selected_component];
        const bool supports_line_numbers = cname == "TextView" || cname == "EditableTextView";
        const bool supports_button_corner_rounding = cname == "Button";
        const bool supports_radio_toggle = cname == "RadioButton";
        const bool supports_checkbox_toggle = cname == "CheckBox";
        const bool supports_radio_selector_toggles = cname == "RadioSelector";
        const bool supports_checkbox_group_toggles = cname == "CheckboxGroup";
        const bool supports_radio_enabled_toggle = cname == "RadioButton";
        const bool supports_checkbox_enabled_toggle = cname == "CheckBox";
        const bool supports_radio_selector_enabled_toggle = cname == "RadioSelector";
        const bool supports_checkbox_group_enabled_toggle = cname == "CheckboxGroup";
    draw_chip(right_x, prev_y - 2, right_x + 170, prev_y + 22, "Component: " + cname,
          34, 43, 56, 64, 85, 112, 236, 240, 247);
        prev_y += 24;

        // --- Customization controls ---
        int ctrl_y = prev_y;
        const int picker_box_x0 = right_x + 110;
        const int picker_box_x1 = right_x + 200;
        const int row_height = 22;

        window.draw_text_line(right_x, ctrl_y, "Background:", 172, 184, 203);
        draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x1, ctrl_y + 16, 66, 78, 96, bg_r, bg_g, bg_b);
        window.draw_text_line(picker_box_x1 + 8, ctrl_y, bg_picker_open ? "^" : "v");
        ctrl_y += row_height;

        if (bg_picker_open) {
            const int cell_w = 18;
            const int cell_h = 16;
            const int cols = 5;
            for (std::size_t i = 0; i < swatches.size(); ++i) {
                const int row = static_cast<int>(i) / cols;
                const int col = static_cast<int>(i) % cols;
                const int sx0 = picker_box_x0 + col * (cell_w + 4);
                const int sy0 = ctrl_y + row * (cell_h + 4);
                window.fill_rect(sx0, sy0, sx0 + cell_w, sy0 + cell_h,
                                 static_cast<unsigned char>(swatches[i].r),
                                 static_cast<unsigned char>(swatches[i].g),
                                 static_cast<unsigned char>(swatches[i].b));
            }
            ctrl_y += 44;
        }

        window.draw_text_line(right_x, ctrl_y, "Foreground:", 172, 184, 203);
        draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x1, ctrl_y + 16, 66, 78, 96, fg_r, fg_g, fg_b);
        window.draw_text_line(picker_box_x1 + 8, ctrl_y, fg_picker_open ? "^" : "v");
        ctrl_y += row_height;

        if (fg_picker_open) {
            const int cell_w = 18;
            const int cell_h = 16;
            const int cols = 5;
            for (std::size_t i = 0; i < swatches.size(); ++i) {
                const int row = static_cast<int>(i) / cols;
                const int col = static_cast<int>(i) % cols;
                const int sx0 = picker_box_x0 + col * (cell_w + 4);
                const int sy0 = ctrl_y + row * (cell_h + 4);
                window.fill_rect(sx0, sy0, sx0 + cell_w, sy0 + cell_h,
                                 static_cast<unsigned char>(swatches[i].r),
                                 static_cast<unsigned char>(swatches[i].g),
                                 static_cast<unsigned char>(swatches[i].b));
            }
            ctrl_y += 44;
        }

        window.draw_text_line(right_x, ctrl_y, "Font size:", 172, 184, 203);
        draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x1, ctrl_y + 16, 66, 78, 96, 34, 40, 52);
        window.draw_text_line(picker_box_x0 + 8, ctrl_y, std::to_string(font_size), 238, 242, 248);
        window.draw_text_line(picker_box_x1 + 8, ctrl_y, font_size_dropdown_open ? "^" : "v");
        ctrl_y += row_height;
        if (font_size_dropdown_open) {
            for (std::size_t i = 0; i < font_sizes.size(); ++i) {
                const int item_y = ctrl_y + static_cast<int>(i) * 18;
                draw_input_box(picker_box_x0, item_y - 2, picker_box_x1, item_y + 14, 66, 78, 96, 34, 40, 52);
                window.draw_text_line(picker_box_x0 + 8, item_y, std::to_string(font_sizes[i]), 226, 232, 240);
            }
            ctrl_y += static_cast<int>(font_sizes.size()) * 18;
        }

        window.draw_text_line(right_x, ctrl_y, "Font:", 172, 184, 203);
        draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x1, ctrl_y + 16, 66, 78, 96, 34, 40, 52);
        window.draw_text_line(picker_box_x0 + 8, ctrl_y, font_labels[std::min<std::size_t>(font_type, font_labels.size() - 1)], 238, 242, 248);
        window.draw_text_line(picker_box_x1 + 8, ctrl_y, font_family_dropdown_open ? "^" : "v");
        ctrl_y += row_height;
        if (font_family_dropdown_open) {
            for (std::size_t i = 0; i < font_labels.size(); ++i) {
                const int item_y = ctrl_y + static_cast<int>(i) * 18;
                draw_input_box(picker_box_x0, item_y - 2, picker_box_x1, item_y + 14, 66, 78, 96, 34, 40, 52);
                window.draw_text_line(picker_box_x0 + 8, item_y, font_labels[i], 226, 232, 240);
            }
            ctrl_y += static_cast<int>(font_labels.size()) * 18;
        }

        window.draw_text_line(right_x, ctrl_y, "Border:", 172, 184, 203);
        draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x1, ctrl_y + 16, 66, 78, 96, 34, 40, 52);
        window.draw_text_line(picker_box_x0 + 8, ctrl_y,
                      border_style_labels[std::min<std::size_t>(border_style, border_style_labels.size() - 1)], 238, 242, 248);
        window.draw_text_line(picker_box_x1 + 8, ctrl_y, border_style_dropdown_open ? "^" : "v");
        ctrl_y += row_height;
        if (border_style_dropdown_open) {
            for (std::size_t i = 0; i < border_style_labels.size(); ++i) {
                const int item_y = ctrl_y + static_cast<int>(i) * 18;
                draw_input_box(picker_box_x0, item_y - 2, picker_box_x1, item_y + 14, 66, 78, 96, 34, 40, 52);
                window.draw_text_line(picker_box_x0 + 8, item_y, border_style_labels[i], 226, 232, 240);
            }
            ctrl_y += static_cast<int>(border_style_labels.size()) * 18;
        }

        window.draw_text_line(right_x, ctrl_y, "Border px:", 172, 184, 203);
        draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x1, ctrl_y + 16, 66, 78, 96, 34, 40, 52);
        window.draw_text_line(picker_box_x0 + 8, ctrl_y, std::to_string(border_thickness), 238, 242, 248);
        window.draw_text_line(picker_box_x1 + 8, ctrl_y, border_thickness_dropdown_open ? "^" : "v");
        ctrl_y += row_height;
        if (border_thickness_dropdown_open) {
            for (std::size_t i = 0; i < border_thicknesses.size(); ++i) {
                const int item_y = ctrl_y + static_cast<int>(i) * 18;
                draw_input_box(picker_box_x0, item_y - 2, picker_box_x1, item_y + 14, 66, 78, 96, 34, 40, 52);
                window.draw_text_line(picker_box_x0 + 8, item_y, std::to_string(border_thicknesses[i]), 226, 232, 240);
            }
            ctrl_y += static_cast<int>(border_thicknesses.size()) * 18;
        }

        if (supports_line_numbers) {
            window.draw_text_line(right_x, ctrl_y, "Line numbers:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                         66, 78, 96,
                             show_line_numbers ? 80 : 45,
                             show_line_numbers ? 140 : 52,
                         show_line_numbers ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, show_line_numbers ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;
        }

        if (supports_button_corner_rounding) {
            auto draw_corner_toggle = [&](const std::string& label, bool enabled) {
                window.draw_text_line(right_x, ctrl_y, label, 172, 184, 203);
                draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                             66, 78, 96,
                                 enabled ? 80 : 45,
                                 enabled ? 140 : 52,
                                 enabled ? 90 : 66);
                window.draw_text_line(picker_box_x0 + 7, ctrl_y, enabled ? "on" : "off", 240, 245, 250);
                ctrl_y += row_height;
            };
            draw_corner_toggle("Round TL:", round_top_left);
            draw_corner_toggle("Round TR:", round_top_right);
            draw_corner_toggle("Round BR:", round_bottom_right);
            draw_corner_toggle("Round BL:", round_bottom_left);
        }

        if (supports_radio_toggle) {
            window.draw_text_line(right_x, ctrl_y, "Selected:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           radio_selected ? 80 : 45,
                           radio_selected ? 140 : 52,
                           radio_selected ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, radio_selected ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;
        }
        if (supports_radio_enabled_toggle) {
            window.draw_text_line(right_x, ctrl_y, "Enabled:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           radio_enabled ? 80 : 45,
                           radio_enabled ? 140 : 52,
                           radio_enabled ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, radio_enabled ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;
        }

        if (supports_checkbox_toggle) {
            window.draw_text_line(right_x, ctrl_y, "Checked:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           checkbox_checked ? 80 : 45,
                           checkbox_checked ? 140 : 52,
                           checkbox_checked ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, checkbox_checked ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;
        }
        if (supports_checkbox_enabled_toggle) {
            window.draw_text_line(right_x, ctrl_y, "Enabled:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           checkbox_enabled ? 80 : 45,
                           checkbox_enabled ? 140 : 52,
                           checkbox_enabled ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, checkbox_enabled ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;
        }

        if (supports_radio_selector_toggles) {
            const bool option_a_selected = !radio_selector_second_selected;
            const bool option_b_selected = radio_selector_second_selected;
            window.draw_text_line(right_x, ctrl_y, "Option A:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           option_a_selected ? 80 : 45,
                           option_a_selected ? 140 : 52,
                           option_a_selected ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, option_a_selected ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;

            window.draw_text_line(right_x, ctrl_y, "Option B:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           option_b_selected ? 80 : 45,
                           option_b_selected ? 140 : 52,
                           option_b_selected ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, option_b_selected ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;
        }
        if (supports_radio_selector_enabled_toggle) {
            window.draw_text_line(right_x, ctrl_y, "Enabled:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           radio_selector_enabled ? 80 : 45,
                           radio_selector_enabled ? 140 : 52,
                           radio_selector_enabled ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, radio_selector_enabled ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;
        }

        if (supports_checkbox_group_toggles) {
            window.draw_text_line(right_x, ctrl_y, "Foo:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           checkbox_group_first_checked ? 80 : 45,
                           checkbox_group_first_checked ? 140 : 52,
                           checkbox_group_first_checked ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, checkbox_group_first_checked ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;

            window.draw_text_line(right_x, ctrl_y, "Bar:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           checkbox_group_second_checked ? 80 : 45,
                           checkbox_group_second_checked ? 140 : 52,
                           checkbox_group_second_checked ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, checkbox_group_second_checked ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;
        }
        if (supports_checkbox_group_enabled_toggle) {
            window.draw_text_line(right_x, ctrl_y, "Enabled:", 172, 184, 203);
            draw_input_box(picker_box_x0, ctrl_y - 2, picker_box_x0 + 34, ctrl_y + 16,
                           66, 78, 96,
                           checkbox_group_enabled ? 80 : 45,
                           checkbox_group_enabled ? 140 : 52,
                           checkbox_group_enabled ? 90 : 66);
            window.draw_text_line(picker_box_x0 + 7, ctrl_y, checkbox_group_enabled ? "on" : "off", 240, 245, 250);
            ctrl_y += row_height;
        }

        ctrl_y += 6;

        // --- Example rendering for each type ---
        int comp_h = std::max(32, font_size + 14);
        int comp_w = 180;
        std::string display_text = custom_text.empty() ? cname : custom_text;
        const int preview_top = ctrl_y;
        const int preview_height = std::max(96, controls_card_bottom - preview_top - 16);
        fill_outline(right_x, preview_top, w - 24, preview_top + preview_height, 66, 78, 96, 255, 255, 255);
        window.draw_text_line(right_x + 10, preview_top + 8, "Live preview", 48, 60, 78);
        const int preview_content_y = preview_top + 30;
        if (cname == "Button") {
            const auto rendered = graphics::components::Component::button(
                display_text,
                true,
                false,
                static_cast<std::size_t>(std::max(10, comp_w / 8)),
                round_top_left,
                round_top_right,
                round_bottom_right,
                round_bottom_left).render();
            fill_outline(right_x + 12, preview_content_y - 4, right_x + 240, preview_content_y + 34, 84, 108, 138, bg_r, bg_g, bg_b);
            int line_y = preview_content_y + 8;
            for (const auto& line : rendered) {
                window.draw_text_line(right_x + 20, line_y, line, fg_r, fg_g, fg_b);
                line_y += 18;
            }
            comp_h = std::max(comp_h, static_cast<int>(rendered.size()) * 18);
        } else if (cname == "TextView") {
            const auto rendered = graphics::components::Component::text_view(
                display_text,
                static_cast<std::size_t>(std::max(12, comp_w / 8)),
                true,
                static_cast<graphics::components::BorderStyle>(border_style),
                static_cast<std::size_t>(border_thickness),
                show_line_numbers).render();
            int line_y = preview_content_y;
            for (const auto& line : rendered) {
                window.draw_text_line(right_x + 12, line_y, line, fg_r, fg_g, fg_b);
                line_y += 18;
            }
            comp_h = std::max(comp_h, static_cast<int>(rendered.size()) * 18);
        } else if (cname == "EditableTextView") {
            const auto rendered = graphics::components::Component::editable_text_view(
                display_text,
                display_text.size(),
                true,
                static_cast<std::size_t>(std::max(12, comp_w / 8)),
                true,
                static_cast<graphics::components::BorderStyle>(border_style),
                static_cast<std::size_t>(border_thickness),
                show_line_numbers,
                true,
                4,
                static_cast<std::size_t>(font_size),
                {"alpha", "beta", "gamma"}).render();
            int line_y = preview_content_y;
            for (const auto& line : rendered) {
                window.draw_text_line(right_x + 12, line_y, line, fg_r, fg_g, fg_b);
                line_y += 18;
            }
            comp_h = std::max(comp_h, static_cast<int>(rendered.size()) * 18);
        } else if (cname == "RadioButton") {
            const unsigned char border_r = static_cast<unsigned char>(radio_enabled ? 84 : 132);
            const unsigned char border_g = static_cast<unsigned char>(radio_enabled ? 108 : 142);
            const unsigned char border_b = static_cast<unsigned char>(radio_enabled ? 138 : 156);
            const unsigned char face_r = static_cast<unsigned char>(radio_enabled ? 236 : 214);
            const unsigned char face_g = static_cast<unsigned char>(radio_enabled ? 240 : 220);
            const unsigned char face_b = static_cast<unsigned char>(radio_enabled ? 247 : 228);
            const unsigned char text_r = static_cast<unsigned char>(radio_enabled ? 32 : 120);
            const unsigned char text_g = static_cast<unsigned char>(radio_enabled ? 42 : 126);
            const unsigned char text_b = static_cast<unsigned char>(radio_enabled ? 56 : 136);
            window.draw_arc(right_x + 22, preview_content_y + 10, 9, 0.0, 360.0, border_r, border_g, border_b, 2);
            window.draw_arc(right_x + 22, preview_content_y + 10, 8, 0.0, 360.0, face_r, face_g, face_b, 2);
            if (radio_selected) {
                window.draw_rounded_rect(right_x + 18, preview_content_y + 6, right_x + 26, preview_content_y + 14, 4,
                                         static_cast<unsigned char>(radio_enabled ? bg_r : 160),
                                         static_cast<unsigned char>(radio_enabled ? bg_g : 160),
                                         static_cast<unsigned char>(radio_enabled ? bg_b : 160),
                                         true);
            }
            window.draw_text_line(right_x + 44, preview_content_y + 4, display_text, text_r, text_g, text_b);
        } else if (cname == "CheckBox") {
            const unsigned char border_r = static_cast<unsigned char>(checkbox_enabled ? 84 : 132);
            const unsigned char border_g = static_cast<unsigned char>(checkbox_enabled ? 108 : 142);
            const unsigned char border_b = static_cast<unsigned char>(checkbox_enabled ? 138 : 156);
            const unsigned char face_r = static_cast<unsigned char>(checkbox_enabled ? 236 : 214);
            const unsigned char face_g = static_cast<unsigned char>(checkbox_enabled ? 240 : 220);
            const unsigned char face_b = static_cast<unsigned char>(checkbox_enabled ? 247 : 228);
            const unsigned char text_r = static_cast<unsigned char>(checkbox_enabled ? 32 : 120);
            const unsigned char text_g = static_cast<unsigned char>(checkbox_enabled ? 42 : 126);
            const unsigned char text_b = static_cast<unsigned char>(checkbox_enabled ? 56 : 136);
            window.draw_rounded_rect(right_x + 12, preview_content_y, right_x + 32, preview_content_y + 20, 4, border_r, border_g, border_b, false);
            window.draw_rounded_rect(right_x + 13, preview_content_y + 1, right_x + 31, preview_content_y + 19, 3, face_r, face_g, face_b, true);
            if (checkbox_checked) {
                window.draw_rounded_rect(right_x + 17, preview_content_y + 5, right_x + 27, preview_content_y + 15, 2,
                                         static_cast<unsigned char>(checkbox_enabled ? bg_r : 160),
                                         static_cast<unsigned char>(checkbox_enabled ? bg_g : 160),
                                         static_cast<unsigned char>(checkbox_enabled ? bg_b : 160),
                                         true);
            }
            window.draw_text_line(right_x + 44, preview_content_y + 4, display_text, text_r, text_g, text_b);
        } else if (cname == "LayoutGroup") {
            window.draw_text_line(right_x + 12, preview_content_y, "Vertical layout group", fg_r, fg_g, fg_b);
        } else if (cname == "MenuBar") {
            fill_outline(right_x + 12, preview_content_y, right_x + comp_w + 12, preview_content_y + 24, 84, 108, 138, bg_r, bg_g, bg_b);
            window.draw_text_line(right_x + 22, preview_content_y + 6, display_text, fg_r, fg_g, fg_b);
        } else if (cname == "DropdownMenu") {
            fill_outline(right_x + 12, preview_content_y, right_x + 132, preview_content_y + 24, 84, 108, 138, bg_r, bg_g, bg_b);
            window.draw_text_line(right_x + 22, preview_content_y + 6, display_text, fg_r, fg_g, fg_b);
        } else if (cname == "Toolbar") {
            fill_outline(right_x + 12, preview_content_y, right_x + comp_w + 12, preview_content_y + 24, 84, 108, 138, bg_r, bg_g, bg_b);
            window.draw_text_line(right_x + 22, preview_content_y + 6, display_text, fg_r, fg_g, fg_b);
        } else if (cname == "DockPanel") {
            fill_outline(right_x + 12, preview_content_y, right_x + comp_w + 12, preview_content_y + 80, 84, 108, 138, bg_r, bg_g, bg_b);
            window.draw_text_line(right_x + 22, preview_content_y + 10, display_text, fg_r, fg_g, fg_b);
        } else if (cname == "LayerList") {
            window.draw_text_line(right_x + 12, preview_content_y, "- Layer 1\n- Layer 2\n- Layer 3", fg_r, fg_g, fg_b);
        } else if (cname == "PropertyInspector") {
            window.draw_text_line(right_x + 12, preview_content_y, "property: value\nother: 123", fg_r, fg_g, fg_b);
        } else if (cname == "FileTree") {
            window.draw_text_line(right_x + 12, preview_content_y, "/root\n  folder/\n    file.txt", fg_r, fg_g, fg_b);
        } else if (cname == "RadioSelector") {
            const bool option_a_selected = !radio_selector_second_selected;
            const bool option_b_selected = radio_selector_second_selected;
            const unsigned char border_r = static_cast<unsigned char>(radio_selector_enabled ? 84 : 132);
            const unsigned char border_g = static_cast<unsigned char>(radio_selector_enabled ? 108 : 142);
            const unsigned char border_b = static_cast<unsigned char>(radio_selector_enabled ? 138 : 156);
            const unsigned char face_r = static_cast<unsigned char>(radio_selector_enabled ? 236 : 214);
            const unsigned char face_g = static_cast<unsigned char>(radio_selector_enabled ? 240 : 220);
            const unsigned char face_b = static_cast<unsigned char>(radio_selector_enabled ? 247 : 228);
            const unsigned char text_r = static_cast<unsigned char>(radio_selector_enabled ? 32 : 120);
            const unsigned char text_g = static_cast<unsigned char>(radio_selector_enabled ? 42 : 126);
            const unsigned char text_b = static_cast<unsigned char>(radio_selector_enabled ? 56 : 136);
            window.draw_arc(right_x + 22, preview_content_y + 9, 8, 0.0, 360.0, border_r, border_g, border_b, 2);
            window.draw_arc(right_x + 22, preview_content_y + 9, 7, 0.0, 360.0, face_r, face_g, face_b, 2);
            if (option_a_selected) {
                window.draw_rounded_rect(right_x + 19, preview_content_y + 6, right_x + 25, preview_content_y + 12, 3,
                                         static_cast<unsigned char>(radio_selector_enabled ? bg_r : 160),
                                         static_cast<unsigned char>(radio_selector_enabled ? bg_g : 160),
                                         static_cast<unsigned char>(radio_selector_enabled ? bg_b : 160),
                                         true);
            }
            window.draw_text_line(right_x + 40, preview_content_y + 2, "Option A", text_r, text_g, text_b);
            window.draw_arc(right_x + 22, preview_content_y + 30, 8, 0.0, 360.0, border_r, border_g, border_b, 2);
            window.draw_arc(right_x + 22, preview_content_y + 30, 7, 0.0, 360.0, face_r, face_g, face_b, 2);
            if (option_b_selected) {
                window.draw_rounded_rect(right_x + 19, preview_content_y + 27, right_x + 25, preview_content_y + 33, 3,
                                         static_cast<unsigned char>(radio_selector_enabled ? bg_r : 160),
                                         static_cast<unsigned char>(radio_selector_enabled ? bg_g : 160),
                                         static_cast<unsigned char>(radio_selector_enabled ? bg_b : 160),
                                         true);
            }
            window.draw_text_line(right_x + 40, preview_content_y + 23, "Option B", text_r, text_g, text_b);
        } else if (cname == "CheckboxGroup") {
            const unsigned char border_r = static_cast<unsigned char>(checkbox_group_enabled ? 84 : 132);
            const unsigned char border_g = static_cast<unsigned char>(checkbox_group_enabled ? 108 : 142);
            const unsigned char border_b = static_cast<unsigned char>(checkbox_group_enabled ? 138 : 156);
            const unsigned char face_r = static_cast<unsigned char>(checkbox_group_enabled ? 236 : 214);
            const unsigned char face_g = static_cast<unsigned char>(checkbox_group_enabled ? 240 : 220);
            const unsigned char face_b = static_cast<unsigned char>(checkbox_group_enabled ? 247 : 228);
            const unsigned char text_r = static_cast<unsigned char>(checkbox_group_enabled ? 32 : 120);
            const unsigned char text_g = static_cast<unsigned char>(checkbox_group_enabled ? 42 : 126);
            const unsigned char text_b = static_cast<unsigned char>(checkbox_group_enabled ? 56 : 136);
            window.draw_rounded_rect(right_x + 12, preview_content_y, right_x + 30, preview_content_y + 18, 3, border_r, border_g, border_b, false);
            window.draw_rounded_rect(right_x + 13, preview_content_y + 1, right_x + 29, preview_content_y + 17, 2, face_r, face_g, face_b, true);
            if (checkbox_group_first_checked) {
                window.draw_rounded_rect(right_x + 17, preview_content_y + 5, right_x + 25, preview_content_y + 13, 2,
                                         static_cast<unsigned char>(checkbox_group_enabled ? bg_r : 160),
                                         static_cast<unsigned char>(checkbox_group_enabled ? bg_g : 160),
                                         static_cast<unsigned char>(checkbox_group_enabled ? bg_b : 160),
                                         true);
            }
            window.draw_text_line(right_x + 38, preview_content_y + 2, "Foo", text_r, text_g, text_b);
            window.draw_rounded_rect(right_x + 12, preview_content_y + 22, right_x + 30, preview_content_y + 40, 3, border_r, border_g, border_b, false);
            window.draw_rounded_rect(right_x + 13, preview_content_y + 23, right_x + 29, preview_content_y + 39, 2, face_r, face_g, face_b, true);
            if (checkbox_group_second_checked) {
                window.draw_rounded_rect(right_x + 17, preview_content_y + 27, right_x + 25, preview_content_y + 35, 2,
                                         static_cast<unsigned char>(checkbox_group_enabled ? bg_r : 160),
                                         static_cast<unsigned char>(checkbox_group_enabled ? bg_g : 160),
                                         static_cast<unsigned char>(checkbox_group_enabled ? bg_b : 160),
                                         true);
            }
            window.draw_text_line(right_x + 38, preview_content_y + 24, "Bar", text_r, text_g, text_b);
        } else if (cname == "ColorPicker") {
            fill_outline(right_x + 12, preview_content_y, right_x + 132, preview_content_y + 32, 84, 108, 138, bg_r, bg_g, bg_b);
            window.draw_text_line(right_x + 146, preview_content_y + 8,
                                  "rgb(" + std::to_string(bg_r) + "," + std::to_string(bg_g) + "," + std::to_string(bg_b) + ")",
                                  fg_r, fg_g, fg_b);
        } else if (cname == "TabbedView") {
            fill_outline(right_x + 12, preview_content_y, right_x + comp_w + 12, preview_content_y + 26, 74, 96, 126, 37, 44, 58);
            fill_outline(right_x + 24, preview_content_y + 4, right_x + 98, preview_content_y + 22, 96, 150, 230, 51, 69, 94);
            window.draw_text_line(right_x + 32, preview_content_y + 6, "Design", 244, 247, 252);
            window.draw_text_line(right_x + 108, preview_content_y + 6, "Inspect", 182, 192, 209);
            window.draw_text_line(right_x + 176, preview_content_y + 6, "Chat", 182, 192, 209);
            fill_outline(right_x + 12, preview_content_y + 30, right_x + comp_w + 12, preview_content_y + comp_h + 44, 84, 108, 138, bg_r, bg_g, bg_b);
            window.draw_text_line(right_x + 24, preview_content_y + 40, "Selected tab subview", fg_r, fg_g, fg_b);
        }
        ctrl_y = preview_top + preview_height + 16;

        // --- Text box for editing ---
        window.draw_text_line(right_x, controls_card_bottom + 20, "Generated XML", 236, 240, 247);
        window.draw_text_line(right_x, controls_card_bottom + 42, "Text:", 172, 184, 203);
        draw_input_box(right_x + 50, controls_card_bottom + 40, right_x + 270, controls_card_bottom + 64,
                     text_box_focused ? 96 : 66,
                     text_box_focused ? 150 : 78,
                     text_box_focused ? 230 : 96,
                     248, 251, 255);
        std::string text_box_display = custom_text.empty() ? "(type here)" : custom_text;
        if (text_box_focused) {
            text_box_display += "|";
        }
        window.draw_text_line(right_x + 58, controls_card_bottom + 46, text_box_display,
                              custom_text.empty() ? 118 : 0,
                              custom_text.empty() ? 126 : 0,
                              custom_text.empty() ? 140 : 0);

        // --- Lower half: generated XML snippet ---
        const int snippet_top = controls_card_bottom + 76;
        window.draw_text_line(right_x, snippet_top, "Resulting XML snippet", 236, 240, 247);
        fill_outline(right_x, snippet_top + 24, w - 24, h - 24, 66, 78, 96, 20, 25, 33);

        const std::string xml_text = xml_escape(display_text);
        const std::string color_bg =
            std::to_string(bg_r) + "," + std::to_string(bg_g) + "," + std::to_string(bg_b);
        const std::string color_fg =
            std::to_string(fg_r) + "," + std::to_string(fg_g) + "," + std::to_string(fg_b);
        const std::string font_name = (font_type == 0) ? "default" : (font_type == 1) ? "monospace" : "sans";
        const std::string border_name = lower_copy(border_style_labels[std::min<std::size_t>(border_style, border_style_labels.size() - 1)]);

        std::vector<std::string> snippet_lines;
        const std::string tag = lower_copy(cname);
        if (cname == "TextView") {
            snippet_lines.push_back("<text_view text=\"" + xml_text + "\" wrap=\"true\"");
        } else if (cname == "EditableTextView") {
            snippet_lines.push_back("<editable_text_view text=\"" + xml_text + "\" wrap=\"true\"");
        } else if (cname == "ColorPicker") {
            snippet_lines.push_back("<color_picker background=\"" + color_bg + "\" />");
        } else if (cname == "Button") {
            snippet_lines.push_back("<button label=\"" + xml_text + "\"");
        } else if (cname == "RadioButton") {
            snippet_lines.push_back("<radio label=\"" + xml_text + "\"");
        } else if (cname == "CheckBox") {
            snippet_lines.push_back("<checkbox label=\"" + xml_text + "\"");
        } else if (cname == "RadioSelector") {
            snippet_lines.push_back("<radio_selector options=\"Option A,Option B\"");
        } else if (cname == "CheckboxGroup") {
            snippet_lines.push_back("<checkbox_group options=\"Foo,Bar\"");
        } else {
            snippet_lines.push_back("<" + tag + " text=\"" + xml_text + "\"");
        }

        if (cname != "ColorPicker") {
            if (cname == "TextView" || cname == "EditableTextView") {
                snippet_lines.push_back("  border-style=\"" + border_name + "\"");
                snippet_lines.push_back("  border-thickness=\"" + std::to_string(border_thickness) + "\"");
                snippet_lines.push_back("  line-numbers=\"" + std::string(show_line_numbers ? "true" : "false") + "\"");
                snippet_lines.push_back("  background=\"" + color_bg + "\"");
                snippet_lines.push_back("  foreground=\"" + color_fg + "\"");
                snippet_lines.push_back("  font_size=\"" + std::to_string(font_size) + "\"");
                snippet_lines.push_back("  font=\"" + font_name + "\" />");
            } else if (cname == "Button") {
                snippet_lines.push_back("  width=\"" + std::to_string(std::max(10, comp_w / 8)) + "\"");
                snippet_lines.push_back("  round-top-left=\"" + std::string(round_top_left ? "true" : "false") + "\"");
                snippet_lines.push_back("  round-top-right=\"" + std::string(round_top_right ? "true" : "false") + "\"");
                snippet_lines.push_back("  round-bottom-right=\"" + std::string(round_bottom_right ? "true" : "false") + "\"");
                snippet_lines.push_back("  round-bottom-left=\"" + std::string(round_bottom_left ? "true" : "false") + "\" />");
            } else if (cname == "RadioButton") {
                snippet_lines.push_back("  selected=\"" + std::string(radio_selected ? "true" : "false") + "\"");
                snippet_lines.push_back("  enabled=\"" + std::string(radio_enabled ? "true" : "false") + "\" />");
            } else if (cname == "CheckBox") {
                snippet_lines.push_back("  checked=\"" + std::string(checkbox_checked ? "true" : "false") + "\"");
                snippet_lines.push_back("  enabled=\"" + std::string(checkbox_enabled ? "true" : "false") + "\" />");
            } else if (cname == "RadioSelector") {
                snippet_lines.push_back("  selected=\"" + std::string(radio_selector_second_selected ? "1" : "0") + "\" />");
            } else if (cname == "CheckboxGroup") {
                snippet_lines.push_back("  checked=\"" + std::string(checkbox_group_first_checked ? "true" : "false") + "," +
                                        std::string(checkbox_group_second_checked ? "true" : "false") + "\" />");
            } else {
                snippet_lines.push_back("  background=\"" + color_bg + "\"");
                snippet_lines.push_back("  foreground=\"" + color_fg + "\"");
                snippet_lines.push_back("  font_size=\"" + std::to_string(font_size) + "\"");
                snippet_lines.push_back("  font=\"" + font_name + "\" />");
            }
        }

        int snippet_y = snippet_top + 36;
        for (const auto& line : snippet_lines) {
            window.draw_text_line(right_x + 12, snippet_y, line, 218, 224, 235);
            snippet_y += 18;
            if (snippet_y > h - 12) {
                break;
            }
        }
    };

    window.set_render_hooks(std::move(hooks));
    if (!window.create()) {
        std::cerr << "Failed to create screen_builder window.\n";
        return 2;
    }

    window.show();
    window.request_redraw();
    while (window.is_open()) {
        if (!window.pump_events()) {
            break;
        }

        graphics::full_application_window::PointerState pointer{};
        if (window.query_pointer_state(pointer)) {
            const bool left = pointer.left_button_down;
            const bool click = left && !prev_left;
            const bool drag = left && prev_left;
            int w = pointer.client_width;
            int h = pointer.client_height;
            const int content_top = 96;
            const int controls_card_bottom = std::max(content_top + 246, (h / 2));

            const Button individual_tab_button{12, 56, 228, 88, "Individual components"};
            const Button full_display_tab_button{228, 56, 392, 88, "Full display"};
            const bool over_individual_tab = point_in_button(individual_tab_button, pointer.x, pointer.y);
            const bool over_full_display_tab = point_in_button(full_display_tab_button, pointer.x, pointer.y);

            // Start dragging
            if (click && pointer.inside) {
                if (over_individual_tab) {
                    active_tab = BuilderTab::IndividualComponents;
                    dragging_component_splitter = false;
                    needs_redraw = true;
                } else if (over_full_display_tab) {
                    active_tab = BuilderTab::FullDisplay;
                    dragging_component_splitter = false;
                    text_box_focused = false;
                    bg_picker_open = false;
                    fg_picker_open = false;
                    font_size_dropdown_open = false;
                    font_family_dropdown_open = false;
                    border_style_dropdown_open = false;
                    border_thickness_dropdown_open = false;
                    needs_redraw = true;
                } else if (active_tab == BuilderTab::FullDisplay) {
                    const Button preview_button{12, 108, 220, 134, "Open preview window"};
                    if (point_in_button(preview_button, pointer.x, pointer.y)) {
                        run_preview_window(example_editor.text(), example_paths[selected_example], full_display_parse);
                        needs_redraw = true;
                    }
                } else {
                    bool over_component_splitter = pointer.x >= component_splitter - 4 && pointer.x <= component_splitter + 4;
                    int right_x = component_splitter + 12;
                    const std::string cname = component_names[selected_component];
                    const bool supports_line_numbers = cname == "TextView" || cname == "EditableTextView";
                    const bool supports_button_corner_rounding = cname == "Button";
                    const bool supports_radio_toggle = cname == "RadioButton";
                    const bool supports_checkbox_toggle = cname == "CheckBox";
                    const bool supports_radio_selector_toggles = cname == "RadioSelector";
                    const bool supports_checkbox_group_toggles = cname == "CheckboxGroup";
                    const bool supports_radio_enabled_toggle = cname == "RadioButton";
                    const bool supports_checkbox_enabled_toggle = cname == "CheckBox";
                    const bool supports_radio_selector_enabled_toggle = cname == "RadioSelector";
                    const bool supports_checkbox_group_enabled_toggle = cname == "CheckboxGroup";
                    int ctrl_y = content_top + 76;
                    const int picker_box_x0 = right_x + 110;
                    const int picker_box_x1 = right_x + 200;
                    const int row_height = 22;

                    const bool over_bg_picker_toggle =
                        pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 + 20 &&
                        pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                    const int bg_palette_y = ctrl_y + row_height;
                    ctrl_y += row_height + (bg_picker_open ? 44 : 0);

                    const bool over_fg_picker_toggle =
                        pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 + 20 &&
                        pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                    const int fg_palette_y = ctrl_y + row_height;
                    ctrl_y += row_height + (fg_picker_open ? 44 : 0);

                    const bool over_font_size_toggle =
                        pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 + 20 &&
                        pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                    const int font_size_list_y = ctrl_y + row_height;
                    ctrl_y += row_height + (font_size_dropdown_open ? static_cast<int>(font_sizes.size()) * 18 : 0);

                    const bool over_font_family_toggle =
                        pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 + 20 &&
                        pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                    const int font_family_list_y = ctrl_y + row_height;
                    ctrl_y += row_height + (font_family_dropdown_open ? static_cast<int>(font_labels.size()) * 18 : 0);

                    const bool over_border_style_toggle =
                        pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 + 20 &&
                        pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                    const int border_style_list_y = ctrl_y + row_height;
                    ctrl_y += row_height + (border_style_dropdown_open ? static_cast<int>(border_style_labels.size()) * 18 : 0);

                    const bool over_border_thickness_toggle =
                        pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 + 20 &&
                        pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                    const int border_thickness_list_y = ctrl_y + row_height;
                    ctrl_y += row_height + (border_thickness_dropdown_open ? static_cast<int>(border_thicknesses.size()) * 18 : 0);

                    bool over_line_numbers_toggle = false;
                    if (supports_line_numbers) {
                        over_line_numbers_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_round_top_left_toggle = false;
                    bool over_round_top_right_toggle = false;
                    bool over_round_bottom_right_toggle = false;
                    bool over_round_bottom_left_toggle = false;
                    if (supports_button_corner_rounding) {
                        over_round_top_left_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                        over_round_top_right_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                        over_round_bottom_right_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                        over_round_bottom_left_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_radio_toggle = false;
                    if (supports_radio_toggle) {
                        over_radio_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_checkbox_toggle = false;
                    if (supports_checkbox_toggle) {
                        over_checkbox_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_radio_selector_option_a_toggle = false;
                    bool over_radio_selector_option_b_toggle = false;
                    if (supports_radio_selector_toggles) {
                        over_radio_selector_option_a_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                        over_radio_selector_option_b_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_checkbox_group_foo_toggle = false;
                    bool over_checkbox_group_bar_toggle = false;
                    if (supports_checkbox_group_toggles) {
                        over_checkbox_group_foo_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                        over_checkbox_group_bar_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_radio_enabled_toggle = false;
                    if (supports_radio_enabled_toggle) {
                        over_radio_enabled_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_checkbox_enabled_toggle = false;
                    if (supports_checkbox_enabled_toggle) {
                        over_checkbox_enabled_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_radio_selector_enabled_toggle = false;
                    if (supports_radio_selector_enabled_toggle) {
                        over_radio_selector_enabled_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_checkbox_group_enabled_toggle = false;
                    if (supports_checkbox_group_enabled_toggle) {
                        over_checkbox_group_enabled_toggle =
                            pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 34 &&
                            pointer.y >= ctrl_y - 2 && pointer.y <= ctrl_y + 16;
                        ctrl_y += row_height;
                    }

                    bool over_text_box = pointer.x >= right_x + 50 && pointer.x <= right_x + 270 &&
                                         pointer.y >= controls_card_bottom + 40 && pointer.y <= controls_card_bottom + 64;

                    if (over_component_splitter) dragging_component_splitter = true;
                    else if (over_bg_picker_toggle) {
                    text_box_focused = false;
                    bg_picker_open = !bg_picker_open;
                    fg_picker_open = false;
                    font_size_dropdown_open = false;
                    font_family_dropdown_open = false;
                    border_style_dropdown_open = false;
                    border_thickness_dropdown_open = false;
                    needs_redraw = true;
                } else if (over_fg_picker_toggle) {
                    text_box_focused = false;
                    fg_picker_open = !fg_picker_open;
                    bg_picker_open = false;
                    font_size_dropdown_open = false;
                    font_family_dropdown_open = false;
                    border_style_dropdown_open = false;
                    border_thickness_dropdown_open = false;
                    needs_redraw = true;
                } else if (over_font_size_toggle) {
                    text_box_focused = false;
                    font_size_dropdown_open = !font_size_dropdown_open;
                    bg_picker_open = false;
                    fg_picker_open = false;
                    font_family_dropdown_open = false;
                    border_style_dropdown_open = false;
                    border_thickness_dropdown_open = false;
                    needs_redraw = true;
                } else if (over_font_family_toggle) {
                    text_box_focused = false;
                    font_family_dropdown_open = !font_family_dropdown_open;
                    bg_picker_open = false;
                    fg_picker_open = false;
                    font_size_dropdown_open = false;
                    border_style_dropdown_open = false;
                    border_thickness_dropdown_open = false;
                    needs_redraw = true;
                } else if (over_border_style_toggle) {
                    text_box_focused = false;
                    border_style_dropdown_open = !border_style_dropdown_open;
                    bg_picker_open = false;
                    fg_picker_open = false;
                    font_size_dropdown_open = false;
                    font_family_dropdown_open = false;
                    border_thickness_dropdown_open = false;
                    needs_redraw = true;
                } else if (over_border_thickness_toggle) {
                    text_box_focused = false;
                    border_thickness_dropdown_open = !border_thickness_dropdown_open;
                    bg_picker_open = false;
                    fg_picker_open = false;
                    font_size_dropdown_open = false;
                    font_family_dropdown_open = false;
                    border_style_dropdown_open = false;
                    needs_redraw = true;
                } else if (over_line_numbers_toggle) {
                    text_box_focused = false;
                    show_line_numbers = !show_line_numbers;
                    border_style_dropdown_open = false;
                    border_thickness_dropdown_open = false;
                    needs_redraw = true;
                } else if (over_round_top_left_toggle) {
                    text_box_focused = false;
                    round_top_left = !round_top_left;
                    needs_redraw = true;
                } else if (over_round_top_right_toggle) {
                    text_box_focused = false;
                    round_top_right = !round_top_right;
                    needs_redraw = true;
                } else if (over_round_bottom_right_toggle) {
                    text_box_focused = false;
                    round_bottom_right = !round_bottom_right;
                    needs_redraw = true;
                } else if (over_round_bottom_left_toggle) {
                    text_box_focused = false;
                    round_bottom_left = !round_bottom_left;
                    needs_redraw = true;
                } else if (over_radio_toggle) {
                    text_box_focused = false;
                    if (radio_enabled) {
                        radio_selected = !radio_selected;
                    }
                    needs_redraw = true;
                } else if (over_checkbox_toggle) {
                    text_box_focused = false;
                    if (checkbox_enabled) {
                        checkbox_checked = !checkbox_checked;
                    }
                    needs_redraw = true;
                } else if (over_radio_selector_option_a_toggle) {
                    text_box_focused = false;
                    if (radio_selector_enabled) {
                        radio_selector_second_selected = false;
                    }
                    needs_redraw = true;
                } else if (over_radio_selector_option_b_toggle) {
                    text_box_focused = false;
                    if (radio_selector_enabled) {
                        radio_selector_second_selected = true;
                    }
                    needs_redraw = true;
                } else if (over_checkbox_group_foo_toggle) {
                    text_box_focused = false;
                    if (checkbox_group_enabled) {
                        checkbox_group_first_checked = !checkbox_group_first_checked;
                    }
                    needs_redraw = true;
                } else if (over_checkbox_group_bar_toggle) {
                    text_box_focused = false;
                    if (checkbox_group_enabled) {
                        checkbox_group_second_checked = !checkbox_group_second_checked;
                    }
                    needs_redraw = true;
                } else if (over_radio_enabled_toggle) {
                    text_box_focused = false;
                    radio_enabled = !radio_enabled;
                    needs_redraw = true;
                } else if (over_checkbox_enabled_toggle) {
                    text_box_focused = false;
                    checkbox_enabled = !checkbox_enabled;
                    needs_redraw = true;
                } else if (over_radio_selector_enabled_toggle) {
                    text_box_focused = false;
                    radio_selector_enabled = !radio_selector_enabled;
                    needs_redraw = true;
                } else if (over_checkbox_group_enabled_toggle) {
                    text_box_focused = false;
                    checkbox_group_enabled = !checkbox_group_enabled;
                    needs_redraw = true;
                } else if (bg_picker_open && pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 5 * 22 && pointer.y >= bg_palette_y && pointer.y <= bg_palette_y + 40) {
                    text_box_focused = false;
                    const int col = (pointer.x - picker_box_x0) / 22;
                    const int row = (pointer.y - bg_palette_y) / 20;
                    const int idx = row * 5 + col;
                    if (idx >= 0 && idx < static_cast<int>(swatches.size())) {
                        bg_r = swatches[idx].r;
                        bg_g = swatches[idx].g;
                        bg_b = swatches[idx].b;
                        bg_picker_open = false;
                        needs_redraw = true;
                    }
                } else if (fg_picker_open && pointer.x >= picker_box_x0 && pointer.x <= picker_box_x0 + 5 * 22 && pointer.y >= fg_palette_y && pointer.y <= fg_palette_y + 40) {
                    text_box_focused = false;
                    const int col = (pointer.x - picker_box_x0) / 22;
                    const int row = (pointer.y - fg_palette_y) / 20;
                    const int idx = row * 5 + col;
                    if (idx >= 0 && idx < static_cast<int>(swatches.size())) {
                        fg_r = swatches[idx].r;
                        fg_g = swatches[idx].g;
                        fg_b = swatches[idx].b;
                        fg_picker_open = false;
                        needs_redraw = true;
                    }
                } else if (font_size_dropdown_open && pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 && pointer.y >= font_size_list_y - 2 && pointer.y <= font_size_list_y + static_cast<int>(font_sizes.size()) * 18) {
                    text_box_focused = false;
                    const int idx = (pointer.y - font_size_list_y) / 18;
                    if (idx >= 0 && idx < static_cast<int>(font_sizes.size())) {
                        font_size = font_sizes[idx];
                        font_size_dropdown_open = false;
                        needs_redraw = true;
                    }
                } else if (font_family_dropdown_open && pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 && pointer.y >= font_family_list_y - 2 && pointer.y <= font_family_list_y + static_cast<int>(font_labels.size()) * 18) {
                    text_box_focused = false;
                    const int idx = (pointer.y - font_family_list_y) / 18;
                    if (idx >= 0 && idx < static_cast<int>(font_labels.size())) {
                        font_type = idx;
                        font_family_dropdown_open = false;
                        needs_redraw = true;
                    }
                } else if (border_style_dropdown_open && pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 && pointer.y >= border_style_list_y - 2 && pointer.y <= border_style_list_y + static_cast<int>(border_style_labels.size()) * 18) {
                    text_box_focused = false;
                    const int idx = (pointer.y - border_style_list_y) / 18;
                    if (idx >= 0 && idx < static_cast<int>(border_style_labels.size())) {
                        border_style = idx;
                        border_style_dropdown_open = false;
                        needs_redraw = true;
                    }
                } else if (border_thickness_dropdown_open && pointer.x >= picker_box_x0 && pointer.x <= picker_box_x1 && pointer.y >= border_thickness_list_y - 2 && pointer.y <= border_thickness_list_y + static_cast<int>(border_thicknesses.size()) * 18) {
                    text_box_focused = false;
                    const int idx = (pointer.y - border_thickness_list_y) / 18;
                    if (idx >= 0 && idx < static_cast<int>(border_thicknesses.size())) {
                        border_thickness = border_thicknesses[idx];
                        border_thickness_dropdown_open = false;
                        needs_redraw = true;
                    }
                }
                else if (over_text_box) {
                    text_box_focused = true;
                    needs_redraw = true;
                } else {
                    text_box_focused = false;
                    // Detect click in the middle panel (component list)
                    int comp_y = 128;
                    for (size_t i = 0; i < component_names.size(); ++i) {
                        int y0 = comp_y - 6;
                        int y1 = comp_y + 16;
                        if (pointer.x >= 12 && pointer.x <= component_splitter - 12 && pointer.y >= y0 && pointer.y <= y1) {
                            selected_component = static_cast<int>(i);
                            needs_redraw = true;
                            break;
                        }
                        comp_y += 24;
                        if (comp_y > h - 18) break;
                    }
                }
                }
            }
            // Dragging
            if (dragging_component_splitter && left && active_tab == BuilderTab::IndividualComponents) {
                component_splitter = pointer.x;
                needs_redraw = true;
            }
            // Release
            if (!left) {
                dragging_component_splitter = false;
            }
            prev_left = left;
        }

#if defined(_WIN32)
        if (text_box_focused) {
            if (apply_text_box_input(custom_text, previous_key_state)) {
                needs_redraw = true;
            }
        } else {
            for (std::size_t i = 0; i < previous_key_state.size(); ++i) {
                previous_key_state[i] = (GetAsyncKeyState(static_cast<int>(i)) & 0x8000) != 0;
            }
        }
#endif

        if (needs_redraw) {
            window.request_redraw();
            needs_redraw = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(6));
    }

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    const Options options = parse_args(argc, argv);

    app_assets::file_editor::TextFileEditor editor;
    if (!editor.load(options.xml_path)) {
        std::cerr << "Failed to open XML file: " << options.xml_path << "\n";
        return 1;
    }

    std::cout << "screen_builder loaded: " << options.xml_path << "\n";
    if (!options.force_cli) {
        std::cout << "Starting GUI editor mode (use --cli for terminal REPL mode).\n";
        return run_gui_editor(options, editor);
    }

    print_help();

    os_generics::xml_screen_descriptor::XmlScreenDescriptorParser parser;

    while (true) {
        std::cout << "\nscreen_builder> ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\nInput stream closed.\n";
            return 0;
        }

        input = trim(input);
        if (input.empty()) {
            continue;
        }

        std::stringstream ss(input);
        std::string command;
        ss >> command;

        if (command == "help") {
            print_help();
            continue;
        }

        if (command == "view") {
            std::string s_start;
            std::string s_count;
            ss >> s_start >> s_count;
            int start = 1;
            int count = 40;
            if (!s_start.empty()) {
                parse_int(s_start, start);
            }
            if (!s_count.empty()) {
                parse_int(s_count, count);
            }
            print_lines(editor, start, count);
            continue;
        }

        if (command == "set") {
            std::string s_line;
            ss >> s_line;
            int line_no = 0;
            if (!parse_int(s_line, line_no) || line_no < 1 || line_no > static_cast<int>(editor.lines().size())) {
                std::cout << "Invalid line number.\n";
                continue;
            }
            std::string replacement;
            std::getline(ss, replacement);
            replacement = trim(replacement);
            if (!editor.replace_line(line_no, replacement)) {
                std::cout << "Replace failed.\n";
                continue;
            }
            std::cout << "Updated line " << line_no << ".\n";
            continue;
        }

        if (command == "insert") {
            std::string s_line;
            ss >> s_line;
            int line_no = 0;
            if (!parse_int(s_line, line_no) || line_no < 1 || line_no > static_cast<int>(editor.lines().size()) + 1) {
                std::cout << "Invalid line number.\n";
                continue;
            }
            std::string text;
            std::getline(ss, text);
            text = trim(text);
            if (!editor.insert_line(line_no, text)) {
                std::cout << "Insert failed.\n";
                continue;
            }
            std::cout << "Inserted before line " << line_no << ".\n";
            continue;
        }

        if (command == "delete") {
            std::string s_line;
            ss >> s_line;
            int line_no = 0;
            if (!parse_int(s_line, line_no) || line_no < 1 || line_no > static_cast<int>(editor.lines().size())) {
                std::cout << "Invalid line number.\n";
                continue;
            }
            if (!editor.delete_line(line_no)) {
                std::cout << "Delete failed.\n";
                continue;
            }
            std::cout << "Deleted line " << line_no << ".\n";
            continue;
        }

        if (command == "save") {
            if (!editor.save()) {
                std::cout << "Save failed.\n";
            } else {
                std::cout << "Saved: " << options.xml_path << "\n";
            }
            continue;
        }

        if (command == "reload") {
            if (!editor.reload()) {
                std::cout << "Reload failed.\n";
            } else {
                std::cout << "Reloaded: " << options.xml_path << "\n";
            }
            continue;
        }

        if (command == "preview") {
            const std::string xml_text = editor.text();
            const auto parsed = parser.parse_string(xml_text);
            if (!parsed.ok()) {
                std::cout << "Parse errors:\n";
                for (const auto& error : parsed.errors) {
                    std::cout << "  - " << error << "\n";
                }
                std::cout << "Opening preview with error list.\n";
            }
            run_preview_window(xml_text, options.xml_path, parsed);
            continue;
        }

        if (command == "quit" || command == "exit" || command == "q") {
            std::cout << "Bye.\n";
            return 0;
        }

        std::cout << "Unknown command. Type 'help'.\n";
    }
}
