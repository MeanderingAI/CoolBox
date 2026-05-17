#include "ai_chat.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace tools {
namespace ai_chat {
namespace {

std::string normalize_text(const std::string& text) {
    std::string normalized;
    normalized.reserve(text.size());

    bool previous_space = true;
    for (char ch : text) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch) || ch == '+' || ch == '-' || ch == '_' || ch == '.') {
            normalized.push_back(static_cast<char>(std::tolower(uch)));
            previous_space = false;
        } else if (!previous_space) {
            normalized.push_back(' ');
            previous_space = true;
        }
    }

    while (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }
    return normalized;
}

bool starts_with(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool contains_phrase(const std::string& value, const std::string& phrase) {
    return value.find(phrase) != std::string::npos;
}

int trailing_int(const std::string& value, int fallback) {
    std::istringstream stream(value);
    std::string token;
    int parsed = fallback;
    while (stream >> token) {
        try {
            std::size_t pos = 0;
            const int number = std::stoi(token, &pos);
            if (pos == token.size()) {
                parsed = number;
            }
        } catch (...) {
        }
    }
    return parsed;
}

bool is_explicit_cad_command(const std::string& normalized) {
    if (normalized.empty()) {
        return false;
    }

    static const std::array<const char*, 29> prefixes = {
        "add", "addline", "addarc", "addcircle", "list", "select", "move", "resize",
        "fill", "duplicate", "delete", "clear", "snap", "grid", "snapthreshold", "snapinfo",
        "pan", "yaw", "pitch", "zoom", "resetview", "cam", "export", "save", "load",
        "undo", "redo", "history", "help"
    };

    for (const char* prefix : prefixes) {
        if (normalized == prefix || starts_with(normalized, std::string(prefix) + " ")) {
            return true;
        }
    }

    return normalized == "q" || normalized == "quit" || normalized == "exit" ||
           normalized == "!!" || starts_with(normalized, "!");
}

std::string map_natural_phrase_to_command(const std::string& normalized) {
    if (normalized.empty()) {
        return std::string();
    }

    if (starts_with(normalized, "pan left") || contains_phrase(normalized, " pan left")) {
        const int amount = trailing_int(normalized, 20);
        return "pan -" + std::to_string(std::max(1, amount)) + " 0";
    }

    if (starts_with(normalized, "pan right") || contains_phrase(normalized, " pan right")) {
        const int amount = trailing_int(normalized, 20);
        return "pan " + std::to_string(std::max(1, amount)) + " 0";
    }

    if (starts_with(normalized, "pan up") || contains_phrase(normalized, " pan up")) {
        const int amount = trailing_int(normalized, 20);
        return "pan 0 " + std::to_string(std::max(1, amount));
    }

    if (starts_with(normalized, "pan down") || contains_phrase(normalized, " pan down")) {
        const int amount = trailing_int(normalized, 20);
        return "pan 0 -" + std::to_string(std::max(1, amount));
    }

    if (contains_phrase(normalized, "save scene")) {
        return "save";
    }

    if (contains_phrase(normalized, "load scene") || contains_phrase(normalized, "open scene")) {
        return "load";
    }

    if (contains_phrase(normalized, "export preview") || contains_phrase(normalized, "save preview")) {
        return "export";
    }

    if (contains_phrase(normalized, "clear selection") || normalized == "cancel") {
        return "esc";
    }

    if (starts_with(normalized, "zoom in") || contains_phrase(normalized, " zoom in")) {
        const int amount = trailing_int(normalized, 90);
        return "zoom " + std::to_string(std::max(1, amount));
    }

    if (starts_with(normalized, "zoom out") || contains_phrase(normalized, " zoom out")) {
        const int amount = trailing_int(normalized, 90);
        return "zoom -" + std::to_string(std::max(1, amount));
    }

    if (normalized == "repeat last command") {
        return "!!";
    }

    if (starts_with(normalized, "repeat command ") || starts_with(normalized, "history entry ")) {
        const int index = trailing_int(normalized, -1);
        if (index >= 0) {
            return "!" + std::to_string(index);
        }
    }

    if (contains_phrase(normalized, "add rectangle") || contains_phrase(normalized, "draw rectangle") ||
        contains_phrase(normalized, "create rectangle")) {
        return "add";
    }

    if (contains_phrase(normalized, "add line") || contains_phrase(normalized, "draw line") ||
        normalized == "line") {
        return "addline";
    }

    if (contains_phrase(normalized, "add arc") || contains_phrase(normalized, "draw arc")) {
        return "addarc";
    }

    if (contains_phrase(normalized, "add circle") || contains_phrase(normalized, "draw circle") ||
        contains_phrase(normalized, "create circle")) {
        return "addcircle";
    }

    if (starts_with(normalized, "list") || starts_with(normalized, "show objects")) {
        return "list";
    }

    if (normalized == "undo") {
        return "undo";
    }
    if (normalized == "redo") {
        return "redo";
    }
    if (normalized == "help") {
        return "help";
    }
    if (normalized == "quit" || normalized == "exit") {
        return "q";
    }

    if (is_explicit_cad_command(normalized)) {
        return normalized;
    }

    return std::string();
}

} // namespace

InterpretedCommand AiChatSpeechBridge::interpret_speech(const std::string& speech_text) const {
    const std::string normalized = normalize_text(speech_text);
    const std::string cad_command = map_natural_phrase_to_command(normalized);

    InterpretedCommand result;
    result.speech_text = speech_text;
    result.cad_command = cad_command;
    result.recognized = !cad_command.empty();
    return result;
}

bool AiChatSpeechBridge::submit_speech(const std::string& speech_text) {
    const InterpretedCommand interpreted = interpret_speech(speech_text);
    history_.push_back(interpreted);
    if (!interpreted.recognized) {
        return false;
    }
    pending_commands_.push_back(interpreted.cad_command);
    return true;
}

bool AiChatSpeechBridge::has_pending_commands() const {
    return !pending_commands_.empty();
}

std::string AiChatSpeechBridge::pop_next_command() {
    if (pending_commands_.empty()) {
        return std::string();
    }
    const std::string command = pending_commands_.front();
    pending_commands_.pop_front();
    return command;
}

const std::vector<InterpretedCommand>& AiChatSpeechBridge::history() const {
    return history_;
}

} // namespace ai_chat
} // namespace tools
