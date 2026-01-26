#pragma once
#include <map>
#include <string>
#include <vector>
#include "utils/unix_commands/unix_commands.hpp"

struct MakeHelpCache {
    std::map<std::string, std::vector<std::string>> categories;
    std::string raw;
    void refresh(const std::string& workspace_path = ".") {
        auto lines = UnixCommands::make_help(workspace_path);
        raw.clear();
        categories.clear();
        std::string current;
        for (const auto& line : lines) {
            raw += line + "\n";
            if (line.find("[APPS]") != std::string::npos) current = "apps";
            else if (line.find("[DEMOS]") != std::string::npos) current = "demos";
            else if (line.find("[SERVICES]") != std::string::npos) current = "services";
            else if (line.find("[LIBRARIES]") != std::string::npos) current = "libraries";
            else if (!current.empty() && !line.empty() && line[0] != '=') categories[current].push_back(line);
        }
    }
};
