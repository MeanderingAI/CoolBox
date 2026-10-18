#ifndef COOLBOX_INTERACTIVE_PLOT_VIEWER_HPP
#define COOLBOX_INTERACTIVE_PLOT_VIEWER_HPP

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

namespace coolbox_graphics {
namespace detail {

inline std::string html_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char character : value) {
        switch (character) {
        case '&': escaped += "&amp;"; break;
        case '"': escaped += "&quot;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '\'': escaped += "&#39;"; break;
        default: escaped += character; break;
        }
    }
    return escaped;
}

inline std::string base64_encode(const std::uint8_t* data, std::size_t size) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(((size + 2) / 3) * 4);
    for (std::size_t i = 0; i < size; i += 3) {
        const std::uint32_t first = data[i];
        const std::uint32_t second = i + 1 < size ? data[i + 1] : 0;
        const std::uint32_t third = i + 2 < size ? data[i + 2] : 0;
        const std::uint32_t block = (first << 16) | (second << 8) | third;
        result += alphabet[(block >> 18) & 63];
        result += alphabet[(block >> 12) & 63];
        result += i + 1 < size ? alphabet[(block >> 6) & 63] : '=';
        result += i + 2 < size ? alphabet[block & 63] : '=';
    }
    return result;
}

inline std::string make_html(int width, int height,
                             const std::uint8_t* pixels,
                             const std::string& title,
                             const std::string& save_path) {
    const auto byte_count = static_cast<std::size_t>(width) *
                            static_cast<std::size_t>(height) * 4;
    const std::string encoded = base64_encode(pixels, byte_count);
    const std::string safe_title = html_escape(title.empty() ? "CoolBox Plot" : title);
    const auto filename = std::filesystem::path(save_path).filename().string();
    const std::string encoded_filename = base64_encode(
        reinterpret_cast<const std::uint8_t*>(filename.data()), filename.size());
    return R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>)HTML" + safe_title + R"HTML(</title>
<style>
:root{color-scheme:dark;font:14px "Segoe UI",system-ui,sans-serif;background:#10151f;color:#e7edf8}
*{box-sizing:border-box}body{margin:0;height:100vh;display:flex;flex-direction:column}
header{height:54px;flex:none;display:flex;align-items:center;gap:18px;padding:0 20px;
background:#171f2d;border-bottom:1px solid #29364a}
h1{font-size:15px;font-weight:600;margin:0;flex:1}
button{border:1px solid #3b4c65;background:#202c3d;color:#f1f5fb;padding:8px 13px;
border-radius:6px;cursor:pointer}button:hover{background:#2b3b52}
#viewport{position:relative;flex:1;overflow:hidden;background:#0d121b;cursor:grab}
#viewport.dragging{cursor:grabbing}canvas{width:100%;height:100%;display:block}
footer{height:34px;flex:none;padding:8px 16px;background:#171f2d;color:#aab8cc;
font-size:12px;border-top:1px solid #29364a}
</style>
</head>
<body>
<header><h1>)HTML" + safe_title + R"HTML(</h1><button id="reset">Reset view</button>
<button id="save">Save PNG</button></header>
<main id="viewport"><canvas id="view"></canvas></main>
<footer>Scroll to zoom | Drag to pan | Save PNG exports the original plot | Press S to save</footer>
<script>
(() => {
  const width = )HTML" + std::to_string(width) + R"HTML(;
  const height = )HTML" + std::to_string(height) + R"HTML(;
  const source = new Uint8ClampedArray(atob(")HTML" + encoded + R"HTML(").split("").map(c=>c.charCodeAt(0)));
  const original = new ImageData(source, width, height);
  const viewport = document.getElementById("viewport");
  const canvas = document.getElementById("view");
  const context = canvas.getContext("2d");
  const sourceCanvas = document.createElement("canvas");
  sourceCanvas.width = width; sourceCanvas.height = height;
  sourceCanvas.getContext("2d").putImageData(original, 0, 0);
  const saveName = new TextDecoder().decode(
    Uint8Array.from(atob(")HTML" + encoded_filename +
        R"HTML("), c => c.charCodeAt(0))) || "plot.png";
  let zoom = 1, panX = 0, panY = 0, dragging = false, lastX = 0, lastY = 0;
  function draw() {
    const dpr = window.devicePixelRatio || 1;
    const w = viewport.clientWidth, h = viewport.clientHeight;
    canvas.width = Math.max(1, Math.round(w * dpr));
    canvas.height = Math.max(1, Math.round(h * dpr));
    context.setTransform(dpr, 0, 0, dpr, 0, 0);
    context.fillStyle = "#0d121b"; context.fillRect(0, 0, w, h);
    const scale = Math.min(w / width, h / height) * zoom;
    const x = (w - width * scale) / 2 + panX;
    const y = (h - height * scale) / 2 + panY;
    context.imageSmoothingEnabled = true;
    context.drawImage(sourceCanvas, x, y, width * scale, height * scale);
  }
  viewport.addEventListener("wheel", event => {
    event.preventDefault();
    const oldScale = Math.min(viewport.clientWidth / width, viewport.clientHeight / height) * zoom;
    const factor = event.deltaY < 0 ? 1.15 : 1 / 1.15;
    const nextZoom = Math.min(32, Math.max(0.05, zoom * factor));
    const nextScale = Math.min(viewport.clientWidth / width, viewport.clientHeight / height) * nextZoom;
    const centerX = viewport.clientWidth / 2, centerY = viewport.clientHeight / 2;
    const x = event.clientX - viewport.getBoundingClientRect().left - centerX;
    const y = event.clientY - viewport.getBoundingClientRect().top - centerY;
    panX = x - ((x - panX) / oldScale) * nextScale;
    panY = y - ((y - panY) / oldScale) * nextScale;
    zoom = nextZoom; draw();
  }, {passive:false});
  viewport.addEventListener("pointerdown", event => {
    dragging = true; lastX = event.clientX; lastY = event.clientY;
    viewport.classList.add("dragging"); viewport.setPointerCapture(event.pointerId);
  });
  viewport.addEventListener("pointermove", event => {
    if (!dragging) return;
    panX += event.clientX - lastX; panY += event.clientY - lastY;
    lastX = event.clientX; lastY = event.clientY; draw();
  });
  viewport.addEventListener("pointerup", () => {
    dragging = false; viewport.classList.remove("dragging");
  });
  document.getElementById("reset").addEventListener("click", () => {
    zoom = 1; panX = 0; panY = 0; draw();
  });
  document.getElementById("save").addEventListener("click", () => {
    const exportCanvas = document.createElement("canvas");
    exportCanvas.width = width; exportCanvas.height = height;
    exportCanvas.getContext("2d").putImageData(original, 0, 0);
    const link = document.createElement("a");
    link.href = exportCanvas.toDataURL("image/png"); link.download = saveName;
    link.click();
  });
  document.addEventListener("keydown", event => {
    if ((event.key === "s" || event.key === "S") && !event.ctrlKey && !event.metaKey)
      document.getElementById("save").click();
  });
  window.addEventListener("resize", draw); draw();
})();
</script>
</body></html>)HTML";
}

inline std::filesystem::path write_viewer_file(const std::string& html) {
    static std::atomic<unsigned long long> sequence{0};
    const auto directory = std::filesystem::temp_directory_path();
#ifdef _WIN32
    const auto unique = sequence.fetch_add(1, std::memory_order_relaxed);
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = directory / ("coolbox-interactive-plot-" +
        std::to_string(stamp) + "-" + std::to_string(unique) + ".html");
    std::ofstream output(path, std::ios::binary | std::ios::out);
    if (!output) throw std::runtime_error("could not create the temporary interactive plot page");
    output.write(html.data(), static_cast<std::streamsize>(html.size()));
    output.close();
    if (!output) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        throw std::runtime_error("could not write the temporary interactive plot page");
    }
#else
    std::filesystem::path path;
    int descriptor = -1;
    for (int attempt = 0; attempt < 10 && descriptor < 0; ++attempt) {
        const auto unique = sequence.fetch_add(1, std::memory_order_relaxed);
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = directory / ("coolbox-interactive-plot-" +
            std::to_string(stamp) + "-" + std::to_string(unique) + ".html");
        descriptor = ::open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
        if (descriptor < 0 && errno != EEXIST) {
            throw std::runtime_error("could not create the temporary interactive plot page");
        }
    }
    if (descriptor < 0) {
        throw std::runtime_error("could not allocate a unique interactive plot page");
    }
    std::size_t written = 0;
    while (written < html.size()) {
        const auto result = ::write(descriptor, html.data() + written, html.size() - written);
        if (result < 0 && errno == EINTR) continue;
        if (result <= 0) {
            ::close(descriptor);
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
            throw std::runtime_error("could not write the temporary interactive plot page");
        }
        written += static_cast<std::size_t>(result);
    }
    if (::close(descriptor) != 0) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        throw std::runtime_error("could not finish writing the temporary interactive plot page");
    }
#endif
    return path;
}

inline void open_viewer_file(const std::filesystem::path& path) {
#ifdef _WIN32
    const auto native_path = path.wstring();
    const auto result = reinterpret_cast<std::intptr_t>(
        ShellExecuteW(nullptr, L"open", native_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    if (result <= 32) throw std::runtime_error("could not open the interactive plot in the default browser");
#elif defined(__APPLE__)
    pid_t process = 0;
    std::string native_path = path.string();
    char open[] = "open";
    char* arguments[] = {open, native_path.data(), nullptr};
    const int result = posix_spawnp(&process, open, nullptr, nullptr, arguments, environ);
    if (result != 0) throw std::runtime_error("could not launch the default browser for the interactive plot");
    std::thread([process] { int status = 0; waitpid(process, &status, 0); }).detach();
#else
    pid_t process = 0;
    std::string native_path = path.string();
    char xdg_open[] = "xdg-open";
    char* arguments[] = {xdg_open, native_path.data(), nullptr};
    const int result = posix_spawnp(&process, xdg_open, nullptr, nullptr, arguments, environ);
    if (result != 0) throw std::runtime_error("could not launch the default browser for the interactive plot");
    std::thread([process] { int status = 0; waitpid(process, &status, 0); }).detach();
#endif
}

} // namespace detail

inline void show_interactive_plot(int width, int height,
                                  const std::uint8_t* pixels,
                                  const std::string& title,
                                  const std::string& save_path) {
    if (width <= 0 || height <= 0 || !pixels) {
        throw std::invalid_argument("interactive plot requires valid canvas pixels and dimensions");
    }
    const auto w = static_cast<std::size_t>(width);
    const auto h = static_cast<std::size_t>(height);
    if (w > std::numeric_limits<std::size_t>::max() / 4 / h) {
        throw std::invalid_argument("interactive plot dimensions overflow");
    }
    const auto html = detail::make_html(width, height, pixels, title, save_path);
    const auto path = detail::write_viewer_file(html);
    try {
        detail::open_viewer_file(path);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        throw;
    }
}

} // namespace coolbox_graphics

#endif
