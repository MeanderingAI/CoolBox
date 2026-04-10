#ifndef COOLBOX__LIBRARIES_BACKAGES_TOOLS_BOWER_SHELL_HEADERS_BOWER_SHELL_HPP
#define COOLBOX__LIBRARIES_BACKAGES_TOOLS_BOWER_SHELL_HEADERS_BOWER_SHELL_HPP

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace tools {
namespace bower_shell {

enum class JobState {
    Running,
    Completed,
    Failed
};

struct CommandResult {
    int exit_code = 0;
    bool launched_in_background = false;
    int background_job_id = -1;
    std::vector<std::string> output;
};

struct BackgroundJob {
    int id = -1;
    std::string command_line;
    JobState state = JobState::Running;
    std::size_t ticks_remaining = 0;
    int exit_code = 0;
    std::vector<std::string> output;
};

class ShellSession {
public:
    ShellSession();
    explicit ShellSession(std::filesystem::path working_directory);

    const std::filesystem::path& working_directory() const;
    void set_working_directory(const std::filesystem::path& path);

    CommandResult execute(const std::string& command_line);
    void tick(std::size_t steps = 1);
    std::vector<BackgroundJob> jobs() const;
    std::vector<BackgroundJob> take_completed_jobs();
    std::string prompt() const;

private:
    struct ParsedCommand {
        std::string name;
        std::vector<std::string> arguments;
        bool run_in_background = false;
    };

    std::filesystem::path working_directory_;
    int next_job_id_ = 1;
    std::vector<BackgroundJob> running_jobs_;
    std::vector<BackgroundJob> completed_jobs_;

    ParsedCommand parse_command_line(const std::string& command_line) const;
    CommandResult run_parsed_command(const ParsedCommand& command);
    CommandResult launch_background_job(const ParsedCommand& command);
    BackgroundJob build_background_job(const ParsedCommand& command) const;

    CommandResult command_help() const;
    CommandResult command_pwd() const;
    CommandResult command_cd(const ParsedCommand& command);
    CommandResult command_ls(const ParsedCommand& command) const;
    CommandResult command_echo(const ParsedCommand& command) const;
    CommandResult command_cat(const ParsedCommand& command) const;
    CommandResult command_mkdir(const ParsedCommand& command) const;
    CommandResult command_touch(const ParsedCommand& command) const;
    CommandResult command_sleep(const ParsedCommand& command) const;
    CommandResult command_jobs() const;
    CommandResult command_wait(const ParsedCommand& command);
    CommandResult command_unknown(const ParsedCommand& command) const;

    std::filesystem::path resolve_path(const std::string& value) const;
};

std::string to_string(JobState state);

} // namespace bower_shell
} // namespace tools

#endif  // COOLBOX__LIBRARIES_BACKAGES_TOOLS_BOWER_SHELL_HEADERS_BOWER_SHELL_HPP