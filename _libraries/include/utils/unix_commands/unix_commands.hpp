        
#ifndef UNIX_COMMANDS_HPP
#define UNIX_COMMANDS_HPP

#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include <stdexcept>

class UnixCommands {
public:

    // Lambda for generating the find command string given a workspace path
    static inline const auto FIND_SHARED_LIBS_CMD = [](const std::string& workspace_path) {
        return "find " + workspace_path + "/build/src -type f ( -name '*.so' -o -name '*.dylib' ) 2>/dev/null";
    };

    // Lambda for generating the bin directory path given a workspace path
    static inline const auto BIN_DIR_PATH = [](const std::string& workspace_path) {
        return workspace_path + "/build/bin";
    };

    // Lambda for generating the cmake build command given a target name
    static inline const auto CMAKE_BUILD_CMD = [](const std::string& target_name) {
        return "cmake --build build --target " + target_name + " -j8";
    };

    // Run a shell command and return output lines
    static std::vector<std::string> run(const std::string& cmd) {
        std::vector<std::string> result;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            line.erase(line.find_last_not_of(" \n\r\t") + 1);
            result.push_back(line);
        }
        pclose(pipe);
        return result;
    }

    // Run a shell command and return the first line (or empty string)
    static std::string run_single(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) throw std::runtime_error("popen() failed!");
        char buffer[1024];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            result = buffer;
            result.erase(result.find_last_not_of(" \n\r\t") + 1);
        }
        pclose(pipe);
        return result;
    }

    // Centralized command string for finding shared libraries
    static std::string find_shared_libs_cmd(const std::string& workspace_path) {
        return FIND_SHARED_LIBS_CMD(workspace_path);
    }


};

#endif // UNIX_COMMANDS_HPP
