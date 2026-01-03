// ...existing code...
#pragma once
#include "http1_server.h"
#include "http2_server.h"
#include "http3_server.h"
#include "file_system/file_watcher.h"
#include <memory>
#include <thread>
#include <vector>
#include <functional>

namespace networking {
namespace rest_api {

class UnifiedHttpServer {
public:
    UnifiedHttpServer(int port, size_t num_threads = 4);
    ~UnifiedHttpServer();

    void add_route(std::shared_ptr<Route> route);
    void load_routes(const std::vector<std::shared_ptr<Route>>& routes);
    void use(Middleware middleware);
    void enable_cors(const std::string& origin = "*");
    void start();
    void stop();
    bool is_running() const;

    // Hot reload support
    // Set the list of files to watch for changes (e.g., static HTML, CSS, JS)
    void set_hot_reload_files(const std::vector<std::string>& files);
    // Set a callback to be invoked when a watched file is modified
    void set_hot_reload_callback(const std::function<void(const std::string&)>& cb);

private:
    int port_;
    size_t num_threads_;
    std::unique_ptr<Http1Server> http1_;
    std::unique_ptr<Http2Server> http2_;
    std::unique_ptr<Http3Server> http3_;
    std::thread t1_, t2_, t3_;
    bool running_;

    // File watcher for hot reload (watches files and triggers reload callbacks)
    std::unique_ptr<file_system::FileWatcher> file_watcher_;
    std::vector<std::string> watched_files_;
    std::function<void(const std::string&)> reload_callback_;
    // Start/stop the file watcher thread
    void start_file_watcher();
    void stop_file_watcher();
};

} // namespace rest_api
} // namespace networking
