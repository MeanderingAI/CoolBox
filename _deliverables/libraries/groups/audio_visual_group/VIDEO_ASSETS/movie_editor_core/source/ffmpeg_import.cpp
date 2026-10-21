#include "../headers/movie_editor_core.h"
#include "ffmpeg_tools.hpp"

#include <cstdlib>
#include <exception>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace trekker::movie_editor {
namespace fs = std::filesystem;

VideoImportCache::~VideoImportCache() {
    if (directory.empty()) return;
    std::error_code error;
    fs::remove_all(directory, error);
    if (error) std::cerr << "Could not remove video import cache " << directory << ": " << error.message() << '\n';
}

namespace {

#ifdef _WIN32
std::wstring quote_argument(const std::wstring& value) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (wchar_t c : value) {
        if (c == L'\\') {
            ++slashes;
        } else {
            result.append(slashes * (c == L'"' ? 2 : 1), L'\\');
            slashes = 0;
            if (c == L'"') result += L'\\';
            result += c;
        }
    }
    result.append(slashes * 2, L'\\');
    return result + L'"';
}
#endif

std::string run_tool(const fs::path& executable, const std::vector<std::string>& args,
                     const fs::path& log_path, const detail::InputProducer& input = {}) {
    std::exception_ptr input_error;
#ifdef _WIN32
    std::wstring command = quote_argument(executable.wstring());
    for (const auto& arg : args) command += L" " + quote_argument(fs::path(arg).wstring());
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE log = CreateFileW(log_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (log == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create FFmpeg diagnostic log.");
    HANDLE read_pipe = nullptr, write_pipe = nullptr;
    if (input && (!CreatePipe(&read_pipe, &write_pipe, &security, 0) ||
                  !SetHandleInformation(write_pipe, HANDLE_FLAG_INHERIT, 0))) {
        if (read_pipe) CloseHandle(read_pipe);
        if (write_pipe) CloseHandle(write_pipe);
        CloseHandle(log);
        throw std::runtime_error("Cannot create FFmpeg input pipe.");
    }
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = log;
    startup.hStdError = log;
    startup.hStdInput = input ? read_pipe : GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION process{};
    const BOOL started = CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE,
                                       CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    const DWORD start_error = GetLastError();
    CloseHandle(log);
    if (read_pipe) CloseHandle(read_pipe);
    if (!started) {
        if (write_pipe) CloseHandle(write_pipe);
        throw std::runtime_error("Cannot start " + executable.string() + " (Windows error " +
                                 std::to_string(start_error) +
                                 "). Install FFmpeg and add its bin folder to PATH, or set COOLBOX_FFMPEG.");
    }
    CloseHandle(process.hThread);
    if (input) {
        try {
            input([write_pipe](const void* data, std::size_t size) {
                auto* bytes = static_cast<const char*>(data);
                while (size) {
                    const auto requested = static_cast<DWORD>(std::min<std::size_t>(size, 1024 * 1024));
                    DWORD written = 0;
                    if (!WriteFile(write_pipe, bytes, requested, &written, nullptr) || !written)
                        throw std::runtime_error("FFmpeg input pipe closed (Windows error " +
                                                 std::to_string(GetLastError()) + ").");
                    bytes += written;
                    size -= written;
                }
            });
        } catch (...) {
            input_error = std::current_exception();
        }
        CloseHandle(write_pipe);
    }
    const DWORD waited = WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 1;
    const BOOL got_exit = GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hProcess);
    if (waited != WAIT_OBJECT_0 || !got_exit) throw std::runtime_error("Failed to wait for FFmpeg.");
#else
    std::vector<std::string> storage{executable.string()};
    storage.insert(storage.end(), args.begin(), args.end());
    std::vector<char*> argv;
    for (auto& arg : storage) argv.push_back(arg.data());
    argv.push_back(nullptr);
    const int log = open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (log < 0) throw std::runtime_error("Cannot create FFmpeg diagnostic log.");
    // A socket works as stdin while allowing SIGPIPE suppression per write.
    int sockets[2] = {-1, -1};
    if (input && socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        close(log);
        throw std::runtime_error("Cannot create FFmpeg input socket.");
    }
    if (input) {
        bool ready = fcntl(sockets[0], F_SETFD, FD_CLOEXEC) == 0 &&
                     fcntl(sockets[1], F_SETFD, FD_CLOEXEC) == 0;
#ifdef __APPLE__
        const int enabled = 1;
        ready = ready && setsockopt(sockets[0], SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) == 0;
#endif
        if (!ready) {
            close(sockets[0]);
            close(sockets[1]);
            close(log);
            throw std::runtime_error("Cannot configure FFmpeg input socket.");
        }
    }
    const pid_t child = fork();
    if (child == 0) {
        if (dup2(log, STDOUT_FILENO) < 0 || dup2(log, STDERR_FILENO) < 0) _exit(126);
        close(log);
        if (input) {
            if (dup2(sockets[1], STDIN_FILENO) < 0) _exit(126);
            close(sockets[0]);
            close(sockets[1]);
        }
        execvp(argv.front(), argv.data());
        _exit(127);
    }
    close(log);
    if (input) close(sockets[1]);
    if (child < 0) {
        if (input) close(sockets[0]);
        throw std::runtime_error("Cannot start FFmpeg process.");
    }
    if (input) {
        try {
            input([socket = sockets[0]](const void* data, std::size_t size) {
                auto* bytes = static_cast<const char*>(data);
                while (size) {
#ifdef __APPLE__
                    constexpr int flags = 0; // SO_NOSIGPIPE is set above.
#else
                    constexpr int flags = MSG_NOSIGNAL;
#endif
                    const auto written = send(socket, bytes, std::min<std::size_t>(size, 1024 * 1024), flags);
                    if (written < 0 && errno == EINTR) continue;
                    if (written <= 0) throw std::runtime_error("FFmpeg input pipe closed (errno " +
                                                              std::to_string(errno) + ").");
                    bytes += written;
                    size -= static_cast<std::size_t>(written);
                }
            });
        } catch (...) {
            input_error = std::current_exception();
        }
        close(sockets[0]);
    }
    int status = 0;
    pid_t waited;
    do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
    if (waited < 0) throw std::runtime_error("Failed to wait for FFmpeg.");
    const int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
#endif
    std::ifstream log_file(log_path);
    std::ostringstream output;
    output << log_file.rdbuf();
    if (input_error) {
        try {
            std::rethrow_exception(input_error);
        } catch (const std::exception& error) {
            throw std::runtime_error(std::string("FFmpeg input failed: ") + error.what() + "\n" + output.str());
        }
    }
    if (exit_code != 0) {
        throw std::runtime_error(executable.filename().string() + " failed: " + output.str() +
                                 "\nInstall FFmpeg (including ffprobe) and add it to PATH, or set COOLBOX_FFMPEG.");
    }
    return output.str();
}

} // namespace

std::shared_ptr<VideoImportCache> detail::make_video_cache() {
    auto cache = std::make_shared<VideoImportCache>();
    std::random_device random;
    for (int attempt = 0; attempt < 10; ++attempt) {
        const auto candidate = fs::temp_directory_path() / ("coolbox_video_" + std::to_string(random()));
        if (fs::create_directory(candidate)) {
            cache->directory = candidate.string();
            break;
        }
    }
    if (cache->directory.empty()) throw std::runtime_error("Cannot create a temporary video import directory.");
    return cache;
}

fs::path detail::ffmpeg_executable() {
    const char* configured = std::getenv("COOLBOX_FFMPEG");
    if (configured && *configured) return fs::path(configured);
#ifdef _WIN32
    auto complete = [](const fs::path& executable) {
        return fs::is_regular_file(executable) &&
               fs::is_regular_file(executable.parent_path() / L"ffprobe.exe");
    };
    std::vector<wchar_t> buffer(32768);
    const DWORD length = SearchPathW(nullptr, L"ffmpeg.exe", nullptr,
                                     static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
    if (length > 0 && length < buffer.size()) {
        const fs::path candidate(std::wstring(buffer.data(), length));
        if (complete(candidate)) return candidate;
    }
    for (const wchar_t* variable : {L"LOCALAPPDATA", L"ProgramFiles"}) {
        const wchar_t* root = _wgetenv(variable);
        if (!root || !*root) continue;
        const fs::path packages = fs::path(root) /
            (std::wstring(variable) == L"LOCALAPPDATA" ? L"Microsoft\\WinGet\\Packages" : L"WinGet\\Packages");
        if (!fs::is_directory(packages)) continue;
        for (const auto& package : fs::directory_iterator(packages)) {
            if (!package.is_directory() ||
                package.path().filename().wstring().find(L"Gyan.FFmpeg.Essentials_") != 0) continue;
            for (const auto& entry : fs::recursive_directory_iterator(package.path())) {
                if (entry.path().filename() == L"ffmpeg.exe" && complete(entry.path()))
                    return entry.path();
            }
        }
    }
    throw std::runtime_error("FFmpeg and ffprobe were not found. Run the Windows movie editor setup "
        "to install them, or set COOLBOX_FFMPEG to ffmpeg.exe with ffprobe.exe in the same folder.");
#else
    return fs::path("ffmpeg");
#endif
}

std::string detail::run_ffmpeg(const std::vector<std::string>& arguments, const fs::path& log,
                             const InputProducer& input) {
    return run_tool(detail::ffmpeg_executable(), arguments, log, input);
}

MediaAsset decode_ffmpeg_video(const std::string& path) {
    if (!fs::is_regular_file(path)) throw std::runtime_error("Video file not found: " + path);
    auto cache = detail::make_video_cache();
    const fs::path directory(cache->directory);
    const fs::path ffmpeg = detail::ffmpeg_executable();
    fs::path ffprobe = ffmpeg.parent_path() / "ffprobe";
#ifdef _WIN32
    ffprobe += ".exe";
#endif
    const std::string source = fs::absolute(path).string();
    const auto audio = run_tool(ffprobe, {"-v", "error", "-select_streams", "a:0",
        "-show_entries", "stream=codec_type", "-of", "csv=p=0", source}, directory / "probe.log");

    run_tool(ffmpeg, {"-nostdin", "-v", "error", "-y", "-i", source, "-map", "0:v:0",
        "-an", "-vf", "fps=24", "-c:v", "png", "-pix_fmt", "rgb24",
        "-compression_level", "1", "-pred", "mixed", "-start_number", "0",
        (directory / "frame_%08d.png").string()}, directory / "video.log");

    MediaAsset asset;
    asset.type = MediaType::ImportedVideo;
    asset.path = source;
    asset.fps = 24.0;
    asset.import_cache = cache;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".png") asset.frame_files.push_back(entry.path().string());
    }
    std::sort(asset.frame_files.begin(), asset.frame_files.end());
    if (asset.frame_files.empty()) throw std::runtime_error("FFmpeg decoded no video frames.");
    asset.duration_us = static_cast<std::int64_t>(asset.frame_files.size() * 1'000'000.0 / asset.fps);
    if (audio.find("audio") != std::string::npos) {
        asset.audio_path = (directory / "audio.wav").string();
        run_tool(ffmpeg, {"-nostdin", "-v", "error", "-y", "-i", source, "-map", "0:a:0",
            "-vn", "-ac", "2", "-ar", "48000", "-c:a", "pcm_s16le", asset.audio_path}, directory / "audio.log");
    }
    return asset;
}

} // namespace trekker::movie_editor
