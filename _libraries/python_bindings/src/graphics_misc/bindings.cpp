#include "graphics_misc/bindings.hpp"

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

#include "graphics.h"
#include "wave_generator.hpp"

namespace py = pybind11;

namespace {

std::string color_repr(const graphics::Color& color) {
    std::ostringstream stream;
    stream << "Color(r=" << static_cast<int>(color.r)
           << ", g=" << static_cast<int>(color.g)
           << ", b=" << static_cast<int>(color.b)
           << ", a=" << static_cast<int>(color.a)
           << ")";
    return stream.str();
}

void bind_color_constants(py::module_& graphics_module) {
    graphics_module.attr("BLACK") = py::cast(graphics::Colors::Black);
    graphics_module.attr("WHITE") = py::cast(graphics::Colors::White);
    graphics_module.attr("RED") = py::cast(graphics::Colors::Red);
    graphics_module.attr("GREEN") = py::cast(graphics::Colors::Green);
    graphics_module.attr("BLUE") = py::cast(graphics::Colors::Blue);
    graphics_module.attr("ORANGE") = py::cast(graphics::Colors::Orange);
    graphics_module.attr("PURPLE") = py::cast(graphics::Colors::Purple);
    graphics_module.attr("CYAN") = py::cast(graphics::Colors::Cyan);
    graphics_module.attr("GRAY") = py::cast(graphics::Colors::Gray);
    graphics_module.attr("DARK_GRAY") = py::cast(graphics::Colors::DarkGray);
    graphics_module.attr("LIGHT_GRAY") = py::cast(graphics::Colors::LightGray);
}

py::array_t<std::uint8_t> canvas_to_numpy(const graphics::Canvas& canvas) {
    py::array_t<std::uint8_t> array({canvas.height(), canvas.width(), 4});
    const auto byte_count = static_cast<std::size_t>(canvas.width()) * static_cast<std::size_t>(canvas.height()) * 4U;
    std::memcpy(array.mutable_data(), canvas.data(), byte_count);
    return array;
}

} // namespace

void bind_graphics(py::module_& parent_module) {
    py::module_ graphics_module = parent_module.def_submodule("graphics", "Graphics and chart rendering utilities");

    py::class_<graphics::Color>(graphics_module, "Color")
        .def(py::init<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>(),
             py::arg("r") = 0,
             py::arg("g") = 0,
             py::arg("b") = 0,
             py::arg("a") = 255)
        .def_readwrite("r", &graphics::Color::r)
        .def_readwrite("g", &graphics::Color::g)
        .def_readwrite("b", &graphics::Color::b)
        .def_readwrite("a", &graphics::Color::a)
        .def("__repr__", &color_repr);

    bind_color_constants(graphics_module);

    py::enum_<graphics::GraphType>(graphics_module, "GraphType")
        .value("LINE", graphics::GraphType::Line)
        .value("BAR", graphics::GraphType::Bar)
        .value("SCATTER", graphics::GraphType::Scatter);

    py::class_<graphics::Canvas>(graphics_module, "Canvas")
        .def(py::init<int, int, graphics::Color>(),
             py::arg("width"),
             py::arg("height"),
             py::arg("background") = graphics::Colors::White)
        .def("width", &graphics::Canvas::width)
        .def("height", &graphics::Canvas::height)
        .def("set_pixel", &graphics::Canvas::set_pixel,
             py::arg("x"), py::arg("y"), py::arg("color"))
        .def("get_pixel", &graphics::Canvas::get_pixel,
             py::arg("x"), py::arg("y"))
        .def("fill", &graphics::Canvas::fill, py::arg("color"))
        .def("draw_line", &graphics::Canvas::draw_line,
             py::arg("x0"), py::arg("y0"), py::arg("x1"), py::arg("y1"),
             py::arg("color"), py::arg("thickness") = 1)
        .def("draw_rect", &graphics::Canvas::draw_rect,
             py::arg("x"), py::arg("y"), py::arg("width"), py::arg("height"),
             py::arg("color"), py::arg("filled") = false)
        .def("draw_circle", &graphics::Canvas::draw_circle,
             py::arg("cx"), py::arg("cy"), py::arg("radius"),
             py::arg("color"), py::arg("filled") = false)
        .def("draw_text", &graphics::Canvas::draw_text,
             py::arg("x"), py::arg("y"), py::arg("text"), py::arg("color"), py::arg("scale") = 1)
        .def("save_bmp", &graphics::Canvas::save_bmp, py::arg("path"))
        .def("save_png", &graphics::Canvas::save_png, py::arg("path"))
        .def("save_jpg", &graphics::Canvas::save_jpg, py::arg("path"), py::arg("quality") = 90)
        .def("to_numpy", &canvas_to_numpy,
             "Return a copy of the canvas pixels as an RGBA numpy array with shape (height, width, 4)");

    py::class_<graphics::DataSeries>(graphics_module, "DataSeries")
        .def(py::init<>())
        .def(py::init<std::string, std::vector<double>, std::vector<double>, graphics::Color>(),
             py::arg("label"),
             py::arg("x_values"),
             py::arg("y_values"),
             py::arg("color") = graphics::Colors::Blue)
        .def_readwrite("label", &graphics::DataSeries::label)
        .def_readwrite("x_values", &graphics::DataSeries::x_values)
        .def_readwrite("y_values", &graphics::DataSeries::y_values)
        .def_readwrite("color", &graphics::DataSeries::color);

    py::class_<graphics::Graph>(graphics_module, "Graph")
        .def(py::init<int, int, graphics::GraphType>(),
             py::arg("width") = 800,
             py::arg("height") = 600,
             py::arg("graph_type") = graphics::GraphType::Line)
        .def("set_title", &graphics::Graph::set_title, py::arg("title"))
        .def("set_x_label", &graphics::Graph::set_x_label, py::arg("label"))
        .def("set_y_label", &graphics::Graph::set_y_label, py::arg("label"))
        .def("set_type", &graphics::Graph::set_type, py::arg("graph_type"))
        .def("add_series", &graphics::Graph::add_series, py::arg("series"))
        .def("render", &graphics::Graph::render);

    py::class_<graphics::Table>(graphics_module, "Table")
        .def(py::init<>())
        .def("set_headers", &graphics::Table::set_headers, py::arg("headers"))
        .def("add_row", &graphics::Table::add_row, py::arg("row"))
        .def("set_cell_padding", &graphics::Table::set_cell_padding, py::arg("pixels"))
        .def("set_font_scale", &graphics::Table::set_font_scale, py::arg("scale"))
        .def("set_header_color", &graphics::Table::set_header_color, py::arg("color"))
        .def("set_border_color", &graphics::Table::set_border_color, py::arg("color"))
        .def("set_alternate_row_color", &graphics::Table::set_alternate_row_color, py::arg("color"))
        .def("render", &graphics::Table::render);
}

void bind_misc(py::module_& parent_module) {
    py::module_ misc_module = parent_module.def_submodule("misc", "Miscellaneous utility libraries");
    py::module_ wave_module = misc_module.def_submodule("wave_generator", "Waveform generation utilities");

    py::enum_<utils::wave_generator::WavePattern>(wave_module, "WavePattern")
        .value("SINE", utils::wave_generator::WavePattern::Sine)
        .value("SQUARE", utils::wave_generator::WavePattern::Square)
        .value("TRIANGLE", utils::wave_generator::WavePattern::Triangle)
        .value("SAWTOOTH", utils::wave_generator::WavePattern::Sawtooth)
        .value("REVERSE_SAWTOOTH", utils::wave_generator::WavePattern::ReverseSawtooth)
        .value("PULSE", utils::wave_generator::WavePattern::Pulse)
        .value("WHITE_NOISE", utils::wave_generator::WavePattern::WhiteNoise);

    py::class_<utils::wave_generator::WaveConfig>(wave_module, "WaveConfig")
        .def(py::init<>())
        .def_readwrite("amplitude", &utils::wave_generator::WaveConfig::amplitude)
        .def_readwrite("frequency_hz", &utils::wave_generator::WaveConfig::frequency_hz)
        .def_readwrite("phase_radians", &utils::wave_generator::WaveConfig::phase_radians)
        .def_readwrite("offset", &utils::wave_generator::WaveConfig::offset)
        .def_readwrite("duty_cycle", &utils::wave_generator::WaveConfig::duty_cycle)
        .def_readwrite("noise_seed", &utils::wave_generator::WaveConfig::noise_seed);

    wave_module.def("sample_at", &utils::wave_generator::sample_at,
                    py::arg("time_seconds"),
                    py::arg("pattern"),
                    py::arg("config") = utils::wave_generator::WaveConfig{});
    wave_module.def("generate_samples", &utils::wave_generator::generate_samples,
                    py::arg("sample_count"),
                    py::arg("sample_rate_hz"),
                    py::arg("pattern"),
                    py::arg("config") = utils::wave_generator::WaveConfig{});
    wave_module.def("generate_samples_for_duration", &utils::wave_generator::generate_samples_for_duration,
                    py::arg("duration_seconds"),
                    py::arg("sample_rate_hz"),
                    py::arg("pattern"),
                    py::arg("config") = utils::wave_generator::WaveConfig{});
}