#include "ai_chat.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Options {
    std::string transcript_file;
    std::string out_script = "build/ai_chat_cad_commands.txt";
    std::string cad_builder_path;
    bool run_cad_builder = false;
    bool verbose = true;
};

void print_usage() {
    std::cout << "ai_chat_voice_bridge\n";
    std::cout << "Usage:\n";
    std::cout << "  ai_chat_voice_bridge [--transcript-file <path>] [--out-script <path>]\\n";
    std::cout << "                      [--run-cad-builder <path-to-cad_builder.exe>] [--quiet]\n";
    std::cout << "\n";
    std::cout << "Input mode:\n";
    std::cout << "  - If --transcript-file is set, reads one transcript line per command from file.\n";
    std::cout << "  - Otherwise reads transcript lines from stdin until EOF.\n";
    std::cout << "\n";
    std::cout << "Output:\n";
    std::cout << "  - Writes mapped cad_builder commands to --out-script.\n";
    std::cout << "  - Prepends mode selection '2' and appends 'q' so script can drive blank CAD mode.\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  type speech_lines.txt | ai_chat_voice_bridge --run-cad-builder ";
    std::cout << "build\\_deliverables\\apps\\cad_builder\\Debug\\cad_builder.exe\n";
}

Options parse_args(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage();
            std::exit(0);
        }
        if (arg == "--transcript-file" && i + 1 < argc) {
            options.transcript_file = argv[++i];
            continue;
        }
        if (arg == "--out-script" && i + 1 < argc) {
            options.out_script = argv[++i];
            continue;
        }
        if (arg == "--run-cad-builder" && i + 1 < argc) {
            options.run_cad_builder = true;
            options.cad_builder_path = argv[++i];
            continue;
        }
        if (arg == "--quiet") {
            options.verbose = false;
            continue;
        }

        std::cerr << "Unknown or incomplete argument: " << arg << "\n";
        print_usage();
        std::exit(2);
    }

    return options;
}

bool ensure_parent_dir(const std::filesystem::path& file_path) {
    std::error_code error;
    const std::filesystem::path parent = file_path.parent_path();
    if (parent.empty()) {
        return true;
    }
    if (std::filesystem::exists(parent, error)) {
        return true;
    }
    return std::filesystem::create_directories(parent, error);
}

std::istream* open_transcript_source(const Options& options, std::ifstream& file_stream) {
    if (options.transcript_file.empty()) {
        return &std::cin;
    }

    file_stream.open(options.transcript_file);
    if (!file_stream) {
        return nullptr;
    }
    return &file_stream;
}

bool write_cad_script(const std::string& output_path, const std::vector<std::string>& commands) {
    const std::filesystem::path path(output_path);
    if (!ensure_parent_dir(path)) {
        return false;
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }

    out << "2\n";
    for (const auto& command : commands) {
        out << command << "\n";
    }
    out << "q\n";
    return static_cast<bool>(out);
}

int run_cad_builder_with_script(const std::string& cad_builder_path, const std::string& script_path) {
    std::ostringstream cmd;
    cmd << "type \"" << script_path << "\" | \"" << cad_builder_path << "\"";
    return std::system(cmd.str().c_str());
}

} // namespace

int main(int argc, char** argv) {
    const Options options = parse_args(argc, argv);

    std::ifstream transcript_file_stream;
    std::istream* transcript_stream = open_transcript_source(options, transcript_file_stream);
    if (transcript_stream == nullptr) {
        std::cerr << "Failed to open transcript file: " << options.transcript_file << "\n";
        return 1;
    }

    tools::ai_chat::AiChatSpeechBridge bridge;
    std::vector<std::string> recognized_commands;

    if (options.verbose) {
        std::cout << "Listening for transcript lines (EOF to finish)...\n";
    }

    std::string speech_line;
    while (std::getline(*transcript_stream, speech_line)) {
        if (speech_line.empty()) {
            continue;
        }

        const bool recognized = bridge.submit_speech(speech_line);
        if (recognized) {
            const std::string command = bridge.pop_next_command();
            recognized_commands.push_back(command);
            if (options.verbose) {
                std::cout << "speech: " << speech_line << "\n";
                std::cout << "  -> cad: " << command << "\n";
            }
        } else if (options.verbose) {
            std::cout << "speech: " << speech_line << "\n";
            std::cout << "  -> ignored (not recognized)\n";
        }
    }

    if (!write_cad_script(options.out_script, recognized_commands)) {
        std::cerr << "Failed to write command script: " << options.out_script << "\n";
        return 1;
    }

    std::cout << "Wrote " << recognized_commands.size() << " CAD command(s) to: "
              << options.out_script << "\n";

    if (options.run_cad_builder) {
        const int exit_code = run_cad_builder_with_script(options.cad_builder_path, options.out_script);
        std::cout << "cad_builder exit code: " << exit_code << "\n";
        return exit_code;
    }

    return 0;
}
