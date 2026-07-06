#include "bower_shell.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <system_error>

namespace tools {
namespace bower_shell {

namespace {

std::string join_arguments(const std::vector<std::string>& arguments, std::size_t start_index = 0) {
    std::ostringstream stream;
    for (std::size_t index = start_index; index < arguments.size(); ++index) {
        if (index != start_index) {
            stream << ' ';
        }
        stream << arguments[index];
    }
    return stream.str();
}

std::vector<std::string> tokenize(const std::string& command_line) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;

    for (char ch : command_line) {
        if (ch == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (!in_quotes && std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

int parse_non_negative_int(const std::string& value, int fallback = 0) {
    try {
        const int parsed = std::stoi(value);
        return parsed < 0 ? fallback : parsed;
    } catch (...) {
        return fallback;
    }
}

} // namespace

std::string to_string(JobState state) {
    switch (state) {
    case JobState::Running:
        return "running";
    case JobState::Completed:
        return "completed";
    case JobState::Failed:
        return "failed";
    }
    return "unknown";
}

ShellSession::ShellSession()
    : ShellSession(std::filesystem::current_path()) {}

ShellSession::ShellSession(std::filesystem::path working_directory)
    : working_directory_(std::move(working_directory)) {}

const std::filesystem::path& ShellSession::working_directory() const {
    return working_directory_;
}

void ShellSession::set_working_directory(const std::filesystem::path& path) {
    working_directory_ = path;
}

ShellSession::ParsedCommand ShellSession::parse_command_line(const std::string& command_line) const {
    ParsedCommand parsed;
    std::vector<std::string> tokens = tokenize(command_line);
    if (tokens.empty()) {
        return parsed;
    }

    if (tokens.back() == "&") {
        parsed.run_in_background = true;
        tokens.pop_back();
    } else if (!tokens.back().empty() && tokens.back().back() == '&') {
        parsed.run_in_background = true;
        tokens.back().pop_back();
        if (tokens.back().empty()) {
            tokens.pop_back();
        }
    }

    if (tokens.empty()) {
        return parsed;
    }

    parsed.name = tokens.front();
    parsed.arguments.assign(tokens.begin() + 1, tokens.end());
    return parsed;
}

CommandResult ShellSession::execute(const std::string& command_line) {
    ParsedCommand command = parse_command_line(command_line);
    if (command.name.empty()) {
        return {};
    }
    if (command.run_in_background && command.name != "jobs" && command.name != "wait") {
        return launch_background_job(command);
    }
    return run_parsed_command(command);
}

CommandResult ShellSession::run_parsed_command(const ParsedCommand& command) {
    if (command.name == "help") {
        return command_help();
    }
    if (command.name == "pwd") {
        return command_pwd();
    }
    if (command.name == "cd") {
        return command_cd(command);
    }
    if (command.name == "ls") {
        return command_ls(command);
    }
    if (command.name == "echo") {
        return command_echo(command);
    }
    if (command.name == "cat") {
        return command_cat(command);
    }
    if (command.name == "mkdir") {
        return command_mkdir(command);
    }
    if (command.name == "touch") {
        return command_touch(command);
    }
    if (command.name == "sleep") {
        return command_sleep(command);
    }
    if (command.name == "jobs") {
        return command_jobs();
    }
    if (command.name == "wait") {
        return command_wait(command);
    }
    return command_unknown(command);
}

BackgroundJob ShellSession::build_background_job(const ParsedCommand& command) {
    BackgroundJob job;
    job.command_line = command.name;
    if (!command.arguments.empty()) {
        job.command_line += ' ';
        job.command_line += join_arguments(command.arguments);
    }

    if (command.name == "sleep") {
        const int ticks = command.arguments.empty() ? 1 : parse_non_negative_int(command.arguments.front(), 1);
        job.ticks_remaining = static_cast<std::size_t>((std::max)(ticks, 1));
        job.output.push_back("sleep scheduled for " + std::to_string(job.ticks_remaining) + " tick(s)");
        return job;
    }

    job.ticks_remaining = 1;
    ParsedCommand foreground_command = command;
    foreground_command.run_in_background = false;
    CommandResult result = run_parsed_command(foreground_command);
    job.exit_code = result.exit_code;
    job.state = result.exit_code == 0 ? JobState::Running : JobState::Failed;
    job.output = result.output;
    return job;
}

CommandResult ShellSession::launch_background_job(const ParsedCommand& command) {
    BackgroundJob job = build_background_job(command);
    job.id = next_job_id_++;
    running_jobs_.push_back(job);

    CommandResult result;
    result.launched_in_background = true;
    result.background_job_id = job.id;
    result.output.push_back("started job [" + std::to_string(job.id) + "] " + job.command_line);
    return result;
}

void ShellSession::tick(std::size_t steps) {
    for (std::size_t step = 0; step < steps; ++step) {
        std::vector<BackgroundJob> still_running;
        still_running.reserve(running_jobs_.size());
        for (BackgroundJob job : running_jobs_) {
            if (job.ticks_remaining > 0) {
                --job.ticks_remaining;
            }
            if (job.ticks_remaining == 0) {
                if (job.state != JobState::Failed) {
                    job.state = JobState::Completed;
                    if (job.output.empty()) {
                        job.output.push_back("job finished: " + job.command_line);
                    }
                }
                completed_jobs_.push_back(job);
            } else {
                still_running.push_back(job);
            }
        }
        running_jobs_.swap(still_running);
    }
}

std::vector<BackgroundJob> ShellSession::jobs() const {
    std::vector<BackgroundJob> snapshot = running_jobs_;
    snapshot.insert(snapshot.end(), completed_jobs_.begin(), completed_jobs_.end());
    return snapshot;
}

std::vector<BackgroundJob> ShellSession::take_completed_jobs() {
    std::vector<BackgroundJob> finished = completed_jobs_;
    completed_jobs_.clear();
    return finished;
}

std::string ShellSession::prompt() const {
    return "bower:" + working_directory_.string() + "$ ";
}

CommandResult ShellSession::command_help() const {
    CommandResult result;
    result.output = {
        "Available commands:",
        "  help               Show this help message",
        "  pwd                Print working directory",
        "  cd <path>          Change working directory",
        "  ls [path]          List files and folders",
        "  echo <text>        Print text",
        "  cat <file>         Print file contents",
        "  mkdir <path>       Create a directory",
        "  touch <path>       Create an empty file if needed",
        "  sleep <ticks>      Simulate work for a number of ticks",
        "  jobs               Show background jobs",
        "  wait [job-id]      Advance until a job or all jobs complete",
        "Append '&' to run a command in the background."
    };
    return result;
}

CommandResult ShellSession::command_pwd() const {
    CommandResult result;
    result.output.push_back(working_directory_.string());
    return result;
}

CommandResult ShellSession::command_cd(const ParsedCommand& command) {
    CommandResult result;
    const std::filesystem::path destination = command.arguments.empty()
        ? std::filesystem::current_path()
        : resolve_path(command.arguments.front());

    std::error_code error;
    if (!std::filesystem::exists(destination, error) || !std::filesystem::is_directory(destination, error)) {
        result.exit_code = 1;
        result.output.push_back("cd: directory not found: " + destination.string());
        return result;
    }

    working_directory_ = std::filesystem::weakly_canonical(destination, error);
    if (error) {
        working_directory_ = destination;
    }
    result.output.push_back(working_directory_.string());
    return result;
}

CommandResult ShellSession::command_ls(const ParsedCommand& command) const {
    CommandResult result;
    const std::filesystem::path directory = command.arguments.empty()
        ? working_directory_
        : resolve_path(command.arguments.front());

    std::error_code error;
    if (!std::filesystem::exists(directory, error) || !std::filesystem::is_directory(directory, error)) {
        result.exit_code = 1;
        result.output.push_back("ls: directory not found: " + directory.string());
        return result;
    }

    std::vector<std::filesystem::directory_entry> entries;
    for (std::filesystem::directory_iterator it(directory, std::filesystem::directory_options::skip_permission_denied, error);
         !error && it != std::filesystem::directory_iterator();
         it.increment(error)) {
        entries.push_back(*it);
    }

    if (error) {
        result.exit_code = 1;
        result.output.push_back("ls: unable to read directory: " + directory.string());
        return result;
    }

    std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
        const std::string left_name = left.path().filename().string();
        const std::string right_name = right.path().filename().string();
        return left_name < right_name;
    });

    for (const auto& entry : entries) {
        const bool is_directory = entry.is_directory(error);
        result.output.push_back((is_directory ? "[dir] " : "[file] ") + entry.path().filename().string());
    }
    if (result.output.empty()) {
        result.output.push_back("(empty)");
    }
    return result;
}

CommandResult ShellSession::command_echo(const ParsedCommand& command) const {
    CommandResult result;
    result.output.push_back(join_arguments(command.arguments));
    return result;
}

CommandResult ShellSession::command_cat(const ParsedCommand& command) const {
    CommandResult result;
    if (command.arguments.empty()) {
        result.exit_code = 1;
        result.output.push_back("cat: file path required");
        return result;
    }

    const std::filesystem::path file_path = resolve_path(command.arguments.front());
    std::ifstream input(file_path, std::ios::binary);
    if (!input) {
        result.exit_code = 1;
        result.output.push_back("cat: unable to open: " + file_path.string());
        return result;
    }

    std::string line;
    while (std::getline(input, line)) {
        result.output.push_back(line);
    }
    return result;
}

CommandResult ShellSession::command_mkdir(const ParsedCommand& command) const {
    CommandResult result;
    if (command.arguments.empty()) {
        result.exit_code = 1;
        result.output.push_back("mkdir: path required");
        return result;
    }

    const std::filesystem::path directory = resolve_path(command.arguments.front());
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        result.exit_code = 1;
        result.output.push_back("mkdir: unable to create: " + directory.string());
        return result;
    }
    result.output.push_back("created: " + directory.string());
    return result;
}

CommandResult ShellSession::command_touch(const ParsedCommand& command) const {
    CommandResult result;
    if (command.arguments.empty()) {
        result.exit_code = 1;
        result.output.push_back("touch: path required");
        return result;
    }

    const std::filesystem::path file_path = resolve_path(command.arguments.front());
    std::error_code error;
    std::filesystem::create_directories(file_path.parent_path(), error);
    std::ofstream output(file_path, std::ios::app);
    if (!output) {
        result.exit_code = 1;
        result.output.push_back("touch: unable to write: " + file_path.string());
        return result;
    }
    result.output.push_back("touched: " + file_path.string());
    return result;
}

CommandResult ShellSession::command_sleep(const ParsedCommand& command) const {
    CommandResult result;
    const int ticks = command.arguments.empty() ? 1 : parse_non_negative_int(command.arguments.front(), 1);
    result.output.push_back("slept for " + std::to_string((std::max)(ticks, 1)) + " tick(s)");
    return result;
}

CommandResult ShellSession::command_jobs() const {
    CommandResult result;
    for (const BackgroundJob& job : running_jobs_) {
        result.output.push_back("[" + std::to_string(job.id) + "] " + to_string(job.state) + " " + job.command_line);
    }
    for (const BackgroundJob& job : completed_jobs_) {
        result.output.push_back("[" + std::to_string(job.id) + "] " + to_string(job.state) + " " + job.command_line);
    }
    if (result.output.empty()) {
        result.output.push_back("no jobs");
    }
    return result;
}

CommandResult ShellSession::command_wait(const ParsedCommand& command) {
    CommandResult result;
    if (running_jobs_.empty()) {
        result.output.push_back("no running jobs");
        return result;
    }

    if (!command.arguments.empty()) {
        const int requested_id = parse_non_negative_int(command.arguments.front(), -1);
        if (requested_id < 0) {
            result.exit_code = 1;
            result.output.push_back("wait: invalid job id");
            return result;
        }
        while (std::any_of(running_jobs_.begin(), running_jobs_.end(), [requested_id](const BackgroundJob& job) {
            return job.id == requested_id;
        })) {
            tick();
        }
    } else {
        while (!running_jobs_.empty()) {
            tick();
        }
    }

    for (BackgroundJob& job : completed_jobs_) {
        result.output.push_back("[" + std::to_string(job.id) + "] " + to_string(job.state) + " " + job.command_line);
        result.output.insert(result.output.end(), job.output.begin(), job.output.end());
    }
    return result;
}

CommandResult ShellSession::command_unknown(const ParsedCommand& command) const {
    CommandResult result;
    result.exit_code = 127;
    result.output.push_back("unknown command: " + command.name);
    return result;
}

std::filesystem::path ShellSession::resolve_path(const std::string& value) const {
    const std::filesystem::path raw_path(value);
    if (raw_path.is_absolute()) {
        return raw_path;
    }
    return working_directory_ / raw_path;
}

} // namespace bower_shell
} // namespace tools