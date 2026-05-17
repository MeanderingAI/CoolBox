#include "ai_chat.hpp"
#include "tyst_framework.hpp"

namespace {

using tools::ai_chat::AiChatSpeechBridge;

TYST_TEST(AiChat, MapsNaturalPhrases) {
    AiChatSpeechBridge bridge;
    const auto interpreted = bridge.interpret_speech("please add line");
    TYST_EXPECT_TRUE(interpreted.recognized);
    TYST_EXPECT_EQ(interpreted.cad_command, std::string("addline"));
}

TYST_TEST(AiChat, SupportsDirectCadCommandSpeech) {
    AiChatSpeechBridge bridge;
    const auto interpreted = bridge.interpret_speech("move line 2 10 -5");
    TYST_EXPECT_TRUE(interpreted.recognized);
    TYST_EXPECT_EQ(interpreted.cad_command, std::string("move line 2 10 -5"));
}

TYST_TEST(AiChat, QueuesRecognizedCommands) {
    AiChatSpeechBridge bridge;
    TYST_EXPECT_TRUE(bridge.submit_speech("zoom in 120"));
    TYST_EXPECT_TRUE(bridge.has_pending_commands());
    TYST_EXPECT_EQ(bridge.pop_next_command(), std::string("zoom 120"));
    TYST_EXPECT_TRUE(!bridge.has_pending_commands());
}

TYST_TEST(AiChat, UnknownSpeechIsTrackedButNotQueued) {
    AiChatSpeechBridge bridge;
    TYST_EXPECT_TRUE(!bridge.submit_speech("this is not a cad instruction"));
    TYST_EXPECT_TRUE(!bridge.has_pending_commands());
    TYST_EXPECT_EQ(bridge.history().size(), static_cast<std::size_t>(1));
    TYST_EXPECT_TRUE(!bridge.history().front().recognized);
}

TYST_TEST(AiChat, ReplayPhraseMapsToHistoryCommand) {
    AiChatSpeechBridge bridge;
    const auto interpreted = bridge.interpret_speech("repeat command 3");
    TYST_EXPECT_TRUE(interpreted.recognized);
    TYST_EXPECT_EQ(interpreted.cad_command, std::string("!3"));
}

} // namespace
