#ifndef COOLBOX_LIBRARIES_GROUPS_APP_ASSETS_AI_CHAT_HEADERS_AI_CHAT_HPP
#define COOLBOX_LIBRARIES_GROUPS_APP_ASSETS_AI_CHAT_HEADERS_AI_CHAT_HPP

#include <deque>
#include <string>
#include <vector>

namespace tools {
namespace ai_chat {

struct InterpretedCommand {
    std::string speech_text;
    std::string cad_command;
    bool recognized = false;
};

class AiChatSpeechBridge {
public:
    InterpretedCommand interpret_speech(const std::string& speech_text) const;

    bool submit_speech(const std::string& speech_text);
    bool has_pending_commands() const;
    std::string pop_next_command();

    const std::vector<InterpretedCommand>& history() const;

private:
    std::vector<InterpretedCommand> history_;
    std::deque<std::string> pending_commands_;
};

} // namespace ai_chat
} // namespace tools

#endif
