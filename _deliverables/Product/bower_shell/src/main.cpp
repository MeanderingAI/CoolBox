#include <iostream>
#include <string>
#include <vector>

#include "bower_shell.hpp"
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
#include "installer_abstraction.hpp"
#endif

namespace {

void print_lines(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        std::cout << line << '\n';
    }
}

std::string join_arguments(int argc, char** argv, int start_index) {
    std::string joined;
    for (int index = start_index; index < argc; ++index) {
        if (index != start_index) {
            joined += ' ';
        }
        joined += argv[index];
    }
    return joined;
}

} // namespace

int main(int argc, char** argv) {
    tools::bower_shell::ShellSession session;
#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    const auto pre_release_screen = os_generics::installer::create_pre_release_screen("Bower Shell", "bower_shell");
#endif

    if (argc > 2 && std::string(argv[1]) == "-c") {
        const auto result = session.execute(join_arguments(argc, argv, 2));
        print_lines(result.output);
        return result.exit_code;
    }

#if defined(COOLBOX_PRODUCT_INSTALLER_ABSTRACTIONS)
    std::cout << os_generics::installer::render_pre_release_text(pre_release_screen) << '\n';
#endif
    std::cout << "CoolBox bower_shell\n";
    std::cout << "Type 'help' for commands, 'exit' to quit.\n";

    std::string line;
    while (true) {
        for (const auto& job : session.take_completed_jobs()) {
            std::cout << "[job " << job.id << "] " << tools::bower_shell::to_string(job.state)
                      << " " << job.command_line << '\n';
            print_lines(job.output);
        }

        std::cout << session.prompt();
        if (!std::getline(std::cin, line)) {
            break;
        }
        if (line == "exit" || line == "quit") {
            break;
        }
        const auto result = session.execute(line);
        print_lines(result.output);
        session.tick();
    }

    return 0;
}