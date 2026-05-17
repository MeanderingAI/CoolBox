#ifndef COOLBOX_APPS_CAD_BUILDER_CAD_BUILDER_WINDOW_HPP
#define COOLBOX_APPS_CAD_BUILDER_CAD_BUILDER_WINDOW_HPP

#include "graphics.h"

#include <string>
#include <vector>

namespace cad_builder {

struct RectangleModel {
    int x = 100;
    int y = 100;
    int width = 200;
    int height = 120;
    bool filled = true;
};

struct LineModel {
    int x1 = 100;
    int y1 = 100;
    int x2 = 220;
    int y2 = 160;
};

struct ArcModel {
    int center_x = 220;
    int center_y = 220;
    int radius = 80;
    int start_degrees = 0;
    int end_degrees = 120;
};

struct CameraState {
    double pan_x = 0.0;
    double pan_y = 0.0;
    double yaw_degrees = 20.0;
    double pitch_degrees = -12.0;
    double zoom = 900.0;
};

class CadBuilderWindow {
public:
    CadBuilderWindow(int width = 1280, int height = 720);

    void add_rectangle(const RectangleModel& rectangle);
    void set_rectangles(const std::vector<RectangleModel>& rectangles);
    const std::vector<RectangleModel>& rectangles() const;
    bool update_rectangle(std::size_t index, const RectangleModel& rectangle);
    bool remove_rectangle(std::size_t index);
    void clear_rectangles();

    void add_line(const LineModel& line);
    void set_lines(const std::vector<LineModel>& lines);
    const std::vector<LineModel>& lines() const;
    bool update_line(std::size_t index, const LineModel& line);
    bool remove_line(std::size_t index);
    void clear_lines();

    void add_arc(const ArcModel& arc);
    void set_arcs(const std::vector<ArcModel>& arcs);
    const std::vector<ArcModel>& arcs() const;
    bool update_arc(std::size_t index, const ArcModel& arc);
    bool remove_arc(std::size_t index);
    void clear_arcs();

    void clear_all_geometry();

    bool set_selected_index(std::size_t index);
    void clear_selection();
    int selected_index() const;
    bool set_selected_line_index(std::size_t index);
    int selected_line_index() const;
    bool set_selected_arc_index(std::size_t index);
    int selected_arc_index() const;

    void set_camera(const CameraState& camera);
    CameraState camera() const;

    void pan(double dx, double dy);
    void rotate(double delta_yaw_degrees, double delta_pitch_degrees);
    void zoom(double delta_zoom);
    void reset_camera();

    void set_status_text(const std::string& text);
    void set_snap_marker(int x, int y, bool active);

    void render();
    bool save_view(const std::string& path) const;

    std::size_t rectangle_count() const;
    const graphics::Canvas& canvas() const;

private:
    int width_;
    int height_;
    graphics::Canvas canvas_;
    std::vector<RectangleModel> rectangles_;
    std::vector<LineModel> lines_;
    std::vector<ArcModel> arcs_;
    CameraState camera_;
    int selected_index_ = -1;
    int selected_line_index_ = -1;
    int selected_arc_index_ = -1;
    std::string status_text_;
    bool snap_marker_active_ = false;
    int snap_marker_x_ = 0;
    int snap_marker_y_ = 0;
};

} // namespace cad_builder

#endif
