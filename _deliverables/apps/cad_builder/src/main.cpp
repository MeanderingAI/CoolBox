#include "cad_file_processing.hpp"
#include "cad_builder_window.hpp"
#include "components.hpp"
#include "full_application_window.hpp"
#include "graphics.h"
#include "os_dialog.hpp"
#include "ai_chat.hpp"
#include "xml_screen_descriptor_parser.hpp"

#include <cctype>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

namespace {

struct Options {
    std::string input_path;
    std::string output_path;
    std::string screen_xml_path;
    bool use_window = false;
    bool launch_gui = false;
};

struct AppState {
    cad_builder::CadBuilderWindow viewport{1280, 720};
};

struct UndoHistory {
    std::vector<std::string> undo_stack;
    std::vector<std::string> redo_stack;
};

enum class SelectionKind {
    None,
    Rectangle,
    Line,
    Arc
};

struct SelectionState {
    SelectionKind kind = SelectionKind::None;
    int index = -1;
};

enum class SnapMode {
    Off,
    Grid,
    Endpoint,
    Center
};

struct SnapSettings {
    SnapMode mode = SnapMode::Grid;
    int grid_size = 10;
    int threshold = 30;
};

struct SnapFeedback {
    bool snapped = false;
    int x = 0;
    int y = 0;
};

struct CommandHistory {
    std::vector<std::string> entries;
};

std::string serialize_scene(const AppState& state) {
    std::ostringstream out;
    out << "HKSCENE2\n";
    const auto camera = state.viewport.camera();
    out << "camera="
        << camera.pan_x << ","
        << camera.pan_y << ","
        << camera.yaw_degrees << ","
        << camera.pitch_degrees << ","
        << camera.zoom << "\n";
    out << "rectangles=" << state.viewport.rectangles().size() << "\n";
    for (const auto& rect : state.viewport.rectangles()) {
        out << "rect="
            << rect.x << ","
            << rect.y << ","
            << rect.width << ","
            << rect.height << ","
            << (rect.filled ? 1 : 0)
            << "\n";
    }
    out << "lines=" << state.viewport.lines().size() << "\n";
    for (const auto& line : state.viewport.lines()) {
        out << "line="
            << line.x1 << ","
            << line.y1 << ","
            << line.x2 << ","
            << line.y2
            << "\n";
    }
    out << "arcs=" << state.viewport.arcs().size() << "\n";
    for (const auto& arc : state.viewport.arcs()) {
        out << "arc="
            << arc.center_x << ","
            << arc.center_y << ","
            << arc.radius << ","
            << arc.start_degrees << ","
            << arc.end_degrees
            << "\n";
    }
    return out.str();
}

bool load_scene_from_text(AppState& state, const std::string& text) {
    std::istringstream in(text);
    std::string line;
    if (!std::getline(in, line) || (line != "HKSCENE1" && line != "HKSCENE2")) {
        return false;
    }

    std::vector<cad_builder::RectangleModel> loaded_rectangles;
    std::vector<cad_builder::LineModel> loaded_lines;
    std::vector<cad_builder::ArcModel> loaded_arcs;
    bool has_camera = false;
    cad_builder::CameraState loaded_camera = state.viewport.camera();

    while (std::getline(in, line)) {
        if (line.rfind("camera=", 0) == 0) {
            std::stringstream camera_stream(line.substr(7));
            std::string token;
            std::vector<double> values;
            while (std::getline(camera_stream, token, ',')) {
                try {
                    values.push_back(std::stod(token));
                } catch (...) {
                    values.clear();
                    break;
                }
            }
            if (values.size() == 5U) {
                loaded_camera.pan_x = values[0];
                loaded_camera.pan_y = values[1];
                loaded_camera.yaw_degrees = values[2];
                loaded_camera.pitch_degrees = values[3];
                loaded_camera.zoom = values[4];
                has_camera = true;
            }
            continue;
        }

        if (line.rfind("rect=", 0) == 0) {
            std::stringstream ss(line.substr(5));
            std::string token;
            std::vector<int> values;
            while (std::getline(ss, token, ',')) {
                try {
                    values.push_back(std::stoi(token));
                } catch (...) {
                    values.clear();
                    break;
                }
            }
            if (values.size() == 5U) {
                cad_builder::RectangleModel rect;
                rect.x = values[0];
                rect.y = values[1];
                rect.width = values[2];
                rect.height = values[3];
                rect.filled = values[4] != 0;
                loaded_rectangles.push_back(rect);
            }
            continue;
        }

        if (line.rfind("line=", 0) == 0) {
            std::stringstream ss(line.substr(5));
            std::string token;
            std::vector<int> values;
            while (std::getline(ss, token, ',')) {
                try {
                    values.push_back(std::stoi(token));
                } catch (...) {
                    values.clear();
                    break;
                }
            }
            if (values.size() == 4U) {
                cad_builder::LineModel model;
                model.x1 = values[0];
                model.y1 = values[1];
                model.x2 = values[2];
                model.y2 = values[3];
                loaded_lines.push_back(model);
            }
            continue;
        }

        if (line.rfind("arc=", 0) == 0) {
            std::stringstream ss(line.substr(4));
            std::string token;
            std::vector<int> values;
            while (std::getline(ss, token, ',')) {
                try {
                    values.push_back(std::stoi(token));
                } catch (...) {
                    values.clear();
                    break;
                }
            }
            if (values.size() == 5U) {
                cad_builder::ArcModel model;
                model.center_x = values[0];
                model.center_y = values[1];
                model.radius = values[2];
                model.start_degrees = values[3];
                model.end_degrees = values[4];
                loaded_arcs.push_back(model);
            }
            continue;
        }
    }

    state.viewport.set_rectangles(loaded_rectangles);
    state.viewport.set_lines(loaded_lines);
    state.viewport.set_arcs(loaded_arcs);
    if (has_camera) {
        state.viewport.set_camera(loaded_camera);
    }
    return true;
}

void push_undo_snapshot(const AppState& state, UndoHistory& history) {
    history.undo_stack.push_back(serialize_scene(state));
    if (history.undo_stack.size() > 64U) {
        history.undo_stack.erase(history.undo_stack.begin());
    }
    history.redo_stack.clear();
}

int snap_to_grid(int value, int grid_size) {
    if (grid_size <= 1) {
        return value;
    }
    const double scaled = static_cast<double>(value) / static_cast<double>(grid_size);
    return static_cast<int>(std::lround(scaled)) * grid_size;
}

void add_endpoint_candidates(const AppState& state, std::vector<std::pair<int, int>>& candidates) {
    for (const auto& line : state.viewport.lines()) {
        candidates.emplace_back(line.x1, line.y1);
        candidates.emplace_back(line.x2, line.y2);
    }
    for (const auto& rect : state.viewport.rectangles()) {
        candidates.emplace_back(rect.x, rect.y);
        candidates.emplace_back(rect.x + rect.width, rect.y);
        candidates.emplace_back(rect.x + rect.width, rect.y + rect.height);
        candidates.emplace_back(rect.x, rect.y + rect.height);
    }
}

void add_center_candidates(const AppState& state, std::vector<std::pair<int, int>>& candidates) {
    for (const auto& rect : state.viewport.rectangles()) {
        candidates.emplace_back(rect.x + rect.width / 2, rect.y + rect.height / 2);
    }
    for (const auto& arc : state.viewport.arcs()) {
        candidates.emplace_back(arc.center_x, arc.center_y);
    }
    for (const auto& line : state.viewport.lines()) {
        candidates.emplace_back((line.x1 + line.x2) / 2, (line.y1 + line.y2) / 2);
    }
}

SnapFeedback apply_snap_point(const AppState& state, const SnapSettings& snap, int& x, int& y) {
    SnapFeedback feedback;
    if (snap.mode == SnapMode::Off) {
        return feedback;
    }

    if (snap.mode == SnapMode::Grid) {
        x = snap_to_grid(x, snap.grid_size);
        y = snap_to_grid(y, snap.grid_size);
        feedback.snapped = true;
        feedback.x = x;
        feedback.y = y;
        return feedback;
    }

    std::vector<std::pair<int, int>> candidates;
    if (snap.mode == SnapMode::Endpoint) {
        add_endpoint_candidates(state, candidates);
    } else if (snap.mode == SnapMode::Center) {
        add_center_candidates(state, candidates);
    }

    if (candidates.empty()) {
        return feedback;
    }

    int best_x = x;
    int best_y = y;
    long long best_dist = static_cast<long long>(snap.threshold) * static_cast<long long>(snap.threshold) + 1;
    for (const auto& candidate : candidates) {
        const long long dx = static_cast<long long>(candidate.first) - static_cast<long long>(x);
        const long long dy = static_cast<long long>(candidate.second) - static_cast<long long>(y);
        const long long dist = dx * dx + dy * dy;
        if (dist < best_dist) {
            best_dist = dist;
            best_x = candidate.first;
            best_y = candidate.second;
        }
    }

    if (best_dist <= static_cast<long long>(snap.threshold) * static_cast<long long>(snap.threshold)) {
        x = best_x;
        y = best_y;
        feedback.snapped = true;
        feedback.x = x;
        feedback.y = y;
    }
    return feedback;
}

std::string snap_mode_to_string(SnapMode mode) {
    switch (mode) {
    case SnapMode::Off:
        return "off";
    case SnapMode::Grid:
        return "grid";
    case SnapMode::Endpoint:
        return "endpoint";
    case SnapMode::Center:
        return "center";
    }
    return "off";
}

void clear_selection_state(AppState& state, SelectionState& selection) {
    selection.kind = SelectionKind::None;
    selection.index = -1;
    state.viewport.clear_selection();
}

void print_usage() {
    std::cout << "cad_builder\n";
    std::cout << "Usage:\n";
    std::cout << "  cad_builder [--input <cad-file>] [--output <hk_cad-file>] [--window] [--launch-gui] [--screen-xml <file>]\n";
    std::cout << "\n";
    std::cout << "If input/output are omitted, cad_builder starts a blank workspace menu:\n";
    std::cout << "  1) Open CAD file\n";
    std::cout << "  2) New blank project (add rectangles)\n";
}

Options parse_args(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_usage();
            std::exit(0);
        }

        if (arg == "--window") {
            options.use_window = true;
            continue;
        }

        if (arg == "--launch-gui") {
            options.launch_gui = true;
            continue;
        }

        if (arg == "--screen-xml" && i + 1 < argc) {
            options.screen_xml_path = argv[++i];
            continue;
        }

        if (arg == "--input" && i + 1 < argc) {
            options.input_path = argv[++i];
            continue;
        }

        if (arg == "--output" && i + 1 < argc) {
            options.output_path = argv[++i];
            continue;
        }

        std::cerr << "Unknown or incomplete argument: " << arg << "\n";
        print_usage();
        std::exit(2);
    }

    return options;
}

std::string pick_input_with_dialog() {
    app_builder::os_generics::OsDialog dialog;
    app_builder::os_generics::DialogRequest request;
    request.action = app_builder::os_generics::DialogAction::OpenFile;
    request.title = "Select CAD File";
    request.initial_path = std::filesystem::current_path().string();
    request.allow_native_ui = true;
    request.filters = {
        {"CAD Files", {"*.dwg", "*.step", "*.stp", "*.stl", "*.sldprt", "*.sldasm", "*.slddrw", "*.rvt", "*.iges", "*.igs", "*.sat", "*.CATPart", "*.CATProduct", "*.ipt", "*.aim", "*.3dm", "*.jt"}},
        {"All Files", {"*.*"}}
    };

    const auto result = dialog.show(request);
    if (!result.accepted || result.selected_paths.empty()) {
        return std::string();
    }

    return result.selected_paths.front();
}

std::string pick_output_with_dialog(const std::string& input_path) {
    app_builder::os_generics::OsDialog dialog;
    app_builder::os_generics::DialogRequest request;
    request.action = app_builder::os_generics::DialogAction::SaveFile;
    request.title = "Save .hk_cad File";
    request.initial_path = std::filesystem::path(input_path).parent_path().string();
    request.suggested_name = std::filesystem::path(input_path).stem().string() + ".hk_cad";
    request.allow_native_ui = true;
    request.filters = {
        {"HK CAD", {"*.hk_cad"}},
        {"All Files", {"*.*"}}
    };

    const auto result = dialog.show(request);
    if (!result.accepted || result.selected_paths.empty()) {
        return std::string();
    }

    return result.selected_paths.front();
}

std::string pick_scene_path_with_dialog(bool save_mode) {
    std::cout << (save_mode ? "Scene save path (.hk_scene)" : "Scene load path (.hk_scene)")
              << " [blank_project.hk_scene]: ";
    std::string path;
    std::getline(std::cin, path);
    if (path.empty()) {
        path = "blank_project.hk_scene";
    }
    return path;
}

void configure_window_shell() {
    using namespace graphics::components;
    using namespace graphics::full_application_window;

    WindowConfig config("cad_builder", 1100, 760, true, true);
    FullApplicationWindow window(config);

    MenuBarModel menu_bar;
    MenuModel file_menu("File");
    file_menu.add_item(MenuItem::action("Open CAD", "Ctrl+O"));
    file_menu.add_item(MenuItem::action("Export HK CAD", "Ctrl+E"));
    file_menu.add_item(MenuItem::divider());
    file_menu.add_item(MenuItem::action("Exit", "Alt+F4"));
    menu_bar.add_menu(file_menu);

    window.set_menu_bar(menu_bar);
    window.set_platform_style(graphics::windows::PlatformStyle::Windows);
    window.create();
    window.show();

    std::cout << "Configured GRAPHICS shell with backend: " << window.backend_name() << "\n";
}

int read_int_or_default(const std::string& prompt, int default_value) {
    std::cout << prompt << " [" << default_value << "]: ";
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) {
        return default_value;
    }
    try {
        return std::stoi(line);
    } catch (...) {
        return default_value;
    }
}

bool read_yes_no(const std::string& prompt, bool default_value) {
    std::cout << prompt << " [" << (default_value ? "Y/n" : "y/N") << "]: ";
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) {
        return default_value;
    }
    const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
    return c == 'y';
}

bool save_scene_file(const AppState& state, const std::string& path) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }

    out << serialize_scene(state);
    return static_cast<bool>(out);
}

bool load_scene_file(AppState& state, const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    return load_scene_from_text(state, buffer.str());
}

struct GuiDescriptorRuntime {
    std::string main_title = "cad_builder interactive viewport";
    int main_width = 1280;
    int main_height = 720;
    graphics::windows::PlatformStyle platform = graphics::windows::PlatformStyle::Windows;

    bool ai_open_by_default = false;
    bool inspector_open_by_default = false;
    bool toolbox_open_by_default = false;

    std::string ai_title = "AI Chat (app_assets)";
    std::string inspector_title = "Inspector";
    std::string toolbox_title = "Toolbox";
    bool has_main_menu_bar = false;
    graphics::components::MenuBarModel main_menu_bar;

    std::vector<std::string> main_toolbar_actions;
    std::vector<std::string> ai_layout_lines;
    std::vector<std::string> inspector_layout_lines;
    std::vector<std::string> toolbox_tool_labels = {"Select", "Rectangle", "Line", "Arc"};
};

std::string normalize_lower(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

bool contains_token(const std::string& text, const std::string& token) {
    return normalize_lower(text).find(normalize_lower(token)) != std::string::npos;
}

void collect_layout_groups(
    const graphics::components::ComponentHolder& holder,
    std::vector<std::pair<std::string, const graphics::components::ComponentHolder*>>& out_groups) {
    for (const auto& component : holder.components()) {
        if (component.type() == graphics::components::ComponentType::LayoutGroup) {
            const auto* nested = component.layout_group();
            if (nested != nullptr) {
                out_groups.emplace_back(component.label(), nested);
                collect_layout_groups(*nested, out_groups);
            }
        }
    }
}

std::vector<std::string> extract_tool_labels(const graphics::components::ComponentHolder& holder) {
    std::vector<std::string> labels;
    for (const auto& component : holder.components()) {
        if (component.type() == graphics::components::ComponentType::RadioButton ||
            component.type() == graphics::components::ComponentType::Button) {
            if (!component.label().empty()) {
                labels.push_back(component.label());
            }
        }
        if (component.type() == graphics::components::ComponentType::LayoutGroup) {
            const auto* nested = component.layout_group();
            if (nested != nullptr) {
                const auto nested_labels = extract_tool_labels(*nested);
                labels.insert(labels.end(), nested_labels.begin(), nested_labels.end());
            }
        }
    }
    return labels;
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

GuiDescriptorRuntime load_gui_descriptor_runtime(const Options& options) {
    GuiDescriptorRuntime runtime;

    std::filesystem::path xml_path;
    if (!options.screen_xml_path.empty()) {
        xml_path = options.screen_xml_path;
    } else {
        const std::filesystem::path default_path =
            std::filesystem::path("_deliverables") / "apps" / "cad_builder" / "assets" / "default_screen_layout.xml";
        if (std::filesystem::exists(default_path)) {
            xml_path = default_path;
        }
    }

    if (xml_path.empty() || !std::filesystem::exists(xml_path)) {
        return runtime;
    }

    os_generics::xml_screen_descriptor::XmlScreenDescriptorParser parser;
    const auto parsed = parser.parse_file(xml_path.string());
    if (!parsed.ok()) {
        std::cout << "XML descriptor parse errors (" << xml_path.string() << "):\n";
        for (const auto& error : parsed.errors) {
            std::cout << "  - " << error << "\n";
        }
        return runtime;
    }

    runtime.main_title = parsed.descriptor.title;
    runtime.main_width = static_cast<int>(std::max<std::size_t>(640U, parsed.descriptor.width));
    runtime.main_height = static_cast<int>(std::max<std::size_t>(480U, parsed.descriptor.height));
    runtime.platform = parsed.descriptor.platform;

    std::vector<const graphics::components::Component*> all_components;
    collect_components_recursive(parsed.descriptor.layout, all_components);
    for (const auto* component : all_components) {
        if (component == nullptr) {
            continue;
        }
        if (!runtime.has_main_menu_bar && component->type() == graphics::components::ComponentType::MenuBar) {
            const auto* model = component->menu_bar_model();
            if (model != nullptr) {
                runtime.main_menu_bar = *model;
                runtime.has_main_menu_bar = true;
            }
        }
        if (component->type() == graphics::components::ComponentType::Toolbar) {
            const auto* toolbar = component->toolbar_model();
            if (toolbar != nullptr && !toolbar->actions.empty()) {
                runtime.main_toolbar_actions = toolbar->actions;
            }
        }
    }

    if (!runtime.main_toolbar_actions.empty()) {
        runtime.toolbox_tool_labels = runtime.main_toolbar_actions;
    }

    std::vector<std::pair<std::string, const graphics::components::ComponentHolder*>> groups;
    collect_layout_groups(parsed.descriptor.layout, groups);

    for (const auto& group : groups) {
        const std::string label = group.first;
        const auto* holder = group.second;
        if (holder == nullptr) {
            continue;
        }

        if (contains_token(label, "ai")) {
            runtime.ai_title = label.empty() ? runtime.ai_title : label;
            runtime.ai_layout_lines = holder->render();
            continue;
        }

        if (contains_token(label, "inspect")) {
            runtime.inspector_title = label.empty() ? runtime.inspector_title : label;
            runtime.inspector_layout_lines = holder->render();
            continue;
        }

        if (contains_token(label, "tool")) {
            runtime.toolbox_title = label.empty() ? runtime.toolbox_title : label;
            const auto labels = extract_tool_labels(*holder);
            if (!labels.empty()) {
                runtime.toolbox_tool_labels = labels;
            }
        }
    }

    std::cout << "Loaded GUI XML descriptor: " << xml_path.string() << "\n";
    return runtime;
}

void add_rectangle_to_blank_project(AppState& state, const SnapSettings& snap) {
    cad_builder::RectangleModel rectangle;
    rectangle.x = read_int_or_default("Rectangle x", 100);
    rectangle.y = read_int_or_default("Rectangle y", 100);
    rectangle.width = read_int_or_default("Rectangle width", 200);
    rectangle.height = read_int_or_default("Rectangle height", 120);
    rectangle.filled = read_yes_no("Filled rectangle?", true);

    const SnapFeedback feedback = apply_snap_point(state, snap, rectangle.x, rectangle.y);
    state.viewport.set_snap_marker(feedback.x, feedback.y, feedback.snapped);

    state.viewport.add_rectangle(rectangle);
    std::cout << "Added rectangle #" << state.viewport.rectangle_count() << "\n";
}

void add_line_to_blank_project(AppState& state, const SnapSettings& snap) {
    cad_builder::LineModel line;
    line.x1 = read_int_or_default("Line x1", 100);
    line.y1 = read_int_or_default("Line y1", 100);
    line.x2 = read_int_or_default("Line x2", 220);
    line.y2 = read_int_or_default("Line y2", 160);
    const SnapFeedback start_feedback = apply_snap_point(state, snap, line.x1, line.y1);
    const SnapFeedback end_feedback = apply_snap_point(state, snap, line.x2, line.y2);
    const SnapFeedback feedback = end_feedback.snapped ? end_feedback : start_feedback;
    state.viewport.set_snap_marker(feedback.x, feedback.y, feedback.snapped);
    state.viewport.add_line(line);
    std::cout << "Added line #" << state.viewport.lines().size() << "\n";
}

void add_arc_to_blank_project(AppState& state, const SnapSettings& snap) {
    cad_builder::ArcModel arc;
    arc.center_x = read_int_or_default("Arc center x", 220);
    arc.center_y = read_int_or_default("Arc center y", 220);
    arc.radius = std::max(1, read_int_or_default("Arc radius", 80));
    arc.start_degrees = read_int_or_default("Arc start degrees", 0);
    arc.end_degrees = read_int_or_default("Arc end degrees", 120);
    const SnapFeedback feedback = apply_snap_point(state, snap, arc.center_x, arc.center_y);
    state.viewport.set_snap_marker(feedback.x, feedback.y, feedback.snapped);
    state.viewport.add_arc(arc);
    std::cout << "Added arc #" << state.viewport.arcs().size() << "\n";
}

void list_rectangles(const AppState& state) {
    const auto& rectangles = state.viewport.rectangles();
    if (rectangles.empty()) {
        std::cout << "Rectangles: none\n";
    } else {
        std::cout << "Rectangles:\n";
        for (std::size_t i = 0; i < rectangles.size(); ++i) {
            const auto& rect = rectangles[i];
            const bool selected = static_cast<int>(i) == state.viewport.selected_index();
            std::cout << "  [" << i << "]"
                      << (selected ? " * " : "   ")
                      << "x=" << rect.x
                      << " y=" << rect.y
                      << " w=" << rect.width
                      << " h=" << rect.height
                      << " filled=" << (rect.filled ? "yes" : "no")
                      << "\n";
        }
    }

    const auto& lines = state.viewport.lines();
    if (!lines.empty()) {
        std::cout << "Lines:\n";
        for (std::size_t i = 0; i < lines.size(); ++i) {
            const auto& line = lines[i];
            const bool selected = static_cast<int>(i) == state.viewport.selected_line_index();
            std::cout << "  [" << i << "]"
                      << (selected ? " * " : "   ")
                      << " x1=" << line.x1
                      << " y1=" << line.y1
                      << " x2=" << line.x2
                      << " y2=" << line.y2
                      << "\n";
        }
    } else {
        std::cout << "Lines: none\n";
    }

    const auto& arcs = state.viewport.arcs();
    if (!arcs.empty()) {
        std::cout << "Arcs:\n";
        for (std::size_t i = 0; i < arcs.size(); ++i) {
            const auto& arc = arcs[i];
            const bool selected = static_cast<int>(i) == state.viewport.selected_arc_index();
            std::cout << "  [" << i << "]"
                      << (selected ? " * " : "   ")
                      << " cx=" << arc.center_x
                      << " cy=" << arc.center_y
                      << " r=" << arc.radius
                      << " start=" << arc.start_degrees
                      << " end=" << arc.end_degrees
                      << "\n";
        }
    } else {
        std::cout << "Arcs: none\n";
    }
}

void print_camera_state(const AppState& state) {
    const auto camera = state.viewport.camera();
    std::cout << "Camera"
              << " pan=(" << camera.pan_x << ", " << camera.pan_y << ")"
              << " yaw=" << camera.yaw_degrees
              << " pitch=" << camera.pitch_degrees
              << " zoom=" << camera.zoom
              << "\n";
}

void save_blank_preview(const AppState& state) {
    std::cout << "Preview output path (.png or .bmp): ";
    std::string path;
    std::getline(std::cin, path);
    if (path.empty()) {
        path = "cad_builder_preview.png";
    }

    const bool ok = state.viewport.save_view(path);

    if (ok) {
        std::cout << "Saved preview: " << path << "\n";
    } else {
        std::cout << "Failed to save preview: " << path << "\n";
    }
}

void save_blank_scene(const AppState& state) {
    std::string path = pick_scene_path_with_dialog(true);
    if (path.empty()) {
        std::cout << "Scene save canceled.\n";
        return;
    }

    if (save_scene_file(state, path)) {
        std::cout << "Saved scene: " << path << "\n";
    } else {
        std::cout << "Failed to save scene: " << path << "\n";
    }
}

void load_blank_scene(AppState& state) {
    std::string path = pick_scene_path_with_dialog(false);
    if (path.empty()) {
        std::cout << "Scene load canceled.\n";
        return;
    }

    if (load_scene_file(state, path)) {
        std::cout << "Loaded scene: " << path
                  << " (rectangles: " << state.viewport.rectangle_count() << ")\n";
    } else {
        std::cout << "Failed to load scene: " << path << "\n";
    }
}

void save_live_view(const AppState& state) {
    const std::filesystem::path live_preview = std::filesystem::path("build") / "cad_builder_live_view.png";
    if (state.viewport.save_view(live_preview.string())) {
        std::cout << "Viewport updated: " << live_preview.string() << "\n";
    }
}

bool launch_viewport_gui(AppState& state, std::string& out_message, const Options& options) {
    using namespace graphics::full_application_window;

    auto gpu_backend_summary = []() {
        std::string summary;
#if defined(COOLBOX_HAS_CUDA)
        summary += "CUDA ";
#endif
#if defined(COOLBOX_HAS_OPENCL)
        summary += "OpenCL ";
#endif
#if defined(COOLBOX_HAS_OPENGL)
        summary += "OpenGL ";
#endif
        if (summary.empty()) {
            return std::string("none detected");
        }
        return summary;
    };

    enum class GuiTool {
        Select,
        Rectangle,
        Line,
        Arc
    };

    const GuiDescriptorRuntime descriptor_runtime = load_gui_descriptor_runtime(options);

    GuiTool active_tool = GuiTool::Select;
    bool ai_chat_dialog_open = descriptor_runtime.ai_open_by_default;
    bool inspector_dialog_open = descriptor_runtime.inspector_open_by_default;
    bool toolbox_dialog_open = descriptor_runtime.toolbox_open_by_default;
    tools::ai_chat::AiChatSpeechBridge ai_chat_bridge;
    std::string ai_chat_last_result = "No command interpreted yet.";
    bool drawing_active = false;
    int draw_start_x = 0;
    int draw_start_y = 0;
    int draw_last_x = 0;
    int draw_last_y = 0;
    bool prev_left_down = false;
    bool prev_ai_left_down = false;
    bool prev_inspector_left_down = false;
    bool prev_toolbox_left_down = false;

    std::unique_ptr<FullApplicationWindow> ai_chat_window;
    std::unique_ptr<FullApplicationWindow> inspector_window;
    std::unique_ptr<FullApplicationWindow> toolbox_window;

    int client_width = 1280;
    int client_height = 720;

    auto clamp_to_canvas = [](int value, int max_value) {
        if (max_value <= 0) {
            return 0;
        }
        if (value < 0) {
            return 0;
        }
        if (value >= max_value) {
            return max_value - 1;
        }
        return value;
    };

    auto tool_name = [&](GuiTool tool) {
        switch (tool) {
        case GuiTool::Select:
            return std::string("select");
        case GuiTool::Rectangle:
            return std::string("rectangle");
        case GuiTool::Line:
            return std::string("line");
        case GuiTool::Arc:
            return std::string("arc");
        }
        return std::string("select");
    };

    auto gui_pick_scene_path = [&](bool save_mode) -> std::string {
        app_builder::os_generics::OsDialog dialog;
        app_builder::os_generics::DialogRequest request;
        request.action = save_mode
            ? app_builder::os_generics::DialogAction::SaveFile
            : app_builder::os_generics::DialogAction::OpenFile;
        request.title = save_mode ? "Save Scene (.hk_scene)" : "Load Scene (.hk_scene)";
        request.initial_path = std::filesystem::current_path().string();
        request.allow_native_ui = true;
        request.filters = {
            {"HK Scene", {"*.hk_scene"}},
            {"All Files", {"*.*"}}
        };
        if (save_mode) {
            request.suggested_name = "blank_project.hk_scene";
        }

        const auto result = dialog.show(request);
        if (!result.accepted || result.selected_paths.empty()) {
            return std::string();
        }
        return result.selected_paths.front();
    };

    auto select_rectangle_at_canvas_point = [&](int cx, int cy) {
        const auto& rectangles = state.viewport.rectangles();
        for (int i = static_cast<int>(rectangles.size()) - 1; i >= 0; --i) {
            const auto& rect = rectangles[static_cast<std::size_t>(i)];
            if (cx >= rect.x && cx <= rect.x + rect.width && cy >= rect.y && cy <= rect.y + rect.height) {
                state.viewport.set_selected_index(static_cast<std::size_t>(i));
                state.viewport.set_status_text("tool=select | rectangle #" + std::to_string(i));
                return;
            }
        }
        state.viewport.clear_selection();
        state.viewport.set_status_text("tool=select | no selection");
    };

    WindowConfig config(descriptor_runtime.main_title,
                        descriptor_runtime.main_width,
                        descriptor_runtime.main_height,
                        true,
                        true);
    FullApplicationWindow window(config);

    graphics::components::MenuBarModel menu_bar;
    if (descriptor_runtime.has_main_menu_bar && descriptor_runtime.main_menu_bar.menus.size() >= 2U) {
        menu_bar = descriptor_runtime.main_menu_bar;
    } else {
        graphics::components::MenuModel file_menu("File");
        file_menu.add_item(graphics::components::MenuItem::action("Open CAD", "Ctrl+O"));
        file_menu.add_item(graphics::components::MenuItem::action("Export HK CAD", "Ctrl+E"));
        file_menu.add_item(graphics::components::MenuItem::action("About Release"));
        file_menu.add_item(graphics::components::MenuItem::divider());
        file_menu.add_item(graphics::components::MenuItem::action("Exit", "Alt+F4"));

        graphics::components::MenuModel views_menu("Views");
        views_menu.add_item(graphics::components::MenuItem::action("AI Chat Dialog"));
        views_menu.add_item(graphics::components::MenuItem::action("Inspector Dialog"));
        views_menu.add_item(graphics::components::MenuItem::action("Toolbox Selector"));
        menu_bar.add_menu(file_menu);
        menu_bar.add_menu(views_menu);
    }
    window.set_menu_bar(menu_bar);
    window.set_platform_style(descriptor_runtime.platform);

#if defined(_WIN32)
    struct MouseDragState {
        bool active = false;
        int last_x = 0;
        int last_y = 0;
    } left_drag;

    struct RotationDragState {
        bool active = false;
        int last_x = 0;
        int last_y = 0;
    } right_drag;
#endif

    RenderHooks hooks;
    hooks.on_render = [&](const RenderEvent& event) {
        (void)event;
        window.present_canvas(state.viewport.canvas());
    };

    window.set_render_hooks(std::move(hooks));
    if (!window.create()) {
        out_message = "Failed to create full_application_window.";
        return false;
    }
    std::cout << "GPU backends detected: " << gpu_backend_summary() << "\n";
#if defined(_WIN32) && defined(COOLBOX_HAS_OPENGL)
    std::cout << "Renderer path: OpenGL texture blit (GPU present) with CPU native-blit fallback.\n";
#else
    std::cout << "Renderer path: CPU canvas raster + native window blit.\n";
#endif
    window.show();
    window.request_redraw();

#if defined(_WIN32)
    auto sync_view_menu_checks = [&]() {
        window.set_menu_item_checked(1, 0, ai_chat_dialog_open);
        window.set_menu_item_checked(1, 1, inspector_dialog_open);
        window.set_menu_item_checked(1, 2, toolbox_dialog_open);
    };

    window.set_menu_command_handler([&](std::size_t menu_index,
                                        std::size_t item_index,
                                        const graphics::components::MenuModel&,
                                        const graphics::components::MenuItem&) {
        if (menu_index == 0 && item_index == 0) {
            const std::string path = gui_pick_scene_path(false);
            if (!path.empty()) {
                if (load_scene_file(state, path)) {
                    state.viewport.set_status_text("loaded: " + path);
                } else {
                    state.viewport.set_status_text("load failed: " + path);
                }
            }
            return;
        }

        if (menu_index == 0 && item_index == 1) {
            const std::string path = gui_pick_scene_path(true);
            if (!path.empty()) {
                if (save_scene_file(state, path)) {
                    state.viewport.set_status_text("saved: " + path);
                } else {
                    state.viewport.set_status_text("save failed: " + path);
                }
            }
            return;
        }

        if (menu_index == 0 && item_index == 2) {
            window.show_info_dialog("About cad_builder",
                                    "cad_builder\nRelease: v1.3.4\nInteractive viewport with OpenGL fallback path.");
            return;
        }

        if (menu_index == 0 && item_index == 4) {
            window.close();
            return;
        }

        if (menu_index == 1 && item_index == 0) {
            ai_chat_dialog_open = !ai_chat_dialog_open;
            state.viewport.set_status_text(std::string("views | ai_chat=") + (ai_chat_dialog_open ? "open" : "closed"));
            sync_view_menu_checks();
            return;
        }

        if (menu_index == 1 && item_index == 1) {
            inspector_dialog_open = !inspector_dialog_open;
            state.viewport.set_status_text(std::string("views | inspector=") + (inspector_dialog_open ? "open" : "closed"));
            sync_view_menu_checks();
            return;
        }

        if (menu_index == 1 && item_index == 2) {
            toolbox_dialog_open = !toolbox_dialog_open;
            state.viewport.set_status_text(std::string("views | toolbox=") + (toolbox_dialog_open ? "open" : "closed"));
            sync_view_menu_checks();
            return;
        }
    });

    sync_view_menu_checks();

    auto ensure_ai_chat_window = [&]() {
        if (!ai_chat_dialog_open || ai_chat_window) {
            return;
        }
        WindowConfig cfg(descriptor_runtime.ai_title, 430, 240, true, true);
        ai_chat_window = std::make_unique<FullApplicationWindow>(cfg);
        RenderHooks chat_hooks;
        chat_hooks.on_render = [&](const RenderEvent& event) {
            (void)event;
            if (!ai_chat_window) {
                return;
            }
            int w = 0;
            int h = 0;
            if (!ai_chat_window->client_size(w, h)) {
                return;
            }
            ai_chat_window->clear_background(24, 30, 38);
            int y = 12;
            if (!descriptor_runtime.ai_layout_lines.empty()) {
                for (const auto& line : descriptor_runtime.ai_layout_lines) {
                    ai_chat_window->draw_text_line(14, y, line);
                    y += 20;
                    if (y > 96) {
                        break;
                    }
                }
            } else {
                ai_chat_window->draw_text_line(14, y, descriptor_runtime.ai_title);
                y += 30;
                ai_chat_window->draw_text_line(14, y, "Click: Demo 'please add line'");
                y += 28;
            }
            ai_chat_window->draw_text_line(14, y, ai_chat_last_result);
            y += 28;
            ai_chat_window->draw_text_line(14, y, "Click top-right X zone to close");
            ai_chat_window->fill_rect(14, 136, 220, 168, 54, 88, 126);
            ai_chat_window->draw_text_line(24, 146, "Run demo");
            ai_chat_window->fill_rect(w - 34, 8, w - 8, 28, 110, 58, 58);
            ai_chat_window->draw_text_line(w - 24, 12, "X");
        };
        ai_chat_window->set_render_hooks(std::move(chat_hooks));
        if (!ai_chat_window->create()) {
            ai_chat_window.reset();
            ai_chat_dialog_open = false;
            return;
        }
        ai_chat_window->show();
    };

    auto ensure_inspector_window = [&]() {
        if (!inspector_dialog_open || inspector_window) {
            return;
        }
        WindowConfig cfg(descriptor_runtime.inspector_title, 420, 240, true, true);
        inspector_window = std::make_unique<FullApplicationWindow>(cfg);
        RenderHooks inspector_hooks;
        inspector_hooks.on_render = [&](const RenderEvent& event) {
            (void)event;
            if (!inspector_window) {
                return;
            }
            int w = 0;
            int h = 0;
            if (!inspector_window->client_size(w, h)) {
                return;
            }
            inspector_window->clear_background(30, 36, 42);
            const auto camera = state.viewport.camera();
            const std::string cam_line =
                "cam pan(" + std::to_string(static_cast<int>(camera.pan_x)) + "," +
                std::to_string(static_cast<int>(camera.pan_y)) + ") yaw=" +
                std::to_string(static_cast<int>(camera.yaw_degrees)) + " pitch=" +
                std::to_string(static_cast<int>(camera.pitch_degrees));
            const std::string count_line =
                "rect/line/arc: " + std::to_string(state.viewport.rectangles().size()) + "/" +
                std::to_string(state.viewport.lines().size()) + "/" +
                std::to_string(state.viewport.arcs().size());
            const std::string tool_line = "tool: " + tool_name(active_tool);
            int y = 12;
            if (!descriptor_runtime.inspector_layout_lines.empty()) {
                for (const auto& line : descriptor_runtime.inspector_layout_lines) {
                    inspector_window->draw_text_line(14, y, line);
                    y += 20;
                    if (y > 86) {
                        break;
                    }
                }
            } else {
                inspector_window->draw_text_line(14, y, descriptor_runtime.inspector_title);
                y += 30;
            }
            inspector_window->draw_text_line(14, y, cam_line);
            y += 24;
            inspector_window->draw_text_line(14, y, count_line);
            y += 24;
            inspector_window->draw_text_line(14, y, tool_line);
            y += 24;
            inspector_window->draw_text_line(14, y, "Close with X in top-right");
            inspector_window->fill_rect(w - 34, 8, w - 8, 28, 110, 58, 58);
            inspector_window->draw_text_line(w - 24, 12, "X");
        };
        inspector_window->set_render_hooks(std::move(inspector_hooks));
        if (!inspector_window->create()) {
            inspector_window.reset();
            inspector_dialog_open = false;
            return;
        }
        inspector_window->show();
    };

    auto ensure_toolbox_window = [&]() {
        if (!toolbox_dialog_open || toolbox_window) {
            return;
        }
        WindowConfig cfg(descriptor_runtime.toolbox_title, 240, 230, true, true);
        toolbox_window = std::make_unique<FullApplicationWindow>(cfg);
        RenderHooks toolbox_hooks;
        toolbox_hooks.on_render = [&](const RenderEvent& event) {
            (void)event;
            if (!toolbox_window) {
                return;
            }
            int w = 0;
            int h = 0;
            if (!toolbox_window->client_size(w, h)) {
                return;
            }
            toolbox_window->clear_background(26, 32, 40);
            toolbox_window->draw_text_line(12, 10, descriptor_runtime.toolbox_title);

            std::vector<std::pair<std::string, GuiTool>> toolbox_items = {
                {"Select", GuiTool::Select},
                {"Rectangle", GuiTool::Rectangle},
                {"Line", GuiTool::Line},
                {"Arc", GuiTool::Arc}
            };
            for (std::size_t i = 0; i < toolbox_items.size() && i < descriptor_runtime.toolbox_tool_labels.size(); ++i) {
                if (!descriptor_runtime.toolbox_tool_labels[i].empty()) {
                    toolbox_items[i].first = descriptor_runtime.toolbox_tool_labels[i];
                }
            }

            auto draw_tool = [&](int top, const std::string& label, GuiTool t) {
                if (!toolbox_window) {
                    return;
                }
                if (active_tool == t) {
                    toolbox_window->fill_rect(14, top, w - 14, top + 28, 52, 92, 138);
                } else {
                    toolbox_window->fill_rect(14, top, w - 14, top + 28, 70, 78, 90);
                }
                toolbox_window->draw_text_line(24, top + 8, label);
            };

            for (std::size_t i = 0; i < toolbox_items.size(); ++i) {
                const int top = 36 + static_cast<int>(i) * 32;
                draw_tool(top, toolbox_items[i].first, toolbox_items[i].second);
            }

            toolbox_window->fill_rect(w - 34, 8, w - 8, 28, 110, 58, 58);
            toolbox_window->draw_text_line(w - 24, 12, "X");
        };
        toolbox_window->set_render_hooks(std::move(toolbox_hooks));
        if (!toolbox_window->create()) {
            toolbox_window.reset();
            toolbox_dialog_open = false;
            return;
        }
        toolbox_window->show();
    };
#endif

    while (window.is_open()) {
#if defined(_WIN32)
        if (!ai_chat_dialog_open && ai_chat_window) {
            ai_chat_window->close();
            ai_chat_window.reset();
            sync_view_menu_checks();
        }
        if (!inspector_dialog_open && inspector_window) {
            inspector_window->close();
            inspector_window.reset();
            sync_view_menu_checks();
        }
        if (!toolbox_dialog_open && toolbox_window) {
            toolbox_window->close();
            toolbox_window.reset();
            sync_view_menu_checks();
        }

        ensure_ai_chat_window();
        ensure_inspector_window();
        ensure_toolbox_window();

        if (!window.pump_events()) {
            break;
        }

        PointerState pointer_state{};
        if (window.query_pointer_state(pointer_state)) {
            const bool inside = pointer_state.inside;

            client_width = std::max(1, pointer_state.client_width);
            client_height = std::max(1, pointer_state.client_height);

            const bool left_down = pointer_state.left_button_down;
            const bool right_down = pointer_state.right_button_down;
            const bool left_pressed = left_down && !prev_left_down;
            const bool left_released = !left_down && prev_left_down;

            const int canvas_x = clamp_to_canvas((pointer_state.x * state.viewport.canvas().width()) / client_width, state.viewport.canvas().width());
            const int canvas_y = clamp_to_canvas((pointer_state.y * state.viewport.canvas().height()) / client_height, state.viewport.canvas().height());

            if (left_pressed && inside) {
                if (active_tool == GuiTool::Select) {
                    select_rectangle_at_canvas_point(canvas_x, canvas_y);
                } else {
                    drawing_active = true;
                    draw_start_x = canvas_x;
                    draw_start_y = canvas_y;
                    draw_last_x = canvas_x;
                    draw_last_y = canvas_y;
                }
            }

            if (left_down && inside && active_tool == GuiTool::Select && !drawing_active) {
                if (!left_drag.active) {
                    left_drag.active = true;
                    left_drag.last_x = pointer_state.x;
                    left_drag.last_y = pointer_state.y;
                } else {
                    const int dx = pointer_state.x - left_drag.last_x;
                    const int dy = pointer_state.y - left_drag.last_y;
                    if (dx != 0 || dy != 0) {
                        state.viewport.pan(-static_cast<double>(dx), static_cast<double>(dy));
                        left_drag.last_x = pointer_state.x;
                        left_drag.last_y = pointer_state.y;
                    }
                }
            } else {
                left_drag.active = false;
            }

            if (drawing_active && left_down) {
                draw_last_x = canvas_x;
                draw_last_y = canvas_y;
            }

            if (drawing_active && left_released) {
                if (active_tool == GuiTool::Rectangle) {
                    cad_builder::RectangleModel rectangle;
                    rectangle.x = std::min(draw_start_x, draw_last_x);
                    rectangle.y = std::min(draw_start_y, draw_last_y);
                    rectangle.width = std::max(1, std::abs(draw_last_x - draw_start_x));
                    rectangle.height = std::max(1, std::abs(draw_last_y - draw_start_y));
                    rectangle.filled = false;
                    state.viewport.add_rectangle(rectangle);
                    state.viewport.set_status_text("tool=rectangle | added");
                } else if (active_tool == GuiTool::Line) {
                    cad_builder::LineModel line;
                    line.x1 = draw_start_x;
                    line.y1 = draw_start_y;
                    line.x2 = draw_last_x;
                    line.y2 = draw_last_y;
                    state.viewport.add_line(line);
                    state.viewport.set_status_text("tool=line | added");
                } else if (active_tool == GuiTool::Arc) {
                    const int dx = draw_last_x - draw_start_x;
                    const int dy = draw_last_y - draw_start_y;
                    const double radius_f = std::sqrt(static_cast<double>(dx * dx + dy * dy));
                    cad_builder::ArcModel arc;
                    arc.center_x = draw_start_x;
                    arc.center_y = draw_start_y;
                    arc.radius = std::max(1, static_cast<int>(std::lround(radius_f)));
                    arc.start_degrees = 0;
                    const double radians = std::atan2(static_cast<double>(draw_start_y - draw_last_y), static_cast<double>(draw_last_x - draw_start_x));
                    int end_deg = static_cast<int>(std::lround(radians * (180.0 / 3.14159265358979323846)));
                    if (end_deg < 0) {
                        end_deg += 360;
                    }
                    if (end_deg == 0) {
                        end_deg = 120;
                    }
                    arc.end_degrees = end_deg;
                    state.viewport.add_arc(arc);
                    state.viewport.set_status_text("tool=arc | added");
                }
                drawing_active = false;
            }

            if (right_down && inside) {
                if (!right_drag.active) {
                    right_drag.active = true;
                    right_drag.last_x = pointer_state.x;
                    right_drag.last_y = pointer_state.y;
                } else {
                    const int dx = pointer_state.x - right_drag.last_x;
                    const int dy = pointer_state.y - right_drag.last_y;
                    if (dx != 0 || dy != 0) {
                        state.viewport.rotate(static_cast<double>(dx) * -0.35, static_cast<double>(dy) * 0.28);
                        right_drag.last_x = pointer_state.x;
                        right_drag.last_y = pointer_state.y;
                    }
                }
            } else {
                right_drag.active = false;
            }

            prev_left_down = left_down;
        }

        if (ai_chat_window) {
            if (!ai_chat_window->pump_events() || !ai_chat_window->is_open()) {
                ai_chat_window.reset();
                ai_chat_dialog_open = false;
                sync_view_menu_checks();
            } else {
                PointerState p{};
                if (ai_chat_window->query_pointer_state(p)) {
                        const bool inside = p.inside;
                        const bool left = p.left_button_down;
                        const bool press = left && !prev_ai_left_down;
                        if (press && inside) {
                            const bool in_close = p.x >= p.client_width - 34 && p.x <= p.client_width - 8 && p.y >= 8 && p.y <= 28;
                            const bool in_demo = p.x >= 14 && p.x <= 220 && p.y >= 136 && p.y <= 168;
                            if (in_close) {
                                ai_chat_window->close();
                                ai_chat_window.reset();
                                ai_chat_dialog_open = false;
                                sync_view_menu_checks();
                            } else if (in_demo) {
                                const auto interpreted = ai_chat_bridge.interpret_speech("please add line");
                                if (interpreted.recognized) {
                                    ai_chat_last_result = "Mapped: 'please add line' -> '" + interpreted.cad_command + "'";
                                    state.viewport.set_status_text("ai_chat demo mapped to: " + interpreted.cad_command);
                                } else {
                                    ai_chat_last_result = "Demo phrase not recognized.";
                                }
                                ai_chat_window->request_redraw();
                            }
                        }
                        prev_ai_left_down = left;
                }
            }
        } else {
            prev_ai_left_down = false;
        }

        if (inspector_window) {
            if (!inspector_window->pump_events() || !inspector_window->is_open()) {
                inspector_window.reset();
                inspector_dialog_open = false;
                sync_view_menu_checks();
            } else {
                PointerState p{};
                if (inspector_window->query_pointer_state(p)) {
                        const bool inside = p.inside;
                        const bool left = p.left_button_down;
                        const bool press = left && !prev_inspector_left_down;
                        if (press && inside && p.x >= p.client_width - 34 && p.x <= p.client_width - 8 && p.y >= 8 && p.y <= 28) {
                            inspector_window->close();
                            inspector_window.reset();
                            inspector_dialog_open = false;
                            sync_view_menu_checks();
                        }
                        prev_inspector_left_down = left;
                    inspector_window->request_redraw();
                }
            }
        } else {
            prev_inspector_left_down = false;
        }

        if (toolbox_window) {
            if (!toolbox_window->pump_events() || !toolbox_window->is_open()) {
                toolbox_window.reset();
                toolbox_dialog_open = false;
                sync_view_menu_checks();
            } else {
                PointerState p{};
                if (toolbox_window->query_pointer_state(p)) {
                        const bool inside = p.inside;
                        const bool left = p.left_button_down;
                        const bool press = left && !prev_toolbox_left_down;
                        if (press && inside) {
                            const bool in_close = p.x >= p.client_width - 34 && p.x <= p.client_width - 8 && p.y >= 8 && p.y <= 28;
                            if (in_close) {
                                toolbox_window->close();
                                toolbox_window.reset();
                                toolbox_dialog_open = false;
                                sync_view_menu_checks();
                            } else if (p.x >= 14 && p.x <= p.client_width - 14) {
                                if (p.y >= 36 && p.y <= 64) {
                                    active_tool = GuiTool::Select;
                                } else if (p.y >= 68 && p.y <= 96) {
                                    active_tool = GuiTool::Rectangle;
                                } else if (p.y >= 100 && p.y <= 128) {
                                    active_tool = GuiTool::Line;
                                } else if (p.y >= 132 && p.y <= 160) {
                                    active_tool = GuiTool::Arc;
                                }
                                state.viewport.set_status_text("tool=" + tool_name(active_tool));
                                toolbox_window->request_redraw();
                            }
                        }
                        prev_toolbox_left_down = left;
                }
            }
        } else {
            prev_toolbox_left_down = false;
        }

        window.request_redraw();
#else
        if (!window.pump_events()) {
            break;
        }
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    save_live_view(state);
    out_message = "Closed interactive full_application_window.";
    return true;
}

bool parse_double(const std::string& value, double& out) {
    try {
        std::size_t pos = 0;
        out = std::stod(value, &pos);
        return pos == value.size();
    } catch (...) {
        return false;
    }
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

void print_blank_help() {
    std::cout << "CAD commands:\n";
    std::cout << "  help                              Show this command list\n";
    std::cout << "  add                               Add rectangle (interactive)\n";
    std::cout << "  addline                           Add line (interactive)\n";
    std::cout << "  addarc                            Add arc (interactive)\n";
    std::cout << "  addcircle                         Add full circle (arc 0..360)\n";
    std::cout << "  list                              List rectangles, lines, arcs\n";
    std::cout << "  select <index>                    Select rectangle by index\n";
    std::cout << "  select <rect|line|arc> <index>    Select typed geometry\n";
    std::cout << "  move <dx> <dy>                    Move selected geometry\n";
    std::cout << "  move <type> <i> <dx> <dy>         Move geometry by type/index\n";
    std::cout << "  resize <a> <b>                    Resize selected geometry\n";
    std::cout << "  resize <type> <i> <a> <b>         Resize geometry by type/index\n";
    std::cout << "  fill [on|off|toggle]              Change selected rectangle fill\n";
    std::cout << "  duplicate [index]                 Duplicate selected geometry\n";
    std::cout << "  duplicate <type> <index>          Duplicate by type/index\n";
    std::cout << "  delete [index]                    Delete selected geometry\n";
    std::cout << "  delete <type> <index>             Delete by type/index\n";
    std::cout << "  clear                             Remove all geometry\n";
    std::cout << "  snap [off|grid|endpoint|center]   Set snap mode\n";
    std::cout << "  grid <size>                       Set grid snap step\n";
    std::cout << "  snapthreshold <pixels>            Set endpoint/center snap radius\n";
    std::cout << "  snapinfo                          Show active snap settings\n";
    std::cout << "  pan <dx> <dy>                     Pan camera\n";
    std::cout << "  yaw <degrees>                     Rotate camera around Y\n";
    std::cout << "  pitch <degrees>                   Rotate camera pitch\n";
    std::cout << "  zoom <delta>                      Zoom in/out\n";
    std::cout << "  resetview                         Reset camera\n";
    std::cout << "  cam                               Print camera state\n";
    std::cout << "  export [path]                     Save viewport image\n";
    std::cout << "  launch_gui                        Open full_application_window viewport\n";
    std::cout << "  save [path]                       Save .hk_scene\n";
    std::cout << "  load [path]                       Load .hk_scene\n";
    std::cout << "  undo                              Undo last edit\n";
    std::cout << "  redo                              Redo last undone edit\n";
    std::cout << "  history                           Show command history\n";
    std::cout << "  !!                                Repeat last command\n";
    std::cout << "  !<n>                              Replay command history index\n";
    std::cout << "  ESC                               Clear selection\n";
    std::cout << "  q                                 Quit\n";
    std::cout << "DXF aliases: L (line), A (arc), C (circle), ESC (cancel)\n";
    std::cout << "Short aliases: j/l/i/k pan, u/o yaw, n/m pitch, +/- zoom\n";
}

int run_blank_workspace_flow() {
    AppState state;
    UndoHistory history;
    CommandHistory command_history;
    SelectionState selection;
    SnapSettings snap;

    auto update_status = [&](const std::string& note) {
        std::ostringstream status;
        status << "snap=" << snap_mode_to_string(snap.mode)
               << " grid=" << snap.grid_size
               << " thr=" << snap.threshold;
        if (!note.empty()) {
            status << " | " << note;
        }
        state.viewport.set_status_text(status.str());
    };

    update_status("ready");
    save_live_view(state);

    std::cout << "Started blank CAD workspace (white canvas 1280x720).\n";
    std::cout << "Type 'help' to list CAD commands.\n";
    while (true) {
        std::cout << "\ncad> ";

        std::string action;
        if (!std::getline(std::cin, action)) {
            std::cout << "\nInput stream closed. Exiting blank workspace.\n";
            return 0;
        }
        if (action.empty()) {
            continue;
        }

        if (action == "!!") {
            if (command_history.entries.empty()) {
                std::cout << "No previous command to repeat.\n";
                continue;
            }
            action = command_history.entries.back();
            std::cout << "replay> " << action << "\n";
        } else if (action.size() > 1U && action[0] == '!') {
            int replay_index = -1;
            if (!parse_int(action.substr(1), replay_index) || replay_index < 0 || replay_index >= static_cast<int>(command_history.entries.size())) {
                std::cout << "Invalid history index.\n";
                continue;
            }
            action = command_history.entries[static_cast<std::size_t>(replay_index)];
            std::cout << "replay> " << action << "\n";
        }

        command_history.entries.push_back(action);
        if (command_history.entries.size() > 200U) {
            command_history.entries.erase(command_history.entries.begin());
        }

        std::stringstream parser(action);
        std::string raw_command;
        parser >> raw_command;
        std::string command = raw_command;
        std::transform(command.begin(), command.end(), command.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (raw_command == "L") {
            command = "addline";
        } else if (raw_command == "A") {
            command = "addarc";
        } else if (raw_command == "C") {
            command = "addcircle";
        } else if (raw_command == "ESC") {
            command = "esc";
        }

        std::vector<std::string> args;
        std::string arg;
        while (parser >> arg) {
            args.push_back(arg);
        }

        auto parse_target = [&](const std::string& type_token, const std::string& index_token, SelectionKind& kind_out, int& index_out) -> bool {
            if (!parse_int(index_token, index_out) || index_out < 0) {
                return false;
            }
            if (type_token == "rect" || type_token == "rectangle") {
                kind_out = SelectionKind::Rectangle;
                return true;
            }
            if (type_token == "line") {
                kind_out = SelectionKind::Line;
                return true;
            }
            if (type_token == "arc") {
                kind_out = SelectionKind::Arc;
                return true;
            }
            return false;
        };

        if (command == "help" || command == "h" || command == "?") {
            print_blank_help();
            continue;
        }

        if (command == "history") {
            if (command_history.entries.empty()) {
                std::cout << "History is empty.\n";
                continue;
            }
            for (std::size_t i = 0; i < command_history.entries.size(); ++i) {
                std::cout << "  [" << i << "] " << command_history.entries[i] << "\n";
            }
            continue;
        }

        if (command == "esc") {
            clear_selection_state(state, selection);
            save_live_view(state);
            std::cout << "Selection cleared.\n";
            continue;
        }

        if (command == "add" || command == "r") {
            push_undo_snapshot(state, history);
            add_rectangle_to_blank_project(state, snap);
            selection.kind = SelectionKind::Rectangle;
            selection.index = static_cast<int>(state.viewport.rectangles().size()) - 1;
            state.viewport.set_selected_index(static_cast<std::size_t>(selection.index));
            save_live_view(state);
            continue;
        }

        if (command == "addline") {
            push_undo_snapshot(state, history);
            add_line_to_blank_project(state, snap);
            selection.kind = SelectionKind::Line;
            selection.index = static_cast<int>(state.viewport.lines().size()) - 1;
            state.viewport.set_selected_line_index(static_cast<std::size_t>(selection.index));
            save_live_view(state);
            continue;
        }

        if (command == "addarc") {
            push_undo_snapshot(state, history);
            add_arc_to_blank_project(state, snap);
            selection.kind = SelectionKind::Arc;
            selection.index = static_cast<int>(state.viewport.arcs().size()) - 1;
            state.viewport.set_selected_arc_index(static_cast<std::size_t>(selection.index));
            save_live_view(state);
            continue;
        }

        if (command == "addcircle") {
            push_undo_snapshot(state, history);
            cad_builder::ArcModel arc;
            arc.center_x = read_int_or_default("Circle center x", 220);
            arc.center_y = read_int_or_default("Circle center y", 220);
            arc.radius = std::max(1, read_int_or_default("Circle radius", 80));
            arc.start_degrees = 0;
            arc.end_degrees = 360;
            const SnapFeedback feedback = apply_snap_point(state, snap, arc.center_x, arc.center_y);
            state.viewport.set_snap_marker(feedback.x, feedback.y, feedback.snapped);
            state.viewport.add_arc(arc);
            selection.kind = SelectionKind::Arc;
            selection.index = static_cast<int>(state.viewport.arcs().size()) - 1;
            state.viewport.set_selected_arc_index(static_cast<std::size_t>(selection.index));
            save_live_view(state);
            continue;
        }

        if (command == "list") {
            list_rectangles(state);
            continue;
        }

        if (command == "select") {
            if (args.size() == 1U) {
                int index = -1;
                if (!parse_int(args[0], index) || index < 0 || !state.viewport.set_selected_index(static_cast<std::size_t>(index))) {
                    std::cout << "Invalid rectangle index.\n";
                    continue;
                }
                selection.kind = SelectionKind::Rectangle;
                selection.index = index;
                save_live_view(state);
                std::cout << "Selected rectangle #" << index << "\n";
                continue;
            }
            if (args.size() != 2U) {
                std::cout << "Usage: select <index> OR select <rect|line|arc> <index>\n";
                continue;
            }

            const std::string type = args[0];
            int index = -1;
            if (!parse_int(args[1], index) || index < 0) {
                std::cout << "Invalid index.\n";
                continue;
            }

            if (type == "rect" || type == "rectangle") {
                if (!state.viewport.set_selected_index(static_cast<std::size_t>(index))) {
                    std::cout << "Invalid rectangle index.\n";
                    continue;
                }
                selection.kind = SelectionKind::Rectangle;
            } else if (type == "line") {
                if (!state.viewport.set_selected_line_index(static_cast<std::size_t>(index))) {
                    std::cout << "Invalid line index.\n";
                    continue;
                }
                selection.kind = SelectionKind::Line;
            } else if (type == "arc") {
                if (!state.viewport.set_selected_arc_index(static_cast<std::size_t>(index))) {
                    std::cout << "Invalid arc index.\n";
                    continue;
                }
                selection.kind = SelectionKind::Arc;
            } else {
                std::cout << "Unknown type. Use rect, line, or arc.\n";
                continue;
            }
            selection.index = index;
            save_live_view(state);
            std::cout << "Selected " << type << " #" << index << "\n";
            continue;
        }

        if (command == "move") {
            SelectionKind target_kind = selection.kind;
            int target_index = selection.index;
            int dx = 0;
            int dy = 0;

            if (args.size() == 2U) {
                if (target_kind == SelectionKind::None || target_index < 0) {
                    std::cout << "No selection. Use: select <rect|line|arc> <index> or move <type> <index> <dx> <dy>\n";
                    continue;
                }
                if (!parse_int(args[0], dx) || !parse_int(args[1], dy)) {
                    std::cout << "Invalid move values.\n";
                    continue;
                }
            } else if (args.size() == 4U) {
                if (!parse_target(args[0], args[1], target_kind, target_index) ||
                    !parse_int(args[2], dx) || !parse_int(args[3], dy)) {
                    std::cout << "Usage: move <dx> <dy> OR move <rect|line|arc> <index> <dx> <dy>\n";
                    continue;
                }
            } else {
                std::cout << "Usage: move <dx> <dy> OR move <rect|line|arc> <index> <dx> <dy>\n";
                continue;
            }

            push_undo_snapshot(state, history);
            SnapFeedback feedback;

            if (target_kind == SelectionKind::Rectangle) {
                if (target_index < 0 || target_index >= static_cast<int>(state.viewport.rectangles().size())) {
                    std::cout << "Invalid rectangle index.\n";
                    continue;
                }
                auto rect = state.viewport.rectangles()[static_cast<std::size_t>(target_index)];
                int new_x = rect.x + dx;
                int new_y = rect.y + dy;
                feedback = apply_snap_point(state, snap, new_x, new_y);
                rect.x = new_x;
                rect.y = new_y;
                state.viewport.update_rectangle(static_cast<std::size_t>(target_index), rect);
                selection.kind = SelectionKind::Rectangle;
                selection.index = target_index;
                state.viewport.set_selected_index(static_cast<std::size_t>(target_index));
            } else if (target_kind == SelectionKind::Line) {
                if (target_index < 0 || target_index >= static_cast<int>(state.viewport.lines().size())) {
                    std::cout << "Invalid line index.\n";
                    continue;
                }
                auto line = state.viewport.lines()[static_cast<std::size_t>(target_index)];
                int x1 = line.x1 + dx;
                int y1 = line.y1 + dy;
                int x2 = line.x2 + dx;
                int y2 = line.y2 + dy;
                int snap_x1 = x1;
                int snap_y1 = y1;
                feedback = apply_snap_point(state, snap, snap_x1, snap_y1);
                const int delta_x = snap_x1 - x1;
                const int delta_y = snap_y1 - y1;
                line.x1 = x1 + delta_x;
                line.y1 = y1 + delta_y;
                line.x2 = x2 + delta_x;
                line.y2 = y2 + delta_y;
                state.viewport.update_line(static_cast<std::size_t>(target_index), line);
                selection.kind = SelectionKind::Line;
                selection.index = target_index;
                state.viewport.set_selected_line_index(static_cast<std::size_t>(target_index));
            } else if (target_kind == SelectionKind::Arc) {
                if (target_index < 0 || target_index >= static_cast<int>(state.viewport.arcs().size())) {
                    std::cout << "Invalid arc index.\n";
                    continue;
                }
                auto arc = state.viewport.arcs()[static_cast<std::size_t>(target_index)];
                int cx = arc.center_x + dx;
                int cy = arc.center_y + dy;
                feedback = apply_snap_point(state, snap, cx, cy);
                arc.center_x = cx;
                arc.center_y = cy;
                state.viewport.update_arc(static_cast<std::size_t>(target_index), arc);
                selection.kind = SelectionKind::Arc;
                selection.index = target_index;
                state.viewport.set_selected_arc_index(static_cast<std::size_t>(target_index));
            } else {
                std::cout << "No valid selection.\n";
                continue;
            }

            state.viewport.set_snap_marker(feedback.x, feedback.y, feedback.snapped);
            save_live_view(state);
            continue;
        }

        if (command == "resize") {
            int a = 0;
            int b = 0;
            SelectionKind target_kind = selection.kind;
            int target_index = selection.index;

            if (args.size() == 2U) {
                if (target_kind == SelectionKind::None || target_index < 0) {
                    std::cout << "No selection. Use: select <rect|line|arc> <index> or resize <type> <index> <a> <b>\n";
                    continue;
                }
                if (!parse_int(args[0], a) || !parse_int(args[1], b)) {
                    std::cout << "Invalid resize values.\n";
                    continue;
                }
            } else if (args.size() == 4U) {
                if (!parse_target(args[0], args[1], target_kind, target_index) ||
                    !parse_int(args[2], a) || !parse_int(args[3], b)) {
                    std::cout << "Usage: resize <a> <b> OR resize <rect|line|arc> <index> <a> <b>\n";
                    continue;
                }
            } else {
                std::cout << "Usage: resize <a> <b> OR resize <rect|line|arc> <index> <a> <b>\n";
                continue;
            }

            if (target_kind == SelectionKind::Rectangle) {
                if (a <= 0 || b <= 0) {
                    std::cout << "Rectangle width/height must be positive.\n";
                    continue;
                }
                if (target_index < 0 || target_index >= static_cast<int>(state.viewport.rectangles().size())) {
                    std::cout << "Invalid rectangle index.\n";
                    continue;
                }
                push_undo_snapshot(state, history);
                auto rect = state.viewport.rectangles()[static_cast<std::size_t>(target_index)];
                rect.width = a;
                rect.height = b;
                state.viewport.update_rectangle(static_cast<std::size_t>(target_index), rect);
                selection.kind = SelectionKind::Rectangle;
                selection.index = target_index;
                state.viewport.set_selected_index(static_cast<std::size_t>(target_index));
            } else if (target_kind == SelectionKind::Line) {
                if (target_index < 0 || target_index >= static_cast<int>(state.viewport.lines().size())) {
                    std::cout << "Invalid line index.\n";
                    continue;
                }
                push_undo_snapshot(state, history);
                auto line = state.viewport.lines()[static_cast<std::size_t>(target_index)];
                line.x2 = line.x1 + a;
                line.y2 = line.y1 + b;
                state.viewport.update_line(static_cast<std::size_t>(target_index), line);
                selection.kind = SelectionKind::Line;
                selection.index = target_index;
                state.viewport.set_selected_line_index(static_cast<std::size_t>(target_index));
            } else if (target_kind == SelectionKind::Arc) {
                if (a <= 0) {
                    std::cout << "Arc radius must be positive.\n";
                    continue;
                }
                if (target_index < 0 || target_index >= static_cast<int>(state.viewport.arcs().size())) {
                    std::cout << "Invalid arc index.\n";
                    continue;
                }
                push_undo_snapshot(state, history);
                auto arc = state.viewport.arcs()[static_cast<std::size_t>(target_index)];
                arc.radius = a;
                arc.end_degrees = arc.start_degrees + b;
                state.viewport.update_arc(static_cast<std::size_t>(target_index), arc);
                selection.kind = SelectionKind::Arc;
                selection.index = target_index;
                state.viewport.set_selected_arc_index(static_cast<std::size_t>(target_index));
            } else {
                std::cout << "No valid selection.\n";
                continue;
            }
            save_live_view(state);
            continue;
        }

        if (command == "fill") {
            if (selection.kind != SelectionKind::Rectangle || selection.index < 0) {
                std::cout << "Fill applies to selected rectangle only.\n";
                continue;
            }
            push_undo_snapshot(state, history);
            auto rect = state.viewport.rectangles()[static_cast<std::size_t>(selection.index)];
            if (args.empty() || args[0] == "toggle") {
                rect.filled = !rect.filled;
            } else if (args[0] == "on" || args[0] == "1") {
                rect.filled = true;
            } else if (args[0] == "off" || args[0] == "0") {
                rect.filled = false;
            } else {
                std::cout << "Usage: fill [on|off|toggle]\n";
                continue;
            }
            state.viewport.update_rectangle(static_cast<std::size_t>(selection.index), rect);
            save_live_view(state);
            continue;
        }

        if (command == "duplicate" || command == "dup") {
            SelectionKind target_kind = selection.kind;
            int source_index = selection.index;
            if (args.size() == 1U) {
                if (!parse_int(args[0], source_index)) {
                    std::cout << "Usage: duplicate [index] OR duplicate <rect|line|arc> <index>\n";
                    continue;
                }
                target_kind = SelectionKind::Rectangle;
            } else if (args.size() == 2U) {
                if (!parse_target(args[0], args[1], target_kind, source_index)) {
                    std::cout << "Usage: duplicate [index] OR duplicate <rect|line|arc> <index>\n";
                    continue;
                }
            } else if (!args.empty()) {
                std::cout << "Usage: duplicate [index] OR duplicate <rect|line|arc> <index>\n";
                continue;
            }

            if (target_kind == SelectionKind::Rectangle) {
                if (source_index < 0 || source_index >= static_cast<int>(state.viewport.rectangles().size())) {
                    std::cout << "Invalid rectangle index.\n";
                    continue;
                }
                push_undo_snapshot(state, history);
                auto copy = state.viewport.rectangles()[static_cast<std::size_t>(source_index)];
                copy.x += 20;
                copy.y += 20;
                const SnapFeedback feedback = apply_snap_point(state, snap, copy.x, copy.y);
                state.viewport.set_snap_marker(feedback.x, feedback.y, feedback.snapped);
                state.viewport.add_rectangle(copy);
                selection.kind = SelectionKind::Rectangle;
                selection.index = static_cast<int>(state.viewport.rectangles().size()) - 1;
                state.viewport.set_selected_index(static_cast<std::size_t>(selection.index));
            } else if (target_kind == SelectionKind::Line) {
                if (source_index < 0 || source_index >= static_cast<int>(state.viewport.lines().size())) {
                    std::cout << "Invalid line index.\n";
                    continue;
                }
                push_undo_snapshot(state, history);
                auto copy = state.viewport.lines()[static_cast<std::size_t>(source_index)];
                copy.x1 += 20;
                copy.y1 += 20;
                copy.x2 += 20;
                copy.y2 += 20;
                state.viewport.add_line(copy);
                selection.kind = SelectionKind::Line;
                selection.index = static_cast<int>(state.viewport.lines().size()) - 1;
                state.viewport.set_selected_line_index(static_cast<std::size_t>(selection.index));
            } else if (target_kind == SelectionKind::Arc) {
                if (source_index < 0 || source_index >= static_cast<int>(state.viewport.arcs().size())) {
                    std::cout << "Invalid arc index.\n";
                    continue;
                }
                push_undo_snapshot(state, history);
                auto copy = state.viewport.arcs()[static_cast<std::size_t>(source_index)];
                copy.center_x += 20;
                copy.center_y += 20;
                const SnapFeedback feedback = apply_snap_point(state, snap, copy.center_x, copy.center_y);
                state.viewport.set_snap_marker(feedback.x, feedback.y, feedback.snapped);
                state.viewport.add_arc(copy);
                selection.kind = SelectionKind::Arc;
                selection.index = static_cast<int>(state.viewport.arcs().size()) - 1;
                state.viewport.set_selected_arc_index(static_cast<std::size_t>(selection.index));
            } else {
                std::cout << "No selected geometry to duplicate.\n";
                continue;
            }
            save_live_view(state);
            continue;
        }

        if (command == "delete") {
            SelectionKind target_kind = selection.kind;
            int index = selection.index;
            if (args.size() == 1U) {
                if (!parse_int(args[0], index)) {
                    std::cout << "Usage: delete [index] OR delete <rect|line|arc> <index>\n";
                    continue;
                }
                target_kind = SelectionKind::Rectangle;
            } else if (args.size() == 2U) {
                if (!parse_target(args[0], args[1], target_kind, index)) {
                    std::cout << "Usage: delete [index] OR delete <rect|line|arc> <index>\n";
                    continue;
                }
            } else if (!args.empty()) {
                std::cout << "Usage: delete [index] OR delete <rect|line|arc> <index>\n";
                continue;
            }

            if (target_kind == SelectionKind::Rectangle) {
                if (index < 0 || index >= static_cast<int>(state.viewport.rectangles().size())) {
                    std::cout << "Invalid rectangle index.\n";
                    continue;
                }
                push_undo_snapshot(state, history);
                state.viewport.remove_rectangle(static_cast<std::size_t>(index));
            } else if (target_kind == SelectionKind::Line) {
                if (index < 0 || index >= static_cast<int>(state.viewport.lines().size())) {
                    std::cout << "Invalid line index.\n";
                    continue;
                }
                push_undo_snapshot(state, history);
                state.viewport.remove_line(static_cast<std::size_t>(index));
            } else if (target_kind == SelectionKind::Arc) {
                if (index < 0 || index >= static_cast<int>(state.viewport.arcs().size())) {
                    std::cout << "Invalid arc index.\n";
                    continue;
                }
                push_undo_snapshot(state, history);
                state.viewport.remove_arc(static_cast<std::size_t>(index));
            } else {
                std::cout << "No selected geometry to delete.\n";
                continue;
            }
            clear_selection_state(state, selection);
            save_live_view(state);
            continue;
        }

        if (command == "clear") {
            push_undo_snapshot(state, history);
            state.viewport.clear_all_geometry();
            clear_selection_state(state, selection);
            save_live_view(state);
            continue;
        }

        if (command == "snap") {
            if (args.size() != 1U) {
                std::cout << "Usage: snap [off|grid|endpoint|center]\n";
                continue;
            }
            if (args[0] == "off") {
                snap.mode = SnapMode::Off;
            } else if (args[0] == "grid") {
                snap.mode = SnapMode::Grid;
            } else if (args[0] == "endpoint") {
                snap.mode = SnapMode::Endpoint;
            } else if (args[0] == "center") {
                snap.mode = SnapMode::Center;
            } else {
                std::cout << "Unknown snap mode.\n";
                continue;
            }
            update_status("snap mode changed");
            std::cout << "Snap mode: " << snap_mode_to_string(snap.mode) << "\n";
            continue;
        }

        if (command == "grid") {
            if (args.size() != 1U) {
                std::cout << "Usage: grid <size>\n";
                continue;
            }
            int size = 0;
            if (!parse_int(args[0], size) || size <= 0) {
                std::cout << "Grid size must be positive.\n";
                continue;
            }
            snap.grid_size = size;
            update_status("grid changed");
            std::cout << "Grid snap size set to " << size << "\n";
            continue;
        }

        if (command == "snapthreshold") {
            if (args.size() != 1U) {
                std::cout << "Usage: snapthreshold <pixels>\n";
                continue;
            }
            int threshold = 0;
            if (!parse_int(args[0], threshold) || threshold < 1) {
                std::cout << "Threshold must be a positive integer.\n";
                continue;
            }
            snap.threshold = threshold;
            update_status("threshold changed");
            std::cout << "Snap threshold set to " << threshold << "\n";
            continue;
        }

        if (command == "snapinfo") {
            update_status("snap info");
            std::cout << "Snap mode=" << snap_mode_to_string(snap.mode)
                      << " grid=" << snap.grid_size
                      << " threshold=" << snap.threshold
                      << "\n";
            continue;
        }

        if (command == "pan") {
            if (args.size() != 2U) {
                std::cout << "Usage: pan <dx> <dy>\n";
                continue;
            }
            double dx = 0.0;
            double dy = 0.0;
            if (!parse_double(args[0], dx) || !parse_double(args[1], dy)) {
                std::cout << "Invalid pan values.\n";
                continue;
            }
            state.viewport.pan(dx, dy);
            save_live_view(state);
            continue;
        }

        if (command == "yaw") {
            if (args.size() != 1U) {
                std::cout << "Usage: yaw <degrees>\n";
                continue;
            }
            double delta = 0.0;
            if (!parse_double(args[0], delta)) {
                std::cout << "Invalid yaw value.\n";
                continue;
            }
            state.viewport.rotate(delta, 0.0);
            save_live_view(state);
            continue;
        }

        if (command == "pitch") {
            if (args.size() != 1U) {
                std::cout << "Usage: pitch <degrees>\n";
                continue;
            }
            double delta = 0.0;
            if (!parse_double(args[0], delta)) {
                std::cout << "Invalid pitch value.\n";
                continue;
            }
            state.viewport.rotate(0.0, delta);
            save_live_view(state);
            continue;
        }

        if (command == "zoom") {
            if (args.size() != 1U) {
                std::cout << "Usage: zoom <delta>\n";
                continue;
            }
            double delta = 0.0;
            if (!parse_double(args[0], delta)) {
                std::cout << "Invalid zoom value.\n";
                continue;
            }
            state.viewport.zoom(delta);
            save_live_view(state);
            continue;
        }

        if (command == "resetview") {
            state.viewport.reset_camera();
            save_live_view(state);
            continue;
        }

        if (command == "cam") {
            print_camera_state(state);
            continue;
        }

        if (command == "export" || command == "v") {
            if (args.empty()) {
                save_blank_preview(state);
            } else {
                const bool ok = state.viewport.save_view(args[0]);
                std::cout << (ok ? "Saved preview: " : "Failed to save preview: ") << args[0] << "\n";
            }
            continue;
        }

        if (command == "launch_gui" || command == "gui") {
            std::string launch_message;
            Options options;
            if (launch_viewport_gui(state, launch_message, options)) {
                std::cout << launch_message << "\n";
            } else {
                std::cout << launch_message << "\n";
            }
            continue;
        }

        if (command == "save" || command == "s") {
            std::string path = args.empty() ? pick_scene_path_with_dialog(true) : args[0];
            if (path.empty()) {
                std::cout << "Scene save canceled.\n";
                continue;
            }
            if (save_scene_file(state, path)) {
                std::cout << "Saved scene: " << path << "\n";
            } else {
                std::cout << "Failed to save scene: " << path << "\n";
            }
            continue;
        }

        if (command == "load" || command == "g") {
            std::string path = args.empty() ? pick_scene_path_with_dialog(false) : args[0];
            if (path.empty()) {
                std::cout << "Scene load canceled.\n";
                continue;
            }
            push_undo_snapshot(state, history);
            if (load_scene_file(state, path)) {
                std::cout << "Loaded scene: " << path
                          << " (rectangles: " << state.viewport.rectangle_count() << ")\n";
                clear_selection_state(state, selection);
                save_live_view(state);
            } else {
                std::cout << "Failed to load scene: " << path << "\n";
            }
            continue;
        }

        if (command == "undo") {
            if (history.undo_stack.empty()) {
                std::cout << "Nothing to undo.\n";
                continue;
            }
            history.redo_stack.push_back(serialize_scene(state));
            const std::string previous = history.undo_stack.back();
            history.undo_stack.pop_back();
            if (!load_scene_from_text(state, previous)) {
                std::cout << "Undo failed due to invalid snapshot.\n";
                continue;
            }
            clear_selection_state(state, selection);
            save_live_view(state);
            std::cout << "Undo applied.\n";
            continue;
        }

        if (command == "redo") {
            if (history.redo_stack.empty()) {
                std::cout << "Nothing to redo.\n";
                continue;
            }
            history.undo_stack.push_back(serialize_scene(state));
            const std::string next = history.redo_stack.back();
            history.redo_stack.pop_back();
            if (!load_scene_from_text(state, next)) {
                std::cout << "Redo failed due to invalid snapshot.\n";
                continue;
            }
            clear_selection_state(state, selection);
            save_live_view(state);
            std::cout << "Redo applied.\n";
            continue;
        }

        if (command == "j" || command == "l" || command == "i" || command == "k" ||
            command == "u" || command == "o" || command == "n" || command == "m" ||
            command == "+" || command == "-") {
            if (command == "j") {
                state.viewport.pan(-20.0, 0.0);
            } else if (command == "l") {
                state.viewport.pan(20.0, 0.0);
            } else if (command == "i") {
                state.viewport.pan(0.0, 20.0);
            } else if (command == "k") {
                state.viewport.pan(0.0, -20.0);
            } else if (command == "u") {
                state.viewport.rotate(-5.0, 0.0);
            } else if (command == "o") {
                state.viewport.rotate(5.0, 0.0);
            } else if (command == "n") {
                state.viewport.rotate(0.0, -4.0);
            } else if (command == "m") {
                state.viewport.rotate(0.0, 4.0);
            } else if (command == "+") {
                state.viewport.zoom(90.0);
            } else {
                state.viewport.zoom(-90.0);
            }
            save_live_view(state);
            continue;
        }

        if (command == "q" || command == "quit" || command == "exit") {
            std::cout << "Exiting blank workspace.\n";
            return 0;
        }

        std::cout << "Unknown command. Type 'help' for CAD commands.\n";
    }
}

int run_launch_gui_flow() {
    AppState state;
    std::string launch_message;
    Options options;
    if (!launch_viewport_gui(state, launch_message, options)) {
        std::cerr << launch_message << "\n";
        return 2;
    }
    std::cout << launch_message << "\n";
    return 0;
}

int run_open_file_flow(std::string input_path, std::string output_path) {
    if (input_path.empty()) {
        input_path = pick_input_with_dialog();
    }
    if (input_path.empty()) {
        std::cerr << "No input CAD file selected.\n";
        return 1;
    }

    if (output_path.empty()) {
        output_path = pick_output_with_dialog(input_path);
    }
    if (output_path.empty()) {
        std::cerr << "No output path selected.\n";
        return 1;
    }

    app_builder::cad_generics::ProcessingOptions processing_options;
    processing_options.strict_extension_check = true;
    processing_options.require_file_to_exist = true;

    const auto result = app_builder::cad_generics::process_and_export_hk_cad(
        input_path,
        output_path,
        processing_options);

    if (!result.success) {
        std::cerr << "cad_builder failed: " << result.message << "\n";
        return 2;
    }

    std::cout << "cad_builder success\n";
    std::cout << "  input : " << input_path << "\n";
    std::cout << "  output: " << output_path << "\n";
    std::cout << "  type  : " << app_builder::cad_generics::cad_file_type_to_string(result.file.type) << "\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Options options = parse_args(argc, argv);

    if (options.use_window) {
        configure_window_shell();
    }

    if (options.launch_gui) {
        AppState state;
        std::string launch_message;
        if (!launch_viewport_gui(state, launch_message, options)) {
            std::cerr << launch_message << "\n";
            return 2;
        }
        std::cout << launch_message << "\n";
        return 0;
    }

    // Batch mode remains supported for scripted use.
    if (!options.input_path.empty()) {
        return run_open_file_flow(options.input_path, options.output_path);
    }

    std::cout << "cad_builder startup\n";
    std::cout << "  1) Open CAD file\n";
    std::cout << "  2) New blank project\n";
    std::cout << "Choose mode [1/2]: ";

    std::string mode;
    std::getline(std::cin, mode);

    if (mode == "2") {
        return run_blank_workspace_flow();
    }

    return run_open_file_flow(std::string(), std::string());
}
