#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace file_system {

class FileWatcher {
public:
    using Callback = std::function<void(const std::string& path)>;
    FileWatcher(const std::vector<std::string>& files, std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
    ~FileWatcher();
    void start(const Callback& on_modified);
    void stop();
private:
    struct Impl;
    Impl* impl_;
};

} // namespace file_system
