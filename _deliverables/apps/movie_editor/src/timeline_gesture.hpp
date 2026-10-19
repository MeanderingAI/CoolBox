#pragma once

#include "movie_editor_core.h"
#include <algorithm>
#include <cmath>

namespace movie_editor_app {
enum class DragKind { Move, TrimStart, TrimEnd };

inline trekker::timeline::Clip preview_edit(const trekker::timeline::Clip& original,
                                          DragKind kind, std::int64_t delta_us,
                                          std::int64_t source_duration_us) {
    auto result = original;
    constexpr std::int64_t frame_us = 1'000'000 / 24;
    delta_us = static_cast<std::int64_t>(std::llround(static_cast<double>(delta_us) / frame_us)) * frame_us;
    const auto minimum = std::max<std::int64_t>(1, static_cast<std::int64_t>(frame_us * original.speed));
    if (kind == DragKind::Move) {
        result.position_us = std::max<std::int64_t>(0, original.position_us + delta_us);
    } else if (kind == DragKind::TrimStart) {
        const auto source_delta = static_cast<std::int64_t>(delta_us * original.speed);
        const auto earliest = std::max<std::int64_t>(0, original.in_point_us -
            static_cast<std::int64_t>(original.position_us * original.speed));
        result.in_point_us = std::clamp(original.in_point_us + source_delta, earliest,
                                        std::max(earliest, original.out_point_us - minimum));
        result.position_us = original.position_us + static_cast<std::int64_t>(
            (result.in_point_us - original.in_point_us) / original.speed);
    } else {
        result.out_point_us = std::clamp(original.out_point_us +
            static_cast<std::int64_t>(delta_us * original.speed),
            std::min(source_duration_us, original.in_point_us + minimum), source_duration_us);
    }
    return result;
}
}
