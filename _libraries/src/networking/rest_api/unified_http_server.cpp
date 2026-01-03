#include "networking/rest_api/unified_http_server.h"
#include <iostream>

#include "file_system/file_watcher.h"
#include "advanced_logging/advanced_logging.h"


namespace networking {
namespace rest_api {

static advanced_logging::Logger unified_server_logger("unified_http_server.log");

UnifiedHttpServer::UnifiedHttpServer(int port, size_t num_threads)
    : port_(port), num_threads_(num_threads), running_(false) {
    unified_server_logger.info("UnifiedHttpServer constructed on port " + std::to_string(port) + ", threads: " + std::to_string(num_threads));
    try {
        http1_ = std::make_unique<Http1Server>(port, num_threads);
        http2_ = std::make_unique<Http2Server>(port, num_threads);
        http3_ = std::make_unique<Http3Server>(port, num_threads);
    } catch (const std::exception& ex) {
        unified_server_logger.error(std::string("Exception during server construction: ") + ex.what());
        running_ = false;
    }
    // file_watcher_ is null until set_hot_reload_files is called
}

UnifiedHttpServer::~UnifiedHttpServer() {
    stop();
    stop_file_watcher();
}
// Set the list of files to watch for hot reload
void UnifiedHttpServer::set_hot_reload_files(const std::vector<std::string>& files) {
    watched_files_ = files;
    start_file_watcher();
}

// Set the callback to be invoked when a watched file is modified
void UnifiedHttpServer::set_hot_reload_callback(const std::function<void(const std::string&)>& cb) {
    reload_callback_ = cb;
    // Optionally, immediately set up the callback for file_watcher if running
}

// Start the file watcher thread for hot reload
void UnifiedHttpServer::start_file_watcher() {
    stop_file_watcher();
    if (!watched_files_.empty()) {
        file_watcher_ = std::make_unique<file_system::FileWatcher>(watched_files_);
        // When a file is modified, log and trigger reload on all protocol servers
        file_watcher_->start([this](const std::string& path) {
            unified_server_logger.info("File modified: " + path);
            if (http1_) http1_->reload_file(path);
            if (http2_) http2_->reload_file(path);
            if (http3_) http3_->reload_file(path);
            if (reload_callback_) reload_callback_(path);
        });
        unified_server_logger.info("File watcher started for hot reload.");
    }
}

// Stop the file watcher thread
void UnifiedHttpServer::stop_file_watcher() {
    if (file_watcher_) {
        file_watcher_->file_system::FileWatcher::stop();
        file_watcher_.reset();
        unified_server_logger.info("File watcher stopped.");
    }
}

void UnifiedHttpServer::add_route(std::shared_ptr<Route> route) {
    http1_->add_route(route);
    http2_->add_route(route);
    http3_->add_route(route);
}

void UnifiedHttpServer::load_routes(const std::vector<std::shared_ptr<Route>>& routes) {
    http1_->load_routes(routes);
    http2_->load_routes(routes);
    http3_->load_routes(routes);
}

void UnifiedHttpServer::use(Middleware middleware) {
    http1_->use(middleware);
    http2_->use(middleware);
    http3_->use(middleware);
}

void UnifiedHttpServer::enable_cors(const std::string& origin) {
    http1_->enable_cors(origin);
    http2_->enable_cors(origin);
    http3_->enable_cors(origin);
}

void UnifiedHttpServer::start() {
    start_file_watcher();
    unified_server_logger.info("UnifiedHttpServer starting all protocols...");
    running_ = true;
    try {
        t1_ = std::thread([this]() {
            unified_server_logger.info("HTTP/1.1 server thread starting. Thread ID: " + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
            try {
                unified_server_logger.info("HTTP/1.1 about to call start()");
                http1_->start();
                unified_server_logger.info("HTTP/1.1 start() returned");
            } catch (const std::exception& ex) {
                unified_server_logger.error(std::string("Exception in HTTP/1.1 server thread: ") + ex.what());
            }
            unified_server_logger.info("HTTP/1.1 about to call is_running()");
            bool running = http1_ ? http1_->is_running() : false;
            unified_server_logger.info(std::string("HTTP/1.1 is_running(): ") + (http1_ ? (running ? "true" : "false") : "null"));
            unified_server_logger.info("HTTP/1.1 server thread exited");
        });
        t2_ = std::thread([this]() {
            unified_server_logger.info("HTTP/2 server thread starting. Thread ID: " + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
            try {
                unified_server_logger.info("HTTP/2 about to call start()");
                http2_->start();
                unified_server_logger.info("HTTP/2 start() returned");
            } catch (const std::exception& ex) {
                unified_server_logger.error(std::string("Exception in HTTP/2 server thread: ") + ex.what());
            }
            unified_server_logger.info("HTTP/2 about to call is_running()");
            bool running = http2_ ? http2_->is_running() : false;
            unified_server_logger.info(std::string("HTTP/2 is_running(): ") + (http2_ ? (running ? "true" : "false") : "null"));
            unified_server_logger.info("HTTP/2 server thread exited");
        });
        t3_ = std::thread([this]() {
            unified_server_logger.info("HTTP/3 server thread starting. Thread ID: " + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
            try {
                unified_server_logger.info("HTTP/3 about to call start()");
                http3_->start();
                unified_server_logger.info("HTTP/3 start() returned");
            } catch (const std::exception& ex) {
                unified_server_logger.error(std::string("Exception in HTTP/3 server thread: ") + ex.what());
            }
            unified_server_logger.info("HTTP/3 about to call is_running()");
            bool running = http3_ ? http3_->is_running() : false;
            unified_server_logger.info(std::string("HTTP/3 is_running(): ") + (http3_ ? (running ? "true" : "false") : "null"));
            unified_server_logger.info("HTTP/3 server thread exited");
        });
        unified_server_logger.info("All protocol server threads started");
        // Add a short sleep and check is_running status
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        unified_server_logger.info(std::string("[DEBUG] After threads started, is_running() = ") + (is_running() ? "true" : "false"));
    } catch (const std::exception& ex) {
        unified_server_logger.error(std::string("Exception during server start: ") + ex.what());
        running_ = false;
    }
}

void UnifiedHttpServer::stop() {
    stop_file_watcher();
    unified_server_logger.info("UnifiedHttpServer stopping all protocols...");
    running_ = false;
    if (http1_) http1_->stop();
    if (http2_) http2_->stop();
    if (http3_) http3_->stop();
    if (t1_.joinable()) {
        unified_server_logger.info("Joining HTTP/1.1 server thread...");
        t1_.join();
        unified_server_logger.info("HTTP/1.1 server thread joined.");
    }
    if (t2_.joinable()) {
        unified_server_logger.info("Joining HTTP/2 server thread...");
        t2_.join();
        unified_server_logger.info("HTTP/2 server thread joined.");
    }
    if (t3_.joinable()) {
        unified_server_logger.info("Joining HTTP/3 server thread...");
        t3_.join();
        unified_server_logger.info("HTTP/3 server thread joined.");
    }
    unified_server_logger.info("UnifiedHttpServer stopped.");
}

bool UnifiedHttpServer::is_running() const {
    bool status = running_ && http1_ && http2_ && http3_ &&
                  http1_->is_running() && http2_->is_running() && http3_->is_running();
    if (!status) {
        unified_server_logger.warn("UnifiedHttpServer is_running() check failed. running_: " + std::to_string(running_));
        unified_server_logger.warn(std::string("http1_ is ") + (http1_ ? "not null" : "null"));
        unified_server_logger.warn(std::string("http2_ is ") + (http2_ ? "not null" : "null"));
        unified_server_logger.warn(std::string("http3_ is ") + (http3_ ? "not null" : "null"));
        if (http1_) unified_server_logger.warn(std::string("http1_->is_running(): ") + (http1_->is_running() ? "true" : "false"));
        if (http2_) unified_server_logger.warn(std::string("http2_->is_running(): ") + (http2_->is_running() ? "true" : "false"));
        if (http3_) unified_server_logger.warn(std::string("http3_->is_running(): ") + (http3_->is_running() ? "true" : "false"));
    }
    return status;
}

} // namespace rest_api
} // namespace networking
