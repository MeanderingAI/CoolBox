#include "bower_shell.hpp"
#include "tyst_framework.hpp"

#include <filesystem>

namespace {

using tools::bower_shell::JobState;
using tools::bower_shell::ShellSession;

TYST_TEST(BowerShell, EchoCommandReturnsOutput) {
    ShellSession session(std::filesystem::current_path());
    const auto result = session.execute("echo hello world");
    TYST_EXPECT_EQ(result.exit_code, 0);
    TYST_EXPECT_EQ(result.output.size(), 1U);
    TYST_EXPECT_EQ(result.output.front(), "hello world");
}

TYST_TEST(BowerShell, BackgroundSleepFinishesAfterTicks) {
    ShellSession session(std::filesystem::current_path());
    const auto launch = session.execute("sleep 2 &");
    TYST_EXPECT_TRUE(launch.launched_in_background);
    TYST_EXPECT_TRUE(launch.background_job_id > 0);

    session.tick();
    const auto running_jobs = session.jobs();
    TYST_EXPECT_EQ(running_jobs.size(), 1U);
    TYST_EXPECT_EQ(running_jobs.front().state, JobState::Running);

    session.tick();
    const auto completed = session.take_completed_jobs();
    TYST_EXPECT_EQ(completed.size(), 1U);
    TYST_EXPECT_EQ(completed.front().state, JobState::Completed);
}

TYST_TEST(BowerShell, WaitCompletesBackgroundJob) {
    ShellSession session(std::filesystem::current_path());
    session.execute("sleep 3 &");
    const auto waited = session.execute("wait");
    TYST_EXPECT_EQ(waited.exit_code, 0);
    TYST_EXPECT_TRUE(!waited.output.empty());
}

} // namespace