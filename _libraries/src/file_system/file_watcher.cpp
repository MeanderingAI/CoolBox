#include "file_system/file_watcher.h"
#include <thread>
#include <atomic>
#include <unordered_map>
#include <sys/stat.h>

namespace file_system {

struct FileWatcher::Impl {
    std::vector<std::string> files;
    std::chrono::milliseconds interval;
    std::atomic<bool> running{false};
    std::thread watcher_thread;
    std::unordered_map<std::string, time_t> last_modified;
    FileWatcher::Callback callback;

    void watch() {
        while (running) {
            for (const auto& file : files) {
                struct stat st;
                if (stat(file.c_str(), &st) == 0) {
                    auto it = last_modified.find(file);
                    if (it == last_modified.end()) {
                        last_modified[file] = st.st_mtime;
                    } else if (it->second != st.st_mtime) {
                        it->second = st.st_mtime;
                        if (callback) callback(file);
                    }
                }
            }
            std::this_thread::sleep_for(interval);
        }
    }
};

FileWatcher::FileWatcher(const std::vector<std::string>& files, std::chrono::milliseconds interval)
    : impl_(new Impl{files, interval}) {}

FileWatcher::~FileWatcher() { stop(); delete impl_; }

void FileWatcher::start(const Callback& on_modified) {
    if (impl_->running) return;
    impl_->callback = on_modified;
    impl_->running = true;
    impl_->watcher_thread = std::thread([this]{ impl_->watch(); });
}

void FileWatcher::stop() {
    if (!impl_->running) return;
    impl_->running = false;
    if (impl_->watcher_thread.joinable()) impl_->watcher_thread.join();
}

} // namespace file_system
