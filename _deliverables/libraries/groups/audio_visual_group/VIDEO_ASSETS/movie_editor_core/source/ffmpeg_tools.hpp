#pragma once

#include "../headers/movie_editor_core.h"
#include <filesystem>
#include <functional>

namespace trekker::movie_editor::detail {
std::shared_ptr<VideoImportCache> make_video_cache();
using ByteWriter = std::function<void(const void*, std::size_t)>;
using InputProducer = std::function<void(const ByteWriter&)>;
std::filesystem::path ffmpeg_executable();
std::string run_ffmpeg(const std::vector<std::string>& arguments, const std::filesystem::path& log,
                      const InputProducer& input = {});
}
