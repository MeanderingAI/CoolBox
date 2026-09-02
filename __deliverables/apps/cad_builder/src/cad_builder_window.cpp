#include "cad_builder_window.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace cad_builder {
namespace {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

constexpr double kPi = 3.14159265358979323846;

double to_radians(double degrees) {
    return degrees * kPi / 180.0;
}

Vec3 rotate_y(Vec3 point, double yaw_rad) {
    const double c = std::cos(yaw_rad);
    const double s = std::sin(yaw_rad);
    return Vec3{point.x * c + point.z * s, point.y, -point.x * s + point.z * c};
}

Vec3 rotate_x(Vec3 point, double pitch_rad) {
    const double c = std::cos(pitch_rad);
    const double s = std::sin(pitch_rad);
    return Vec3{point.x, point.y * c - point.z * s, point.y * s + point.z * c};
}

bool project_point(const Vec3& world,
                   const CameraState& camera,
                   int canvas_width,
                   int canvas_height,
                   int& out_x,
                   int& out_y) {
    Vec3 point = world;
    point.x -= camera.pan_x;
    point.y -= camera.pan_y;

    point = rotate_y(point, to_radians(camera.yaw_degrees));
    point = rotate_x(point, to_radians(camera.pitch_degrees));

    const double camera_distance = 1200.0;
    const double z = point.z + camera_distance;
    if (z < 20.0) {
        return false;
    }

    const double perspective = camera.zoom / z;
    const double sx = point.x * perspective + static_cast<double>(canvas_width) * 0.5;
    const double sy = -point.y * perspective + static_cast<double>(canvas_height) * 0.5;

    out_x = static_cast<int>(std::lround(sx));
    out_y = static_cast<int>(std::lround(sy));
    return true;
}

void draw_world_line(graphics::Canvas& canvas,
                     const CameraState& camera,
                     const Vec3& a,
                     const Vec3& b,
                     graphics::Color color,
                     int thickness = 1) {
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;

    if (!project_point(a, camera, canvas.width(), canvas.height(), x0, y0)) {
        return;
    }
    if (!project_point(b, camera, canvas.width(), canvas.height(), x1, y1)) {
        return;
    }

    canvas.draw_line(x0, y0, x1, y1, color, thickness);
}

} // namespace

CadBuilderWindow::CadBuilderWindow(int width, int height)
    : width_(width)
    , height_(height)
    , canvas_(width, height, graphics::Colors::White) {
    render();
}

void CadBuilderWindow::add_rectangle(const RectangleModel& rectangle) {
    rectangles_.push_back(rectangle);
    selected_index_ = static_cast<int>(rectangles_.size()) - 1;
    render();
}

void CadBuilderWindow::set_rectangles(const std::vector<RectangleModel>& rectangles) {
    rectangles_ = rectangles;
    selected_index_ = -1;
    render();
}

const std::vector<RectangleModel>& CadBuilderWindow::rectangles() const {
    return rectangles_;
}

bool CadBuilderWindow::update_rectangle(std::size_t index, const RectangleModel& rectangle) {
    if (index >= rectangles_.size()) {
        return false;
    }
    rectangles_[index] = rectangle;
    render();
    return true;
}

bool CadBuilderWindow::remove_rectangle(std::size_t index) {
    if (index >= rectangles_.size()) {
        return false;
    }
    rectangles_.erase(rectangles_.begin() + static_cast<std::ptrdiff_t>(index));
    if (rectangles_.empty()) {
        selected_index_ = -1;
    } else if (selected_index_ >= static_cast<int>(rectangles_.size())) {
        selected_index_ = static_cast<int>(rectangles_.size()) - 1;
    }
    render();
    return true;
}

void CadBuilderWindow::clear_rectangles() {
    rectangles_.clear();
    selected_index_ = -1;
    render();
}

void CadBuilderWindow::add_line(const LineModel& line) {
    lines_.push_back(line);
    selected_line_index_ = static_cast<int>(lines_.size()) - 1;
    selected_index_ = -1;
    selected_arc_index_ = -1;
    render();
}

void CadBuilderWindow::set_lines(const std::vector<LineModel>& lines) {
    lines_ = lines;
    selected_line_index_ = -1;
    render();
}

const std::vector<LineModel>& CadBuilderWindow::lines() const {
    return lines_;
}

bool CadBuilderWindow::update_line(std::size_t index, const LineModel& line) {
    if (index >= lines_.size()) {
        return false;
    }
    lines_[index] = line;
    render();
    return true;
}

bool CadBuilderWindow::remove_line(std::size_t index) {
    if (index >= lines_.size()) {
        return false;
    }
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(index));
    if (lines_.empty()) {
        selected_line_index_ = -1;
    } else if (selected_line_index_ >= static_cast<int>(lines_.size())) {
        selected_line_index_ = static_cast<int>(lines_.size()) - 1;
    }
    render();
    return true;
}

void CadBuilderWindow::clear_lines() {
    lines_.clear();
    selected_line_index_ = -1;
    render();
}

void CadBuilderWindow::add_arc(const ArcModel& arc) {
    arcs_.push_back(arc);
    selected_arc_index_ = static_cast<int>(arcs_.size()) - 1;
    selected_index_ = -1;
    selected_line_index_ = -1;
    render();
}

void CadBuilderWindow::set_arcs(const std::vector<ArcModel>& arcs) {
    arcs_ = arcs;
    selected_arc_index_ = -1;
    render();
}

const std::vector<ArcModel>& CadBuilderWindow::arcs() const {
    return arcs_;
}

bool CadBuilderWindow::update_arc(std::size_t index, const ArcModel& arc) {
    if (index >= arcs_.size()) {
        return false;
    }
    arcs_[index] = arc;
    render();
    return true;
}

bool CadBuilderWindow::remove_arc(std::size_t index) {
    if (index >= arcs_.size()) {
        return false;
    }
    arcs_.erase(arcs_.begin() + static_cast<std::ptrdiff_t>(index));
    if (arcs_.empty()) {
        selected_arc_index_ = -1;
    } else if (selected_arc_index_ >= static_cast<int>(arcs_.size())) {
        selected_arc_index_ = static_cast<int>(arcs_.size()) - 1;
    }
    render();
    return true;
}

void CadBuilderWindow::clear_arcs() {
    arcs_.clear();
    selected_arc_index_ = -1;
    render();
}

void CadBuilderWindow::clear_all_geometry() {
    rectangles_.clear();
    lines_.clear();
    arcs_.clear();
    selected_index_ = -1;
    render();
}

bool CadBuilderWindow::set_selected_index(std::size_t index) {
    if (index >= rectangles_.size()) {
        return false;
    }
    selected_index_ = static_cast<int>(index);
    selected_line_index_ = -1;
    selected_arc_index_ = -1;
    render();
    return true;
}

void CadBuilderWindow::clear_selection() {
    selected_index_ = -1;
    selected_line_index_ = -1;
    selected_arc_index_ = -1;
    render();
}

int CadBuilderWindow::selected_index() const {
    return selected_index_;
}

bool CadBuilderWindow::set_selected_line_index(std::size_t index) {
    if (index >= lines_.size()) {
        return false;
    }
    selected_line_index_ = static_cast<int>(index);
    selected_index_ = -1;
    selected_arc_index_ = -1;
    render();
    return true;
}

int CadBuilderWindow::selected_line_index() const {
    return selected_line_index_;
}

bool CadBuilderWindow::set_selected_arc_index(std::size_t index) {
    if (index >= arcs_.size()) {
        return false;
    }
    selected_arc_index_ = static_cast<int>(index);
    selected_index_ = -1;
    selected_line_index_ = -1;
    render();
    return true;
}

int CadBuilderWindow::selected_arc_index() const {
    return selected_arc_index_;
}

void CadBuilderWindow::set_camera(const CameraState& camera) {
    camera_ = camera;
    render();
}

CameraState CadBuilderWindow::camera() const {
    return camera_;
}

void CadBuilderWindow::pan(double dx, double dy) {
    camera_.pan_x += dx;
    camera_.pan_y += dy;
    render();
}

void CadBuilderWindow::rotate(double delta_yaw_degrees, double delta_pitch_degrees) {
    camera_.yaw_degrees += delta_yaw_degrees;
    camera_.pitch_degrees = std::clamp(camera_.pitch_degrees + delta_pitch_degrees, -85.0, 85.0);
    render();
}

void CadBuilderWindow::zoom(double delta_zoom) {
    camera_.zoom = std::clamp(camera_.zoom + delta_zoom, 240.0, 3200.0);
    render();
}

void CadBuilderWindow::reset_camera() {
    camera_ = CameraState{};
    render();
}

void CadBuilderWindow::set_status_text(const std::string& text) {
    status_text_ = text;
    render();
}

void CadBuilderWindow::set_snap_marker(int x, int y, bool active) {
    snap_marker_active_ = active;
    snap_marker_x_ = x;
    snap_marker_y_ = y;
    render();
}

void CadBuilderWindow::render() {
    canvas_.fill(graphics::Colors::White);

    // Draw a simple 3D floor grid.
    for (int i = -12; i <= 12; ++i) {
        const double t = static_cast<double>(i) * 40.0;
        draw_world_line(canvas_, camera_, Vec3{-480.0, 0.0, t}, Vec3{480.0, 0.0, t}, graphics::Colors::LightGray, 1);
        draw_world_line(canvas_, camera_, Vec3{t, 0.0, -480.0}, Vec3{t, 0.0, 480.0}, graphics::Colors::LightGray, 1);
    }

    // Axes.
    draw_world_line(canvas_, camera_, Vec3{0.0, 0.0, 0.0}, Vec3{220.0, 0.0, 0.0}, graphics::Colors::Red, 2);
    draw_world_line(canvas_, camera_, Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 220.0, 0.0}, graphics::Colors::Green, 2);
    draw_world_line(canvas_, camera_, Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 0.0, 220.0}, graphics::Colors::Blue, 2);

    // Rectangle quads placed on a drawing plane in front of the camera.
    for (std::size_t i = 0; i < rectangles_.size(); ++i) {
        const auto& rectangle = rectangles_[i];
        const double x = static_cast<double>(rectangle.x) - 640.0;
        const double y = 360.0 - static_cast<double>(rectangle.y);
        const double w = static_cast<double>(rectangle.width);
        const double h = static_cast<double>(rectangle.height);
        const double z = 180.0;

        const Vec3 p0{x, y, z};
        const Vec3 p1{x + w, y, z};
        const Vec3 p2{x + w, y - h, z};
        const Vec3 p3{x, y - h, z};

        const bool selected = static_cast<int>(i) == selected_index_;
        const graphics::Color color = selected
            ? graphics::Colors::Red
            : (rectangle.filled ? graphics::Colors::Blue : graphics::Colors::DarkGray);
        draw_world_line(canvas_, camera_, p0, p1, color, selected ? 3 : 2);
        draw_world_line(canvas_, camera_, p1, p2, color, selected ? 3 : 2);
        draw_world_line(canvas_, camera_, p2, p3, color, selected ? 3 : 2);
        draw_world_line(canvas_, camera_, p3, p0, color, selected ? 3 : 2);
    }

    for (std::size_t i = 0; i < lines_.size(); ++i) {
        const auto& line = lines_[i];
        const bool selected = static_cast<int>(i) == selected_line_index_;
        const Vec3 p0{static_cast<double>(line.x1) - 640.0, 360.0 - static_cast<double>(line.y1), 170.0};
        const Vec3 p1{static_cast<double>(line.x2) - 640.0, 360.0 - static_cast<double>(line.y2), 170.0};
        draw_world_line(canvas_, camera_, p0, p1, selected ? graphics::Colors::Red : graphics::Colors::Black, selected ? 3 : 2);
    }

    for (std::size_t i = 0; i < arcs_.size(); ++i) {
        const auto& arc = arcs_[i];
        const bool selected = static_cast<int>(i) == selected_arc_index_;
        int start = arc.start_degrees;
        int end = arc.end_degrees;
        if (end < start) {
            end += 360;
        }
        const int steps = std::max(12, (end - start) / 8);
        Vec3 prev{};
        bool has_prev = false;
        for (int i = 0; i <= steps; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(steps);
            const double angle = static_cast<double>(start) + static_cast<double>(end - start) * t;
            const double angle_rad = to_radians(angle);
            const Vec3 cur{
                static_cast<double>(arc.center_x) - 640.0 + static_cast<double>(arc.radius) * std::cos(angle_rad),
                360.0 - static_cast<double>(arc.center_y) + static_cast<double>(arc.radius) * std::sin(angle_rad),
                165.0
            };
            if (has_prev) {
                draw_world_line(canvas_, camera_, prev, cur, selected ? graphics::Colors::Red : graphics::Colors::DarkGray, selected ? 3 : 2);
            }
            prev = cur;
            has_prev = true;
        }
    }

    canvas_.draw_text(10, 10, "cad_builder 3D viewport", graphics::Colors::Black, 1);
    canvas_.draw_text(10, 28, "rectangles: " + std::to_string(rectangles_.size()), graphics::Colors::DarkGray, 1);
    canvas_.draw_text(10, 46, "selected r/l/a: " + std::to_string(selected_index_) + "/" + std::to_string(selected_line_index_) + "/" + std::to_string(selected_arc_index_), graphics::Colors::DarkGray, 1);
    canvas_.draw_text(10, 64, "lines/arcs: " + std::to_string(lines_.size()) + "/" + std::to_string(arcs_.size()), graphics::Colors::DarkGray, 1);
    canvas_.draw_text(10, 82, "zoom: " + std::to_string(static_cast<int>(camera_.zoom)), graphics::Colors::DarkGray, 1);
    if (!status_text_.empty()) {
        canvas_.draw_text(10, 100, status_text_, graphics::Colors::DarkGray, 1);
    }

    if (snap_marker_active_) {
        const int sx = snap_marker_x_;
        const int sy = snap_marker_y_;
        canvas_.draw_line(sx - 8, sy, sx + 8, sy, graphics::Colors::Green, 2);
        canvas_.draw_line(sx, sy - 8, sx, sy + 8, graphics::Colors::Green, 2);
        canvas_.draw_text(sx + 10, sy - 10, "snap", graphics::Colors::Green, 1);
    }
}

bool CadBuilderWindow::save_view(const std::string& path) const {
    if (path.size() >= 4U) {
        const std::string ext = path.substr(path.size() - 4U);
        if (ext == ".bmp" || ext == ".BMP") {
            return canvas_.save_bmp(path);
        }
    }
    return canvas_.save_png(path);
}

std::size_t CadBuilderWindow::rectangle_count() const {
    return rectangles_.size();
}

const graphics::Canvas& CadBuilderWindow::canvas() const {
    return canvas_;
}

} // namespace cad_builder
