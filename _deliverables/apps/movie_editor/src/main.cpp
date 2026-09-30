#include "cli_tools.hpp"
#include "editor_window.hpp"
#include "movie_editor_core.h"

#include <iostream>
#include <string>

namespace {

struct Options {
    bool show_help = false;
    bool cli_mode = false;
    std::string project_path;
    std::string export_avi_path;
    std::string export_gif_path;
    std::string export_wav_path;
    double fps = 24.0;
    std::size_t width = 640;
    std::size_t height = 360;
    std::uint32_t audio_rate = 48000;
    std::uint32_t audio_channels = 2;
    std::int64_t gif_start_us = 0;
    std::int64_t gif_end_us = -1;
    int gif_colors = 256;
    std::string wav_encoding = "pcm16";
};

Options parse_args(int argc, const char* const argv[]) {
    os_generics::cli::CommandLineParser parser;
    parser.set_program_name("movie_editor");
    parser.set_description(
        "CoolBox non-linear video editor: import image-sequence/still/audio media onto a "
        "multi-track timeline, preview/scrub in a windowed GUI, and export to AVI/GIF/WAV.");

    parser.add_option({"help", 'h', false, false, "", "Show help and exit."});
    parser.add_option({"cli", '\0', false, false, "", "Headless mode: load --project and run any requested exports, then exit (no window)."});
    parser.add_option({"project", 'p', true, false, "PATH", "Project JSON file to load."});
    parser.add_option({"export-avi", '\0', true, false, "PATH", "Export the loaded project's full timeline to an AVI file."});
    parser.add_option({"export-gif", '\0', true, false, "PATH", "Export the loaded project as an animated GIF."});
    parser.add_option({"export-wav", '\0', true, false, "PATH", "Export the loaded project's audio mix to a WAV file."});
    parser.add_option({"fps", '\0', true, false, "N", "Frame rate for video export (default 24)."});
    parser.add_option({"width", '\0', true, false, "N", "Output width in pixels (default 640)."});
    parser.add_option({"height", '\0', true, false, "N", "Output height in pixels (default 360)."});
    parser.add_option({"audio-rate", '\0', true, false, "N", "Output audio sample rate (default 48000)."});
    parser.add_option({"audio-channels", '\0', true, false, "N", "Output audio channel count (default 2)."});
    parser.add_option({"gif-start-us", '\0', true, false, "N", "GIF export range start, microseconds (default 0)."});
    parser.add_option({"gif-end-us", '\0', true, false, "N", "GIF export range end, microseconds (default: full timeline)."});
    parser.add_option({"gif-colors", '\0', true, false, "N", "GIF palette size, 2-256 (default 256)."});
    parser.add_option({"wav-encoding", '\0', true, false, "pcm16|adpcm", "Audio export encoding (default pcm16)."});

    const auto result = parser.parse_argv(argc, argv);
    Options options;
    if (!result.ok()) {
        for (const auto& error : result.errors) std::cerr << "Error: " << error << "\n";
        std::cerr << "\n" << parser.render_help() << "\n";
        std::exit(1);
    }
    if (result.has_option("help")) {
        std::cout << parser.render_help() << "\n";
        options.show_help = true;
        return options;
    }

    options.cli_mode = result.has_option("cli");
    options.project_path = result.option_value("project");
    options.export_avi_path = result.option_value("export-avi");
    options.export_gif_path = result.option_value("export-gif");
    options.export_wav_path = result.option_value("export-wav");
    options.fps = std::stod(result.option_value("fps", "24.0"));
    options.width = static_cast<std::size_t>(std::stoul(result.option_value("width", "640")));
    options.height = static_cast<std::size_t>(std::stoul(result.option_value("height", "360")));
    options.audio_rate = static_cast<std::uint32_t>(std::stoul(result.option_value("audio-rate", "48000")));
    options.audio_channels = static_cast<std::uint32_t>(std::stoul(result.option_value("audio-channels", "2")));
    options.gif_start_us = std::stoll(result.option_value("gif-start-us", "0"));
    options.gif_end_us = std::stoll(result.option_value("gif-end-us", "-1"));
    options.gif_colors = std::stoi(result.option_value("gif-colors", "256"));
    options.wav_encoding = result.option_value("wav-encoding", "pcm16");
    return options;
}

int run_headless(const Options& options) {
    if (options.project_path.empty()) {
        std::cerr << "--cli mode (or any --export-* flag) requires --project <path>\n";
        return 1;
    }

    trekker::movie_editor::EditorProject project;
    try {
        project.load_project(options.project_path);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load project '" << options.project_path << "': " << e.what() << "\n";
        return 1;
    }

    std::cout << "Loaded project: " << options.project_path << "\n";
    std::cout << "  Tracks: " << project.timeline().tracks().size() << "\n";
    std::cout << "  Media assets: " << project.media_bin().assets().size() << "\n";
    std::cout << "  Duration: " << (project.duration_us() / 1'000'000.0) << "s\n";

    bool did_export = false;
    try {
        if (!options.export_avi_path.empty()) {
            project.export_avi(options.export_avi_path, options.fps, options.width, options.height,
                               options.audio_rate, options.audio_channels);
            std::cout << "Exported AVI: " << options.export_avi_path << "\n";
            did_export = true;
        }
        if (!options.export_gif_path.empty()) {
            const std::int64_t end_us = options.gif_end_us >= 0 ? options.gif_end_us : project.duration_us();
            project.export_gif(options.export_gif_path, options.gif_start_us, end_us, options.fps,
                               options.width, options.height, options.gif_colors);
            std::cout << "Exported GIF: " << options.export_gif_path << "\n";
            did_export = true;
        }
        if (!options.export_wav_path.empty()) {
            const auto encoding = options.wav_encoding == "adpcm"
                ? trekker::audio::container::WavEncoding::ImaAdpcm
                : trekker::audio::container::WavEncoding::PCM16;
            project.export_audio(options.export_wav_path, encoding, options.audio_rate, options.audio_channels);
            std::cout << "Exported audio: " << options.export_wav_path << "\n";
            did_export = true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Export failed: " << e.what() << "\n";
        return 1;
    }

    if (!did_export) {
        std::cout << "(No --export-* flag given; nothing rendered. Use --export-avi/--export-gif/--export-wav.)\n";
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    const Options options = parse_args(argc, argv);
    if (options.show_help) return 0;

    const bool wants_export = !options.export_avi_path.empty() || !options.export_gif_path.empty() ||
                              !options.export_wav_path.empty();

    if (options.cli_mode || wants_export) {
        return run_headless(options);
    }

    trekker::movie_editor::EditorProject project;
    if (!options.project_path.empty()) {
        try {
            project.load_project(options.project_path);
        } catch (const std::exception& e) {
            std::cerr << "Failed to load project '" << options.project_path << "': " << e.what() << "\n";
            return 1;
        }
    }

    movie_editor_app::MovieEditorWindow window(std::move(project));
    return window.run() ? 0 : 1;
}
